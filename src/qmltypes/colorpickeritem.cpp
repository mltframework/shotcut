/*
 * Copyright (c) 2014-2024 Meltytech, LLC
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

#include "colorpickeritem.h"

#include <QApplication>
#include <QGuiApplication>
#include <QImage>
#include <QScreen>
#include <QTimer>

ColorPickerItem::ColorPickerItem(QObject *parent)
    : QObject(parent)
{
    connect(this, SIGNAL(pickColor()), &m_selector, SLOT(startSelection()));
    connect(&m_selector, SIGNAL(screenSelected(const QRect &)), this,
            SLOT(screenSelected(const QRect &)));
    connect(&m_selector, SIGNAL(cancelled()), SIGNAL(cancelled()));
}

void ColorPickerItem::screenSelected(const QRect &rect)
{
    m_selectedRect = rect;
    // Give the frame buffer time to clear the selector window before
    // grabbing the color.
    QTimer::singleShot(200, this, SLOT(grabColor()));
}

void ColorPickerItem::grabColor()
{
    QScreen *screen = QGuiApplication::screenAt(m_selectedRect.topLeft());
    QPixmap screenGrab = screen->grabWindow(0, m_selectedRect.x(), m_selectedRect.y(),
                                            m_selectedRect.width(), m_selectedRect.height());
    QImage image = screenGrab.toImage();
    int numPixel = qMax(image.width() * image.height(), 1);
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
