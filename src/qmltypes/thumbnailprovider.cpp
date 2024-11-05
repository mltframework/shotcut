/*
 * Copyright (c) 2013-2023 Meltytech, LLC
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
#include "util.h"
#include "settings.h"

#include <Logger.h>

ThumbnailProvider::ThumbnailProvider()
    : QQuickImageProvider(QQmlImageProviderBase::Image,
                          QQmlImageProviderBase::ForceAsynchronousImageLoading)
    , m_profile("atsc_720p_60")
{
}

QImage ThumbnailProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QImage result;

    // id is [hash]/mlt_service/resource#frameNumber[!]
    // optional trailing '!' means to force update
    int index = id.lastIndexOf('#');

    if (index != -1) {
        QString myId = id;
        bool force = id.endsWith('!');
        if (force)
            myId = id.left(id.size() - 1);
        QString hash = myId.section('/', 0, 0);
        QString service = myId.section('/', 1, 1);
        QString resource = myId.section('/', 2);
        int frameNumber = myId.mid(index + 1).toInt();
        Mlt::Properties properties;

        // Scale the frameNumber to ThumbnailProvider profile's fps.
        frameNumber = qRound(frameNumber / MLT.profile().fps() * m_profile.fps());

        resource = resource.left(resource.lastIndexOf('#'));
        resource = Util::removeQueryString(resource);
        properties.set("_profile", m_profile.get_profile(), 0);

        QString key = cacheKey(properties, service, resource, hash, frameNumber);
        result = DB.getThumbnail(key);
        if (force || result.isNull()) {
            if (service == "avformat-novalidate")
                service = "avformat";
            else if (service.startsWith("xml"))
                service = "xml-nogl";
            Mlt::Producer producer;
            if (service == "count") {
                producer = Mlt::Producer(m_profile, service.toUtf8().constData(), "loader-nogl");
            } else if (!Settings.playerGPU() || (service != "xml-nogl" && service != "consumer")) {
                producer = Mlt::Producer(m_profile, service.toUtf8().constData(), resource.toUtf8().constData());
            }
            if (producer.is_valid()) {
                result = makeThumbnail(producer, frameNumber, requestedSize);
                DB.putThumbnail(key, result);
            }
        }
    }
    if (result.isNull()) {
        result = QImage(1, 1, QImage::Format_Alpha8);
        result.fill(0);
    }
    if (size)
        *size = result.size();
    return result;
}

QString ThumbnailProvider::cacheKey(Mlt::Properties &properties, const QString &service,
                                    const QString &resource, const QString &hash, int frameNumber)
{
    QString time = properties.frames_to_time(frameNumber, mlt_time_clock);
    // Reduce the precision to centiseconds to increase chance for cache hit
    // without much loss of accuracy.
    time = time.left(time.size() - 1);
    QString key;
    if (hash.isEmpty()) {
        key = QStringLiteral("%1 %2 %3")
              .arg(service)
              .arg(resource)
              .arg(time);
        QCryptographicHash hash(QCryptographicHash::Sha1);
        hash.addData(key.toUtf8());
        key = hash.result().toHex();
    } else {
        key = QStringLiteral("%1 %2").arg(hash).arg(time);
    }
    return key;
}

QImage ThumbnailProvider::makeThumbnail(Mlt::Producer &producer, int frameNumber,
                                        const QSize &requestedSize)
{
    Mlt::Filter scaler(m_profile, "swscale");
    Mlt::Filter padder(m_profile, "resize");
    Mlt::Filter converter(m_profile, "avcolor_space");
    int height = PlaylistModel::THUMBNAIL_HEIGHT * 2;
    int width = PlaylistModel::THUMBNAIL_WIDTH * 2;

    if (!requestedSize.isEmpty()) {
        width = requestedSize.width();
        height = requestedSize.height();
    }

    producer.attach(scaler);
    producer.attach(padder);
    producer.attach(converter);
    return MLT.image(producer, frameNumber, width, height);
}
