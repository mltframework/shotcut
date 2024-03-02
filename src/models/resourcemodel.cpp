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

#include "resourcemodel.h"

#include <Logger.h>
#include "util.h"
#include "shotcut_mlt_properties.h"

#include <Mlt.h>

class ProducerFinder : public Mlt::Parser
{
public:
    ProducerFinder(ResourceModel *model)
        : Mlt::Parser()
        , m_model(model)
    {}

    int on_start_producer(Mlt::Producer *producer)
    {
        if (!m_isBackgroundTrack && producer->parent().get("resource") != QString("<tractor>")) {
            if (!m_isTransition)
                m_clipIndex++;
            if (!producer->is_blank()) {
                QString location;
                if (m_trackIndex == -1) {
                    location = QObject::tr("Playlist Clip: %1").arg(m_clipIndex + 1);
                } else {
                    if (m_isTransition) {
                        location = QObject::tr("Track: %1, Clip: %2 (transition)").arg(m_trackName).arg(m_clipIndex + 1);
                    } else {
                        location = QObject::tr("Track: %1, Clip: %2").arg(m_trackName).arg(m_clipIndex + 1);
                    }
                }
                m_model->add(producer, location);
            }
        }
        return 0;
    }

    int on_start_filter(Mlt::Filter *)
    {
        return 0;
    }
    int on_end_producer(Mlt::Producer *)
    {
        return 0;
    }
    int on_start_playlist(Mlt::Playlist *playlist)
    {
        if (playlist->get("id") == QString("background")) {
            m_isBackgroundTrack = true;
        } else {
            m_trackName = playlist->get(kTrackNameProperty);
        }
        return 0;
    }
    int on_end_playlist(Mlt::Playlist *)
    {
        m_trackName.clear();
        m_isBackgroundTrack = false;
        return 0;
    }
    int on_start_tractor(Mlt::Tractor *)
    {
        if (!m_isTimeline) {
            m_isTimeline = true;
        } else {
            m_isTransition = true;
            // Count both clips in a transition as the same index;
            m_clipIndex++;
        }
        return 0;
    }
    int on_end_tractor(Mlt::Tractor *)
    {
        m_isTransition = false;
        return 0;
    }
    int on_start_multitrack(Mlt::Multitrack *)
    {
        return 0;
    }
    int on_end_multitrack(Mlt::Multitrack *)
    {
        return 0;
    }
    int on_start_track()
    {
        if (!m_isTransition) {
            m_trackIndex++;
            m_clipIndex = -1;
        }
        return 0;
    }
    int on_end_track()
    {
        return 0;
    }
    int on_end_filter(Mlt::Filter *)
    {
        return 0;
    }
    int on_start_transition(Mlt::Transition *)
    {
        return 0;
    }
    int on_end_transition(Mlt::Transition *)
    {
        return 0;
    }
    int on_start_chain(Mlt::Chain *)
    {
        return 0;
    }
    int on_end_chain(Mlt::Chain *)
    {
        return 0;
    }
    int on_start_link(Mlt::Link *)
    {
        return 0;
    }
    int on_end_link(Mlt::Link *)
    {
        return 0;
    }
private:
    ResourceModel *m_model;
    bool m_isTimeline = false;
    bool m_isTransition = false;
    bool m_isBackgroundTrack = false;
    int m_trackIndex = -1;
    int m_clipIndex = -1;
    QString m_trackName;
};

QString appendLocation(QString &currentLocations, const QString &location)
{
    if (currentLocations.isEmpty()) {
        return location;
    } else {
        return currentLocations + "\n" + location;
    }
}

ResourceModel::ResourceModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

ResourceModel::~ResourceModel()
{
}

void ResourceModel::search(Mlt::Producer *producer)
{
    if (!producer) {
        return;
    }
    ProducerFinder parser(this);
    beginResetModel();
    parser.start(*producer);
    std::sort(m_producers.begin(), m_producers.end(), [ & ](Mlt::Producer & a, Mlt::Producer & b) {
        return Util::GetFilenameFromProducer(&a, true).compare(Util::GetFilenameFromProducer(&b, true),
                                                               Qt::CaseInsensitive) < 0;
    });
    endResetModel();
}

