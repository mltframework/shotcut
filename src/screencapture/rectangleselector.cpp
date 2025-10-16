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

#include "rectangleselector.h"
#include <QDebug>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>

RectangleSelector::RectangleSelector(QWidget *parent)
    : QWidget(parent,
              Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                  | Qt::X11BypassWindowManagerHint)
    , m_selecting(false)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    // Ensure we can receive key events like Esc
    setFocusPolicy(Qt::StrongFocus);
    activateWindow();
    setFocus(Qt::ActiveWindowFocusReason);

    // Make fullscreen
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        setGeometry(screen->geometry());
    }
}

RectangleSelector::~RectangleSelector() {}

void RectangleSelector::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw semi-transparent overlay
    painter.fillRect(rect(), QColor(0, 0, 0, 100));

    if (m_selecting) {
        QRect selection = getSelectionRect();

        // Clear the selection area
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(selection, Qt::transparent);

        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        // Draw selection border
        painter.setPen(QPen(QColor(100, 150, 255), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(selection);

        // Draw corner handles
        const int handleSize = 8;
        painter.setBrush(QColor(100, 150, 255));
        painter.drawRect(selection.topLeft().x() - handleSize / 2,
                         selection.topLeft().y() - handleSize / 2,
                         handleSize,
                         handleSize);
        painter.drawRect(selection.topRight().x() - handleSize / 2,
                         selection.topRight().y() - handleSize / 2,
                         handleSize,
                         handleSize);
        painter.drawRect(selection.bottomLeft().x() - handleSize / 2,
                         selection.bottomLeft().y() - handleSize / 2,
                         handleSize,
                         handleSize);
        painter.drawRect(selection.bottomRight().x() - handleSize / 2,
                         selection.bottomRight().y() - handleSize / 2,
                         handleSize,
                         handleSize);

        // Draw dimensions text
        QString dimensions = QString("%1 x %2").arg(selection.width()).arg(selection.height());
        QFont font = painter.font();
        font.setPixelSize(14);
        painter.setFont(font);

        QRect textRect = painter.fontMetrics().boundingRect(dimensions);
        textRect.adjust(-5, -3, 5, 3);

        int textX = selection.center().x() - textRect.width() / 2;
        int textY = selection.top() - textRect.height() - 5;

        if (textY < 0) {
            textY = selection.top() + 5;
        }

        textRect.moveTo(textX, textY);

        painter.setBrush(QColor(30, 30, 30, 200));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(textRect, 3, 3);

        painter.setPen(Qt::white);
        painter.drawText(textRect, Qt::AlignCenter, dimensions);
    }

    // Draw instruction text
    if (!m_selecting) {
        QString instruction = tr("Click and drag to select an area. Press ESC to cancel.");
        QFont font = painter.font();
        font.setPixelSize(16);
        painter.setFont(font);

        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, instruction);
    }

    QWidget::paintEvent(event);
}

void RectangleSelector::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_startPoint = event->pos();
        m_currentPoint = event->pos();
        m_selecting = true;
        update();
    }
}

void RectangleSelector::mouseMoveEvent(QMouseEvent *event)
{
    if (m_selecting) {
        m_currentPoint = event->pos();
        update();
    }
}

void RectangleSelector::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_selecting) {
        QRect selection = getSelectionRect();

        if (selection.width() > 10 && selection.height() > 10) {
            emit rectangleSelected(selection);
            close();
        } else {
            m_selecting = false;
            update();
        }
    }
}

void RectangleSelector::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit canceled();
        close();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

QRect RectangleSelector::getSelectionRect() const
{
    int x = qMin(m_startPoint.x(), m_currentPoint.x());
    int y = qMin(m_startPoint.y(), m_currentPoint.y());
    int w = qAbs(m_currentPoint.x() - m_startPoint.x());
    int h = qAbs(m_currentPoint.y() - m_startPoint.y());

    return QRect(x, y, w, h);
}
