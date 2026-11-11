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

#include "api/apiplaylist.h"

#include "api/apiutil.h"
#include "api/transaction.h"
#include "commands/playlistcommands.h"
#include "docks/playlistdock.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "models/playlistmodel.h"
#include "shotcut_mlt_properties.h"

#include <Mlt.h>
#include <QUndoStack>

#include <memory>

namespace Shotcut {
namespace Api {

ApiPlaylist::ApiPlaylist(QObject *parent)
    : ApiObject(parent)
{
    auto m = model();
    connect(m, &PlaylistModel::created, this, &ApiPlaylist::changed);
    connect(m, &PlaylistModel::cleared, this, &ApiPlaylist::changed);
    connect(m, &PlaylistModel::closed, this, &ApiPlaylist::changed);
    connect(m, &PlaylistModel::loaded, this, &ApiPlaylist::changed);
    connect(m, &PlaylistModel::modified, this, &ApiPlaylist::changed);
    connect(m, &QAbstractItemModel::rowsInserted, this, &ApiPlaylist::changed);
    connect(m, &QAbstractItemModel::rowsRemoved, this, &ApiPlaylist::changed);
    connect(m, &QAbstractItemModel::rowsMoved, this, &ApiPlaylist::changed);
    connect(m, &QAbstractItemModel::modelReset, this, &ApiPlaylist::changed);
}

PlaylistModel *ApiPlaylist::model() const
{
    return MAIN.playlistDock()->model();
}

int ApiPlaylist::count() const
{
    auto playlist = model()->playlist();
    return playlist && playlist->is_valid() ? playlist->count() : 0;
}

QVariantList ApiPlaylist::items() const
{
    QVariantList result;
    const int n = count();
    for (int i = 0; i < n; ++i)
        result.append(itemSnapshot(i));
    return result;
}

QVariant ApiPlaylist::item(int index) const
{
    if (index < 0 || index >= count())
        return fail(ErrorCode::NotFound, tr("Playlist item %1 does not exist.").arg(index));
    return itemSnapshot(index);
}

QVariant ApiPlaylist::addItem(const QVariantMap &options)
{
    ApiCallGuard callGuard;
    if (!checkOptions("addItem", options))
        return QVariant();
    const int index = options.value("index", count()).toInt();
    if (index < 0 || index > count())
        return fail(ErrorCode::InvalidArgument,
                    tr("Insert index must be between 0 and %1.").arg(count()));
    QString errorCode, errorMessage;
    auto producer = prepareMedia(options, errorCode, errorMessage);
    if (!producer)
        return fail(errorCode, errorMessage);
    const int in = producer->get_in();
    const int out = producer->get_out();
    const int before = count();
    Transaction::singleton().touch(tr("Add playlist item"));
    if (index == before)
        MAIN.undoStack()->push(new Playlist::AppendCommand(*model(), MLT.XML(producer.get())));
    else
        MAIN.undoStack()->push(
            new Playlist::InsertCommand(*model(), MLT.XML(producer.get()), index));
    const auto added = itemSnapshot(index);
    if (count() != before + 1 || added.value("in").toInt() != in
        || added.value("out").toInt() != out) {
        const QString error = tr(
            "The playlist item could not be added; the edits are rolled back.");
        Transaction::singleton().fail(error);
        return fail(ErrorCode::Unavailable, error);
    }
    return added;
}

QVariant ApiPlaylist::removeItem(int index)
{
    ApiCallGuard callGuard;
    if (index < 0 || index >= count())
        return fail(ErrorCode::NotFound, tr("Playlist item %1 does not exist.").arg(index));
    const auto removed = itemSnapshot(index);
    Transaction::singleton().touch();
    MAIN.undoStack()->push(new Playlist::RemoveCommand(*model(), index));
    return removed;
}

QVariant ApiPlaylist::moveItem(int index, int toIndex)
{
    ApiCallGuard callGuard;
    if (index < 0 || index >= count())
        return fail(ErrorCode::NotFound, tr("Playlist item %1 does not exist.").arg(index));
    if (toIndex < 0 || toIndex >= count())
        return fail(ErrorCode::InvalidArgument,
                    tr("Destination index must be between 0 and %1.").arg(count() - 1));
    if (index != toIndex) {
        Transaction::singleton().touch();
        MAIN.undoStack()->push(new Playlist::MoveCommand(*model(), index, toIndex));
    }
    return itemSnapshot(toIndex);
}

QVariant ApiPlaylist::trimItem(int index, const QVariantMap &options)
{
    ApiCallGuard callGuard;
    if (!checkOptions("trimItem", options))
        return QVariant();
    if (index < 0 || index >= count())
        return fail(ErrorCode::NotFound, tr("Playlist item %1 does not exist.").arg(index));
    if (options.isEmpty())
        return fail(ErrorCode::InvalidArgument, tr("Give an in frame, an out frame, or both."));
    const auto before = itemSnapshot(index);
    const int in = options.value("in", before.value("in")).toInt();
    const int out = options.value("out", before.value("out")).toInt();
    const int length = before.value("length").toInt();
    if (in < 0 || out < in || out >= length)
        return fail(ErrorCode::InvalidArgument,
                    tr("Use source frames 0 <= in <= out < %1.").arg(length));
    const int oldIn = before.value("in").toInt();
    const int oldOut = before.value("out").toInt();
    if (in == oldIn && out == oldOut)
        return before;
    Transaction::singleton().touch(tr("Trim playlist item"));
    if (in > oldOut && out != oldOut)
        MAIN.undoStack()->push(new Playlist::TrimClipOutCommand(*model(), index, out));
    if (in != oldIn)
        MAIN.undoStack()->push(new Playlist::TrimClipInCommand(*model(), index, in));
    if (in <= oldOut && out != oldOut)
        MAIN.undoStack()->push(new Playlist::TrimClipOutCommand(*model(), index, out));
    return itemSnapshot(index);
}

QVariantMap ApiPlaylist::itemSnapshot(int index) const
{
    QVariantMap result;
    std::unique_ptr<Mlt::ClipInfo> info(model()->playlist()->clip_info(index));
    if (!info || !info->producer || !info->producer->is_valid())
        return result;

    result = producerSnapshot(info->producer);
    result["index"] = index;
    result["in"] = info->frame_in;
    result["out"] = info->frame_out;
    result["duration"] = info->frame_count;
    result["bin"] = QString::fromUtf8(info->producer->get(kShotcutBinsProperty));
    switch (model()->data(model()->index(index, 0), PlaylistModel::FIELD_MEDIA_TYPE_ENUM).toInt()) {
    case PlaylistModel::Video:
        result["mediaType"] = QStringLiteral("video");
        break;
    case PlaylistModel::Image:
        result["mediaType"] = QStringLiteral("image");
        break;
    case PlaylistModel::Audio:
        result["mediaType"] = QStringLiteral("audio");
        break;
    default:
        result["mediaType"] = QStringLiteral("other");
        break;
    }
    return result;
}

} // namespace Api
} // namespace Shotcut
