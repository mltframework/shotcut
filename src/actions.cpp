/*
 * Copyright (c) 2013-2022 Meltytech, LLC
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

#include "actions.h"

#include <Logger.h>

#include <QAction>
#include <QMenu>

const char *ShotcutActions::hardKeyProperty = "_hardkey";
const char *ShotcutActions::groupProperty = "_group";

static QScopedPointer<ShotcutActions> instance;

ShotcutActions &ShotcutActions::singleton()
{
    if (!instance) {
        instance.reset(new ShotcutActions());
    }
    return *instance;
}

void ShotcutActions::add(const QString &key, QAction *action, QString group)
{
    auto iterator = m_actions.find(key);
    if (iterator != m_actions.end()) {
        LOG_ERROR() << "Action already exists" << key;
    }
    action->setObjectName(key);

    if (group.isEmpty()) {
        group = tr("Other");
    }
    action->setProperty(groupProperty, group);

    m_actions[key] = action;
}

void ShotcutActions::loadFromMenu(QMenu *menu, QString group)
{
    if (!menu->title().isEmpty()) {
        if (!group.isEmpty())
            group = group + " > " + menu->menuAction()->iconText();
        else
            group = menu->menuAction()->iconText();
    }

    for (QAction *action : menu->actions()) {
        if (action->property("_placeholder").toBool() == true || action->isSeparator())
            continue;

        if (action->objectName().isEmpty() && action->text().isEmpty())
            continue;

        QMenu *submenu = action->menu();
        if (submenu) {
            loadFromMenu(submenu, group);
        } else {
            if (action->objectName().isEmpty()) {
                // Each action must have a unique object name
                QString newObjectName = group + action->iconText();
                newObjectName = newObjectName.replace(" ", "");
                action->setObjectName(newObjectName);
            }
            action->setProperty(groupProperty, group);
            m_actions[action->objectName()] = action;
        }
    }
}

QAction *ShotcutActions::operator [](const QString &key)
{
    auto iterator = m_actions.find(key);
    if (iterator != m_actions.end()) {
        return iterator.value();
    }
    return nullptr;
}

QList<QString> ShotcutActions::keys()
{
    return m_actions.keys();
}
