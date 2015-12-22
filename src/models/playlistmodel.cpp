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

#include "playlistmodel.h"
#include "util.h"
#include "shotcut_mlt_properties.h"
#include <QUrl>
#include <QImage>
#include <QColor>
#include <QPainter>
#include <QThreadPool>
#include <QtDebug>
#include <QApplication>
#include <QPalette>
#include <QCryptographicHash>
#include <QScopedPointer>

#include "settings.h"
#include "database.h"
#include "mainwindow.h"

static void deleteQImage(QImage* image)
{
    delete image;
}

class UpdateThumbnailTask : public QRunnable
{
    PlaylistModel* m_model;
    Mlt::Producer m_producer;
    Mlt::Producer* m_tempProducer;
    int m_in;
    int m_out;
    int m_row;

public:
    UpdateThumbnailTask(PlaylistModel* model, Mlt::Producer& producer, int in, int out, int row)
        : QRunnable()
        , m_model(model)
        , m_producer(producer)
        , m_tempProducer(0)
        , m_in(in)
        , m_out(out)
        , m_row(row)
    {}

    ~UpdateThumbnailTask()
    {
        delete m_tempProducer;
    }

    Mlt::Producer* tempProducer()
    {
        if (!m_tempProducer) {
            QString service = m_producer.get("mlt_service");
            if (service == "avformat-novalidate")
                service = "avformat";
            else if (service.startsWith("xml"))
                service = "xml-nogl";
            m_tempProducer = new Mlt::Producer(MLT.profile(), service.toUtf8().constData(), m_producer.get("resource"));
            if (m_tempProducer->is_valid()) {
                Mlt::Filter scaler(MLT.profile(), "swscale");
                Mlt::Filter converter(MLT.profile(), "avcolor_space");
                m_tempProducer->attach(scaler);
                m_tempProducer->attach(converter);
            }
        }
        return m_tempProducer;
    }

    QString cacheKey(int frameNumber)
    {
        QString time = m_producer.frames_to_time(frameNumber, mlt_time_clock);
        // Reduce the precision to centiseconds to increase chance for cache hit
        // without much loss of accuracy.
        time = time.left(time.size() - 1);
        QString key;
        QString resource = m_producer.get(kShotcutHashProperty);
        if (resource.isEmpty()) {
            key = QString("%1 %2 %3")
                    .arg(m_producer.get("mlt_service"))
                    .arg(m_producer.get("resource"))
                    .arg(time);
            QCryptographicHash hash(QCryptographicHash::Sha1);
            hash.addData(key.toUtf8());
            key = hash.result().toHex();
        } else {
            key = QString("%1 %2").arg(resource).arg(time);
        }
        return key;
    }

    void run()
    {
        QString setting = Settings.playlistThumbnails();
        if (setting == "hidden")
            return;

        QImage image = DB.getThumbnail(cacheKey(m_in));
        if (image.isNull()) {
            image = makeThumbnail(m_in);
            m_producer.set(kThumbnailInProperty, new QImage(image), 0, (mlt_destructor) deleteQImage, NULL);
            DB.putThumbnail(cacheKey(m_in), image);
        } else {
            m_producer.set(kThumbnailInProperty, new QImage(image), 0, (mlt_destructor) deleteQImage, NULL);
        }
        m_model->showThumbnail(m_row);

        if (setting == "tall" || setting == "wide") {
            image = DB.getThumbnail(cacheKey(m_out));
            if (image.isNull()) {
                image = makeThumbnail(m_out);
                m_producer.set(kThumbnailOutProperty, new QImage(image), 0, (mlt_destructor) deleteQImage, NULL);
                DB.putThumbnail(cacheKey(m_out), image);
            } else {
                m_producer.set(kThumbnailOutProperty, new QImage(image), 0, (mlt_destructor) deleteQImage, NULL);
            }
            m_model->showThumbnail(m_row);
        }
    }

    QImage makeThumbnail(int frameNumber)
    {
        int height = PlaylistModel::THUMBNAIL_HEIGHT * 2;
        int width = height * MLT.profile().dar();
        return MLT.image(*tempProducer(), frameNumber, width, height);
    }

signals:
    void thumbnailUpdated(int row);
};

