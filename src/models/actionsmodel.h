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

#ifndef ACTIONSMODEL_H
#define ACTIONSMODEL_H

#include <QAbstractItemModel>

class ActionsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Columns {
        COLUMN_GROUP = 0,
        COLUMN_ACTION,
        COLUMN_SEQUENCE1,
        COLUMN_SEQUENCE2,
        COLUMN_COUNT
    };
    explicit ActionsModel(QObject *parent = 0);

protected:
    // Implement QAbstractItemModel
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

private:
    QList<QString> m_keys;
};

#endif // ACTIONSMODEL_H
