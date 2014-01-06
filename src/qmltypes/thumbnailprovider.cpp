/*
 * Copyright (c) 2013 Meltytech, LLC
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

#include "thumbnailprovider.h"
#include <QQuickImageProvider>
#include <QCryptographicHash>
#include "mltcontroller.h"
#include "models/playlistmodel.h"
#include "database.h"

#include <QtDebug>

ThumbnailProvider::ThumbnailProvider() :
    QQuickImageProvider(QQmlImageProviderBase::Image,
        QQmlImageProviderBase::ForceAsynchronousImageLoading)
{
}

QImage ThumbnailProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QImage result;

    // id is mlt_service/resource#frameNumber
    int index = id.lastIndexOf('#');

    if (index != -1) {
        QString service = id.section('/', 0, 0);
        QString resource = id.section('/', 1);
        int frameNumber = id.mid(index + 1).toInt();

        if (service.startsWith("xml"))
            service = "xml-nogl";
        resource = resource.left(resource.lastIndexOf('#'));

        Mlt::Producer producer(MLT.profile(), service.toUtf8().constData(), resource.toUtf8().constData());
        if (producer.is_valid()) {
            QString key = cacheKey(producer, frameNumber);
            result = DB.getThumbnail(key);
            if (result.isNull()) {
                result = makeThumbnail(producer, frameNumber, requestedSize);
                DB.putThumbnail(key, result);
            }
            if (size)
                *size = result.size();
        }
    }
    return result;
}

QString ThumbnailProvider::cacheKey(Mlt::Producer& producer, int frameNumber)
{
    QString time = producer.frames_to_time(frameNumber, mlt_time_clock);
    // Reduce the precision to centiseconds to increase chance for cache hit
    // without much loss of accuracy.
    time = time.left(time.size() - 1);
    QString key = QString("%1 %2")
            .arg(producer.get("resource"))
            .arg(time);
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(key.toUtf8());
    return hash.result().toHex();
}

QImage ThumbnailProvider::makeThumbnail(Mlt::Producer &producer, int frameNumber, const QSize& requestedSize)
{
    Mlt::Filter scaler(MLT.profile(), "swscale");
    Mlt::Filter converter(MLT.profile(), "avcolor_space");
    int height = PlaylistModel::THUMBNAIL_HEIGHT * 2;
    int width = height * MLT.profile().dar();

    if (!requestedSize.isEmpty()) {
        width = requestedSize.width();
        height = requestedSize.height();
    }

    producer.attach(scaler);
    producer.attach(converter);
    producer.seek(frameNumber);

    Mlt::Frame* frame = producer.get_frame();
    QImage thumb = MLT.image(frame, width, height);
    delete frame;
    return thumb;
}
