/*
 * Copyright (c) 2020-2021 Meltytech, LLC
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

#ifndef PROXYMANAGER_H
#define PROXYMANAGER_H

#include <QDir>
#include <QString>
#include <QPoint>

namespace Mlt {
class Producer;
class Service;
}

class ProxyManager
{
private:
    ProxyManager() {};

public:
    enum ScanMode {
        Automatic,
        Progressive,
        InterlacedTopFieldFirst,
        InterlacedBottomFieldFirst
    };

    static QDir dir();
    static QString resource(Mlt::Service &producer);
    static void generateVideoProxy(Mlt::Producer &producer, bool fullRange,
                                   ScanMode scanMode = Automatic, const QPoint &aspectRatio = QPoint(), bool replace = true);
    static void generateImageProxy(Mlt::Producer &producer, bool replace = true);
    static bool filterXML(QString &xml, QString root);
    static bool fileExists(Mlt::Producer &producer);
    static bool filePending(Mlt::Producer &producer);
    static bool isValidImage(Mlt::Producer &producer);
    static bool isValidVideo(Mlt::Producer producer);
    static bool generateIfNotExists(Mlt::Producer &producer, bool replace = true);
    static const char *videoFilenameExtension();
    static const char *pendingVideoExtension();
    static const char *imageFilenameExtension();
    static const char *pendingImageExtension();
    static int resolution();
    static void generateIfNotExistsAll(Mlt::Producer &producer);
    static bool removePending();
    static QString GoProProxyFilePath(const QString &resource);
};

#endif // PROXYMANAGER_H
