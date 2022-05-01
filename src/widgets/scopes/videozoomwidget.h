/*
 * Copyright (c) 2019 Meltytech, LLC
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

#ifndef VIDEOZOOMWIDGET_H
#define VIDEOZOOMWIDGET_H

#include "sharedframe.h"

#include <QWidget>
#include <QMutex>

class VideoZoomWidget : public QWidget
{
    Q_OBJECT

public:
    struct PixelValues {
        uint8_t y;
        uint8_t u;
        uint8_t v;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    explicit VideoZoomWidget();

    // May be called from a worker thread.
    void putFrame(SharedFrame frame);

    QPoint getSelectedPixel();
    void setSelectedPixel(QPoint pixel);
    QRect getPixelRect();
    int getZoom();
    PixelValues getPixelValues(const QPoint &pixel);
    void setOffset(QPoint offset);

signals:
    void pixelSelected(const QPoint &);
    void zoomChanged(int zoom);

public slots:
    void lock(bool locked);

private:
    virtual QSize sizeHint() const Q_DECL_OVERRIDE;

    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event)Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event)Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *event)Q_DECL_OVERRIDE;

    QPoint pixelToPos(const QPoint &pixel);
    QPoint posToPixel(const QPoint &pos);
    PixelValues pixelToValues(const QPoint &pixel);

    bool m_locked;
    bool m_selectionInProgress;
    int m_zoom;
    QPoint m_imageOffset;
    QPoint m_mouseGrabPixel;
    QPoint m_selectedPixel;

    // Variables accessed from multiple threads (mutex protected)
    QMutex m_mutex;
    SharedFrame m_frame;
};

#endif // VIDEOZOOMWIDGET_H
