/*
 * Copyright (c) 2026 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "vulkanpreview.h"

#include "Logger.h"
#include "settings.h"
#include "vulkanvideoitem.h"

#include <QCoreApplication>
#include <QEvent>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QPlatformSurfaceEvent>
#include <QQuickGraphicsConfiguration>
#include <QQuickGraphicsDevice>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QWindow>

#include <framework/mlt.h>

#include <atomic>

#ifdef SHOTCUT_VULKAN_PREVIEW
#ifdef Q_OS_WIN
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <rhi/qrhi_platform.h>
#include <QVulkanFunctions>
#include <QVulkanInstance>
#endif

namespace {

std::atomic<int> s_displayNits{0};
std::atomic<int> s_contentNits{0};
std::atomic<int> s_actualNits{0};
std::atomic<int> s_toneMapping{0};
// Whether the HDR window is attached, only to choose tone mapping. MLT keeps
// the attached sides and the generations they have drawn.
std::atomic<int> s_hdrAttached{0};
// Which windows asked to attach. MLT's table may not exist yet when they do, so
// these are replayed when it first does.
std::atomic<int> s_attached[MLT_VULKAN_EXPORT_SIDES];

QMutex s_mutex;
VulkanPreview::Slot s_slots[MLT_VULKAN_EXPORT_SLOTS];
int s_frameWidth = 0;
int s_frameHeight = 0;
int s_bestGeneration = 0;
QList<VulkanVideoItem *> s_items;
bool s_loggedMissingExport = false;
bool s_loggedExportFail = false;

#ifdef SHOTCUT_VULKAN_PREVIEW
QVulkanInstance *s_instance = nullptr;
VkPhysicalDevice s_physicalDevice = VK_NULL_HANDLE;
bool s_deviceFailed = false;
bool s_filterInstalled = false;

class PreviewWindowFilter : public QObject
{
public:
    explicit PreviewWindowFilter(QObject *parent)
        : QObject(parent)
    {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        const QEvent::Type type = event->type();
        if (type == QEvent::PlatformSurface || type == QEvent::Show) {
            if (type == QEvent::PlatformSurface) {
                const auto *surfaceEvent = static_cast<QPlatformSurfaceEvent *>(event);
                if (surfaceEvent->surfaceEventType() != QPlatformSurfaceEvent::SurfaceCreated)
                    return QObject::eventFilter(watched, event);
            }
            if (auto *window = qobject_cast<QQuickWindow *>(watched))
                VulkanPreview::prepare(window);
        }
        return QObject::eventFilter(watched, event);
    }
};

const mlt_vulkan_export_api *exportApi()
{
    static const mlt_vulkan_export_api *cached = nullptr;
    if (cached)
        return cached;
    mlt_properties global = mlt_global_properties();
    if (!global)
        return nullptr;
    mlt_properties props = (mlt_properties) mlt_properties_get_data(global,
                                                                    "placebo.vulkan",
                                                                    nullptr);
    if (!props)
        return nullptr;
    auto *api = static_cast<const mlt_vulkan_export_api *>(
        mlt_properties_get_data(props, "export_api", nullptr));
    // An older MLT has a shorter table. The preview needs every entry.
    if (api && api->size >= int(sizeof(mlt_vulkan_export_api))) {
        cached = api;
        for (int side = 0; side < MLT_VULKAN_EXPORT_SIDES; side++) {
            if (s_attached[side].load())
                api->attach(side);
        }
    }
    return cached;
}

const char *externalMemoryExtensions[] = {
    "VK_KHR_external_memory",
    "VK_KHR_external_semaphore",
#ifdef Q_OS_WIN
    "VK_KHR_external_memory_win32",
    "VK_KHR_external_semaphore_win32",
#else
    "VK_KHR_external_memory_fd",
    "VK_KHR_external_semaphore_fd",
#endif
};

void requestExternalMemoryExtensions()
{
    QByteArray current = qgetenv("QT_VULKAN_DEVICE_EXTENSIONS");
    for (const char *name : externalMemoryExtensions) {
        const QByteArray extension(name);
        if (current.contains(extension))
            continue;
        if (!current.isEmpty() && !current.endsWith(';'))
            current.append(';');
        current.append(extension);
    }
    qputenv("QT_VULKAN_DEVICE_EXTENSIONS", current);
}

bool ensurePhysicalDevice()
{
    if (s_physicalDevice)
        return true;
    if (s_deviceFailed || !s_instance || !s_instance->isValid())
        return false;

    mlt_properties global = mlt_global_properties();
    mlt_properties props = global ? (mlt_properties) mlt_properties_get_data(global,
                                                                             "placebo.vulkan",
                                                                             nullptr)
                                  : nullptr;
    if (!props)
        return false;

    auto *placeboPhys = (VkPhysicalDevice) mlt_properties_get_data(props, "phys_dev", nullptr);
    auto placeboInst = (VkInstance) mlt_properties_get_data(props, "instance", nullptr);
    auto getProc = (PFN_vkGetInstanceProcAddr) mlt_properties_get_data(props,
                                                                       "get_proc_addr",
                                                                       nullptr);
    if (!placeboPhys || !placeboInst || !getProc) {
        s_deviceFailed = true;
        LOG_WARNING() << "Vulkan preview: placebo.vulkan has no physical device";
        return false;
    }
    auto getProps = (PFN_vkGetPhysicalDeviceProperties) getProc(placeboInst,
                                                                "vkGetPhysicalDeviceProperties");
    if (!getProps) {
        s_deviceFailed = true;
        LOG_WARNING() << "Vulkan preview: vkGetPhysicalDeviceProperties is missing";
        return false;
    }
    VkPhysicalDeviceProperties wanted{};
    getProps(placeboPhys, &wanted);

    QVulkanFunctions *functions = s_instance->functions();
    uint32_t count = 0;
    functions->vkEnumeratePhysicalDevices(s_instance->vkInstance(), &count, nullptr);
    QList<VkPhysicalDevice> devices;
    devices.resize(int(count));
    if (count)
        functions->vkEnumeratePhysicalDevices(s_instance->vkInstance(), &count, devices.data());
    for (VkPhysicalDevice device : devices) {
        VkPhysicalDeviceProperties properties{};
        functions->vkGetPhysicalDeviceProperties(device, &properties);
        if (properties.vendorID == wanted.vendorID && properties.deviceID == wanted.deviceID) {
            s_physicalDevice = device;
            LOG_INFO() << "Vulkan preview device" << properties.deviceName << "vendor"
                       << properties.vendorID << "device" << properties.deviceID;
            return true;
        }
    }
    s_deviceFailed = true;
    LOG_WARNING() << "Vulkan preview: Qt found no device matching vendor" << wanted.vendorID
                  << "device" << wanted.deviceID;
    return false;
}
#endif

} // namespace

QByteArray VulkanPreview::graphicsBackend()
{
    QByteArray backend = qEnvironmentVariableIsSet("QSG_RHI_BACKEND") ? qgetenv("QSG_RHI_BACKEND")
                                                                      : QByteArrayLiteral("d3d11");
#if defined(Q_OS_WIN)
    if (Settings.processingMode() == ShotcutSettings::Linear16GpuCpu
        && backend.toLower() != QByteArrayLiteral("opengl"))
        backend = QByteArrayLiteral("vulkan");
#endif
    return backend;
}

void VulkanPreview::applyGraphicsApi()
{
#if defined(Q_OS_WIN)
    if (Settings.processingMode() == ShotcutSettings::Linear16GpuCpu) {
        const QByteArray backend = graphicsBackend();
        ::qputenv("QSG_RHI_BACKEND", backend);
        QQuickWindow::setGraphicsApi(backend.toLower() == QByteArrayLiteral("opengl")
                                         ? QSGRendererInterface::OpenGL
                                         : QSGRendererInterface::Vulkan);
    }
#endif
    if (!usesSceneGraph())
        return;
    if (createInstance())
        return;
#if defined(Q_OS_WIN)
    LOG_WARNING() << "Vulkan preview instance failed; using Direct3D 11";
    ::qputenv("QSG_RHI_BACKEND", "d3d11");
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
#else
    LOG_WARNING() << "Vulkan preview instance failed; keeping the current drawing method";
#endif
}

bool VulkanPreview::usesSceneGraph()
{
    return Settings.processingMode() == ShotcutSettings::Linear16GpuCpu
           && QQuickWindow::graphicsApi() == QSGRendererInterface::Vulkan;
}

void VulkanPreview::publishAdapter()
{
    if (Settings.processingMode() != ShotcutSettings::Linear16GpuCpu)
        return;
    const uint vendor = Settings.gpuAdapterVendorId();
    if (vendor == 0)
        return;
    ::qputenv("MLT_VULKAN_VENDOR_ID", QByteArray::number(vendor));
    ::qputenv("MLT_VULKAN_DEVICE_ID", QByteArray::number(Settings.gpuAdapterDeviceId()));
}

bool VulkanPreview::active()
{
#ifdef SHOTCUT_VULKAN_PREVIEW
    if (!usesSceneGraph())
        return false;
    if (!s_instance || !s_instance->isValid())
        return false;
    if (!ensurePhysicalDevice())
        return false;
    if (!exportApi()) {
        if (!s_loggedMissingExport) {
            s_loggedMissingExport = true;
            LOG_WARNING() << "Vulkan preview: placebo.vulkan export_api is missing or too old; "
                             "using the standard player";
        }
        return false;
    }
    return true;
#else
    return false;
#endif
}

bool VulkanPreview::createInstance()
{
#ifdef SHOTCUT_VULKAN_PREVIEW
    if (s_instance)
        return s_instance->isValid();
    s_instance = new QVulkanInstance;
    s_instance->setExtensions(QRhiVulkanInitParams::preferredInstanceExtensions());
    if (!s_instance->create()) {
        LOG_WARNING() << "Vulkan preview: QVulkanInstance::create failed";
        delete s_instance;
        s_instance = nullptr;
        return false;
    }
    if (qApp && !s_filterInstalled) {
        qApp->installEventFilter(new PreviewWindowFilter(qApp));
        s_filterInstalled = true;
    }
    // The widget compositor reads this when it creates its VkDevice. That
    // happens on the first expose, which is before prepare() can add the
    // same extensions to a QQuickWindow.
    requestExternalMemoryExtensions();
    return true;
#else
    return false;
#endif
}

bool VulkanPreview::hostVisible(QQuickWindow *window)
{
    if (!window)
        return false;
    if (window->isVisible())
        return true;
    return window->property("shotcutHostVisible").toBool();
}

void VulkanPreview::setHostVisible(QQuickWindow *window, bool visible)
{
    if (!window)
        return;
    window->setProperty("shotcutHostVisible", visible);
    const QList<VulkanVideoItem *> items = s_items;
    for (VulkanVideoItem *item : items) {
        if (!item || item->window() != window)
            continue;
        item->refreshAttached();
        item->update();
    }
}

void VulkanPreview::prepare(QQuickWindow *window)
{
#ifdef SHOTCUT_VULKAN_PREVIEW
    if (!window || !usesSceneGraph())
        return;
    if (window->property("_shotcutVulkanPrepared").toBool())
        return;
    if (!s_instance && !createInstance())
        return;
    if (!ensurePhysicalDevice())
        return;
    if (window->isSceneGraphInitialized()) {
        LOG_WARNING() << "Vulkan preview: scene graph already started for" << window;
        return;
    }
    window->setVulkanInstance(s_instance);
    window->setGraphicsDevice(QQuickGraphicsDevice::fromPhysicalDevice(s_physicalDevice));
    QQuickGraphicsConfiguration configuration = window->graphicsConfiguration();
    QByteArrayList extensions = configuration.deviceExtensions();
    for (const char *name : externalMemoryExtensions) {
        const QByteArray extension(name);
        if (!extensions.contains(extension))
            extensions.append(extension);
    }
    configuration.setDeviceExtensions(extensions);
    window->setGraphicsConfiguration(configuration);
    window->setProperty("_shotcutVulkanPrepared", true);
#else
    Q_UNUSED(window);
#endif
}

void VulkanPreview::prepareCompositor(QWindow *window)
{
#ifdef SHOTCUT_VULKAN_PREVIEW
    if (!usesSceneGraph())
        return;
    if (!s_instance && !createInstance())
        return;
    requestExternalMemoryExtensions();
    if (window && !window->vulkanInstance())
        window->setVulkanInstance(s_instance);

    if (qEnvironmentVariableIsSet("QT_VK_PHYSICAL_DEVICE_INDEX") || !ensurePhysicalDevice())
        return;
    QVulkanFunctions *functions = s_instance->functions();
    uint32_t count = 0;
    functions->vkEnumeratePhysicalDevices(s_instance->vkInstance(), &count, nullptr);
    QList<VkPhysicalDevice> devices;
    devices.resize(int(count));
    if (count)
        functions->vkEnumeratePhysicalDevices(s_instance->vkInstance(), &count, devices.data());
    const int index = devices.indexOf(s_physicalDevice);
    if (index < 0) {
        LOG_WARNING() << "Vulkan preview: compositor could not match the placebo GPU";
        return;
    }
    // QRhi reads this while creating the widget backing-store device. The
    // embedded player then imports pictures on that device.
    qputenv("QT_VK_PHYSICAL_DEVICE_INDEX", QByteArray::number(index));
    LOG_INFO() << "Vulkan preview: widget compositor using physical device" << index;
#else
    Q_UNUSED(window);
#endif
}

void VulkanPreview::syncHdrSettings()
{
    s_displayNits.store(Settings.playerHdrDisplayPeakNits());
    s_contentNits.store(Settings.playerHdrContentPeakNits());
    s_toneMapping.store(Settings.playerHdrToneMapping() ? 1 : 0);
}

void VulkanPreview::setActualDisplayNits(int nits)
{
    s_actualNits.store(nits);
}

bool VulkanPreview::exportFrame(mlt_vulkan_image *image, bool hdrContent)
{
#ifdef SHOTCUT_VULKAN_PREVIEW
    const mlt_vulkan_export_api *api = exportApi();
    if (!image || !api)
        return false;
    mlt_vulkan_export_params request{};
    request.size = sizeof(request);
    // With the HDR window closed, clip inside libplacebo so the SDR shader
    // still has the highlights. With it open, follow the preview setting.
    request.tone_mapping = (s_hdrAttached.load() && s_toneMapping.load()) ? 1 : 0;
    // The exported image is linear and carries no mastering metadata, so
    // libplacebo assumes an SDR peak and never tone maps. Declare the same
    // 1000 nit default the QVideoFrame path uses for HLG and PQ content.
    const int contentNits = s_contentNits.load();
    request.max_content_nits = contentNits > 0 ? contentNits : (hdrContent ? 1000 : 0);
    const int displayNits = s_displayNits.load();
    request.max_display_nits = displayNits > 0 ? displayNits : s_actualNits.load();

    mlt_vulkan_export_desc desc{};
    desc.size = sizeof(desc);
    if (api->render(image, &request, &desc) != 0) {
        if (!s_loggedExportFail) {
            s_loggedExportFail = true;
            LOG_WARNING() << "Vulkan preview export failed; keeping the last picture";
        }
        return false;
    }
    s_loggedExportFail = false;
    if (desc.generation <= 0 || desc.slot < 0 || desc.slot >= MLT_VULKAN_EXPORT_SLOTS)
        return true;
    QMutexLocker lock(&s_mutex);
    s_slots[desc.slot].valid = true;
    s_slots[desc.slot].desc = desc;
    if (desc.generation >= s_bestGeneration) {
        s_bestGeneration = desc.generation;
        s_frameWidth = desc.width;
        s_frameHeight = desc.height;
    }
    return true;
#else
    Q_UNUSED(image);
    Q_UNUSED(hdrContent);
    return false;
#endif
}

void VulkanPreview::notifyItems()
{
    int width = 0;
    int height = 0;
    frameSize(&width, &height);
    const QList<VulkanVideoItem *> items = s_items;
    for (VulkanVideoItem *item : items) {
        if (!item)
            continue;
        item->applyFrameSize(width, height);
        item->update();
    }
}

void VulkanPreview::registerItem(VulkanVideoItem *item)
{
    if (item && !s_items.contains(item))
        s_items.append(item);
}

void VulkanPreview::unregisterItem(VulkanVideoItem *item)
{
    s_items.removeAll(item);
}

void VulkanPreview::setAttached(bool hdr, bool attached)
{
    s_attached[sideIndex(hdr)].store(attached ? 1 : 0);
    if (hdr)
        s_hdrAttached.store(attached ? 1 : 0);
#ifdef SHOTCUT_VULKAN_PREVIEW
    if (const mlt_vulkan_export_api *api = exportApi()) {
        if (attached)
            api->attach(sideIndex(hdr));
        else
            api->detach(sideIndex(hdr));
    }
#endif
}

void VulkanPreview::setSeen(bool hdr, int slot, int generation)
{
#ifdef SHOTCUT_VULKAN_PREVIEW
    if (const mlt_vulkan_export_api *api = exportApi())
        api->submitted(sideIndex(hdr), slot, generation);
#else
    Q_UNUSED(hdr);
    Q_UNUSED(slot);
    Q_UNUSED(generation);
#endif
}

int VulkanPreview::seen(bool hdr, int slot)
{
#ifdef SHOTCUT_VULKAN_PREVIEW
    if (const mlt_vulkan_export_api *api = exportApi())
        return api->seen(sideIndex(hdr), slot);
#else
    Q_UNUSED(hdr);
    Q_UNUSED(slot);
#endif
    return 0;
}

void VulkanPreview::frameSize(int *width, int *height)
{
    QMutexLocker lock(&s_mutex);
    if (width)
        *width = s_frameWidth;
    if (height)
        *height = s_frameHeight;
}

void VulkanPreview::copySlots(Slot out[MLT_VULKAN_EXPORT_SLOTS])
{
    QMutexLocker lock(&s_mutex);
    for (int i = 0; i < MLT_VULKAN_EXPORT_SLOTS; i++)
        out[i] = s_slots[i];
}
