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

#ifndef VIDEOVECTORSCOPEWIDGET_H
#define VIDEOVECTORSCOPEWIDGET_H

#include "scopewidget.h"
#include <QMutex>
#include <QImage>

class VideoVectorScopeWidget Q_DECL_FINAL : public ScopeWidget
{
    Q_OBJECT

public:
    explicit VideoVectorScopeWidget();
    virtual ~VideoVectorScopeWidget();
    QString getTitle() Q_DECL_OVERRIDE;

private:
    enum {
        BLUE_75 = 0,
        CYAN_75,
        GREEN_75,
        YELLOW_75,
        RED_75,
        MAGENTA_75,
        BLUE_100,
        CYAN_100,
        GREEN_100,
        YELLOW_100,
        RED_100,
        MAGENTA_100,
        COLOR_POINT_COUNT,
    };

    // Called in scope thread
    void refreshScope(const QSize &size, bool full) Q_DECL_OVERRIDE;
    void drawGraticuleLines(QPainter &p, qreal lineWidth);
    void drawSkinToneLine(QPainter &p, qreal lineWidth);
    void drawGraticuleMark(QPainter &p, const QPoint &point, QColor color, qreal lineWidth,
                           qreal LineLength);

    // Called in UI thread
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    QRect getCenteredSquare();

    // Only accessed by the scope thread
    SharedFrame m_frame;
    QImage m_renderImg;
    QImage m_graticuleImg;

    // Variables accessed from multiple threads (mutex protected)
    QMutex m_mutex;
    QImage m_displayImg;
    QPoint m_points[COLOR_POINT_COUNT];
    bool m_profileChanged;

private slots:
    void profileChanged();
};

#endif // VIDEOVECTORSCOPEWIDGET_H
