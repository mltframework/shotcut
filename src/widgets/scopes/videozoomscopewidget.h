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

#ifndef VIDEOZOOMSCOPEWIDGET_H
#define VIDEOZOOMSCOPEWIDGET_H

#include "scopewidget.h"
#include "widgets/screenselector.h"

#include <QMutex>
#include <QPoint>
#include <QString>

class QLabel;
class QToolButton;
class VideoZoomWidget;

class VideoZoomScopeWidget Q_DECL_FINAL : public ScopeWidget
{
    Q_OBJECT

public:
    explicit VideoZoomScopeWidget();
    QString getTitle() Q_DECL_OVERRIDE;

private slots:
    void onScreenSelectStarted();
    void onLockToggled(bool enabled);
    void onScreenRectSelected(const QRect &rect);
    void onScreenPointSelected(const QPoint &point);
    void onPixelSelected(const QPoint &pixel);
    void onZoomChanged(int zoom);

private:
    // Called from the scope thread
    void refreshScope(const QSize &size, bool full) Q_DECL_OVERRIDE;

    // Called from UI thread
    Q_INVOKABLE void updateLabels();

    VideoZoomWidget *m_zoomWidget;
    ScreenSelector m_selector;
    QLabel *m_zoomLabel;
    QLabel *m_pixelXLabel;
    QLabel *m_pixelYLabel;
    QLabel *m_rLabel;
    QLabel *m_gLabel;
    QLabel *m_bLabel;
    QLabel *m_yLabel;
    QLabel *m_uLabel;
    QLabel *m_vLabel;
    QToolButton *m_lockButton;
};

#endif // VIDEOZOOMSCOPEWIDGET_H