void ResourceModel::add(Mlt::Producer *producer, const QString &location)
{
    if (producer->is_blank()) {
        // Do not add
    } else if (producer->is_cut()) {
        Mlt::Producer parent = producer->parent();
        QString hash = Util::getHash(parent);
        if (!hash.isEmpty()) {
            if (!exists(hash)) {
                beginInsertRows(QModelIndex(), m_producers.size(), m_producers.size());
                m_producers.append(parent);
                endInsertRows();
            }
            m_locations[hash] = appendLocation(m_locations[hash], location);
        }
    } else {
        QString hash = Util::getHash(*producer);
        if (!hash.isEmpty()) {
            if (!exists(hash)) {
                beginInsertRows(QModelIndex(), m_producers.size(), m_producers.size());
                m_producers.append(*producer);
                endInsertRows();
            }
            m_locations[hash] = appendLocation(m_locations[hash], location);
        }
    }
}

QList<Mlt::Producer> ResourceModel::getProducers(const QModelIndexList &indices)
{
    QList<Mlt::Producer> producers;
    foreach (auto index, indices) {
        int row = index.row();
        if (row >= 0 && row < m_producers.size()) {
            producers << m_producers[row];
        }
    }
    return producers;
}

bool ResourceModel::exists(const QString &hash)
{
    for ( int i = 0; i < m_producers.count(); ++i ) {
        if (Util::getHash(m_producers[i]) == hash) {
            return true;
        }
    }
    return false;
}

int ResourceModel::producerCount()
{
    return m_producers.count();
}

Mlt::Producer ResourceModel::producer(int index)
{
    if (index >= 0 && index < m_producers.count()) {
        return m_producers[index];
    }
    return Mlt::Producer();
}

int ResourceModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_producers.size();
}

int ResourceModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return COLUMN_COUNT;
}

