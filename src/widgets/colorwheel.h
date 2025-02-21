/*
 * Copyright (c) 2013 Meltytech, LLC
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

#ifndef COLORWHEEL_H
#define COLORWHEEL_H

#include <QPainter>
#include <QResizeEvent>
#include <QWidget>

class ColorWheel : public QWidget
{
    Q_OBJECT
public:
    explicit ColorWheel(QWidget *parent = 0);

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;
    QColor color();
    void setColor(const QColor &color);

signals:
    void colorChanged(const QColor &color);

public slots:
    void changeColor(const QColor &color);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);

private:
    QSize m_initialSize;
    QImage m_image;
    bool m_isMouseDown;
    QPoint m_lastPoint;
    int m_margin;
    int m_sliderWidth;
    QRegion m_wheelRegion;
    QRegion m_sliderRegion;
    QColor m_color;
    bool m_isInWheel;
    bool m_isInSquare;

    int wheelSize() const;
    QColor colorForPoint(const QPoint &point);
    void drawWheel();
    void drawWheelDot(QPainter &painter);
    void drawSliderBar(QPainter &painter);
    void drawSlider();
};

#endif // COLORWHEEL_H
