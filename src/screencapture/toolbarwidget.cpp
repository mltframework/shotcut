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
#include <QCheckBox>
#include <QDebug>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

ScreenCaptureToolbar::ScreenCaptureToolbar(bool isRecordingMode, QWidget *parent)
    : QWidget(parent,
              Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                  | Qt::X11BypassWindowManagerHint)
    , m_isRecordingMode(isRecordingMode)
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

    // Create checkboxes
    m_minimizeCheckbox = new QCheckBox(tr("Minimize Shotcut"), this);
    m_audioCheckbox = new QCheckBox(tr("Record Audio"), this);

    // Style checkboxes
    QString checkboxStyle = "QCheckBox {"
                            "  color: white;"
                            "  font-size: 12px;"
                            "  spacing: 5px;"
                            "}"
                            "QCheckBox::indicator {"
                            "  width: 16px;"
                            "  height: 16px;"
                            "  border: 1px solid rgba(255, 255, 255, 150);"
                            "  border-radius: 3px;"
                            "  background-color: rgba(40, 40, 40, 200);"
                            "}"
                            "QCheckBox::indicator:checked {"
                            "  background-color: rgba(255, 50, 50, 200);"
                            "  border: 1px solid rgba(255, 255, 255, 200);"
                            "}"
                            "QCheckBox::indicator:hover {"
                            "  border: 1px solid rgba(255, 255, 255, 220);"
                            "}";

    m_minimizeCheckbox->setStyleSheet(checkboxStyle);
    m_audioCheckbox->setStyleSheet(checkboxStyle);
    m_minimizeCheckbox->setChecked(true); // Default to minimize
    m_audioCheckbox->setChecked(true);    // Default to record audio

    // Create layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Buttons row
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    buttonLayout->addWidget(m_fullscreenButton);
    buttonLayout->addWidget(m_rectangleButton);

    // Connect signals
    connect(m_fullscreenButton,
            &QPushButton::clicked,
            this,
            &ScreenCaptureToolbar::onFullscreenClicked);
    connect(m_rectangleButton,
            &QPushButton::clicked,
            this,
            &ScreenCaptureToolbar::onRectangleClicked);

    // Hide window button if on Wayland or Windows
#ifdef Q_OS_WIN
    const bool withWindowButton = false;
#else
    const bool withWindowButton = !ScreenCapture::isWayland();
#endif
    if (withWindowButton) {
        buttonLayout->addWidget(m_windowButton);
        connect(m_windowButton, &QPushButton::clicked, this, &ScreenCaptureToolbar::onWindowClicked);
    } else {
        m_windowButton->hide();
    }

    mainLayout->addLayout(buttonLayout);

    // Checkboxes row
    QHBoxLayout *checkboxLayout = new QHBoxLayout();
    checkboxLayout->setSpacing(15);
    checkboxLayout->addWidget(m_minimizeCheckbox);
    if (m_isRecordingMode) {
        checkboxLayout->addWidget(m_audioCheckbox);
    } else {
        m_audioCheckbox->hide();
    }
    mainLayout->addLayout(checkboxLayout);

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

ScreenCaptureToolbar::~ScreenCaptureToolbar() {}

void ScreenCaptureToolbar::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw semi-transparent background
    painter.setBrush(QColor(30, 30, 30, 220));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 10, 10);

    QWidget::paintEvent(event);
}

void ScreenCaptureToolbar::onFullscreenClicked()
{
    emit captureModeSelected(static_cast<int>(ScreenCapture::Fullscreen),
                             m_minimizeCheckbox->isChecked(),
                             m_audioCheckbox->isChecked());
    close();
}

void ScreenCaptureToolbar::onRectangleClicked()
{
    emit captureModeSelected(static_cast<int>(ScreenCapture::Rectangle),
                             m_minimizeCheckbox->isChecked(),
                             m_audioCheckbox->isChecked());
    close();
}

void ScreenCaptureToolbar::onWindowClicked()
{
    emit captureModeSelected(static_cast<int>(ScreenCapture::Window),
                             m_minimizeCheckbox->isChecked(),
                             m_audioCheckbox->isChecked());
    close();
}
