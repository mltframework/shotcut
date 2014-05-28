/*
 * Copyright (c) 2014 Meltytech, LLC
 * Author: Brian Matherly <pez4brian@yahoo.com>
 * Inspiration: KDENLIVE colorpickerwidget.cpp by Till Theato (root@ttill.de)
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

#include "colorpickeritem.h"
#include <QTimer>
#include <QIcon>
#include <QMouseEvent>
#include <QApplication>
#include <QGuiApplication>
#include <QDesktopWidget>
#include <QImage>
#include <QScreen>

ScreenSelector::ScreenSelector(QWidget* parent)
    : QFrame(parent)
    , m_selectionInProgress(false)
    , m_SelectionRect()
{
    setFrameStyle(QFrame::Box | QFrame::Plain);
    setWindowOpacity(0.5);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    hide();
}

void ScreenSelector::startSelection()
{
    m_selectionInProgress = false;
    grabMouse(QCursor(QIcon::fromTheme("color-picker", QIcon(":/icons/oxygen/32x32/actions/color-picker.png")).pixmap(22, 22), 0, 21));
    grabKeyboard();
}

void ScreenSelector::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_selectionInProgress) {
        m_selectionInProgress = true;
        show();
        m_SelectionRect = QRect(event->globalPos(), QSize(1,1));
        setGeometry(m_SelectionRect);
    }
    QFrame::mousePressEvent(event);
}

void ScreenSelector::mouseMoveEvent(QMouseEvent *event)
{
    if (m_selectionInProgress) {
        m_SelectionRect.setWidth(event->globalX() - m_SelectionRect.x());
        m_SelectionRect.setHeight(event->globalY() - m_SelectionRect.y());

        if (m_SelectionRect.width() == 0) {
            m_SelectionRect.setWidth(1);
        }
        if (m_SelectionRect.height() == 0) {
            m_SelectionRect.setHeight(1);
        }
        setGeometry(m_SelectionRect.normalized());
    }
    QFrame::mouseMoveEvent(event);
}

void ScreenSelector::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton && m_selectionInProgress == true ) {
        releaseMouse();
        releaseKeyboard();
        m_selectionInProgress = false;
        hide();
        // Give the frame buffer time to clear the selector window before
        // signaling the selection.
        QTimer::singleShot(100, this, SLOT(screenRefreshed()));
    }
    QFrame::mouseReleaseEvent(event);
}

void ScreenSelector::screenRefreshed()
{
    emit screenSelected(m_SelectionRect);
}

ColorPickerItem::ColorPickerItem(QObject* parent)
    : QObject(parent)
    , m_selector(NULL)
{
    m_selector = new ScreenSelector(0);
    connect(m_selector, SIGNAL(screenSelected(QRect)), this, SLOT(slotScreenSelected(QRect)));
}

ColorPickerItem::~ColorPickerItem()
{
    delete m_selector;
}

void ColorPickerItem::pickColor()
{
    m_selector->startSelection();
}

void ColorPickerItem::slotScreenSelected(QRect pickerRect)
{
    pickerRect = pickerRect.normalized();

    QDesktopWidget* desktop = QApplication::desktop();
    int screenNum = desktop->screenNumber(pickerRect.topLeft());
    QScreen* screen = QGuiApplication::screens()[screenNum];
    QPixmap screenGrab = screen->grabWindow(desktop->winId(), pickerRect.x(), pickerRect.y(), pickerRect.width(), pickerRect.height());
    QImage image = screenGrab.toImage();
    int numPixel = image.width() * image.height();
    int sumR = 0;
    int sumG = 0;
    int sumB = 0;

    for (int x = 0; x < image.width(); ++x) {
        for (int y = 0; y < image.height(); ++y) {
            QColor color = image.pixel(x, y);
            sumR += color.red();
            sumG += color.green();
            sumB += color.blue();
        }
    }

    QColor avgColor(sumR / numPixel, sumG / numPixel, sumB / numPixel);
    emit colorPicked(avgColor);
}
