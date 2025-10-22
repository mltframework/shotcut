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

#ifndef SCOPECONTROLLER_H
#define SCOPECONTROLLER_H

#include "sharedframe.h"

#include <QObject>
#include <QString>

class QMainWindow;
class QMenu;
class QWidget;

/**
 * @class ScopeController
 * @brief 示波器控制器
 * 
 * 这个类负责创建和管理应用程序中所有的示波器窗口（也称为“范围”或“Scopes”）。
 * 它通过一个模板方法 `createScopeDock` 来统一创建各种音频和视频示波器的停靠窗口，
 * 并将它们的切换动作添加到菜单中。
 * 
 * 注意：此类被标记为 `Q_DECL_FINAL`，意味着它不能被继承。
 */
class ScopeController Q_DECL_FINAL : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数。
     * @param mainWindow 应用程序的主窗口，示波器停靠窗口将被添加到此窗口。
     * @param menu 示波器菜单，示波器的切换动作将被添加到此菜单。
     */
    ScopeController(QMainWindow *mainWindow, QMenu *menu);

signals:
    /**
     * @brief 当有新的视频帧可用于分析时发射此信号。
     * @param frame 包含新帧数据的 SharedFrame 对象。
     * 
     * 注意：在提供的 `.cpp` 实现中，此信号并未被发射。它可能是为未来的功能或
     * 其他组件（如 ScopeDock）与示波器 Widget 通信而预留的接口。
     */
    void newFrame(const SharedFrame &frame);

private:
    /**
     * @brief 模板函数，用于创建一个特定类型的示波器停靠窗口。
     * 
     * 这是创建示波器的核心方法。它接受一个示波器 Widget 类型作为模板参数，
     * 实例化该 Widget，将其包装在一个 `ScopeDock` 停靠窗口中，并将其添加到主窗口和菜单。
     * 
     * @tparam ScopeTYPE 要创建的示波器 Widget 的具体类型（例如 AudioWaveformScopeWidget）。
     * @param mainWindow 应用程序的主窗口。
     * @param menu 示波器菜单。
     */
    template<typename ScopeTYPE>
    void createScopeDock(QMainWindow *mainWindow, QMenu *menu);
};

#endif // SCOPECONTROLLER_H
