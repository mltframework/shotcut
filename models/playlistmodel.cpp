/*
 * Copyright (c) 2012 Meltytech, LLC
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

#include "playlistmodel.h"
#include <QDebug>

PlaylistModel::PlaylistModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_playlist(0)
    , m_dropRow(-1)
{
}

PlaylistModel::~PlaylistModel()
{
    delete m_playlist;
}

int PlaylistModel::rowCount(const QModelIndex& /*parent*/) const
{
    return m_playlist? m_playlist->count() : 0;
}

int PlaylistModel::columnCount(const QModelIndex& /*parent*/) const
{
    return COLUMN_COUNT;
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole: {
        const Mlt::ClipInfo* info = &m_clipInfo;
        m_playlist->clip_info(index.row(),
                              const_cast<Mlt::ClipInfo*>(info));
        if (index.column() == COLUMN_RESOURCE) {
            QString result = QString::fromUtf8(m_clipInfo.resource);
            if (result == "<producer>" && m_clipInfo.producer
                    && m_clipInfo.producer->is_valid() && m_clipInfo.producer->get("mlt_service"))
                result = QString::fromUtf8(m_clipInfo.producer->get("mlt_service"));
            return result;
        }
    }
    default:
        return QVariant();
    }
}

QVariant PlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section) {
        case COLUMN_RESOURCE:
            return QString(tr("Clip"));
        default:
            break;
        }
    }
    return QVariant();
}

Qt::DropActions PlaylistModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

bool PlaylistModel::insertRows(int row, int count, const QModelIndex& parent)
{
    m_dropRow = row;
    return true;
}

bool PlaylistModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if (!m_playlist) return false;
    if (row == m_dropRow) return false;
    beginMoveRows(parent, row, row, parent, m_dropRow);
    m_playlist->move(row, m_dropRow);
    endMoveRows();
    m_dropRow = -1;
    emit modified();
    return true;
}

QStringList PlaylistModel::mimeTypes() const
{
    QStringList ls = QAbstractTableModel::mimeTypes();
    ls.append("application/mlt+xml");
    return ls;
}

bool PlaylistModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (data->hasFormat("application/mlt+xml")) {
        emit dropped(data, row);
        return true;
    }
    else if (action == Qt::MoveAction)
        return QAbstractTableModel::dropMimeData(data, action, row, column, parent);
    else
        return false;
}

Qt::ItemFlags PlaylistModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaults = QAbstractTableModel::flags(index);
    if (index.isValid())
        return Qt::ItemIsDragEnabled | defaults;
    else
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaults;
}

void PlaylistModel::clear()
{
    if (!m_playlist) return;
    beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
    m_playlist->clear();
    endRemoveRows();
    emit cleared();
}

void PlaylistModel::load()
{
    if (m_playlist) {
        beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
        m_playlist->clear();
        endRemoveRows();
        delete m_playlist;
    }
    m_playlist = new Mlt::Playlist(*MLT.producer());
    if (!m_playlist->is_valid()) {
        delete m_playlist;
        m_playlist = 0;
        return;
    }
    beginInsertRows(QModelIndex(), 0, m_playlist->count() - 1);
    endInsertRows();
    // do not let opening a clip change the profile!
    MLT.profile().set_explicit(true);
    emit loaded();
}

void PlaylistModel::append(Mlt::Producer* producer)
{
    createIfNeeded();
    int count = m_playlist->count();
    beginInsertRows(QModelIndex(), count, count);
    m_playlist->append(*producer, producer->get_in(), producer->get_out());
    endInsertRows();
    emit modified();
}

void PlaylistModel::insert(Mlt::Producer* producer, int row)
{
    createIfNeeded();
    beginInsertRows(QModelIndex(), row, row);
    m_playlist->insert(*producer, row, producer->get_in(), producer->get_out());
    endInsertRows();
    emit modified();
}

void PlaylistModel::remove(int row)
{
    if (!m_playlist) return;
    beginRemoveRows(QModelIndex(), row, row);
    m_playlist->remove(row);
    endRemoveRows();
    if (m_playlist->count() == 0)
        emit cleared();
    else
        emit modified();
}

void PlaylistModel::update(int row, int in, int out)
{
    if (!m_playlist) return;
    m_playlist->resize_clip(row, in, out);
    emit modified();
}

void PlaylistModel::appendBlank(int frames)
{
    createIfNeeded();
    int count = m_playlist->count();
    beginInsertRows(QModelIndex(), count, count);
    m_playlist->blank(frames - 1);
    endInsertRows();
    emit modified();
}

void PlaylistModel::insertBlank(int frames, int row)
{
    createIfNeeded();
    beginInsertRows(QModelIndex(), row, row);
    m_playlist->insert_blank(row, frames - 1);
    endInsertRows();
    emit modified();
}

void PlaylistModel::close()
{
    if (!m_playlist) return;
    clear();
    delete m_playlist;
    m_playlist = 0;
    emit closed();
}

void PlaylistModel::createIfNeeded()
{
    if (!m_playlist) {
        m_playlist = new Mlt::Playlist(MLT.profile());
        // do not let opening a clip change the profile!
        MLT.profile().set_explicit(true);
        emit created();
    }
}
