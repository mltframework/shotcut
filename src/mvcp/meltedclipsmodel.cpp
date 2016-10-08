/*
 * Copyright (c) 2012-2015 Meltytech, LLC
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

#include "meltedclipsmodel.h"

MeltedClipsModel::MeltedClipsModel(MvcpThread* mvcp, QObject *parent)
    : QAbstractItemModel(parent)
    , m_mvcp(mvcp)
    , m_root(new QObject)
{
    connect(m_mvcp, SIGNAL(clsResult(QObject*, QObjectList*)), this, SLOT(onClsResult(QObject*, QObjectList*)));
    m_root->setObjectName("/");
    m_root->setProperty("dir", true);
    fetch(m_root, m_rootIndex);
}

MeltedClipsModel::~MeltedClipsModel()
{
    delete m_root;
}

QStringList MeltedClipsModel::mimeTypes() const
{
    QStringList ls;
    ls.append("application/mvcp+path");
    return ls;
}

QMimeData *MeltedClipsModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData* mimeData = new QMimeData;
    QObject* o = (QObject*) indexes[0].internalPointer();
    mimeData->setData("application/mvcp+path", o->objectName().toUtf8());
    return mimeData;
}

int MeltedClipsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    QObject* parentObject = m_root;
    if (parent.isValid())
        parentObject = (QObject*) parent.internalPointer();
    fetch(parentObject, parent);
    return parentObject->children().size();
}

int MeltedClipsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2;
}
QVariant MeltedClipsModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::TextAlignmentRole) {
        if (index.column() == 1)
            return Qt::AlignRight;
        else
            return Qt::AlignLeft;
    }
    else if (role == Qt::UserRole && index.isValid()) {
        QObject* o = (QObject*) index.internalPointer();
        return o->objectName();
    }

    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    QObject* o = (QObject*) index.internalPointer();
    if (index.column() == 0) {
        return o->property("name");
    } else if (!o->property("dir").toBool()) {
        float size = o->property("size").toFloat();
        float x = size / 1024 / 1024 / 1024;
        if ( x > 1 )
            return tr("%1 GiB").arg(x, 0, 'f', 1);
        x = size / 1024 / 1024;
        if ( x > 1 )
            return tr("%1 MiB").arg(x, 0, 'f', 1);
        x = size / 1024;
        if ( x > 1 )
            return tr("%1 KiB").arg(x, 0, 'f', 1);
        return tr("%1 B").arg(size);
    } else {
        int n = o->children().size();
        return tr("%n item(s)", "", n);
    }
}

Qt::ItemFlags MeltedClipsModel::flags(const QModelIndex &index) const
{
    if (index.isValid())
        return Qt::ItemIsDragEnabled | QAbstractItemModel::flags(index);
    else
        return 0;
}

QVariant MeltedClipsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        return (section == 0)? tr("Clip") : tr("Size");
    else
        return QVariant();
}

QModelIndex MeltedClipsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    QObject* parentObject = const_cast<QObject*>(m_root);
    if (parent.isValid())
        parentObject = (QObject*) parent.internalPointer();
    fetch(parentObject, parent);
    if (row < parentObject->children().size())
        return createIndex(row, column, parentObject->children().at(row));
    else
        return QModelIndex();
}

QModelIndex MeltedClipsModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || !index.internalPointer())
        return QModelIndex();

    QObject* child = (QObject*) index.internalPointer();
    QObject* parent = (QObject*) child->parent();
    if (!parent || parent == m_root)
        return QModelIndex();
    return createIndex(parent->property("row").toInt(), 0, parent);
}

bool MeltedClipsModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return true;
    } else {
        QObject* parentObject = (QObject*) parent.internalPointer();
        fetch(parentObject, parent);
        return parentObject->property("dir").toBool();
    }
}

void MeltedClipsModel::fetch(QObject *parent, const QModelIndex& index) const
{
    if (!parent->property("dir").toBool())
        return;
    QVariant fetched = parent->property("fetched");
    if (fetched.isValid() && fetched.toBool())
        return;
    parent->setProperty("fetched", true);

    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    QList<QModelIndex> *pending = (QList<QModelIndex>*) &m_pending;
    pending->append(index);
    m_mvcp->cls(parent->objectName(), parent);
}

void MeltedClipsModel::onClsResult(QObject* parent, QObjectList* results)
{
    if (!results->isEmpty()) {
        m_mutex.lock();
        const QModelIndex& index = m_pending.takeFirst();
        int n = results->size();

        for (int i = 0; i < n; i++) {
            QObject* o = results->takeFirst();
            QObject* child = new QObject(parent);
            child->setObjectName(o->objectName());
            child->setProperty("row", i);
            child->setProperty("name", o->property("name"));
            child->setProperty("dir", o->property("dir"));
            child->setProperty("size", o->property("size"));
            delete o;
        }
        m_mutex.unlock();
        beginInsertRows(index, 0, n - 1);
        endInsertRows();
    }
    delete results;
}
