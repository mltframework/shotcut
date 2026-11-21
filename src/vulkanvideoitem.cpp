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

#include "vulkanvideoitem.h"

#include "vulkanpreviewrenderer.h"

#include <QQuickWindow>

VulkanVideoItem::VulkanVideoItem(QQuickItem *parent)
#ifdef SHOTCUT_VULKAN_PREVIEW
    : QQuickRhiItem(parent)
#else
    : QQuickItem(parent)
#endif
{
    VulkanPreview::registerItem(this);
#ifdef SHOTCUT_VULKAN_PREVIEW
    setAlphaBlending(false);
#endif
}

VulkanVideoItem::~VulkanVideoItem()
{
    VulkanPreview::setAttached(m_hdr, false);
    VulkanPreview::unregisterItem(this);
}

void VulkanVideoItem::setHdr(bool hdr)
{
    if (m_hdr == hdr)
        return;
    m_hdr = hdr;
#ifdef SHOTCUT_VULKAN_PREVIEW
    setColorBufferFormat(hdr ? TextureFormat::RGBA16F : TextureFormat::RGBA8);
#endif
    updateAttached();
    emit hdrChanged();
}

void VulkanVideoItem::applyFrameSize(int width, int height)
{
    if (m_frameWidth == width && m_frameHeight == height)
        return;
    m_frameWidth = width;
    m_frameHeight = height;
    emit frameSizeChanged();
}

void VulkanVideoItem::refreshAttached()
{
    updateAttached();
}

void VulkanVideoItem::updateAttached()
{
    const bool attached = isVisible() && VulkanPreview::hostVisible(window());
    // Only the render thread clears attached, and only after it has recorded
    // the generation whose done semaphore it already queued. Clearing it here
    // makes MLT wait a ready semaphore this window already consumed.
    if (attached)
        VulkanPreview::setAttached(m_hdr, true);
    else
        update();
}

void VulkanVideoItem::itemChange(ItemChange change, const ItemChangeData &value)
{
#ifdef SHOTCUT_VULKAN_PREVIEW
    QQuickRhiItem::itemChange(change, value);
#else
    QQuickItem::itemChange(change, value);
#endif
    if ((change == ItemVisibleHasChanged || change == ItemSceneChange) && window()
        && !m_watchedWindow) {
        m_watchedWindow = true;
        connect(window(), &QQuickWindow::visibilityChanged, this, [this](QWindow::Visibility) {
            updateAttached();
        });
    }
    if (change == ItemVisibleHasChanged || change == ItemSceneChange)
        updateAttached();
}

#ifdef SHOTCUT_VULKAN_PREVIEW
QQuickRhiItemRenderer *VulkanVideoItem::createRenderer()
{
    return createVulkanPreviewRenderer();
}
#endif
