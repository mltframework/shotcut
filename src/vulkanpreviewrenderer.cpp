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

#include "vulkanpreviewrenderer.h"

#include "Logger.h"
#include "qmltypes/qmlutilities.h"
#include "vulkanpreview.h"
#include "vulkanvideoitem.h"

#include <QFile>
#include <QQuickWindow>

#include <algorithm>
#include <cstring>

#ifdef SHOTCUT_VULKAN_PREVIEW
#ifdef Q_OS_WIN
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>
#include <rhi/qshader.h>
#include <QVulkanFunctions>
#include <QVulkanInstance>
#include <QtQuick/qquickrhiitem.h>
#ifndef Q_OS_WIN
#include <unistd.h>
#endif
#endif
#ifdef SHOTCUT_VULKAN_PREVIEW

namespace {

const float kVertices[] = {
    -1.f,
    -1.f,
    0.f,
    1.f, // Y-up clip space, bottom-left, uv bottom
    -1.f,
    1.f,
    0.f,
    0.f, // top-left
    1.f,
    -1.f,
    1.f,
    1.f, // bottom-right
    1.f,
    1.f,
    1.f,
    0.f, // top-right
};

QShader loadPreviewShader(const char *fileName)
{
    const QString path = QmlUtilities::qmlDir().filePath(QStringLiteral("views/")
                                                         + QLatin1String(fileName));
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_WARNING() << "Vulkan preview shader missing:" << path;
        return {};
    }
    QShader shader = QShader::fromSerialized(file.readAll());
    if (!shader.isValid())
        LOG_WARNING() << "Vulkan preview shader is invalid:" << path;
    return shader;
}

// Duplicates an exported Win32 handle or POSIX fd. The destructor closes it.
// Vulkan takes ownership of an fd on a successful import, so that path calls release().
class ExternalHandle
{
public:
    explicit ExternalHandle(void *key)
    {
        if (!key)
            return;
#ifdef Q_OS_WIN
        if (!DuplicateHandle(GetCurrentProcess(),
                             HANDLE(key),
                             GetCurrentProcess(),
                             &m_handle,
                             0,
                             FALSE,
                             DUPLICATE_SAME_ACCESS))
            m_handle = nullptr;
#else
        m_fd = ::dup(int(intptr_t(key)));
#endif
    }

    ~ExternalHandle()
    {
#ifdef Q_OS_WIN
        if (m_handle)
            CloseHandle(m_handle);
#else
        if (m_fd >= 0)
            ::close(m_fd);
#endif
    }

    ExternalHandle(const ExternalHandle &) = delete;
    ExternalHandle &operator=(const ExternalHandle &) = delete;

    bool ok() const
    {
#ifdef Q_OS_WIN
        return m_handle != nullptr;
#else
        return m_fd >= 0;
#endif
    }

#ifdef Q_OS_WIN
    HANDLE get() const
    {
        return m_handle;
    }
#else
    int get() const
    {
        return m_fd;
    }
    void release()
    {
        m_fd = -1;
    }
#endif

private:
#ifdef Q_OS_WIN
    HANDLE m_handle = nullptr;
#else
    int m_fd = -1;
#endif
};

class PreviewRenderer : public QQuickRhiItemRenderer
{
public:
    ~PreviewRenderer() override
    {
        flushSeen();
        if (m_roleValid)
            VulkanPreview::setAttached(m_hdr, false);
        // retire() submits m_cmd. Wait for that submit before releasing the
        // semaphores it waits and signals.
        if (m_device && m_devFn && m_retireFence
            && m_devFn->vkWaitForFences(m_device, 1, &m_retireFence, VK_TRUE, 1000000000ull)
                   == VK_SUCCESS) {
            // The last frame's submit still reads the imported image and uses
            // its semaphores. An empty submit's fence signals after every
            // earlier submit on this queue. The wait is bounded because the UI
            // thread must never block on a semaphore MLT has not signaled.
            m_devFn->vkResetFences(m_device, 1, &m_retireFence);
            VkSubmitInfo empty{};
            empty.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            if (m_devFn->vkQueueSubmit(m_queue, 1, &empty, m_retireFence) == VK_SUCCESS)
                m_devFn->vkWaitForFences(m_device, 1, &m_retireFence, VK_TRUE, 1000000000ull);
        }
        releaseImported();
        delete m_pipeline;
        delete m_srb;
        delete m_sampler;
        delete m_vbuf;
        delete m_ubuf;
        if (m_device && m_devFn) {
            if (m_cmd)
                m_devFn->vkFreeCommandBuffers(m_device, m_pool, 1, &m_cmd);
            if (m_retireFence)
                m_devFn->vkDestroyFence(m_device, m_retireFence, nullptr);
            if (m_pool)
                m_devFn->vkDestroyCommandPool(m_device, m_pool, nullptr);
        }
    }

