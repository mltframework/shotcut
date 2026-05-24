/*
 * Copyright (c) 2019-2026 Meltytech, LLC
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

#pragma once

#include <cstdint>

void removeMacosTabBar();
void macosHideFromDock();
void macosSetDockProgress(int percent);
void macosPauseDockProgress(int percent);
void macosResetDockProgress();
void macosFinishDockProgress(bool isSuccess, bool stopped);

/// Override NSScreen.maximumExtendedDynamicRangeColorComponentValue to return
/// the *potential* headroom.  This breaks the chicken-and-egg where Qt's shader
/// won't output > 1.0 because headroom=1, and macOS won't allocate headroom
/// because no content > 1.0 is being rendered.  Once the shader outputs HDR
/// values, macOS allocates real headroom and the override becomes a no-op.
/// Safe: only affects the HDR preview window's video shader (main window uses
/// SDR swapchain format, so Qt's video node ignores hdrInfo for it).
void macosOverrideEdrHeadroom(bool enable);

/// Query the current EDR headroom for the screen hosting the given window.
/// Returns NSScreen.maximumExtendedDynamicRangeColorComponentValue.
/// @param windowId  QWindow::winId()
float macosCurrentEdrHeadroom(uintptr_t windowId);

/// Query the potential (maximum) EDR headroom.
/// Returns NSScreen.maximumPotentialExtendedDynamicRangeColorComponentValue.
float macosPotentialEdrHeadroom(uintptr_t windowId);

/// Query the reference EDR headroom.
/// Returns NSScreen.maximumReferenceExtendedDynamicRangeColorComponentValue.
float macosReferenceEdrHeadroom(uintptr_t windowId);
