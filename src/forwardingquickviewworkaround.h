/*
 * Copyright (c) 2015 Meltytech, LLC
 * Author: Harald Hvaal <harald.hvaal@gmail.com>
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

#ifndef _FORWARDINGQUICKVIEWWORKAROUND_H
#define _FORWARDINGQUICKVIEWWORKAROUND_H

#include <QQuickWidget>
#include <QCoreApplication>

class ForwardingQuickViewWorkaround : public QQuickWidget
{
public:
    ForwardingQuickViewWorkaround(QQmlEngine * engine, QObject * receiver)
        : QQuickWidget(engine, 0)
        , m_receiver(receiver)
    {
    }

protected:
    QObject * m_receiver;

    void keyPressEvent(QKeyEvent* e) {
        QQuickWidget::keyPressEvent(e);
        if (!e->isAccepted())
            qApp->sendEvent(m_receiver, e);
    }

    void keyReleaseEvent(QKeyEvent* e) {
        QQuickWidget::keyReleaseEvent(e);
        if (!e->isAccepted())
            qApp->sendEvent(m_receiver, e);
    }
};


#endif
