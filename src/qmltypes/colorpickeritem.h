/*
 * Copyright (c) 2014-2025 Meltytech, LLC
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

#ifndef COLORPICKERITEM_H
#define COLORPICKERITEM_H

#include "widgets/screenselector.h"

#include <QColor>
#include <QObject>

class ColorPickerItem : public QObject
{
    Q_OBJECT
public:
    explicit ColorPickerItem(QObject *parent = 0);

signals:
    void pickColor(QPoint initialPos = QPoint(-1, -1));
    void colorPicked(const QColor &color);
    void cancelled();

private slots:
    void screenSelected(const QRect &rect);
    void grabColor();
#ifdef Q_OS_LINUX
    void grabColorDBus();
    void gotColorResponse(uint response, const QVariantMap &results);
#endif

private:
    ScreenSelector m_selector;
    QRect m_selectedRect;
};

#endif // COLORPICKERITEM_H
