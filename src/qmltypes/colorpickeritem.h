/*
 * Copyright (c) 2014 Meltytech, LLC
 * Author: Brian Matherly <pez4brian@yahoo.com>
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

#include <QObject>
#include <QColor>
#include <QFrame>

class ScreenSelector;

class ColorPickerItem : public QObject
{
    Q_OBJECT
public:
    explicit ColorPickerItem(QObject* parent = 0);
    virtual ~ColorPickerItem();
    Q_INVOKABLE void pickColor();

signals:
    void colorPicked(const QColor &color);

private:
    ScreenSelector* m_selector;

private slots:
    void slotScreenSelected(QRect);
};

class ScreenSelector : public QFrame
{
    Q_OBJECT
public:
    ScreenSelector(QWidget* parent = 0);
    void startSelection();

signals:
    void screenSelected(QRect);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    bool m_selectionInProgress;
    QRect m_SelectionRect;

private slots:
    void screenRefreshed();
};

#endif // COLORPICKERITEM_H
