/*
 * Copyright (c) 2013-2018 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 * Author: Brian Matherly <code@brianmatherly.com>
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

#ifndef COLORWHEELITEM_H
#define COLORWHEELITEM_H

#include <QImage>
#include <QQuickPaintedItem>

class ColorWheelItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(int red READ red WRITE setRed)
    Q_PROPERTY(int green READ green WRITE setGreen)
    Q_PROPERTY(int blue READ blue WRITE setBlue)
    Q_PROPERTY(qreal redF READ redF WRITE setRedF)
    Q_PROPERTY(qreal greenF READ greenF WRITE setGreenF)
    Q_PROPERTY(qreal blueF READ blueF WRITE setBlueF)
    Q_PROPERTY(qreal step READ step WRITE setStep)
public:
    explicit ColorWheelItem(QQuickItem *parent = 0);
    QColor color();
    void setColor(const QColor &color);
    int red();
    void setRed(int red);
    int green();
    void setGreen(int green);
    int blue();
    void setBlue(int blue);
    qreal redF();
    void setRedF(qreal red);
    qreal greenF();
    void setGreenF(qreal green);
    qreal blueF();
    void setBlueF(qreal blue);
    qreal step();
    void setStep(qreal blue);

signals:
    void colorChanged(const QColor &color);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void hoverMoveEvent(QHoverEvent *event);
    void wheelEvent(QWheelEvent *event);
    void paint(QPainter *painter);

private:
    QImage m_image;
    bool m_isMouseDown;
    QPoint m_lastPoint;
    QSize m_size;
    int m_margin;
    QRegion m_wheelRegion;
    QRegion m_sliderRegion;
    QColor m_color;
    bool m_isInWheel;
    bool m_isInSquare;
    qreal m_step;

    int wheelSize() const;
    QColor colorForPoint(const QPoint &point);
    void drawWheel();
    void drawWheelDot(QPainter &painter);
    void drawSliderBar(QPainter &painter);
    void drawSlider();
    void updateCursor(const QPoint &pos);
};

#endif // COLORWHEELITEM_H