QVariant ResourceModel::data(const QModelIndex &index, int role) const
{
    QVariant result;

    switch (role) {
    case Qt::StatusTipRole:
    case Qt::FontRole:
    case Qt::SizeHintRole:
    case Qt::CheckStateRole:
    case Qt::BackgroundRole:
    case Qt::ForegroundRole:
        return result;
    }

    if (!index.isValid() || index.column() < 0 || index.column() >= COLUMN_COUNT || index.row() < 0
            || index.row() >= m_producers.size()) {
        LOG_ERROR() << "Invalid Index: " << index.row() << index.column() << role;
        return result;
    }

    Mlt::Producer *producer = const_cast<Mlt::Producer *>( &m_producers[index.row()] );
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case COLUMN_INFO:
            break;
        case COLUMN_NAME: {
            QString path = Util::GetFilenameFromProducer(producer, true);
            QFileInfo info(path);
            result = info.fileName();
            break;
        }
        case COLUMN_SIZE: {
            QString path = Util::GetFilenameFromProducer(producer, true);
            QFileInfo info(path);
            double size = (double)info.size() / (double)(1024 * 1024);
            result = QString(tr("%1MB")).arg(QLocale().toString(size, 'f', 2));
            break;
        }
        case COLUMN_VID_DESCRIPTION: {
            int width = producer->get_int("meta.media.width");
            int height = producer->get_int("meta.media.height");
            if (producer->get_int("video_index") >= 0) {
                double frame_rate_num = producer->get_double("meta.media.frame_rate_num");
                double frame_rate_den = producer->get_double("meta.media.frame_rate_den");
                if ( width && height && frame_rate_num && frame_rate_den
                        && (frame_rate_num / frame_rate_den) < 1000) {
                    int index = producer->get_int("video_index");
                    QString key = QString("meta.media.%1.codec.name").arg(index);
                    QString codec(producer->get(key.toLatin1().constData()));
                    double frame_rate = frame_rate_num / frame_rate_den;
                    result = QString(tr("%1 %2x%3 %4fps"))
                             .arg(codec)
                             .arg(width)
                             .arg(height)
                             .arg(QLocale().toString(frame_rate, 'f', 2));
                }
            }
            if (result.isNull() && width > 0 && height > 0 ) {
                result = QString(QObject::tr("%1x%2"))
                         .arg(width)
                         .arg(height);
            }
            break;
        }
        case COLUMN_AUD_DESCRIPTION: {
            if (producer->get_int("audio_index") >= 0) {
                int index = producer->get_int("audio_index");
                QString key = QString("meta.media.%1.codec.name").arg(index);
                QString codec(producer->get(key.toLatin1().constData()));
                if (!codec.isEmpty()) {
                    key = QString("meta.media.%1.codec.channels").arg(index);
                    int channels(producer->get_int(key.toLatin1().constData()));
                    key = QString("meta.media.%1.codec.sample_rate").arg(index);
                    QString sampleRate(producer->get(key.toLatin1().constData()));
                    result = QString("%1 %2ch %3KHz")
                             .arg(codec)
                             .arg(channels)
                             .arg(sampleRate.toDouble() / 1000);
                }
            }
            break;
        }
        default:
            LOG_ERROR() << "Invalid DisplayRole Column" << index.row() << index.column() << roleNames()[role] <<
                        role;
            break;
        }
        break;
    case Qt::ToolTipRole:
        switch (index.column()) {
        case COLUMN_INFO:
            result = Util::getConversionAdvice(producer);
            break;
        case COLUMN_NAME:
        case COLUMN_VID_DESCRIPTION:
        case COLUMN_AUD_DESCRIPTION:
        case COLUMN_SIZE: {
            QString filename = Util::GetFilenameFromProducer(producer, true);
            QString hash = Util::getHash(*producer);
            QString locations = m_locations[hash];
            if (locations.isEmpty()) {
                result = filename;
            } else {
                result = filename + "\n" + locations;
            }
            break;
        }
        default:
            LOG_ERROR() << "Invalid ToolTipRole Column" << index.row() << index.column() << roleNames()[role] <<
                        role;
            break;
        }
        break;
    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case COLUMN_INFO:
        case COLUMN_NAME:
        case COLUMN_VID_DESCRIPTION:
        case COLUMN_AUD_DESCRIPTION:
            result = Qt::AlignLeft;
            break;
        case COLUMN_SIZE:
            result = Qt::AlignRight;
            break;
        default:
            LOG_ERROR() << "Invalid TextAlignmentRole Column" << index.row() << index.column() <<
                        roleNames()[role] << role;
            break;
        }
        break;
    case Qt::DecorationRole:
        switch (index.column()) {
        case COLUMN_INFO:
            if (!Util::getConversionAdvice(producer).isEmpty()) {
                result = QIcon(":/icons/oxygen/32x32/status/task-attempt.png");
            } else {
                result = QIcon(":/icons/oxygen/32x32/status/task-complete.png");
            }
            break;
        case COLUMN_NAME:
        case COLUMN_VID_DESCRIPTION:
        case COLUMN_AUD_DESCRIPTION:
        case COLUMN_SIZE:
            break;
        default:
            LOG_ERROR() << "Invalid DecorationRole Column" << index.row() << index.column() << roleNames()[role]
                        << role;
            break;
        }
        break;
    default:
        LOG_ERROR() << "Invalid Role" << index.row() << index.column() << roleNames()[role] << role;
        break;
    }
    return result;
}

QVariant ResourceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole: {
        switch (section) {
        case COLUMN_NAME:
            return tr("Name");
        case COLUMN_SIZE:
            return tr("Size");
        case COLUMN_VID_DESCRIPTION:
            return tr("Video");
        case COLUMN_AUD_DESCRIPTION:
            return tr("Audio");
        default:
            break;
        }
    }
    case Qt::TextAlignmentRole:
        switch (section) {
        case COLUMN_NAME:
            return Qt::AlignLeft;
        case COLUMN_VID_DESCRIPTION:
        case COLUMN_AUD_DESCRIPTION:
        case COLUMN_SIZE:
            return Qt::AlignCenter;
        default:
            break;
        }
        break;
    }

    return QVariant();
}

QModelIndex ResourceModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if (column < 0 || column >= COLUMN_COUNT || row < 0 || row >= m_producers.size())
        return QModelIndex();
    return createIndex(row, column, (int)0);
}

QModelIndex ResourceModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}
