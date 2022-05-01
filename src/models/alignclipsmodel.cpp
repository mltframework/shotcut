/*
 * Copyright (c) 2022 Meltytech, LLC
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

#include "alignclipsmodel.h"

#include <Logger.h>
#include "mltcontroller.h"

AlignClipsModel::AlignClipsModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

AlignClipsModel::~AlignClipsModel()
{
}

void AlignClipsModel::clear()
{
    beginResetModel();
    m_clips.clear();
    endResetModel();
}

void AlignClipsModel::addClip(const QString &name, int offset, int drift, const QString &error)
{
    beginInsertRows(QModelIndex(), m_clips.size(), m_clips.size());
    ClipAlignment newClip;
    newClip.name = name;
    newClip.offset = offset;
    newClip.drift = drift;
    newClip.error = error;
    newClip.progress = 0;
    m_clips.append(newClip);
    endInsertRows();
}

void AlignClipsModel::updateProgress(int row, int percent)
{
    QModelIndex modelIndex = index(row, COLUMN_NAME);
    if (!modelIndex.isValid() || modelIndex.column() < 0 || modelIndex.column() >= COLUMN_COUNT
            || modelIndex.row() < 0 || modelIndex.row() >= m_clips.size()) {
        LOG_ERROR() << "Invalid Index: " << modelIndex.row() << modelIndex.column();
        return;
    }
    m_clips[modelIndex.row()].progress = percent;
    emit dataChanged(modelIndex, modelIndex);
}

int AlignClipsModel::getProgress(int row) const
{
    if (row < 0 || row > m_clips.size()) {
        LOG_ERROR() << "Invalid row: " << row;
        return 0;
    }
    return m_clips[row].progress;
}

void AlignClipsModel::updateOffsetAndDrift(int row, int offset, double drift, const QString &error)
{
    if (row < 0 || row >= m_clips.size()) {
        LOG_ERROR() << "Invalid Row: " << row;
        return;
    }
    m_clips[row].offset = offset;
    m_clips[row].drift = drift;
    m_clips[row].error = error;
    emit dataChanged(index(row, COLUMN_ERROR), index(row, COLUMN_COUNT - 1));
}

int AlignClipsModel::getOffset(int row)
{
    int offset = 0;
    if (row < 0 || row >= m_clips.size()) {
        LOG_ERROR() << "Invalid Row: " << row;
    } else {
        offset = m_clips[row].offset;
    }
    return offset;
}

double AlignClipsModel::getDrift(int row)
{
    int drift = 0;
    if (row < 0 || row >= m_clips.size()) {
        LOG_ERROR() << "Invalid Row: " << row;
    } else {
        drift = m_clips[row].drift;
    }
    return drift;
}

int AlignClipsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_clips.size();
}

int AlignClipsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return COLUMN_COUNT;
}

QVariant AlignClipsModel::data(const QModelIndex &index, int role) const
{
    QVariant result;

    switch (role) {
    case Qt::StatusTipRole:
    case Qt::FontRole:
    case Qt::SizeHintRole:
    case Qt::DecorationRole:
    case Qt::CheckStateRole:
    case Qt::BackgroundRole:
    case Qt::ForegroundRole:
        return result;
    }

    if (!index.isValid() || index.column() < 0 || index.column() >= COLUMN_COUNT || index.row() < 0
            || index.row() >= m_clips.size()) {
        LOG_ERROR() << "Invalid Index: " << index.row() << index.column() << role;
        return result;
    }

    const ClipAlignment &clip = m_clips[index.row()];
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case COLUMN_ERROR:
            result = clip.error;
            break;
        case COLUMN_NAME:
            result = clip.name;
            break;
        case COLUMN_OFFSET:
            if (clip.progress != 0 && clip.offset != INVALID_OFFSET && MLT.producer()
                    && MLT.producer()->is_valid()) {
                if (clip.offset >= 0) {
                    result = MLT.producer()->frames_to_time(clip.offset, mlt_time_smpte_df);
                } else {
                    result = QString("-") + MLT.producer()->frames_to_time(-clip.offset, mlt_time_smpte_df);
                }
            }
            break;
        /*
                        case COLUMN_DRIFT:
                            if (clip.drift >= 0)
                                result = QLocale().toString(clip.drift * 100.0, 'g', 4);
                            break;
        */
        default:
            LOG_ERROR() << "Invalid Column" << index.row() << index.column() << roleNames()[role] << role;
            break;
        }
        break;
    case Qt::ToolTipRole:
        return clip.error;
    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case COLUMN_NAME:
            result = Qt::AlignLeft;
            break;
        case COLUMN_ERROR:
        case COLUMN_OFFSET:
//                case COLUMN_DRIFT:
            result = Qt::AlignCenter;
            break;
        default:
            LOG_ERROR() << "Invalid Column" << index.row() << index.column() << roleNames()[role] << role;
            break;
        }
        break;
    default:
        LOG_ERROR() << "Invalid Role" << index.row() << index.column() << roleNames()[role] << role;
        break;
    }
    return result;
}

QVariant AlignClipsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case COLUMN_ERROR:
            return QVariant();
        case COLUMN_NAME:
            return tr("Clip");
        case COLUMN_OFFSET:
            return tr("Offset");
//        case COLUMN_DRIFT:
//            return tr("Drift");
        default:
            break;
        }
    }
    return QVariant();
}

QModelIndex AlignClipsModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if (column < 0 || column >= COLUMN_COUNT || row < 0 || row >= m_clips.size())
        return QModelIndex();
    return createIndex(row, column, (int)0);
}

QModelIndex AlignClipsModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}
