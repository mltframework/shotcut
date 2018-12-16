/*
 * Copyright (c) 2015-2016 Meltytech, LLC
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
#include "scopecontroller.h"
#include "widgets/scopes/audioloudnessscopewidget.h"
#include "widgets/scopes/audiopeakmeterscopewidget.h"
#include "widgets/scopes/audiospectrumscopewidget.h"
#include "widgets/scopes/audiowaveformscopewidget.h"
#include "widgets/scopes/videohistogramscopewidget.h"
#include "widgets/scopes/videowaveformscopewidget.h"
#include "docks/scopedock.h"
#include "settings.h"
#include <Logger.h>
#include <QMainWindow>
#include <QMenu>

ScopeController::ScopeController(QMainWindow* mainWindow, QMenu* menu)
  : QObject(mainWindow)
{
    LOG_DEBUG() << "begin";
    QMenu* scopeMenu = menu->addMenu(tr("Scopes"));
    createScopeDock<AudioLoudnessScopeWidget>(mainWindow, scopeMenu);
    createScopeDock<AudioPeakMeterScopeWidget>(mainWindow, scopeMenu);
    createScopeDock<AudioSpectrumScopeWidget>(mainWindow, scopeMenu);
    createScopeDock<AudioWaveformScopeWidget>(mainWindow, scopeMenu);
    if (!Settings.playerGPU()) {
        createScopeDock<VideoHistogramScopeWidget>(mainWindow, scopeMenu);
        createScopeDock<VideoWaveformScopeWidget>(mainWindow, scopeMenu);
    }
    LOG_DEBUG() << "end";
}

template<typename ScopeTYPE> void ScopeController::createScopeDock(QMainWindow* mainWindow, QMenu* menu)
{
    ScopeWidget* scopeWidget = new ScopeTYPE();
    ScopeDock* scopeDock = new ScopeDock(this, scopeWidget);
    scopeDock->hide();
    menu->addAction(scopeDock->toggleViewAction());
    mainWindow->addDockWidget(Qt::RightDockWidgetArea, scopeDock);
}

