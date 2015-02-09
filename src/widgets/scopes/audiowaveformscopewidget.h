/*
 * Copyright (c) 2015 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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

#ifndef AUDIOWAVEFORMSCOPEWIDGET_H
#define AUDIOWAVEFORMSCOPEWIDGET_H

#include "scopewidget.h"
#include <MltFilter.h>
#include <QMutex>
#include <QImage>
#include <QTime>

class AudioWaveformScopeWidget Q_DECL_FINAL : public ScopeWidget
{
    Q_OBJECT
    
public:
    explicit AudioWaveformScopeWidget();
    ~AudioWaveformScopeWidget();
    QString getTitle();

private:
    void refreshScope() Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent*) Q_DECL_OVERRIDE;

    SharedFrame m_frame;
    Mlt::Filter* m_filter;
    QSize m_prevSize;
    QImage m_renderWave;
    QTime m_refreshTime;

    // Variables accessed from multiple threads (mutex protected)
    QMutex m_mutex;
    QImage m_displayWave;
    QSize m_size;
};

#endif // AUDIOWAVEFORMSCOPEWIDGET_H
