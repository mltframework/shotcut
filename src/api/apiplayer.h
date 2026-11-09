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

#ifndef SHOTCUT_API_APIPLAYER_H
#define SHOTCUT_API_APIPLAYER_H

#include "api/apiobject.h"

#include <QVariantMap>

namespace Shotcut {
namespace Api {

class ApiPlayer : public ApiObject
{
    Q_OBJECT
    Q_CLASSINFO("doc",
                "The player (preview) and what it is showing: the timeline, the playlist, or a "
                "single source clip.")
    Q_CLASSINFO("doc.position", "Player position in frames.")
    Q_CLASSINFO("doc.duration", "Length of what the player is showing, in frames.")
    Q_CLASSINFO("doc.inPoint", "In point of the player source, in frames.")
    Q_CLASSINFO("doc.outPoint", "Out point of the player source, in frames.")
    Q_CLASSINFO("doc.playing", "Whether playback is running.")
    Q_CLASSINFO("doc.source", "What the player shows: timeline, playlist, clip, or none.")
    Q_CLASSINFO("doc.tab", "Selected player tab: project or source.")
    Q_PROPERTY(int position READ position NOTIFY positionChanged)
    Q_PROPERTY(int duration READ duration NOTIFY changed)
    Q_PROPERTY(int inPoint READ inPoint NOTIFY changed)
    Q_PROPERTY(int outPoint READ outPoint NOTIFY changed)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY changed)
    Q_PROPERTY(QString source READ source NOTIFY changed)
    Q_PROPERTY(QString tab READ tab NOTIFY changed)

public:
    explicit ApiPlayer(QObject *parent = nullptr);

    int position() const;
    int duration() const;
    int inPoint() const;
    int outPoint() const;
    bool isPlaying() const;
    QString source() const;
    QString tab() const;

    Q_CLASSINFO("mcp.producer", "read")
    Q_CLASSINFO("doc.producer",
                "Describes the media the player is showing: name, service, resource, in, out, "
                "duration, and length.")
    Q_INVOKABLE QVariantMap producer() const;

    Q_CLASSINFO("mcp.seek", "action")
    Q_CLASSINFO("doc.seek",
                "Moves the player to a frame of what it is showing and returns the frame. Use "
                "timeline.seek to show and seek the timeline.")
    Q_CLASSINFO("arg.seek.position", "Frame; clamped to the end.")
    Q_INVOKABLE QVariant seek(int position);

    Q_CLASSINFO("mcp.play", "action")
    Q_CLASSINFO("doc.play", "Starts playback.")
    Q_CLASSINFO("arg.play.speed",
                "number|Playback speed; 1 is normal, negative plays backwards. Defaults to 1.")
    Q_INVOKABLE QVariant play(const QVariantMap &options = {});

    Q_CLASSINFO("mcp.pause", "action")
    Q_CLASSINFO("doc.pause",
                "Pauses playback and returns the position, which may still advance by a frame.")
    Q_INVOKABLE QVariant pause();

signals:
    void changed();
    void positionChanged();

private:
    void onFrameDisplayed();

    int m_lastPosition = -1;
};

} // namespace Api
} // namespace Shotcut

#endif // SHOTCUT_API_APIPLAYER_H
