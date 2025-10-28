/*
 * Copyright (c) 2015-2024 Meltytech, LLC
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

#include "Logger.h"
#include "docks/scopedock.h"
#include "widgets/scopes/audioloudnessscopewidget.h"
#include "widgets/scopes/audiopeakmeterscopewidget.h"
#include "widgets/scopes/audiospectrumscopewidget.h"
#include "widgets/scopes/audiosurroundscopewidget.h"
#include "widgets/scopes/audiovectorscopewidget.h"
#include "widgets/scopes/audiowaveformscopewidget.h"
#include "widgets/scopes/videohistogramscopewidget.h"
#include "widgets/scopes/videorgbparadescopewidget.h"
#include "widgets/scopes/videorgbwaveformscopewidget.h"
#include "widgets/scopes/videovectorscopewidget.h"
#include "widgets/scopes/videowaveformscopewidget.h"
#include "widgets/scopes/videozoomscopewidget.h"

#include <QMainWindow>
#include <QMenu>

/**
 * @class ScopeController
 * @brief 示波器控制器
 * 
 * 这个类负责创建和管理应用程序中所有的示波器窗口（也称为“范围”或“Scopes”）。
 * 它通过模板方法 `createScopeDock` 来统一创建各种音频和视频示波器的停靠窗口，
 * 并将它们的切换动作添加到菜单中。
 */

ScopeController::ScopeController(QMainWindow *mainWindow, QMenu *menu)
    : QObject(mainWindow)
{
    LOG_DEBUG() << "begin";
    // 在指定的菜单中添加一个名为“Scopes”（示波器）的子菜单
    QMenu *scopeMenu = menu->addMenu(tr("Scopes"));
    
    // 使用模板方法创建所有音频示波器的停靠窗口
    createScopeDock<AudioLoudnessScopeWidget>(mainWindow, scopeMenu);     // 音频响度
    createScopeDock<AudioPeakMeterScopeWidget>(mainWindow, scopeMenu);    // 音频峰值表
    createScopeDock<AudioSpectrumScopeWidget>(mainWindow, scopeMenu);     // 音频频谱
    createScopeDock<AudioSurroundScopeWidget>(mainWindow, scopeMenu);     // 环绕声
    createScopeDock<AudioVectorScopeWidget>(mainWindow, scopeMenu);       // 音频矢量图（相位）
    createScopeDock<AudioWaveformScopeWidget>(mainWindow, scopeMenu);     // 音频波形

    // 使用模板方法创建所有视频示波器的停靠窗口
    createScopeDock<VideoHistogramScopeWidget>(mainWindow, scopeMenu);    // 视频直方图
    createScopeDock<VideoRgbParadeScopeWidget>(mainWindow, scopeMenu);    // RGB 分量示波器
    createScopeDock<VideoRgbWaveformScopeWidget>(mainWindow, scopeMenu);  // RGB 波形
    createScopeDock<VideoVectorScopeWidget>(mainWindow, scopeMenu);       // 视频矢量图（色度）
    createScopeDock<VideoWaveformScopeWidget>(mainWindow, scopeMenu);     // 视频波形（亮度）
    createScopeDock<VideoZoomScopeWidget>(mainWindow, scopeMenu);         // 视频放大镜
    LOG_DEBUG() << "end";
}

/**
 * @brief 模板函数，用于创建一个特定类型的示波器停靠窗口。
 * 
 * 这个模板方法是创建示波器的核心。它接受一个示波器 Widget 类型作为模板参数，
 * 实例化该 Widget，将其包装在一个 `ScopeDock` 停靠窗口中，并将其添加到主窗口和菜单。
 * 
 * @tparam ScopeTYPE 要创建的示波器 Widget 的具体类型（例如 AudioWaveformScopeWidget）。
 * @param mainWindow 应用程序的主窗口，停靠窗口将被添加到此窗口。
 * @param menu 示波器菜单，停靠窗口的切换动作将被添加到此菜单。
 */
template<typename ScopeTYPE>
void ScopeController::createScopeDock(QMainWindow *mainWindow, QMenu *menu)
{
    // 1. 创建指定类型的示波器 Widget 实例。
    ScopeWidget *scopeWidget = new ScopeTYPE();
    
    // 2. 创建一个停靠窗口，并将示波器 Widget 作为其中心部件。
    ScopeDock *scopeDock = new ScopeDock(this, scopeWidget);
    
    // 3. 初始时隐藏停靠窗口。
    scopeDock->hide();
    
    // 4. 将停靠窗口的切换动作添加到菜单中。用户可以通过此菜单项显示/隐藏示波器。
    menu->addAction(scopeDock->toggleViewAction());
    
    // 5. 将停靠窗口添加到主窗口的右侧停靠区域。
    mainWindow->addDockWidget(Qt::RightDockWidgetArea, scopeDock);
}
