/*
 * Copyright (c) 2014-2025 Meltytech, LLC
 * Inspiration: KDENLIVE colorpickerwidget.cpp by Till Theato (root@ttill.de)
 * Inspiration: QColorDialog.cpp
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

#include "screenselector.h"

#include "mainwindow.h"

#include <QApplication>
#include <QMouseEvent>

ScreenSelector::ScreenSelector(QWidget *parent)
    : QFrame(parent)
    , m_selectionInProgress(false)
    , m_selectionRect(-1, -1, -1, -1)
    , m_selectionPoint(-1, -1)
    , m_fixedSize(-1, -1)
    , m_boundingRect(-1, -1, -1, -1)
    , m_useDBus(false)
{
    setFrameStyle(QFrame::Box | QFrame::Plain);
    setWindowOpacity(0.5);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    hide();
    setCursor(Qt::CrossCursor);
}

void ScreenSelector::setFixedSize(const QSize &size)
{
    m_fixedSize = size;
}

void ScreenSelector::setBoundingRect(const QRect &rect)
{
    m_boundingRect = rect;
}

void ScreenSelector::setSelectedRect(const QRect &rect)
{
    m_selectionRect = rect;
    lockGeometry(m_selectionRect.normalized());
}

void ScreenSelector::startSelection(QPoint initialPos)
{
#ifdef Q_OS_LINUX
    const auto p = MAIN.geometry().center();
    const auto id = MAIN.window()->winId();
    for (auto screen : QGuiApplication::screens()) {
        if (screen->geometry().contains(p)) {
            m_useDBus = screen->grabWindow(id, p.x(), p.y(), 1, 1).isNull();
            if (m_useDBus) {
                emit screenSelected(m_selectionRect);
                return;
            }
            break;
        }
    }
#endif
    m_selectionInProgress = false;
    grabMouse();
    grabKeyboard();
    MAIN.setCursor(Qt::CrossCursor);

    if (initialPos.x() > -1) {
        m_selectionPoint = initialPos;
    } else {
        m_selectionPoint = QCursor::pos();
    }
    QCursor::setPos(m_selectionPoint);

    if (m_fixedSize.width() > -1) {
        m_selectionRect.setSize(m_fixedSize);
        m_selectionInProgress = true;
    }

    if (m_selectionInProgress) {
        lockGeometry(m_selectionRect.normalized());
        show();
    }

    QApplication::instance()->installEventFilter(this);
}

bool ScreenSelector::eventFilter(QObject *, QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        return onMousePressEvent(static_cast<QMouseEvent *>(event));
    case QEvent::MouseMove:
        return onMouseMoveEvent(static_cast<QMouseEvent *>(event));
    case QEvent::MouseButtonRelease:
        return onMouseReleaseEvent(static_cast<QMouseEvent *>(event));
    case QEvent::KeyPress:
        return onKeyPressEvent(static_cast<QKeyEvent *>(event));
    default:
        break;
    }
    return false;
}

bool ScreenSelector::onMousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_selectionInProgress) {
        m_selectionInProgress = true;
        show();
        m_selectionRect = QRect(event->globalPosition().toPoint(), QSize(1, 1));
        lockGeometry(m_selectionRect.normalized());
    }
    return true;
}

bool ScreenSelector::onMouseMoveEvent(QMouseEvent *event)
{
    if (m_boundingRect.x() > -1 && !m_boundingRect.contains(event->globalPosition().toPoint())) {
        int x = qBound(m_boundingRect.left(),
                       qRound(event->globalPosition().x()),
                       m_boundingRect.right());
        int y = qBound(m_boundingRect.top(),
                       qRound(event->globalPosition().y()),
                       m_boundingRect.bottom());
        QCursor::setPos(x, y);
        return true;
    }

    if (m_selectionInProgress) {
        if (m_fixedSize.width() > -1) {
            // Center the selection around the cursor
            int x = qRound(event->globalPosition().x()) - m_fixedSize.width() / 2;
            int y = qRound(event->globalPosition().y()) - m_fixedSize.height() / 2;
            if (m_boundingRect.x() > -1) {
                x = qBound(m_boundingRect.left(), x, m_boundingRect.right() - m_fixedSize.width());
                y = qBound(m_boundingRect.top(), y, m_boundingRect.bottom() - m_fixedSize.height());
            }
            m_selectionRect = QRect(QPoint(x, y), m_fixedSize);
            m_selectionPoint = event->globalPosition().toPoint();
            emit screenSelected(m_selectionRect);
            emit pointSelected(m_selectionPoint);
        } else {
            m_selectionRect.setWidth(qRound(event->globalPosition().x()) - m_selectionRect.x());
            m_selectionRect.setHeight(qRound(event->globalPosition().y()) - m_selectionRect.y());

            if (m_selectionRect.width() == 0) {
                m_selectionRect.setWidth(1);
            }
            if (m_selectionRect.height() == 0) {
                m_selectionRect.setHeight(1);
            }
        }
        lockGeometry(m_selectionRect.normalized());
    }
    return true;
}

bool ScreenSelector::onMouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_selectionInProgress == true) {
        release();
        emit screenSelected(m_selectionRect);
    }
    return true;
}

bool ScreenSelector::onKeyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        release();
        emit cancelled();
    }
    event->accept();
    return true;
}

void ScreenSelector::lockGeometry(const QRect &rect)
{
    setGeometry(rect);
    setMinimumSize(rect.size());
    setMaximumSize(rect.size());
}

void ScreenSelector::release()
{
    QApplication::instance()->removeEventFilter(this);
    releaseMouse();
    releaseKeyboard();
    MAIN.setCursor(Qt::ArrowCursor);
    m_selectionInProgress = false;
    hide();
}
