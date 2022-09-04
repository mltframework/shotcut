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
#include "settings.h"

#include <QAction>
#include <QMenu>

const char *ShotcutActions::hardKeyProperty = "_hardkey";
const char *ShotcutActions::displayProperty = "_display";
const char *ShotcutActions::defaultKey1Property = "_defaultKey1";
const char *ShotcutActions::defaultKey2Property = "_defaultKey2";

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
    if (iterator != m_actions.end() && iterator.value() != action) {
        LOG_ERROR() << "Action already exists with this key" << key;
        return;
    }
    action->setObjectName(key);

    if (group.isEmpty()) {
        group = tr("Other");
    }
    action->setProperty(displayProperty, group +  " > " + action->iconText());

    QList<QKeySequence> sequences = action->shortcuts();
    if (sequences.size() > 0)
        action->setProperty(defaultKey1Property, sequences[0].toString());
    if (sequences.size() > 1)
        action->setProperty(defaultKey2Property, sequences[1].toString());

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
        if (action->isSeparator() || action->objectName() == "dummyAction")
            continue;

        QMenu *submenu = action->menu();
        if (submenu) {
            loadFromMenu(submenu, group);
        } else {
            if (action->objectName().isEmpty()) {
                // Each action must have a unique object name
                QString newObjectName = group + action->iconText();
                newObjectName = newObjectName.replace(" ", "");
                newObjectName = newObjectName.replace(">", "");
                action->setObjectName(newObjectName);
            }
            add(action->objectName(), action, group);
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

void ShotcutActions::overrideShortcuts(const QString &key, QList<QKeySequence> shortcuts)
{
    QAction *action = m_actions[key];
    if (!action) {
        LOG_ERROR() << "Invalid action" << key;
        return;
    }

    QList<QKeySequence> defaultShortcuts;
    QVariant seq = action->property(defaultKey1Property);
    if (seq.isValid())
        defaultShortcuts << QKeySequence::fromString(seq.toString());
    seq = action->property(defaultKey2Property);
    if (seq.isValid())
        defaultShortcuts << QKeySequence::fromString(seq.toString());

    // Make the lists the same size for easy comparison
    while (shortcuts.size() < 2)
        shortcuts << QKeySequence();
    while (defaultShortcuts.size() < 2)
        defaultShortcuts << QKeySequence();

    if (shortcuts == defaultShortcuts) {
        // Shortcuts are set to default - delete all settings
        Settings.clearShortcuts(action->objectName());
    } else {
        Settings.setShortcuts(action->objectName(), shortcuts);
    }

    action->setShortcuts(shortcuts);
}

void ShotcutActions::loadSavedShortcuts()
{
    for (auto action : m_actions) {
        QList<QKeySequence> shortcutSettings = Settings.shortcuts(action->objectName());
        if (!shortcutSettings.isEmpty())
            action->setShortcuts(shortcutSettings);
    }
}
