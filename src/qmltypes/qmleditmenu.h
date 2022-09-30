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

#ifndef QMLEDITMENU_H
#define QMLEDITMENU_H

#include <QObject>

class QmlEditMenu : public QObject
{
    Q_OBJECT

public:
    explicit QmlEditMenu(QObject *parent = 0);

signals:
    void undoTriggered();
    void redoTriggered();
    void cutTriggered();
    void copyTriggered();
    void pasteTriggered();
    void deleteTriggered();
    void clearTriggered();
    void selectAllTriggered();

public slots:
    void popup();
};

#endif // QMLEDITMENU_H
