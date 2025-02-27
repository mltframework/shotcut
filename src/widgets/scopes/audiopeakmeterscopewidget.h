/*
 * Copyright (c) 2015-2022 Meltytech, LLC
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

#ifndef AUDIOPEAKMETERSCOPEWIDGET_H
#define AUDIOPEAKMETERSCOPEWIDGET_H

#include "scopewidget.h"

#include <QImage>
#include <QMutex>
#include <QVector>

class AudioMeterWidget;

class AudioPeakMeterScopeWidget Q_DECL_FINAL : public ScopeWidget
{
    Q_OBJECT

public:
    explicit AudioPeakMeterScopeWidget();
    QString getTitle() Q_DECL_OVERRIDE;
    void setOrientation(Qt::Orientation orientation) Q_DECL_OVERRIDE;

private:
    // Functions run in scope thread.
    void refreshScope(const QSize &size, bool full) Q_DECL_OVERRIDE;

    // Members accessed by GUI thread.
    AudioMeterWidget *m_audioMeter;
    Qt::Orientation m_orientation;
    int m_channels;

private slots:
    void reconfigureMeter();
};

#endif // AUDIOPEAKMETERSCOPEWIDGET_H