PlaylistModel::PlaylistModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_playlist(0)
    , m_dropRow(-1)
{
    qRegisterMetaType<QVector<int> >("QVector<int>");
}

PlaylistModel::~PlaylistModel()
{
    delete m_playlist;
    m_playlist = 0;
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
    if (!m_playlist) return QVariant();
    switch (role) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole: {
        QScopedPointer<Mlt::ClipInfo> info(m_playlist->clip_info(index.row()));
        switch (index.column()) {
        case COLUMN_INDEX:
            return QString::number(index.row() + 1);
        case COLUMN_RESOURCE: {
            QString result;
            if (role == Qt::DisplayRole) {
                // Prefer caption for display
                if (info->producer && info->producer->is_valid())
                    result = info->producer->get(kShotcutCaptionProperty);
                if (result.isNull())
                    result = Util::baseName(QString::fromUtf8(info->resource));
                if (result == "<producer>" && info->producer && info->producer->is_valid())
                    result = QString::fromUtf8(info->producer->get("mlt_service"));
            } else {
                // Prefer detail or full path for tooltip
                if (info->producer && info->producer->is_valid())
                    result = info->producer->get("shotcut:detail");
                if (result.isNull())
                    result = QString::fromUtf8(info->resource);
                if ((result.isNull() || Util::baseName(result) == "<producer>") && info->producer && info->producer->is_valid())
                    result = info->producer->get(kShotcutCaptionProperty);
                if (result.isNull() && info->producer && info->producer->is_valid())
                    result = QString::fromUtf8(info->producer->get("mlt_service"));
            }
            if (!info->producer->get(kShotcutHashProperty))
                MAIN.getHash(*info->producer);
            return result;
        }
        case COLUMN_IN:
            if (info->producer && info->producer->is_valid()) {
                return info->producer->frames_to_time(info->frame_in);
            } else
                return "";
        case COLUMN_DURATION:
            if (info->producer && info->producer->is_valid()) {
                return info->producer->frames_to_time(info->frame_count);
            } else
                return "";
        case COLUMN_START:
            if (info->producer && info->producer->is_valid()) {
                return info->producer->frames_to_time(info->start);
            }
            else
                return "";
        default:
            break;
        }
        break;
    }
    case Qt::DecorationRole:
        if (index.column() == COLUMN_THUMBNAIL) {
            QScopedPointer<Mlt::Producer> producer(m_playlist->get_clip(index.row()));
            Mlt::Producer parent(producer->get_parent());
            int width = THUMBNAIL_HEIGHT * MLT.profile().dar();
            QString setting = Settings.playlistThumbnails();
            QImage image;

            if (setting == "wide")
                image = QImage(width * 2, THUMBNAIL_HEIGHT, QImage::Format_ARGB32_Premultiplied);
            else if (setting == "tall")
                image = QImage(width, THUMBNAIL_HEIGHT * 2, QImage::Format_ARGB32_Premultiplied);
            else if (setting == "large")
                image = QImage(width * 2, THUMBNAIL_HEIGHT * 2, QImage::Format_ARGB32_Premultiplied);
            else
                image = QImage(width, THUMBNAIL_HEIGHT, QImage::Format_ARGB32_Premultiplied);

            if (parent.is_valid() && parent.get_data(kThumbnailInProperty)) {
                QPainter painter(&image);
                image.fill(QColor(Qt::black).rgb());

                // draw the in thumbnail
                QImage* thumb = (QImage*) parent.get_data(kThumbnailInProperty);
                QRect rect = thumb->rect();
                if (setting != "large") {
                    rect.setWidth(width);
                    rect.setHeight(THUMBNAIL_HEIGHT);
                }
                painter.drawImage(rect, *thumb);

                if ((setting == "wide" || setting == "tall") && parent.get_data(kThumbnailOutProperty)) {
                    // draw the out thumbnail
                    thumb = (QImage*) parent.get_data(kThumbnailOutProperty);
                    if (setting == "wide") {
                        rect.setWidth(width * 2);
                        rect.setLeft(width);
                    }
                    else if (setting == "tall") {
                        rect.setHeight(THUMBNAIL_HEIGHT * 2);
                        rect.setTop(THUMBNAIL_HEIGHT);
                    }
                    painter.drawImage(rect, *thumb);
                }
                painter.end();
            }
            else {
                image.fill(QApplication::palette().base().color().rgb());
            }
            return image;
        }
        break;
    default:
        break;
    }
    return QVariant();
}

