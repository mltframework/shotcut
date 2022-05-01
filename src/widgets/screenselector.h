/*
 * Copyright (c) 2014-2019 Meltytech, LLC
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

#ifndef SCREENSELECTOR_H
#define SCREENSELECTOR_H

#include <QFrame>

class ScreenSelector : public QFrame
{
    Q_OBJECT
public:
    ScreenSelector(QWidget *parent = 0);
    void setFixedSize(const QSize &size);
    void setBoundingRect(const QRect &rect);
    void setSelectedRect(const QRect &rect);

public slots:
    void startSelection(QPoint initialPos = QPoint(-1, -1));

signals:
    void screenSelected(const QRect &);
    void pointSelected(const QPoint &);
    void cancelled();

public:
    bool onMousePressEvent(QMouseEvent *event);
    bool onMouseMoveEvent(QMouseEvent *event);
    bool onMouseReleaseEvent(QMouseEvent *event);
    bool onKeyPressEvent(QKeyEvent *event);

protected:
    bool eventFilter(QObject *, QEvent *event);

private:
    void lockGeometry(const QRect &rect);
    void release();

    bool m_selectionInProgress;
    QRect m_selectionRect;
    QPoint m_selectionPoint;
    QSize m_fixedSize;
    QRect m_boundingRect;
};

#endif // SCREENSELECTOR_H
