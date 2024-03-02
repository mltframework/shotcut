/*
 * Copyright (c) 2023-2024 Meltytech, LLC
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

#ifndef RESOURCEMODEL_H
#define RESOURCEMODEL_H

#include <MltProducer.h>

#include <QAbstractItemModel>

class ResourceModel : public QAbstractItemModel
{
    Q_OBJECT

public:

    enum Columns {
        COLUMN_INFO = 0,
        COLUMN_NAME,
        COLUMN_SIZE,
        COLUMN_VID_DESCRIPTION,
        COLUMN_AUD_DESCRIPTION,
        COLUMN_COUNT,
    };

    explicit ResourceModel(QObject *parent = 0);
    virtual ~ResourceModel();
    void search(Mlt::Producer *producer);
    void add(Mlt::Producer *producer, const QString &location = QString());
    QList<Mlt::Producer> getProducers(const QModelIndexList &indices);
    bool exists(const QString &hash);
    int producerCount();
    Mlt::Producer producer(int index);
    // Implement QAbstractItemModel
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

private:

    QList<Mlt::Producer> m_producers;
    QMap<QString, QString> m_locations;
};

#endif // RESOURCEMODEL_H