QVariant PlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section) {
        case COLUMN_INDEX:
            return tr("#");
        case COLUMN_THUMBNAIL:
            return tr("Thumbnails");
        case COLUMN_RESOURCE:
            return tr("Clip");
        case COLUMN_IN:
            return tr("In");
        case COLUMN_DURATION:
            return tr("Duration");
        case COLUMN_START:
            return tr("Start");
        default:
            break;
        }
    }
    return QVariant();
}

Qt::DropActions PlaylistModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction | Qt::LinkAction;
}

bool PlaylistModel::insertRows(int row, int count, const QModelIndex& parent)
{
    Q_UNUSED(count)
    Q_UNUSED(parent)
    if (!m_playlist) return false;
    if (m_dropRow == -1)
        m_dropRow = row;
    return true;
}

bool PlaylistModel::removeRows(int row, int count, const QModelIndex& parent)
{
    Q_UNUSED(count)
    Q_UNUSED(parent)
    if (!m_playlist || row == m_dropRow || m_dropRow == -1 ) return false;
    if (row < m_dropRow)
        emit moveClip(row, m_dropRow - 1);
    else
        emit moveClip(row, m_dropRow);
    m_dropRow = -1;
    return true;
}

QStringList PlaylistModel::mimeTypes() const
{
    QStringList ls = QAbstractTableModel::mimeTypes();
    ls.append(Mlt::XmlMimeType);
    ls.append("text/uri-list");
    return ls;
}

QMimeData *PlaylistModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData;
    Mlt::ClipInfo* info = m_playlist->clip_info(indexes.first().row());
    if (info) {
        Mlt::Producer* producer = info->producer;
        producer->set_in_and_out(info->frame_in, info->frame_out);
        mimeData->setData(Mlt::XmlMimeType, MLT.XML(producer).toUtf8());
        producer->set_in_and_out(0, -1);
        delete info;
    }
    return mimeData;
}

bool PlaylistModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(column)
    Q_UNUSED(parent)
    // Internal reorder
    if (action == Qt::MoveAction) {
        m_dropRow = row;
        return true;
    }
    // Dragged from player or file manager
    else if (data->hasFormat(Mlt::XmlMimeType) || data->hasUrls()) {
        emit dropped(data, row);
        return true;
    }
    // Dragged from Recent dock
    else if (data->hasFormat("application/x-qabstractitemmodeldatalist")) {
        QByteArray encoded = data->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&encoded, QIODevice::ReadOnly);
        QMap<int,  QVariant> roleDataMap;
        while (!stream.atEnd()) {
            int row, col;
            stream >> row >> col >> roleDataMap;
        }
        if (roleDataMap.contains(Qt::ToolTipRole)) {
            QMimeData *mimeData = new QMimeData;
            QList<QUrl> urls;
            // DisplayRole is just basename, ToolTipRole contains full path
            urls.append(roleDataMap[Qt::ToolTipRole].toUrl());
            mimeData->setUrls(urls);
            emit dropped(mimeData, row);
            return true;
        }
    }
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

QModelIndex PlaylistModel::incrementIndex(const QModelIndex& index) const
{
    if (index.row() + 1 < rowCount())
        return createIndex(index.row() + 1, index.column());
    else
        return QModelIndex();
}

QModelIndex PlaylistModel::decrementIndex(const QModelIndex& index) const
{
    if (index.row() > 0)
        return createIndex(index.row() -1, index.column());
    else
        return QModelIndex();
}

QModelIndex PlaylistModel::createIndex(int row, int column) const
{
    return QAbstractTableModel::createIndex(row, column);
}

void PlaylistModel::clear()
{
    if (!m_playlist) return;
    if (rowCount()) {
        beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
        m_playlist->clear();
        endRemoveRows();
    }
    emit cleared();
}

