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

static const QString categoryProperty = "category";
static QScopedPointer<ShotcutActions> instance;

ShotcutActions &ShotcutActions::singleton()
{
    if (!instance) {
        instance.reset(new ShotcutActions());
    }
    return *instance;
}

void ShotcutActions::add(const QString &key, QAction *action)
{
    auto iterator = m_actions.find(key);
    if (iterator != m_actions.end()) {
        LOG_ERROR() << "Action already exists" << key;
    }
    action->setObjectName(key);
    m_actions[key] = action;
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
