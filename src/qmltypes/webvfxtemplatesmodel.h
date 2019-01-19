/*
 * Copyright (c) 2019 Meltytech, LLC
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

#ifndef WEBVFXTEMPLATESMODEL_H
#define WEBVFXTEMPLATESMODEL_H

#include <QAbstractListModel>
#include <QVariant>

class WebvfxTemplatesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit WebvfxTemplatesModel(QObject *parent = 0);

    enum Roles  {
        PathRole = Qt::UserRole + 1,
        ProtocolRole,
        CategoryRole
    };
    Q_ENUM(Roles)

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    Q_INVOKABLE QVariant data(int row, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;
    Q_INVOKABLE bool needsFolder(int row) const;
    Q_INVOKABLE QString copyTemplate(int row, QString path) const;
    Q_INVOKABLE QString copyTemplate(int row) const;

private:
    QStringList m_list;
};

#endif // WEBVFXTEMPLATESMODEL_H
