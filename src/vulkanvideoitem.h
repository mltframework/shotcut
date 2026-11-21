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

#ifndef VULKANVIDEOITEM_H
#define VULKANVIDEOITEM_H

#include "vulkanpreview.h"

#include <QQuickItem>
#ifdef SHOTCUT_VULKAN_PREVIEW
#include <QtQuick/qquickrhiitem.h>
#endif

// Player and HDR preview both instantiate this. On macOS, and whenever the
// scene graph is not Vulkan, it is an empty item so the shared QML still parses.
#ifdef SHOTCUT_VULKAN_PREVIEW
class VulkanVideoItem : public QQuickRhiItem
#else
class VulkanVideoItem : public QQuickItem
#endif
{
    Q_OBJECT
    Q_PROPERTY(bool hdr READ isHdr WRITE setHdr NOTIFY hdrChanged)
    Q_PROPERTY(int frameWidth READ frameWidth NOTIFY frameSizeChanged)
    Q_PROPERTY(int frameHeight READ frameHeight NOTIFY frameSizeChanged)

public:
    explicit VulkanVideoItem(QQuickItem *parent = nullptr);
    ~VulkanVideoItem() override;

    bool isHdr() const { return m_hdr; }
    void setHdr(bool hdr);
    int frameWidth() const { return m_frameWidth; }
    int frameHeight() const { return m_frameHeight; }
    void applyFrameSize(int width, int height);
    void refreshAttached();

signals:
    void hdrChanged();
    void frameSizeChanged();

protected:
#ifdef SHOTCUT_VULKAN_PREVIEW
    QQuickRhiItemRenderer *createRenderer() override;
#endif
    void itemChange(ItemChange change, const ItemChangeData &value) override;

private:
    void updateAttached();

    bool m_hdr{false};
    bool m_watchedWindow{false};
    int m_frameWidth{0};
    int m_frameHeight{0};
};

#endif
