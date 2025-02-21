/*
 * Copyright (c) 2013-2016 Meltytech, LLC
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

#ifndef THUMBNAILPROVIDER_H
#define THUMBNAILPROVIDER_H

#include <MltProducer.h>
#include <MltProfile.h>
#include <QQuickImageProvider>

class ThumbnailProvider : public QQuickImageProvider
{
public:
    explicit ThumbnailProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);

private:
    QString cacheKey(Mlt::Properties &properties,
                     const QString &service,
                     const QString &resource,
                     const QString &hash,
                     int frameNumber);
    QImage makeThumbnail(Mlt::Producer &, int frameNumber, const QSize &requestedSize);
    Mlt::Profile m_profile;
};

#endif // THUMBNAILPROVIDER_H
