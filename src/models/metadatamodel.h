/*
 * Copyright (c) 2014 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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

#ifndef METADATAMODEL_H
#define METADATAMODEL_H

#include <QAbstractListModel>
#include <QList>

class QmlMetadata;

class MetadataModel : public QAbstractListModel
{
    Q_OBJECT
public:

    enum ModelRoles {
        NameRole = Qt::UserRole + 1,
        HiddenRole,
        FavoriteRole,
        ServiceRole,
        IsAudioRole
    };

    explicit MetadataModel(QObject *parent = 0);

    // Implement QAbstractListModel
    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QHash<int, QByteArray> roleNames() const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    // Direct access to QmlMetadata
    void add(QmlMetadata* data);
    Q_INVOKABLE QmlMetadata* get(int index) const;

private:
    typedef QList<QmlMetadata*> MetadataList;
    MetadataList m_list;
};

#endif // METADATAMODEL_H
