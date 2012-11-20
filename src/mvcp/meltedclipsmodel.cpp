/*
 * Copyright (c) 2012 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * GL shader based on BSD licensed code from Peter Bengtsson:
 * http://www.fourcc.org/source/YUV420P-OpenGL-GLSLang.c
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

class MvcpEntry : public QObject
{
public:
    explicit MvcpEntry(int row,
                       const char* name,
                       const char* fullname,
                       bool isDirectory,
                       unsigned long long size,
                       QObject* parent = 0)
        : QObject(parent)
        , row(row)
        , name(QString::fromUtf8(name))
        , isDirectory(isDirectory)
        , size(size)
    {
        setObjectName(QString::fromUtf8(fullname));
        if (this->name.endsWith('/'))
            this->name.chop(1);
    }

    void fetch(mvcp a_mvcp)
    {
        if (isDirectory && children().size() == 0) {
            mvcp_dir dir = mvcp_dir_init(a_mvcp, objectName().toUtf8().constData());
            int n = mvcp_dir_count(dir);
            for (int i = 0; i < n; i++) {
                mvcp_dir_entry_t entry;
                mvcp_dir_get(dir, i, &entry);
                new MvcpEntry(i, entry.name, entry.full, entry.dir, entry.size, this);
            }
        }
    }

    int row;
    QString name;
    bool isDirectory;
    unsigned long long size;
};

MeltedClipsModel::MeltedClipsModel(mvcp a_mvcp, QObject *parent)
    : QAbstractItemModel(parent)
    , m_mvcp(a_mvcp)
    , m_root(new MvcpEntry(0, "/", "/", true, 0))
{
    m_root->fetch(m_mvcp);
}

MeltedClipsModel::~MeltedClipsModel()
{
}

QStringList MeltedClipsModel::mimeTypes() const
{
    QStringList ls = QAbstractItemModel::mimeTypes();
    ls.append("text/uri-list");
    return ls;
}

int MeltedClipsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    MvcpEntry* parentEntry = m_root;
    if (parent.isValid())
        parentEntry = (MvcpEntry*) parent.internalPointer();
    parentEntry->fetch(m_mvcp);
    return parentEntry->children().size();
}

int MeltedClipsModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}
QVariant MeltedClipsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    MvcpEntry* entry = (MvcpEntry*) index.internalPointer();
    if (index.column() == 0) {
        return entry->name;
    } else if (!entry->isDirectory) {
        unsigned x = entry->size / 1024 / 1024 / 1024;
        if ( x > 0 )
            return tr("%1 G").arg(x);
        x = entry->size / 1024 / 1024;
        if ( x > 0 )
            return tr("%1 M").arg(x);
        x = entry->size / 1024;
        if ( x > 0 )
            return tr("%1 K").arg(x);
        return tr("%1").arg(entry->size);
    } else {
        return QVariant();
    }
}

Qt::ItemFlags MeltedClipsModel::flags(const QModelIndex &index) const
{
    if (index.isValid())
        return QAbstractItemModel::flags(index);
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

    MvcpEntry* parentEntry = const_cast<MvcpEntry*>(m_root);
    if (parent.isValid())
        parentEntry = (MvcpEntry*) parent.internalPointer();
    parentEntry->fetch(m_mvcp);
    return createIndex(row, column, parentEntry->children().at(row));
}

QModelIndex MeltedClipsModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || !index.internalPointer())
        return QModelIndex();

    MvcpEntry* child = (MvcpEntry*) index.internalPointer();
    MvcpEntry* parent = (MvcpEntry*) child->parent();
    if (!parent || parent == m_root)
        return QModelIndex();
    return createIndex(parent->row, 0, parent);
}

bool MeltedClipsModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return true;
    } else {
        MvcpEntry* parentEntry = (MvcpEntry*) parent.internalPointer();
        parentEntry->fetch(m_mvcp);
        return parentEntry->children().size() > 0;
    }
}