    void initialize(QRhiCommandBuffer *cb) override
    {
        if (m_rhi != rhi()) {
            releaseImported();
            delete m_pipeline;
            delete m_srb;
            delete m_sampler;
            delete m_vbuf;
            delete m_ubuf;
            m_pipeline = nullptr;
            m_srb = nullptr;
            m_sampler = nullptr;
            m_vbuf = nullptr;
            m_ubuf = nullptr;
            m_boundTexture = nullptr;
            m_renderPass = nullptr;
            m_rhi = rhi();
            m_deviceReady = false;
            if (m_device && m_devFn && m_pool) {
                if (m_cmd)
                    m_devFn->vkFreeCommandBuffers(m_device, m_pool, 1, &m_cmd);
                m_devFn->vkDestroyCommandPool(m_device, m_pool, nullptr);
            }
            m_pool = VK_NULL_HANDLE;
            m_cmd = VK_NULL_HANDLE;
            m_device = VK_NULL_HANDLE;
        }
        if (!m_rhi || m_rhi->backend() != QRhi::Vulkan)
            return;
        if (!m_deviceReady && !setupDevice())
            return;

        if (!m_vbuf) {
            m_vbuf = m_rhi->newBuffer(QRhiBuffer::Immutable,
                                      QRhiBuffer::VertexBuffer,
                                      sizeof(kVertices));
            m_vbuf->create();
            m_ubuf = m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 256);
            m_ubuf->create();
            m_sampler = m_rhi->newSampler(QRhiSampler::Linear,
                                          QRhiSampler::Linear,
                                          QRhiSampler::None,
                                          QRhiSampler::ClampToEdge,
                                          QRhiSampler::ClampToEdge,
                                          QRhiSampler::ClampToEdge);
            m_sampler->create();
            m_vs = loadPreviewShader("vulkan_preview.vert.qsb");
            m_fs = loadPreviewShader("vulkan_preview.frag.qsb");
            QRhiResourceUpdateBatch *batch = m_rhi->nextResourceUpdateBatch();
            batch->uploadStaticBuffer(m_vbuf, kVertices);
            cb->resourceUpdate(batch);
        }
    }

    void flushSeen()
    {
        if (m_seenPendingSlot < 0)
            return;
        // The previous frame's queue submit, which signals done, has already
        // been queued. MLT may wait that semaphore now without the UI thread.
        VulkanPreview::setSeen(m_hdr, m_seenPendingSlot, m_seenPendingGeneration);
        m_seenPendingSlot = -1;
    }

    static bool sideArmed(bool hdr, const mlt_vulkan_export_desc &desc)
    {
        return desc.side[VulkanPreview::sideIndex(hdr)].armed != 0;
    }

    void synchronize(QQuickRhiItem *item) override
    {
        auto *video = static_cast<VulkanVideoItem *>(item);
        flushSeen();
        m_hdr = video->isHdr();
        m_roleValid = true;
        m_opacity = float(video->opacity());
        m_clearColor = video->window() ? video->window()->color() : QColor(Qt::black);
        const bool show = video->isVisible() && VulkanPreview::hostVisible(video->window());
        VulkanPreview::setAttached(m_hdr, show);
        m_show = show;
        VulkanPreview::copySlots(m_pending);
    }

    void render(QRhiCommandBuffer *cb) override
    {
        if (!m_rhi || !m_deviceReady || !m_vbuf || !m_vs.isValid() || !m_fs.isValid()) {
            clearBlank(cb);
            return;
        }
        if (!m_show) {
            clearBlank(cb);
            return;
        }

        // A ready semaphore is consumed once. Besides this renderer's own
        // submits, honor those of a renderer that a QML reload destroyed.
        int consumed[MLT_VULKAN_EXPORT_SLOTS];
        for (int i = 0; i < MLT_VULKAN_EXPORT_SLOTS; i++)
            consumed[i] = std::max(m_imported[i].submittedGeneration, VulkanPreview::seen(m_hdr, i));

        int drawSlot = -1;
        int drawGeneration = 0;
        for (int i = 0; i < MLT_VULKAN_EXPORT_SLOTS; i++) {
            if (!m_pending[i].valid || !sideArmed(m_hdr, m_pending[i].desc))
                continue;
            const int generation = m_pending[i].desc.generation;
            if (generation <= consumed[i])
                continue;
            if (generation > drawGeneration) {
                drawGeneration = generation;
                drawSlot = i;
            }
        }
        for (int i = 0; i < MLT_VULKAN_EXPORT_SLOTS; i++) {
            if (i == drawSlot || !m_pending[i].valid || !sideArmed(m_hdr, m_pending[i].desc))
                continue;
            const int generation = m_pending[i].desc.generation;
            if (generation <= consumed[i])
                continue;
            if (!ensureImported(i, m_pending[i].desc))
                continue;
            if (retire(m_imported[i])) {
                m_imported[i].submittedGeneration = generation;
                VulkanPreview::setSeen(m_hdr, i, generation);
            }
        }
        if (drawSlot < 0) {
            clearBlank(cb);
            return;
        }
        if (!ensureImported(drawSlot, m_pending[drawSlot].desc)) {
            clearBlank(cb);
            return;
        }
        Imported &imported = m_imported[drawSlot];
        if (!recordOwnership(cb, imported.image, true)) {
            clearBlank(cb);
            return;
        }
        imported.texture->setNativeLayout(int(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        if (!ensurePipeline(imported.texture)) {
            clearBlank(cb);
            return;
        }
        float uniforms[20];
        const QMatrix4x4 matrix = m_rhi->clipSpaceCorrMatrix();
        memcpy(uniforms, matrix.constData(), 16 * sizeof(float));
        // std140 buf: matrix, sdrWhite, encodeSdr, opacity, pad.
        uniforms[16] = 203.f / 80.f;
        uniforms[17] = m_hdr ? 0.f : 1.f;
        uniforms[18] = m_opacity;
        uniforms[19] = 0.f;
        QRhiResourceUpdateBatch *batch = m_rhi->nextResourceUpdateBatch();
        batch->updateDynamicBuffer(m_ubuf, 0, sizeof(uniforms), uniforms);

        const QSize size = renderTarget()->pixelSize();
        cb->beginPass(renderTarget(), Qt::black, {1.f, 0}, batch);
        cb->setGraphicsPipeline(m_pipeline);
        cb->setViewport(QRhiViewport(0, 0, float(size.width()), float(size.height())));
        cb->setShaderResources();
        const QRhiCommandBuffer::VertexInput binding(m_vbuf, 0);
        cb->setVertexInput(0, 1, &binding);
        cb->draw(4);
        cb->endPass();
        // The color texture now holds this frame. A later resize allocates a new
        // uninitialized texture; that one is cleared in clearBlank().
        m_blankTexture = colorTexture();
        m_blankSize = renderTarget() ? renderTarget()->pixelSize() : QSize();

        if (!recordOwnership(cb, imported.image, false))
            return;
        m_wait = imported.ready;
        m_signal = imported.done;
        QRhiVulkanQueueSubmitParams params{};
        params.waitSemaphoreCount = 1;
        params.waitSemaphores = &m_wait;
        params.signalSemaphoreCount = 1;
        params.signalSemaphores = &m_signal;
        m_rhi->setQueueSubmitParams(&params);
        imported.submittedGeneration = drawGeneration;
        // setQueueSubmitParams is consumed by the submit that follows this
        // render. Marking the generation seen now would let MLT wait for done
        // before that submit is queued.
        m_seenPendingSlot = drawSlot;
        m_seenPendingGeneration = drawGeneration;
    }

private:
    struct Imported
    {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkSemaphore ready = VK_NULL_HANDLE;
        VkSemaphore done = VK_NULL_HANDLE;
        void *memoryKey = nullptr;
        void *readyKey = nullptr;
        void *doneKey = nullptr;
        int submittedGeneration = 0;
        QRhiTexture *texture = nullptr;
    };

    bool setupDevice()
    {
        const auto *handles = static_cast<const QRhiVulkanNativeHandles *>(m_rhi->nativeHandles());
        if (!handles || !handles->dev || !handles->inst)
            return false;
        m_device = handles->dev;
        m_family = handles->gfxQueueFamilyIdx;
        m_queue = handles->gfxQueue;
        m_devFn = handles->inst->deviceFunctions(m_device);
        m_instFn = handles->inst->functions();
        if (!m_devFn || !m_instFn)
            return false;
        if (!m_queue)
            m_devFn->vkGetDeviceQueue(m_device, m_family, handles->gfxQueueIdx, &m_queue);
#ifdef Q_OS_WIN
        m_importSemaphore = reinterpret_cast<PFN_vkImportSemaphoreWin32HandleKHR>(
            m_instFn->vkGetDeviceProcAddr(m_device, "vkImportSemaphoreWin32HandleKHR"));
#else
        m_importSemaphore = reinterpret_cast<PFN_vkImportSemaphoreFdKHR>(
            m_instFn->vkGetDeviceProcAddr(m_device, "vkImportSemaphoreFdKHR"));
#endif
        if (!m_importSemaphore || !m_queue)
            return false;

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_family;
        if (m_devFn->vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_pool) != VK_SUCCESS)
            return false;
        VkCommandBufferAllocateInfo allocate{};
        allocate.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate.commandPool = m_pool;
        allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate.commandBufferCount = 1;
        if (m_devFn->vkAllocateCommandBuffers(m_device, &allocate, &m_cmd) != VK_SUCCESS)
            return false;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        if (m_devFn->vkCreateFence(m_device, &fenceInfo, nullptr, &m_retireFence) != VK_SUCCESS)
            return false;
        m_physDevice = handles->physDev;
        m_deviceReady = true;
        return true;
    }

    void releaseImported()
    {
        for (Imported &slot : m_imported)
            destroySlot(slot);
    }

    void destroySlot(Imported &slot)
    {
        delete slot.texture;
        slot.texture = nullptr;
        if (m_device && m_devFn) {
            if (slot.image)
                m_devFn->vkDestroyImage(m_device, slot.image, nullptr);
            if (slot.memory)
                m_devFn->vkFreeMemory(m_device, slot.memory, nullptr);
            if (slot.ready)
                m_devFn->vkDestroySemaphore(m_device, slot.ready, nullptr);
            if (slot.done)
                m_devFn->vkDestroySemaphore(m_device, slot.done, nullptr);
        }
        slot.image = VK_NULL_HANDLE;
        slot.memory = VK_NULL_HANDLE;
        slot.ready = VK_NULL_HANDLE;
        slot.done = VK_NULL_HANDLE;
        slot.memoryKey = nullptr;
        slot.readyKey = nullptr;
        slot.doneKey = nullptr;
    }

    bool importSemaphore(void *key, VkSemaphore *out)
    {
        ExternalHandle handle(key);
        if (!handle.ok())
            return false;
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (m_devFn->vkCreateSemaphore(m_device, &info, nullptr, out) != VK_SUCCESS)
            return false;
#ifdef Q_OS_WIN
        VkImportSemaphoreWin32HandleInfoKHR import{};
        import.sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR;
        import.semaphore = *out;
        import.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
        import.handle = handle.get();
#else
        VkImportSemaphoreFdInfoKHR import{};
        import.sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR;
        import.semaphore = *out;
        import.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
        import.fd = handle.get();
#endif
        const VkResult result = m_importSemaphore(m_device, &import);
#ifndef Q_OS_WIN
        if (result == VK_SUCCESS)
            handle.release();
#endif
        if (result != VK_SUCCESS) {
            m_devFn->vkDestroySemaphore(m_device, *out, nullptr);
            *out = VK_NULL_HANDLE;
            return false;
        }
        return true;
    }

    bool ensureImported(int index, const mlt_vulkan_export_desc &desc)
    {
        Imported &slot = m_imported[index];
        if (!desc.memory || desc.memory_size == 0 || desc.width < 1 || desc.height < 1)
            return false;
        const bool sameImage = slot.image && slot.memoryKey == desc.memory && slot.texture
                               && slot.texture->pixelSize() == QSize(desc.width, desc.height);
        if (!sameImage) {
            delete slot.texture;
            slot.texture = nullptr;
            if (slot.image)
                m_devFn->vkDestroyImage(m_device, slot.image, nullptr);
            if (slot.memory)
                m_devFn->vkFreeMemory(m_device, slot.memory, nullptr);
            slot.image = VK_NULL_HANDLE;
            slot.memory = VK_NULL_HANDLE;
            slot.memoryKey = nullptr;
            if (!importImage(desc, slot))
                return false;
        }
        const mlt_vulkan_export_side &side = desc.side[VulkanPreview::sideIndex(m_hdr)];
        void *readyKey = side.ready;
        void *doneKey = side.done;
        if (!readyKey || !doneKey)
            return false;
        if (slot.readyKey != readyKey) {
            if (slot.ready)
                m_devFn->vkDestroySemaphore(m_device, slot.ready, nullptr);
            slot.ready = VK_NULL_HANDLE;
            if (!importSemaphore(readyKey, &slot.ready))
                return false;
            slot.readyKey = readyKey;
        }
        if (slot.doneKey != doneKey) {
            if (slot.done)
                m_devFn->vkDestroySemaphore(m_device, slot.done, nullptr);
            slot.done = VK_NULL_HANDLE;
            if (!importSemaphore(doneKey, &slot.done))
                return false;
            slot.doneKey = doneKey;
        }
        return slot.image && slot.ready && slot.done && slot.texture;
    }

    bool importImage(const mlt_vulkan_export_desc &desc, Imported &slot)
    {
        ExternalHandle handle(desc.memory);
        if (!handle.ok())
            return false;
        VkExternalMemoryImageCreateInfo external{};
        external.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
        external.handleTypes =
#ifdef Q_OS_WIN
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.pNext = &external;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        imageInfo.extent.width = uint32_t(desc.width);
        imageInfo.extent.height = uint32_t(desc.height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VkImageUsageFlags(desc.usage);
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImage image = VK_NULL_HANDLE;
        if (m_devFn->vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS)
            return false;
        VkMemoryRequirements requirements{};
        m_devFn->vkGetImageMemoryRequirements(m_device, image, &requirements);
        VkPhysicalDeviceMemoryProperties memoryProperties{};
        m_instFn->vkGetPhysicalDeviceMemoryProperties(m_physDevice, &memoryProperties);
        int type = -1;
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
            if ((requirements.memoryTypeBits & (1u << i))
                && (memoryProperties.memoryTypes[i].propertyFlags
                    & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                type = int(i);
                break;
            }
        }
        if (type < 0) {
            m_devFn->vkDestroyImage(m_device, image, nullptr);
            return false;
        }
#ifdef Q_OS_WIN
        VkImportMemoryWin32HandleInfoKHR import{};
        import.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
        import.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
        import.handle = handle.get();
#else
        VkImportMemoryFdInfoKHR import{};
        import.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
        import.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
        import.fd = handle.get();
#endif
        VkMemoryDedicatedAllocateInfo dedicated{};
        dedicated.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
        dedicated.pNext = &import;
        dedicated.image = image;
        VkMemoryAllocateInfo allocate{};
        allocate.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate.pNext = &dedicated;
        allocate.allocationSize = desc.memory_size;
        allocate.memoryTypeIndex = uint32_t(type);
        VkDeviceMemory memory = VK_NULL_HANDLE;
        const VkResult allocated = m_devFn->vkAllocateMemory(m_device, &allocate, nullptr, &memory);
#ifndef Q_OS_WIN
        if (allocated == VK_SUCCESS)
            handle.release();
#endif
        if (allocated != VK_SUCCESS
            || m_devFn->vkBindImageMemory(m_device, image, memory, 0) != VK_SUCCESS) {
            if (memory)
                m_devFn->vkFreeMemory(m_device, memory, nullptr);
            m_devFn->vkDestroyImage(m_device, image, nullptr);
            if (!m_loggedImport) {
                m_loggedImport = true;
                LOG_WARNING() << "Vulkan preview: importing the picture failed";
            }
            return false;
        }
        QRhiTexture *texture = m_rhi->newTexture(QRhiTexture::RGBA16F,
                                                 QSize(desc.width, desc.height));
        if (!texture->createFrom({quint64(image), int(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)})) {
            delete texture;
            m_devFn->vkDestroyImage(m_device, image, nullptr);
            m_devFn->vkFreeMemory(m_device, memory, nullptr);
            return false;
        }
        slot.image = image;
        slot.memory = memory;
        slot.memoryKey = desc.memory;
        slot.texture = texture;
        return true;
    }

    bool retire(Imported &slot)
    {
        if (!m_cmd || !m_retireFence || !slot.image || !slot.ready || !slot.done)
            return false;
        // m_cmd is still pending until the previous retire's fence signals.
        // Resetting it earlier is a lost device.
        if (m_devFn->vkWaitForFences(m_device, 1, &m_retireFence, VK_TRUE, 0) != VK_SUCCESS)
            return false;
        m_devFn->vkResetFences(m_device, 1, &m_retireFence);
        const auto signalFence = [this]() {
            VkSubmitInfo empty{};
            empty.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            m_devFn->vkQueueSubmit(m_queue, 1, &empty, m_retireFence);
        };
        m_devFn->vkResetCommandBuffer(m_cmd, 0);
        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (m_devFn->vkBeginCommandBuffer(m_cmd, &begin) != VK_SUCCESS) {
            signalFence();
            return false;
        }
        barrier(m_cmd, slot.image, true);
        barrier(m_cmd, slot.image, false);
        if (m_devFn->vkEndCommandBuffer(m_cmd) != VK_SUCCESS) {
            signalFence();
            return false;
        }
        VkPipelineStageFlags stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &slot.ready;
        submit.pWaitDstStageMask = &stage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &m_cmd;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &slot.done;
        if (m_devFn->vkQueueSubmit(m_queue, 1, &submit, m_retireFence) == VK_SUCCESS)
            return true;
        signalFence();
        return false;
    }

    bool recordOwnership(QRhiCommandBuffer *cb, VkImage image, bool acquire)
    {
        cb->beginExternal();
        const auto *native = static_cast<const QRhiVulkanCommandBufferNativeHandles *>(
            cb->nativeHandles());
        const bool ok = native && native->commandBuffer;
        if (ok)
            barrier(native->commandBuffer, image, acquire);
        cb->endExternal();
        return ok;
    }

    void barrier(VkCommandBuffer commandBuffer, VkImage image, bool acquire)
    {
        VkImageMemoryBarrier barrierInfo{};
        barrierInfo.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrierInfo.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrierInfo.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrierInfo.srcQueueFamilyIndex = acquire ? VK_QUEUE_FAMILY_EXTERNAL : m_family;
        barrierInfo.dstQueueFamilyIndex = acquire ? m_family : VK_QUEUE_FAMILY_EXTERNAL;
        barrierInfo.image = image;
        barrierInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrierInfo.subresourceRange.levelCount = 1;
        barrierInfo.subresourceRange.layerCount = 1;
        barrierInfo.srcAccessMask = acquire ? 0 : VK_ACCESS_SHADER_READ_BIT;
        barrierInfo.dstAccessMask = acquire ? VK_ACCESS_SHADER_READ_BIT : 0;
        const VkPipelineStageFlags srcStage = acquire ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                                                      : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        const VkPipelineStageFlags dstStage = acquire ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                                                      : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        m_devFn->vkCmdPipelineBarrier(
            commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrierInfo);
    }

    bool ensurePipeline(QRhiTexture *texture)
    {
        QRhiRenderPassDescriptor *pass = renderTarget() ? renderTarget()->renderPassDescriptor()
                                                        : nullptr;
        if (!texture || !pass)
            return false;
        if (m_pipeline && m_boundTexture == texture && m_renderPass == pass)
            return true;
        delete m_pipeline;
        delete m_srb;
        m_pipeline = nullptr;
        m_srb = nullptr;
        m_srb = m_rhi->newShaderResourceBindings();
        m_srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0,
                                                     QRhiShaderResourceBinding::VertexStage
                                                         | QRhiShaderResourceBinding::FragmentStage,
                                                     m_ubuf),
            QRhiShaderResourceBinding::sampledTexture(1,
                                                      QRhiShaderResourceBinding::FragmentStage,
                                                      texture,
                                                      m_sampler),
        });
        if (!m_srb->create())
            return false;
        m_pipeline = m_rhi->newGraphicsPipeline();
        m_pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
        m_pipeline->setShaderStages(
            {{QRhiShaderStage::Vertex, m_vs}, {QRhiShaderStage::Fragment, m_fs}});
        QRhiVertexInputLayout layout;
        layout.setBindings({{4 * sizeof(float)}});
        layout.setAttributes({{0, 0, QRhiVertexInputAttribute::Float2, 0},
                              {0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float)}});
        m_pipeline->setVertexInputLayout(layout);
        m_pipeline->setShaderResourceBindings(m_srb);
        m_pipeline->setRenderPassDescriptor(pass);
        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = false;
        m_pipeline->setTargetBlends({blend});
        m_pipeline->setCullMode(QRhiGraphicsPipeline::None);
        if (!m_pipeline->create())
            return false;
        m_boundTexture = texture;
        m_renderPass = pass;
        return true;
    }

    void clearBlank(QRhiCommandBuffer *cb)
    {
        // QQuickRhiItem creates the color texture uninitialized and does not clear
        // it. A resize calls QRhiTexture::create() again, which also drops the
        // previous contents. Clear each new texture once. Later frames with no
        // new image leave the texture alone so the last picture stays.
        QRhiTexture *texture = colorTexture();
        if (!texture || !renderTarget())
            return;
        const QSize size = texture->pixelSize();
        if (texture == m_blankTexture && size == m_blankSize)
            return;
        cb->beginPass(renderTarget(), m_clearColor, {1.f, 0});
        cb->endPass();
        m_blankTexture = texture;
        m_blankSize = size;
    }

    QRhi *m_rhi = nullptr;
    bool m_deviceReady = false;
    bool m_roleValid = false;
    bool m_hdr = false;
    bool m_show = false;
    bool m_loggedImport = false;
    float m_opacity = 1.f;
    QColor m_clearColor = Qt::black;
    QRhiTexture *m_blankTexture = nullptr;
    QSize m_blankSize;
    VulkanPreview::Slot m_pending[MLT_VULKAN_EXPORT_SLOTS];
    Imported m_imported[MLT_VULKAN_EXPORT_SLOTS];
    int m_seenPendingSlot = -1;
    int m_seenPendingGeneration = 0;

    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physDevice = VK_NULL_HANDLE;
    VkQueue m_queue = VK_NULL_HANDLE;
    uint32_t m_family = 0;
    VkCommandPool m_pool = VK_NULL_HANDLE;
    VkCommandBuffer m_cmd = VK_NULL_HANDLE;
    VkFence m_retireFence = VK_NULL_HANDLE;
    QVulkanDeviceFunctions *m_devFn = nullptr;
    QVulkanFunctions *m_instFn = nullptr;
#ifdef Q_OS_WIN
    PFN_vkImportSemaphoreWin32HandleKHR m_importSemaphore = nullptr;
#else
    PFN_vkImportSemaphoreFdKHR m_importSemaphore = nullptr;
#endif
    VkSemaphore m_wait = VK_NULL_HANDLE;
    VkSemaphore m_signal = VK_NULL_HANDLE;

    QShader m_vs;
    QShader m_fs;
    QRhiBuffer *m_vbuf = nullptr;
    QRhiBuffer *m_ubuf = nullptr;
    QRhiSampler *m_sampler = nullptr;
    QRhiShaderResourceBindings *m_srb = nullptr;
    QRhiGraphicsPipeline *m_pipeline = nullptr;
    QRhiTexture *m_boundTexture = nullptr;
    QRhiRenderPassDescriptor *m_renderPass = nullptr;
};

} // namespace

#endif

QQuickRhiItemRenderer *createVulkanPreviewRenderer()
{
#ifdef SHOTCUT_VULKAN_PREVIEW
    return new PreviewRenderer;
#else
    return nullptr;
#endif
}
