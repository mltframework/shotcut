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
#include <QFileInfo>
#include <QUrl>
#include <QImage>
#include <QColor>
#include <QPainter>
#include <QThreadPool>
#include <QtDebug>
#include <QApplication>
#include <QPalette>
#include "settings.h"

static const char* kThumbnailInProperty = "shotcut:thumbnail-in";
static const char* kThumbnailOutProperty = "shotcut:thumbnail-out";

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
        , m_in(in)
        , m_out(out)
        , m_row(row)
    {
        QString service = producer.get("mlt_service");
        if (service.startsWith("xml"))
            service = "xml-nogl";
        m_tempProducer = new Mlt::Producer(MLT.profile(), service.toUtf8().constData(), producer.get("resource"));
        if (m_tempProducer->is_valid()) {
            Mlt::Filter scaler(MLT.profile(), "swscale");
            Mlt::Filter converter(MLT.profile(), "avcolor_space");
            m_tempProducer->attach(scaler);
            m_tempProducer->attach(converter);
        }
    }
    ~UpdateThumbnailTask()
    {
        delete m_tempProducer;
    }
    void run() {
        m_model->makeThumbnail(*m_tempProducer, m_in, m_out, m_row);
        QImage* image = (QImage*) m_tempProducer->get_data(kThumbnailInProperty);
        if (image)
            m_producer.set(kThumbnailInProperty, new QImage(*image), 0, (mlt_destructor) deleteQImage, NULL);
        image = (QImage*) m_tempProducer->get_data(kThumbnailOutProperty);
        if (image)
            m_producer.set(kThumbnailOutProperty, new QImage(*image), 0, (mlt_destructor) deleteQImage, NULL);
    }
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
        const Mlt::ClipInfo* info = m_playlist->clip_info(index.row());
        switch (index.column()) {
        case COLUMN_INDEX:
            return QString::number(index.row() + 1);
        case COLUMN_RESOURCE: {
            QString result = QString::fromUtf8(info->resource);
            if (result == "<producer>" && info->producer
                    && info->producer->is_valid() && info->producer->get("mlt_service"))
                result = QString::fromUtf8(info->producer->get("mlt_service"));
            // Use basename for display
            if (role == Qt::DisplayRole && result.startsWith('/'))
                result = QFileInfo(result).fileName();
            return result;
        }
        case COLUMN_IN:
            if (info->producer && info->producer->is_valid()) {
                info->producer->set("_shotcut_time", info->frame_in);
                return info->producer->get_time("_shotcut_time");
            } else
                return "";
        case COLUMN_DURATION:
            if (info->producer && info->producer->is_valid()) {
                info->producer->set("_shotcut_time", info->frame_count);
                return info->producer->get_time("_shotcut_time");
            } else
                return "";
        case COLUMN_START:
            if (info->producer && info->producer->is_valid()) {
                info->producer->set("_shotcut_time", info->start);
                return info->producer->get_time("_shotcut_time");
            }
            else
                return "";
        default:
            break;
        }
        delete info;
        break;
    }
    case Qt::DecorationRole:
        if (index.column() == COLUMN_THUMBNAIL) {
            Mlt::Producer* producer = m_playlist->get_clip(index.row());
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
            delete producer;
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
    if (!m_playlist) return false;
    if (m_dropRow == -1)
        m_dropRow = row;
    return true;
}

bool PlaylistModel::removeRows(int row, int count, const QModelIndex& parent)
{
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
    ls.append("application/mlt+xml");
    ls.append("text/uri-list");
    return ls;
}

QMimeData *PlaylistModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("application/mlt+xml", "");
    return mimeData;
}

bool PlaylistModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    // Internal reorder
    if (action == Qt::MoveAction) {
        m_dropRow = row;
        return true;
    }
    // Dragged from player or file manager
    else if (data->hasFormat("application/mlt+xml") || data->hasUrls()) {
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
    // In some versions of MLT, the resource property is the XML filename,
    // but the Mlt::Playlist(Service&) constructor will fail unless it detects
    // the type as playlist, and mlt_service_identify() needs the resource
    // property to say "<playlist>" to identify it as playlist type.
    MLT.producer()->set("resource", "<playlist>");
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

void PlaylistModel::makeThumbnail(Mlt::Producer producer, int in, int out, int row)
{
    QString setting = Settings.playlistThumbnails();
    if (setting == "hidden")
        return;
    int height = PlaylistModel::THUMBNAIL_HEIGHT * 2;
    int width = height * MLT.profile().dar();

    // render the in point thumbnail
    producer.seek(in);
    Mlt::Frame* frame = producer.get_frame();
    QImage thumb = MLT.image(frame, width, height);
    delete frame;
    producer.set(kThumbnailInProperty, new QImage(thumb), 0, (mlt_destructor) deleteQImage, NULL);

    if (setting == "tall" || setting == "wide") {
        // render the out point thumbnail
        producer.seek(out);
        frame = producer.get_frame();
        thumb = MLT.image(frame, width, height);
        delete frame;
        producer.set(kThumbnailOutProperty, new QImage(thumb), 0, (mlt_destructor) deleteQImage, NULL);
    }
    if (row >= 0)
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
