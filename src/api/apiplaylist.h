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

#ifndef SHOTCUT_API_APIPLAYLIST_H
#define SHOTCUT_API_APIPLAYLIST_H

#include "api/apiobject.h"

#include <QVariantList>
#include <QVariantMap>

class PlaylistModel;

namespace Shotcut {
namespace Api {

class ApiPlaylist : public ApiObject
{
    Q_OBJECT
    Q_CLASSINFO("doc", "The playlist (source bin) of media used by the project.")
    Q_CLASSINFO("doc.count", "Number of playlist items.")
    Q_PROPERTY(int count READ count NOTIFY changed)

public:
    explicit ApiPlaylist(QObject *parent = nullptr);

    int count() const;

    Q_CLASSINFO("mcp.items", "read")
    Q_CLASSINFO("doc.items",
                "Lists playlist items with index, name, resource, media type, bin, in, out, "
                "and duration in frames.")
    Q_INVOKABLE QVariantList items() const;

    Q_CLASSINFO("mcp.item", "read")
    Q_CLASSINFO("doc.item", "Returns one playlist item.")
    Q_CLASSINFO("arg.item.index", "Playlist index, starting at 0.")
    Q_INVOKABLE QVariant item(int index) const;

    Q_CLASSINFO("mcp.addItem", "edit")
    Q_CLASSINFO("doc.addItem",
                "Adds a local media file or MLT virtual clip to the playlist and returns the "
                "item. Defaults to appending the full clip (images use Shotcut's default "
                "duration). Does not open the file in the player or change timeline clips. "
                "Proxy generation follows Shotcut settings and is not undone.")
    Q_CLASSINFO("arg.addItem.resource", "string!|Local file path; not a directory or live source.")
    Q_CLASSINFO("arg.addItem.index", "integer|Insert before this index; defaults to count.")
    Q_CLASSINFO("arg.addItem.in", "integer|Inclusive source in frame; defaults to the file's in.")
    Q_CLASSINFO("arg.addItem.out", "integer|Inclusive source out frame; defaults to the file's out.")
    Q_INVOKABLE QVariant addItem(const QVariantMap &options);

    Q_CLASSINFO("mcp.removeItem", "edit")
    Q_CLASSINFO("doc.removeItem",
                "Removes a playlist item and returns what it was. Does not remove timeline "
                "clips or delete media files. Later playlist indexes shift down.")
    Q_CLASSINFO("arg.removeItem.index", "Playlist index.")
    Q_INVOKABLE QVariant removeItem(int index);

    Q_CLASSINFO("mcp.moveItem", "edit")
    Q_CLASSINFO("doc.moveItem",
                "Reorders a playlist item and returns it at its new index. Both indexes refer "
                "to the complete playlist, not the currently displayed bin.")
    Q_CLASSINFO("arg.moveItem.index", "Current playlist index.")
    Q_CLASSINFO("arg.moveItem.toIndex", "Final playlist index, between 0 and count minus 1.")
    Q_INVOKABLE QVariant moveItem(int index, int toIndex);

    Q_CLASSINFO("mcp.trimItem", "edit")
    Q_CLASSINFO("doc.trimItem",
                "Sets inclusive source in/out frames on a playlist item and returns it. "
                "Omitted ends stay unchanged. Does not trim timeline instances.")
    Q_CLASSINFO("arg.trimItem.index", "Playlist index.")
    Q_CLASSINFO("arg.trimItem.in", "integer|New inclusive source in frame.")
    Q_CLASSINFO("arg.trimItem.out", "integer|New inclusive source out frame.")
    Q_INVOKABLE QVariant trimItem(int index, const QVariantMap &options);

signals:
    void changed();

private:
    PlaylistModel *model() const;
    QVariantMap itemSnapshot(int index) const;
};

} // namespace Api
} // namespace Shotcut

#endif // SHOTCUT_API_APIPLAYLIST_H
