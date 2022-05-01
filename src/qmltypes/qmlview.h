/*
 * Copyright (c) 2014-2021 Meltytech, LLC
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

class QWidget;

class QmlView : public QObject
{
    Q_OBJECT

public:
    explicit QmlView(QWidget *qview);
    Q_INVOKABLE QPoint pos();

private:
    QWidget *m_qview;
};

#endif // QMLVIEW_H
