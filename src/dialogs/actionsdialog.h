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

#ifndef ACTIONSDIALOG_H
#define ACTIONSDIALOG_H

#include "models/actionsmodel.h"

#include <QDialog>

class QSortFilterProxyModel;
class QTreeView;

class ActionsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ActionsDialog(QWidget *parent = 0);

private:
    ActionsModel m_model;
    QTreeView *m_table;
    QSortFilterProxyModel *m_proxyModel;
};

#endif // ACTIONSDIALOG_H
