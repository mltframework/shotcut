/*
 * Copyright (c) 2025 Meltytech, LLC
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

#include "windowpicker.h"
#include <QDebug>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
// clang-format off
extern "C" {
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
}
// clang-format on
#endif

WindowPicker::WindowPicker(QWidget *parent)
    : QWidget(parent,
              Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                  | Qt::X11BypassWindowManagerHint)
    , m_highlightedWindow(-1)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);

    // Make fullscreen
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        setGeometry(screen->geometry());
    }

    detectWindows();
}

WindowPicker::~WindowPicker() {}

void WindowPicker::detectWindows()
{
    m_windows.clear();

    // Try X11 window detection
    if (QGuiApplication::platformName() == "xcb") {
        m_windows = getX11Windows();
    }

    qDebug() << "Found" << m_windows.size() << "windows";
}

QList<WindowPicker::WindowInfo> WindowPicker::getX11Windows()
{
    QList<WindowInfo> windows;

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    Display *display = XOpenDisplay(nullptr);
    if (!display) {
        qWarning() << "Could not open X display";
        return windows;
    }

    Window root = DefaultRootWindow(display);
    Window parent;
    Window *children;
    unsigned int nChildren;

    // Get the atom for window state
    Atom wmStateAtom = XInternAtom(display, "_NET_WM_STATE", False);
    Atom wmStateHiddenAtom = XInternAtom(display, "_NET_WM_STATE_HIDDEN", False);

    if (XQueryTree(display, root, &root, &parent, &children, &nChildren)) {
        for (unsigned int i = 0; i < nChildren; i++) {
            XWindowAttributes attrs;
            if (XGetWindowAttributes(display, children[i], &attrs)) {
                // Skip invisible, unmapped, or override-redirect windows
                if (attrs.map_state != IsViewable || attrs.override_redirect) {
                    continue;
                }

                // Check if window is minimized/hidden
                Atom actualType;
                int actualFormat;
                unsigned long numItems, bytesAfter;
                unsigned char *data = nullptr;

                bool isMinimized = false;
                if (XGetWindowProperty(display,
                                       children[i],
                                       wmStateAtom,
                                       0,
                                       (~0L),
                                       False,
                                       XA_ATOM,
                                       &actualType,
                                       &actualFormat,
                                       &numItems,
                                       &bytesAfter,
                                       &data)
                        == Success
                    && data) {
                    Atom *states = (Atom *) data;
                    for (unsigned long j = 0; j < numItems; j++) {
                        if (states[j] == wmStateHiddenAtom) {
                            isMinimized = true;
                            break;
                        }
                    }
                    XFree(data);
                }

                // Skip minimized windows
                if (isMinimized) {
                    continue;
                }

                // Skip our own window and very small windows
                if (attrs.width < 50 || attrs.height < 50) {
                    continue;
                }

                // Get window title
                char *windowName = nullptr;
                XFetchName(display, children[i], &windowName);

                WindowInfo info;
                info.windowId = children[i];
                // Store X11 physical coordinates
                info.physicalGeometry = QRect(attrs.x, attrs.y, attrs.width, attrs.height);
                info.geometry = info.physicalGeometry; // Will be converted below if needed
                info.title = windowName ? QString::fromUtf8(windowName) : QString();

                if (windowName) {
                    XFree(windowName);
                }

                windows.append(info);
            }
        }

        if (children) {
            XFree(children);
        }
    }

    XCloseDisplay(display);
#endif

    // Convert X11 physical coordinates to Qt logical coordinates for display
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        qreal dpr = screen->devicePixelRatio();
        if (!qFuzzyCompare(dpr, 1.0)) {
            for (WindowInfo &window : windows) {
                QRect physical = window.geometry;
                window.geometry = QRect(qRound(physical.x() / dpr),
                                        qRound(physical.y() / dpr),
                                        qRound(physical.width() / dpr),
                                        qRound(physical.height() / dpr));
                qDebug() << "Window:" << window.title << "Physical:" << physical
                         << "Logical:" << window.geometry << "DPR:" << dpr;
            }
        }
    }

    return windows;
}

void WindowPicker::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw semi-transparent overlay
    painter.fillRect(rect(), QColor(0, 0, 0, 100));

    // Highlight the window under cursor
    if (m_highlightedWindow >= 0 && m_highlightedWindow < m_windows.size()) {
        const WindowInfo &window = m_windows[m_highlightedWindow];

        // Clear the window area
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(window.geometry, Qt::transparent);

        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        // Draw highlight border
        painter.setPen(QPen(QColor(100, 150, 255), 3));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(window.geometry);

        // Draw window title
        if (!window.title.isEmpty()) {
            QFont font = painter.font();
            font.setPixelSize(14);
            painter.setFont(font);

            QRect textRect = painter.fontMetrics().boundingRect(window.title);
            textRect.adjust(-5, -3, 5, 3);

            int textX = window.geometry.center().x() - textRect.width() / 2;
            int textY = window.geometry.top() - textRect.height() - 5;

            if (textY < 0) {
                textY = window.geometry.top() + 5;
            }

            textRect.moveTo(textX, textY);

            painter.setBrush(QColor(30, 30, 30, 200));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(textRect, 3, 3);

            painter.setPen(Qt::white);
            painter.drawText(textRect, Qt::AlignCenter, window.title);
        }
    }

    // Draw instruction text
    QString instruction(tr("Click on a window to select it"));
    QFont font = painter.font();
    font.setPixelSize(16);
    painter.setFont(font);

    painter.setPen(Qt::white);
    QRect textRect = rect();
    textRect.setTop(20);
    painter.drawText(textRect, Qt::AlignTop | Qt::AlignHCenter, instruction);

    QWidget::paintEvent(event);
}

void WindowPicker::mouseMoveEvent(QMouseEvent *event)
{
    int oldHighlighted = m_highlightedWindow;
    m_highlightedWindow = findWindowAtPosition(event->pos());

    if (oldHighlighted != m_highlightedWindow) {
        update();
    }
}

void WindowPicker::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_highlightedWindow >= 0 && m_highlightedWindow < m_windows.size()) {
            // Emit physical coordinates for capture
            emit windowSelected(m_windows[m_highlightedWindow].physicalGeometry);
            close();
        }
    }
}

int WindowPicker::findWindowAtPosition(const QPoint &pos)
{
    // Find the topmost window at this position
    for (int i = 0; i < m_windows.size(); ++i) {
        if (m_windows[i].geometry.contains(pos)) {
            return i;
        }
    }
    return -1;
}
