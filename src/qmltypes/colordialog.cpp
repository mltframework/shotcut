/*
 * Copyright (c) 2023-2025 Meltytech, LLC
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
#include "settings.h"
#include "util.h"

#include <QColorDialog>

ColorDialog::ColorDialog(QObject *parent)
    : QObject{parent}
{}

QColor ColorDialog::getColor(const QColor &initial,
                             QWidget *parent,
                             const QString &title,
                             bool showAlpha)
{
    auto flags = Util::getColorDialogOptions();
    if (showAlpha) {
        flags |= QColorDialog::ShowAlphaChannel;
    }

    auto color = initial;
    auto newColor = QColorDialog::getColor(color, parent, title, flags);

    // Save custom colors to settings after dialog closes
    Settings.saveCustomColors();

    if (newColor.isValid() && showAlpha) {
        auto rgb = newColor;
        auto transparent = QColor(0, 0, 0, 0);
        rgb.setAlpha(color.alpha());
        if (newColor.alpha() == 0
            && (rgb != color || (newColor == transparent && color == transparent))) {
            newColor.setAlpha(255);
        }
    }

    return newColor;
}

void ColorDialog::open()
{
    auto newColor = getColor(m_color, nullptr, m_title, m_showAlpha);

    if (newColor.isValid()) {
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

void ColorDialog::setShowAlpha(bool show)
{
    if (show != m_showAlpha) {
        m_showAlpha = show;
        emit showAlphaChanged();
    }
}
