/*
 * Copyright (c) 2014-2018 Meltytech, LLC
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

#include "metadatamodel.h"
#include "qmltypes/qmlmetadata.h"
#include "settings.h"
#include <Logger.h>

MetadataModel::MetadataModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_filter(FavoritesFilter)
    , m_isClipProducer(true)
{
}

int MetadataModel::rowCount(const QModelIndex&) const
{
    return m_list.size();
}

QVariant MetadataModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    QmlMetadata* meta = m_list.at(index.row());

    if(meta) {
        switch(role) {
            case Qt::DisplayRole:
            case NameRole:
                result = meta->name();
                break;
            case HiddenRole:
                result = meta->isHidden();
                break;
            case FavoriteRole:
                result = meta->isFavorite();
                break;
            case ServiceRole:
                result = meta->mlt_service();
                break;
            case IsAudioRole:
                result = meta->isAudio();
                break;
            case NeedsGpuRole:
                result = meta->needsGPU();
                break;
            case VisibleRole:
                result = isVisible(index.row());
                break;
        }
    }

    return result;
}

bool MetadataModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid()) return false;
    switch(role) {
        case FavoriteRole:
        {
            QmlMetadata* meta = m_list.at(index.row());
            meta->setIsFavorite(value.value<bool>());
            emit(dataChanged(index, index));
            break;
        }
    }
    return true;
}

QHash<int, QByteArray> MetadataModel::roleNames() const {
    QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
    roles[NameRole] = "name";
    roles[HiddenRole] = "hidden";
    roles[FavoriteRole] = "favorite";
    roles[ServiceRole] = "service";
    roles[IsAudioRole] = "isAudio";
    roles[NeedsGpuRole] = "needsGpu";
    roles[VisibleRole] = "isVisible";
    return roles;
}

Qt::ItemFlags MetadataModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return Qt::NoItemFlags;
    return QAbstractListModel::flags(index) | Qt::ItemIsEditable | Qt::ItemIsSelectable;
}

void MetadataModel::add(QmlMetadata* data)
{
    int i = 0;
    for( i = 0; i < m_list.size(); i++ ) {
        if (m_list[i]->name().toLower() > data->name().toLower() ) {
            break;
        }
    }

    beginInsertRows(QModelIndex(), i, i);
    m_list.insert(i, data);
    endInsertRows();

    data->setParent(this);
}

QmlMetadata* MetadataModel::get(int index) const
{
    if( index < m_list.size() ) {
        return m_list[index];
    }
    return 0;
}

void MetadataModel::setFilter(MetadataFilter filter)
{
    beginResetModel();
    m_filter = filter;
    emit filterChanged();
    endResetModel();
}

void MetadataModel::setSearch(const QString& search)
{
    beginResetModel();
    m_search = search;
    emit searchChanged();
    endResetModel();
}

bool MetadataModel::isVisible(int row) const
{
    QmlMetadata* meta = m_list.at(row);
    if (meta->isHidden()) return false;
    if (meta->needsGPU() && !Settings.playerGPU()) return false;
    if (!meta->needsGPU() && Settings.playerGPU() && !meta->gpuAlt().isEmpty()) return false;
    if (!meta->isGpuCompatible() && Settings.playerGPU()) return false;
    if (meta->isClipOnly() && !m_isClipProducer) return false;
    if (m_search.isEmpty()) {
        if (m_filter == FavoritesFilter && !meta->isFavorite()) return false;
        if (m_filter == AudioFilter && !meta->isAudio()) return false;
        if (m_filter == VideoFilter && meta->isAudio()) return false;
    } else {
        if (!meta->name().contains(m_search, Qt::CaseInsensitive)) return false;
    }
    return true;
}

void MetadataModel::setIsClipProducer(bool isClipProducer)
{
    beginResetModel();
    m_isClipProducer = isClipProducer;
    endResetModel();
}
