/*
 * Copyright (c) 2012-2013 Meltytech, LLC
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

#ifndef MELTEDCLIPSMODEL_H
#define MELTEDCLIPSMODEL_H

#include <QtCore>
#include "mvcpthread.h"

class MeltedClipsModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit MeltedClipsModel(MvcpThread* mvcp, QObject *parent = 0);
    ~MeltedClipsModel();

    QStringList mimeTypes() const;
    QMimeData* mimeData(const QModelIndexList &indexes) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    bool hasChildren(const QModelIndex &parent) const;

private:
    MvcpThread* m_mvcp;
    QModelIndex m_rootIndex;
    QObject* m_root;
    QMutex m_mutex;
    QList<QModelIndex> m_pending;

    void fetch(QObject* parent, const QModelIndex& index) const;

private slots:
    void onClsResult(QObject* parent, QObjectList* results);
};

#endif // MELTEDCLIPSMODEL_H
