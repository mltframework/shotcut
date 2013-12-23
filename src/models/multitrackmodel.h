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

#ifndef MULTITRACKMODEL_H
#define MULTITRACKMODEL_H

#include <QAbstractItemModel>
#include <QList>
#include <QString>
#include <MltTractor.h>
#include <MltPlaylist.h>

typedef enum {
    PlaylistTrackType = 0,
    BlackTrackType,
    SilentTrackType,
    AudioTrackType,
    VideoTrackType
} TrackType;

typedef struct {
    TrackType type;
    int number;
    int mlt_index;
} Track;

typedef QList<Track> TrackList;

class MultitrackModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    /// Two level model: tracks and clips on track
    enum {
        NameRole = Qt::UserRole + 1,
        ResourceRole,  /// clip only
        ServiceRole,   /// clip only
        IsBlankRole,   /// clip only
        StartRole,     /// clip only
        DurationRole,
        InPointRole,   /// clip only
        OutPointRole,  /// clip only
        FramerateRole, /// clip only
        IsMuteRole,    /// track only
        IsHiddenRole,  /// track only
        IsAudioRole,
        AudioLevelsRole    /// clip only
    };

    explicit MultitrackModel(QObject *parent = 0);
    ~MultitrackModel();

    Mlt::Tractor* tractor() const { return m_tractor; }

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    QHash<int, QByteArray> roleNames() const;
    void audioLevelsReady(const QModelIndex &index);
    void createIfNeeded();
    void addBackgroundTrack();
    void addAudioTrack();
    void addVideoTrack();
    void load();
    void close();

signals:
    void created();
    void loaded();
    void closed();
    void modified();

public slots:
    void refreshTrackList();
    void setTrackName(int row, const QString &value);
    void setTrackMute(int row, bool mute);
    void setTrackHidden(int row, bool hidden);
    void trimClipIn(int trackIndex, int clipIndex, int delta);
    void notifyClipIn(int trackIndex, int clipIndex);
    void trimClipOut(int trackIndex, int clipIndex, int delta);
    void notifyClipOut(int trackIndex, int clipIndex);
    bool moveClip(int trackIndex, int clipIndex, int position);
    void appendClip(int trackIndex, Mlt::Producer &clip);
    void removeClip(int trackIndex, int clipIndex);
    void liftClip(int trackIndex, int clipIndex);

private:
    Mlt::Tractor* m_tractor;
    TrackList m_trackList;

    void moveClipToEnd(Mlt::Playlist& playlist, int trackIndex, int clipIndex, int position);
    void relocateClip(Mlt::Playlist& playlist, int trackIndex, int clipIndex, int position);
    void moveClipInBlank(Mlt::Playlist& playlist, int trackIndex, int clipIndex, int position);
    void consolidateBlanks(Mlt::Playlist& playlist, int trackIndex);
    void getAudioLevels();
    void addBlackTrackIfNeeded();
    void addMissingTransitions();
};

#endif // MULTITRACKMODEL_H
