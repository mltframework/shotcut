/*
 * Copyright (c) 2014 Meltytech, LLC
 * Author: Brian Matherly <pez4brian@yahoo.com>
 * Author: Dan Dennedy <dan@dennedy.org>
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

class EventFilter;

class ScreenSelector : public QFrame
{
    Q_OBJECT
public:
    ScreenSelector(QWidget* parent = 0);

public slots:
    void startSelection();

signals:
    void screenSelected(QRect);
    void colorPicked(const QColor &color);

public:
    bool onMousePressEvent(QMouseEvent *event);
    bool onMouseMoveEvent(QMouseEvent *event);
    bool onMouseReleaseEvent(QMouseEvent *event);
    bool onKeyPressEvent(QKeyEvent *event);

private:
    bool m_selectionInProgress;
    QRect m_selectionRect;
    EventFilter* m_eventFilter;

    void release();

private slots:
    void grabColor();
};

class ColorPickerItem : public QObject
{
    Q_OBJECT
public:
    explicit ColorPickerItem(QObject* parent = 0);

signals:
    void pickColor();
    void colorPicked(const QColor &color);

private:
    ScreenSelector m_selector;
};

#endif // COLORPICKERITEM_H
