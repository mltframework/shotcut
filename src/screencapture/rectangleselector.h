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

#ifndef RECTANGLESELECTOR_H
#define RECTANGLESELECTOR_H

#include <QPoint>
#include <QRect>
#include <QWidget>

class RectangleSelector : public QWidget
{
    Q_OBJECT

public:
    explicit RectangleSelector(QWidget *parent = nullptr);
    ~RectangleSelector();

signals:
    void rectangleSelected(const QRect &rect);
    void canceled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QRect getSelectionRect() const;

    QPoint m_startPoint;
    QPoint m_currentPoint;
    bool m_selecting;
};

#endif // RECTANGLESELECTOR_H
