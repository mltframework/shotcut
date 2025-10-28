/*
 * Copyright (c) 2015-2016 Meltytech, LLC
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

#ifndef _TIMELINEITEMS_H
#define _TIMELINEITEMS_H

// 【功能】：注册时间轴相关项目到QML引擎
// 【说明】：此函数通常在应用程序启动时调用，用于向QML系统注册自定义的时间轴组件
// 【作用】：使C++实现的时间轴项（如剪辑、转场、轨道等）可以在QML界面中使用
// 【调用时机】：应用程序初始化阶段，QML引擎创建之后
void registerTimelineItems();

#endif
