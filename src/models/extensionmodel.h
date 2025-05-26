/*
 * Copyright (c) 2025 Meltytech, LLC
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

#ifndef EXTENSIONMODEL_H
#define EXTENSIONMODEL_H

#include <QAbstractItemModel>

class QmlExtension;

class ExtensionModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Columns {
        COLUMN_STATUS = 0,
        COLUMN_NAME,
        COLUMN_SIZE,
        COLUMN_COUNT,
    };
    explicit ExtensionModel(const QString &id, QObject *parent = 0);
    virtual ~ExtensionModel();
    void clear();
    void addClip(const QString &name, int offset, int speed, const QString &error);
    QString getName(int row) const;
    QString getFormattedDataSize(int row) const;
    QString localPath(int row) const;
    QString url(int row) const;
    bool downloaded(int row) const;
    QModelIndex getIndexForPath(QString path);

protected:
    // Implement QAbstractItemModel
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

private:
    QmlExtension *m_ext;
};

#endif // EXTENSIONMODEL_H
