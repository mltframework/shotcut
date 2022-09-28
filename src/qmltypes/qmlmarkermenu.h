/*
 * Copyright (c) 2022 Meltytech, LLC
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

#ifndef QMLMARKERMENU_H
#define QMLMARKERMENU_H

#include <QObject>

class TimelineDock;

class QmlMarkerMenu : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QObject *target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)

public:
    explicit QmlMarkerMenu(QObject *parent = 0);
    QObject *target();
    void setTarget(QObject *timeline);
    int index();
    void setIndex(int index);

signals:
    void targetChanged();
    void indexChanged();

public slots:
    void popup();

private:
    TimelineDock *m_timeline;
    int m_index;
};

#endif // QMLMARKERMENU_H
