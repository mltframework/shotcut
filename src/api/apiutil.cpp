/*
 * Copyright (c) 2026 Meltytech, LLC
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

#include "api/apiutil.h"

#include "api/apiobject.h"
#include "docks/playlistdock.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "proxymanager.h"
#include "shotcut_mlt_properties.h"
#include "util.h"

#include <Mlt.h>
#include <QCoreApplication>
#include <QFileInfo>

namespace Shotcut {
namespace Api {

QString clipId(Mlt::ClipInfo &info)
{
    if (info.cut && info.cut->is_blank())
        return MLT.ensureHasUuid(*info.cut).toString(QUuid::WithoutBraces);
    if (info.producer && info.producer->is_valid())
        return producerId(*info.producer);
    return QString();
}

QString producerId(Mlt::Producer &producer)
{
    return MLT.ensureHasUuid(producer).toString(QUuid::WithoutBraces);
}

std::unique_ptr<Mlt::Producer> prepareMedia(const QVariantMap &options,
                                            QString &errorCode,
                                            QString &errorMessage)
{
    auto fail = [&](const QString &code, const QString &message) {
        errorCode = code;
        errorMessage = message;
        return std::unique_ptr<Mlt::Producer>();
    };
    auto tr = [](const char *text) { return QCoreApplication::translate("ApiMedia", text); };
    if (options.contains("resource") == options.contains("playlistIndex"))
        return fail(ErrorCode::InvalidArgument, tr("Give exactly one resource or playlistIndex."));
    std::unique_ptr<Mlt::Producer> producer;
    if (options.contains("playlistIndex")) {
        auto playlist = MAIN.playlistDock()->model()->playlist();
        const int index = options.value("playlistIndex").toInt();
        if (!playlist || index < 0 || index >= playlist->count())
            return fail(ErrorCode::NotFound, tr("Playlist item %1 does not exist.").arg(index));
        std::unique_ptr<Mlt::ClipInfo> info(playlist->clip_info(index));
        if (!info || !info->producer || !info->producer->is_valid())
            return fail(ErrorCode::Unavailable, tr("The playlist item cannot be read."));
        producer = std::make_unique<Mlt::Producer>(MLT.profile(),
                                                   "xml-string",
                                                   MLT.XML(info->producer).toUtf8().constData());
        if (producer->is_valid())
            producer->set_in_and_out(info->frame_in, info->frame_out);
    } else {
        const QFileInfo file(options.value("resource").toString());
        if (!file.exists() || !file.isFile() || !file.isReadable())
            return fail(ErrorCode::NotFound,
                        tr("Media file does not exist or is not readable: %1.").arg(file.filePath()));
        const QString path = file.absoluteFilePath();
        if (MAIN.isSourceClipMyProject(path, false))
            return fail(ErrorCode::InvalidArgument,
                        tr("The open project cannot be added to itself."));
        if (MLT.checkFile(path))
            return fail(ErrorCode::Unavailable, tr("Shotcut cannot open media file: %1.").arg(path));
        Mlt::Producer source = file.suffix().compare("mlt", Qt::CaseInsensitive) == 0
                                       || file.suffix().compare("xml", Qt::CaseInsensitive) == 0
                                   ? Util::openMltVirtualClip(path)
                                   : Mlt::Producer(MLT.profile(), path.toUtf8().constData());
        if (!source.is_valid())
            return fail(ErrorCode::Unavailable, tr("Shotcut cannot open media file: %1.").arg(path));
        producer.reset(MLT.setupNewProducer(&source));
    }
    if (!producer || !producer->is_valid()
        || (MLT.isLiveProducer(producer.get()) && !producer->get_int(kShotcutVirtualClip)))
        return fail(ErrorCode::Unavailable, tr("The media must have a finite duration."));
    const int in = options.value("in", producer->get_in()).toInt();
    const int out = options.value("out", producer->get_out()).toInt();
    if (in < 0 || out < in || out >= producer->get_length())
        return fail(ErrorCode::InvalidArgument,
                    tr("Use source frames 0 <= in <= out < %1.").arg(producer->get_length()));
    producer->set_in_and_out(in, out);
    producer->set(kShotcutSkipConvertProperty, 1);
    const auto uuid = QUuid::createUuid();
    MLT.setUuid(*producer, uuid);
    MLT.setUuid(producer->parent(), uuid);
    ProxyManager::generateIfNotExists(*producer);
    return producer;
}

QVariantMap producerSnapshot(Mlt::Producer *producer)
{
    QVariantMap result;
    if (!producer || !producer->is_valid())
        return result;

    Mlt::Producer named(producer->is_cut() ? producer->parent() : Mlt::Producer(*producer));
    if (!named.is_valid())
        return result;
    result["id"] = producerId(named);
    result["name"] = Util::clipDisplayName(&named);
    result["service"] = QString::fromUtf8(named.get("mlt_service"));
    result["resource"] = QString::fromUtf8(named.get("resource"));
    result["comment"] = QString::fromUtf8(named.get(kCommentProperty));
    result["isProxy"] = named.get_int(kIsProxyProperty) != 0;
    result["in"] = producer->get_in();
    result["out"] = producer->get_out();
    result["duration"] = producer->get_playtime();
    result["length"] = producer->get_length();
    return result;
}

} // namespace Api
} // namespace Shotcut
