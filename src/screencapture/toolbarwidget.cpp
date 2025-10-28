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

#include "toolbarwidget.h"
#include "screencapture.h"
#include <QDebug>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QPainter>
#include <QPushButton>
#include <QScreen>

ToolbarWidget::ToolbarWidget(QWidget *parent)
    : QWidget(parent,
              Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                  | Qt::X11BypassWindowManagerHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    // Create buttons
    m_fullscreenButton = new QPushButton(tr("Fullscreen"), this);
    m_rectangleButton = new QPushButton(tr("Rectangle"), this);
    m_windowButton = new QPushButton(tr("Window"), this);

    // Style buttons
    QString buttonStyle = "QPushButton {"
                          "  background-color: rgba(255, 50, 50, 200);"
                          "  color: white;"
                          "  border: 1px solid rgba(255, 255, 255, 100);"
                          "  border-radius: 5px;"
                          "  padding: 10px 20px;"
                          "  font-size: 14px;"
                          "  min-width: 100px;"
                          "}"
                          "QPushButton:hover {"
                          "  background-color: rgba(255, 70, 70, 220);"
                          "  border: 1px solid rgba(255, 255, 255, 150);"
                          "}"
                          "QPushButton:pressed {"
                          "  background-color: rgba(255, 90, 90, 240);"
                          "}";

    m_fullscreenButton->setStyleSheet(buttonStyle);
    m_rectangleButton->setStyleSheet(buttonStyle);
    m_windowButton->setStyleSheet(buttonStyle);

    // Create layout
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);
    layout->addWidget(m_fullscreenButton);
    layout->addWidget(m_rectangleButton);

    // Connect signals
    connect(m_fullscreenButton, &QPushButton::clicked, this, &ToolbarWidget::onFullscreenClicked);
    connect(m_rectangleButton, &QPushButton::clicked, this, &ToolbarWidget::onRectangleClicked);

    // Hide window button if on Wayland or Windows
#ifdef Q_OS_WIN
    const bool withWindowButton = false;
#else
    const bool withWindowButton = !ScreenCapture::isWayland();
#endif
    if (withWindowButton) {
        layout->addWidget(m_windowButton);
        connect(m_windowButton, &QPushButton::clicked, this, &ToolbarWidget::onWindowClicked);
    } else {
        m_windowButton->hide();
    }

    // Position at top center of screen
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        adjustSize();
        int x = screenGeometry.center().x() - width() / 2;
        int y = screenGeometry.top() + 20;
        move(x, y);
    }
}

ToolbarWidget::~ToolbarWidget() {}

void ToolbarWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw semi-transparent background
    painter.setBrush(QColor(30, 30, 30, 220));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 10, 10);

    QWidget::paintEvent(event);
}

void ToolbarWidget::onFullscreenClicked()
{
    emit captureModeSelected(static_cast<int>(ScreenCapture::Fullscreen));
    close();
}

void ToolbarWidget::onRectangleClicked()
{
    emit captureModeSelected(static_cast<int>(ScreenCapture::Rectangle));
    close();
}

void ToolbarWidget::onWindowClicked()
{
    emit captureModeSelected(static_cast<int>(ScreenCapture::Window));
    close();
}
