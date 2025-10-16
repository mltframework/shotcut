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

#ifndef WINDOWPICKER_H
#define WINDOWPICKER_H

#include <QList>
#include <QRect>
#include <QWidget>

class WindowPicker : public QWidget
{
    Q_OBJECT

public:
    explicit WindowPicker(QWidget *parent = nullptr);
    ~WindowPicker();

signals:
    void windowSelected(const QRect &rect);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    struct WindowInfo
    {
        QRect geometry;         // Logical coordinates for Qt display
        QRect physicalGeometry; // Physical coordinates from X11
        QString title;
        unsigned long windowId;
    };

    void detectWindows();
    int findWindowAtPosition(const QPoint &pos);
    QRect getWindowGeometry(unsigned long windowId);
    QList<WindowInfo> getX11Windows();

    QList<WindowInfo> m_windows;
    int m_highlightedWindow;
};

#endif // WINDOWPICKER_H
