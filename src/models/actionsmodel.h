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

class QAction;

class ActionsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Columns {

        COLUMN_ACTION = 0,
        COLUMN_SEQUENCE1,
        COLUMN_SEQUENCE2,
        COLUMN_COUNT
    };
    enum {
        HardKeyRole = Qt::UserRole,
        DefaultKeyRole,
    };
    explicit ActionsModel(QObject *parent = 0);
    QAction *action(const QModelIndex &index) const;

protected:
    // Implement QAbstractItemModel
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QModelIndex index(int row, int column = 0,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QList<QAction *> m_actions;
};

#endif // ACTIONSMODEL_H
