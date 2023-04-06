/*
 * Copyright (c) 2023 Meltytech, LLC
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

#include "util.h"
#include "Logger.h"
#include "mainwindow.h"

class ProducerFinder : public Mlt::Parser
{
public:
    ProducerFinder(ResourceModel *model)
        : Mlt::Parser()
        , m_model(model)
    {}

    int on_start_producer(Mlt::Producer *producer)
    {
        m_model->add(producer);
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
    int on_start_playlist(Mlt::Playlist *)
    {
        return 0;
    }
    int on_end_playlist(Mlt::Playlist *)
    {
        return 0;
    }
    int on_start_tractor(Mlt::Tractor *)
    {
        return 0;
    }
    int on_end_tractor(Mlt::Tractor *)
    {
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
};

ResourceModel::ResourceModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    LOG_DEBUG() << "Start parser";
    Mlt::Producer multitrack(*MAIN.multitrack());
    search(&multitrack);
    Mlt::Producer playlist(*MAIN.playlist());
    search(&playlist);
}

ResourceModel::~ResourceModel()
{
}

void ResourceModel::clear()
{
    beginResetModel();
    m_producers.clear();
    endResetModel();
}

void ResourceModel::search(Mlt::Producer *producer)
{
    ProducerFinder parser(this);
    parser.start(*producer);
}

void ResourceModel::add(Mlt::Producer *producer)
{
    if (producer->is_blank()) {
        // Do not add
    } else if (producer->is_cut()) {
        Mlt::Producer parent = producer->parent();
        QString hash = Util::getHash(parent);
        if (!hash.isEmpty() && !exists(hash)) {
            m_producers.append(parent);
        }
    } else {
        QString hash = Util::getHash(*producer);
        if (!hash.isEmpty() && !exists(hash)) {
            m_producers.append(*producer);
        }
    }
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
    case Qt::DecorationRole:
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
            LOG_ERROR() << "Invalid Column" << index.row() << index.column() << roleNames()[role] << role;
            break;
        }
        break;
    case Qt::ToolTipRole:
        return Util::GetFilenameFromProducer(producer, true);
    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case COLUMN_NAME:
        case COLUMN_VID_DESCRIPTION:
        case COLUMN_AUD_DESCRIPTION:
            result = Qt::AlignLeft;
            break;
        case COLUMN_SIZE:
            result = Qt::AlignRight;
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