void PlaylistModel::load()
{
    if (m_playlist) {
        if (rowCount()) {
            beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
            m_playlist->clear();
            endRemoveRows();
        }
        delete m_playlist;
    }
    // In some versions of MLT, the resource property is the XML filename,
    // but the Mlt::Playlist(Service&) constructor will fail unless it detects
    // the type as playlist, and mlt_service_identify() needs the resource
    // property to say "<playlist>" to identify it as playlist type.
    MLT.producer()->set("mlt_type", "mlt_producer");
    MLT.producer()->set("resource", "<playlist>");
    m_playlist = new Mlt::Playlist(*MLT.producer());
    if (!m_playlist->is_valid()) {
        delete m_playlist;
        m_playlist = 0;
        return;
    }
    if (m_playlist->count() > 0) {
        beginInsertRows(QModelIndex(), 0, m_playlist->count() - 1);
        endInsertRows();
    }
    // do not let opening a clip change the profile!
    MLT.profile().set_explicit(true);
    emit loaded();
}

void PlaylistModel::append(Mlt::Producer& producer)
{
    createIfNeeded();
    int count = m_playlist->count();
    int in = producer.get_in();
    int out = producer.get_out();
    producer.set_in_and_out(0, producer.get_length() - 1);
    QThreadPool::globalInstance()->start(
        new UpdateThumbnailTask(this, producer, in, out, count));
    beginInsertRows(QModelIndex(), count, count);
    m_playlist->append(producer, in, out);
    endInsertRows();
    emit modified();
}

void PlaylistModel::insert(Mlt::Producer& producer, int row)
{
    createIfNeeded();
    int in = producer.get_in();
    int out = producer.get_out();
    producer.set_in_and_out(0, producer.get_length() - 1);
    QThreadPool::globalInstance()->start(
        new UpdateThumbnailTask(this, producer, in, out, row));
    beginInsertRows(QModelIndex(), row, row);
    m_playlist->insert(producer, row, in, out);
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

void PlaylistModel::update(int row, Mlt::Producer& producer)
{
    if (!m_playlist) return;
    int in = producer.get_in();
    int out = producer.get_out();
    producer.set_in_and_out(0, producer.get_length() - 1);
    QThreadPool::globalInstance()->start(
        new UpdateThumbnailTask(this, producer, in, out, row));
    m_playlist->remove(row);
    m_playlist->insert(producer, row, in, out);
    emit dataChanged(createIndex(row, 0), createIndex(row, COLUMN_COUNT - 1));
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

void PlaylistModel::move(int from, int to)
{
    if (!m_playlist) return;
    m_playlist->move(from, to);
    emit dataChanged(createIndex(from, 0), createIndex(from, COLUMN_COUNT - 1));
    emit dataChanged(createIndex(to, 0), createIndex(to, COLUMN_COUNT - 1));
    emit modified();
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

void PlaylistModel::showThumbnail(int row)
{
    emit dataChanged(createIndex(row, COLUMN_THUMBNAIL), createIndex(row, COLUMN_THUMBNAIL));
}

void PlaylistModel::refreshThumbnails()
{
    if (m_playlist && m_playlist->is_valid()) {
        for (int i = 0; i < m_playlist->count(); i++) {
            Mlt::ClipInfo* info = m_playlist->clip_info(i);
            if (info && info->producer && info->producer->is_valid()) {
                QThreadPool::globalInstance()->start(
                    new UpdateThumbnailTask(this, *info->producer, info->frame_in, info->frame_out, i));
            }
            delete info;
        }
    }
}

void PlaylistModel::setPlaylist(Mlt::Playlist& playlist)
{
    if (playlist.is_valid()) {
        if (m_playlist) {
            if (rowCount()) {
                beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
                m_playlist->clear();
                endRemoveRows();
            }
            delete m_playlist;
        }
        m_playlist = new Mlt::Playlist(playlist);
        if (!m_playlist->is_valid()) {
            delete m_playlist;
            m_playlist = 0;
            return;
        }
        if (m_playlist->count() > 0) {
            beginInsertRows(QModelIndex(), 0, m_playlist->count() - 1);
            endInsertRows();
        }
        // do not let opening a clip change the profile!
        MLT.profile().set_explicit(true);
        if (Settings.playerGPU() && Settings.playlistThumbnails() != "hidden")
            refreshThumbnails();
        emit loaded();
    }
}
