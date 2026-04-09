/*
 * Copyright (c) 2026 Meltytech, LLC
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

#include "qmlfiltermenu.h"

#include "qmlapplication.h"

#include <QMenu>

QmlFilterMenu::QmlFilterMenu(QObject *parent)
    : QObject(parent)
{}

void QmlFilterMenu::popup()
{
    QMenu menu;

    QAction manageAddOnFiltersAction(tr("Manage Add-on Filters"));
    connect(&manageAddOnFiltersAction,
            SIGNAL(triggered()),
            this,
            SLOT(onManageAddOnFiltersTriggered()));
    menu.addAction(&manageAddOnFiltersAction);

    menu.exec(QCursor::pos());
}

void QmlFilterMenu::onManageAddOnFiltersTriggered()
{
    QmlApplication::singleton().showAddOnFiltersDialog();
}
