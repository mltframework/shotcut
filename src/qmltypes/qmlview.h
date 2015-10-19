/*
 * Copyright (c) 2014 Meltytech, LLC
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

#ifndef QMLVIEW_H
#define QMLVIEW_H

#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QQuickItem>
#include <QSGTexture>

class QWindow;
class QTimerEvent;
class QSGSimpleTextureNode;

class QmlView : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QPoint pos READ pos);

public:
    explicit QmlView(QWindow* qview);
    QPoint pos();
    Q_INVOKABLE void applyQTBUG47714Workaround(QObject * item);

private:
    QWindow* m_qview;
};

class QTBUG47714WorkaroundRenderListener : public QObject
{
    Q_OBJECT

public:
    QTBUG47714WorkaroundRenderListener(QQuickItem * item);
    void timerEvent(QTimerEvent * event);
    QSGSimpleTextureNode * nodeFromItem();

private slots:
    void beforeSync();
    void afterSync();

private:
    QPointer<QQuickItem> item;
    QSGTexture * oldTexture;
};

#endif // QMLVIEW_H
