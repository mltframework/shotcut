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

#ifndef VULKANPREVIEW_H
#define VULKANPREVIEW_H

#include <framework/mlt_vulkan_export.h>

#include <QByteArray>
#include <QtGui/qtguiglobal.h>

class QQuickWindow;
class QWindow;
class VulkanVideoItem;

// Windows and Linux can sample MLT's exported Vulkan images. macOS stays on
// VideoOutput. The preview is active only for Linear 16-bit GPU when the
// scene graph is already Vulkan.
#if (defined(Q_OS_WIN) || (defined(Q_OS_UNIX) && !defined(Q_OS_MAC))) && QT_CONFIG(vulkan)
#define SHOTCUT_VULKAN_PREVIEW 1
#endif

// Shared state for the Vulkan preview path. The consumer thread renders into
// an exported image. Each window imports that memory on its own VkDevice.
class VulkanPreview
{
public:
    // MLT's export sides: the embedded player and the HDR window each import
    // the image on their own device.
    static constexpr int sideIndex(bool hdr) { return hdr ? 1 : 0; }
    static_assert(MLT_VULKAN_EXPORT_SIDES >= 2, "the player and HDR window need a side each");

    static bool active();
    // QSG_RHI_BACKEND for this process. On Windows, Linear 16-bit GPU is
    // Vulkan unless the inherited backend is OpenGL, which is a safe-mode restart.
    // The watchdog parent and the child both use this so they cannot disagree.
    static QByteArray graphicsBackend();
    // Apply graphicsBackend() before any window exists. If the Vulkan instance
    // cannot be created, Windows falls back to Direct3D 11.
    static void applyGraphicsApi();
    // Linear 16-bit GPU and the scene graph is Vulkan. active() also requires
    // the export API and the shared device.
    static bool usesSceneGraph();
    // The selected GPU, published for MLT before the player creates its device.
    static void publishAdapter();
    static bool createInstance();
    static void prepare(QQuickWindow *window);
    // QQuickWidget replaces the offscreen window's device with the top-level
    // widget compositor. That device is created before prepare() can run, so
    // the compositor itself has to be told which GPU and extensions to use.
    static void prepareCompositor(QWindow *window);
    // QQuickWidget renders through an offscreen window that is never shown.
    // The widget records that visibility here so the embedded player still attaches.
    static bool hostVisible(QQuickWindow *window);
    static void setHostVisible(QQuickWindow *window, bool visible);
    static void syncHdrSettings();
    // The HDR window's real peak, used when the display peak setting is auto.
    static void setActualDisplayNits(int nits);
    static bool exportFrame(mlt_vulkan_image *image, bool hdrContent);
    static void notifyItems();

    static void registerItem(VulkanVideoItem *item);
    static void unregisterItem(VulkanVideoItem *item);
    // The window of this role will wait on the exported images' ready
    // semaphores and signal their done semaphores.
    static void setAttached(bool hdr, bool attached);
    // This role queued the submit that waits and signals for the generation.
    static void setSeen(bool hdr, int slot, int generation);
    // Highest generation a window of this role already queued a wait for. A
    // renderer created after a QML reload must not wait on it again.
    static int seen(bool hdr, int slot);
    static void frameSize(int *width, int *height);

    struct Slot
    {
        bool valid;
        mlt_vulkan_export_desc desc;
    };
    static void copySlots(Slot out[MLT_VULKAN_EXPORT_SLOTS]);

private:
    VulkanPreview() = delete;
};

#endif
