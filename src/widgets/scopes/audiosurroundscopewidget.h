/*
 * Copyright (c) 2024 Meltytech, LLC
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

#ifndef AUDIOSURROUNDSCOPEWIDGET_H
#define AUDIOSURROUNDSCOPEWIDGET_H

#include "scopewidget.h"

#include <QImage>
#include <QMutex>

class AudioSurroundScopeWidget Q_DECL_FINAL : public ScopeWidget
{
    Q_OBJECT

public:
    explicit AudioSurroundScopeWidget();
    virtual ~AudioSurroundScopeWidget();
    QString getTitle() Q_DECL_OVERRIDE;

private:
    // Called in scope thread
    void refreshScope(const QSize &size, bool full) Q_DECL_OVERRIDE;
    void drawGraticule(QPainter &p, qreal lineWidth);

    // Called in UI thread
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

    // Only accessed by the scope thread
    SharedFrame m_frame;
    QImage m_renderImg;
    QImage m_graticuleImg;

    // Variables accessed from multiple threads (mutex protected)
    QMutex m_mutex;
    QImage m_displayImg;
    bool m_channelsChanged;
    int m_channels;
};

#endif // AUDIOSURROUNDSCOPEWIDGET_H
