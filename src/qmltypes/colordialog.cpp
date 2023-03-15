/*
 * Copyright (c) 2023 Meltytech, LLC
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

#include "colordialog.h"
#include "qmlapplication.h"

#include <QColorDialog>

ColorDialog::ColorDialog(QObject *parent)
    : QObject{parent}
{
}

void ColorDialog::open()
{
    auto color = m_color;
    QColorDialog dialog(m_color);
    if (!m_title.isEmpty())
        dialog.setWindowTitle(m_title);
    dialog.setOption(QColorDialog::ShowAlphaChannel);
    dialog.setModal(QmlApplication::dialogModality());
    if (dialog.exec() == QDialog::Accepted) {
        auto newColor = dialog.currentColor();
        auto rgb = newColor;
        auto transparent = QColor(0, 0, 0, 0);
        rgb.setAlpha(color.alpha());
        if (newColor.alpha() == 0 && (rgb != color || (newColor == transparent && color == transparent))) {
            newColor.setAlpha(255);
        }
        setSelectedColor(newColor);
        emit accepted();
    }
}

void ColorDialog::setSelectedColor(const QColor &color)
{
    if (color != m_color) {
        m_color = color;
        emit selectedColorChanged(color);
    }
}

void ColorDialog::setTitle(const QString &title)
{
    if (title != m_title) {
        m_title = title;
        emit titleChanged();
    }
}
