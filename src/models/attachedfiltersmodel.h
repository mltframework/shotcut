/*
 * Copyright (c) 2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#ifndef ATTACHEDFILTERSMODEL_H
#define ATTACHEDFILTERSMODEL_H

#include <QAbstractListModel>
#include <mlt++/MltFilter.h>

class AttachedFiltersModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit AttachedFiltersModel(QObject *parent = 0);

    Mlt::Filter* filterForRow(int row) const;
    int indexForRow(int row) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    Qt::DropActions supportedDropActions() const;
    bool insertRows(int row, int count, const QModelIndex &parent);
    bool removeRows(int row, int count, const QModelIndex &parent);
 
signals:
    void changed();

public slots:
    Mlt::Filter* add(const QString& name);
    void remove(int row);
    void reset();

private:
    int m_rows;
    int m_dropRow;

    void calculateRows();
};

#endif // ATTACHEDFILTERSMODEL_H
