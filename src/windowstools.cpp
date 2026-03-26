/*
 * Copyright (c) 2012-2026 Meltytech, LLC
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

#include "windowstools.h"

#include <shobjidl.h>

WindowsTaskbarButton::WindowsTaskbarButton() {}

WindowsTaskbarButton &WindowsTaskbarButton::getInstance()
{
    static WindowsTaskbarButton *instance = nullptr;
    if (!instance)
        instance = new WindowsTaskbarButton();
    return *instance;
}

static ITaskbarList3 *taskbarListInstance()
{
    static ITaskbarList3 *s_taskbar = nullptr;
    if (!s_taskbar) {
        CoCreateInstance(CLSID_TaskbarList,
                         nullptr,
                         CLSCTX_INPROC_SERVER,
                         IID_ITaskbarList3,
                         reinterpret_cast<void **>(&s_taskbar));
        if (s_taskbar)
            s_taskbar->HrInit();
    }
    return s_taskbar;
}

void WindowsTaskbarButton::setParentWindow(QWindow *window)
{
    m_window = window;
}

void WindowsTaskbarButton::setProgress(int progress)
{
    if (!m_window)
        return;
    ITaskbarList3 *tb = taskbarListInstance();
    if (tb) {
        HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
        tb->SetProgressState(hwnd, TBPF_NORMAL);
        tb->SetProgressValue(hwnd, progress, 100);
    }
}

void WindowsTaskbarButton::pauseProgress(int progress)
{
    if (!m_window)
        return;
    ITaskbarList3 *tb = taskbarListInstance();
    if (tb) {
        HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
        tb->SetProgressValue(hwnd, progress, 100);
        tb->SetProgressState(hwnd, TBPF_PAUSED);
    }
}

void WindowsTaskbarButton::resetProgress()
{
    if (!m_window)
        return;
    ITaskbarList3 *tb = taskbarListInstance();
    if (tb) {
        HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
        tb->SetProgressState(hwnd, TBPF_NOPROGRESS);
    }
}

void WindowsTaskbarButton::finishProgress(bool isSuccess, bool stopped)
{
    if (!m_window)
        return;
    ITaskbarList3 *tb = taskbarListInstance();
    if (!tb)
        return;
    HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
    if (isSuccess) {
        tb->SetProgressValue(hwnd, 100, 100);
        tb->SetProgressState(hwnd, TBPF_NORMAL);
        FLASHWINFO fi{};
        fi.cbSize = sizeof(fi);
        fi.hwnd = hwnd;
        fi.dwFlags = FLASHW_TRAY;
        fi.uCount = 3;
        fi.dwTimeout = 500;
        FlashWindowEx(&fi);
    } else if (stopped) {
        tb->SetProgressState(hwnd, TBPF_PAUSED);
    } else {
        tb->SetProgressValue(hwnd, 100, 100);
        tb->SetProgressState(hwnd, TBPF_ERROR);
    }
}
