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

#ifndef ACTIONS_H
#define ACTIONS_H

#include <QHash>
#include <QObject>

class QAction;
class QMenu;

class ShotcutActions : public QObject
{
    Q_OBJECT

public:

    static const char *hardKeyProperty;
    static const char *displayProperty;

    static ShotcutActions &singleton();
    explicit ShotcutActions() : QObject() {}

    void add(const QString &name, QAction *action, QString group = "");
    void loadFromMenu(QMenu *menu, const QString group = "");
    QAction *operator [](const QString &key);
    QList<QString> keys();

private:
    QHash<QString, QAction *> m_actions;
};

#define Actions ShotcutActions::singleton()

#endif // ACTIONS_H
