/*
 * Copyright (c) 2013-2015 Meltytech, LLC
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
    Q_PROPERTY(int trackHeight READ trackHeight WRITE setTrackHeight NOTIFY trackHeightChanged)
    Q_PROPERTY(double scaleFactor READ scaleFactor WRITE setScaleFactor NOTIFY scaleFactorChanged)

public:
    /// Two level model: tracks and clips on track
    enum {
        NameRole = Qt::UserRole + 1,
        ResourceRole,    /// clip only
        ServiceRole,     /// clip only
        IsBlankRole,     /// clip only
        StartRole,       /// clip only
        DurationRole,
        InPointRole,     /// clip only
        OutPointRole,    /// clip only
        FramerateRole,   /// clip only
        IsMuteRole,      /// track only
        IsHiddenRole,    /// track only
        IsAudioRole,
        AudioLevelsRole, /// clip only
        IsCompositeRole, /// track only
        IsLockedRole,    /// track only
        FadeInRole,      /// clip only
        FadeOutRole,     /// clip only
        IsTransitionRole /// clip only
    };

    explicit MultitrackModel(QObject *parent = 0);
    ~MultitrackModel();

    Mlt::Tractor* tractor() const { return m_tractor; }
    const TrackList& trackList() const { return m_trackList; }

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex index(int row, int column = 0,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    QHash<int, QByteArray> roleNames() const;
    void audioLevelsReady(const QModelIndex &index);
    bool createIfNeeded();
    void addBackgroundTrack();
    int addAudioTrack();
    int addVideoTrack();
    void removeTrack(int trackIndex);
    void load();
    Q_INVOKABLE void reload();
    void close();
    int clipIndex(int trackIndex, int position);
    bool trimClipInValid(int trackIndex, int clipIndex, int delta);
    bool trimClipOutValid(int trackIndex, int clipIndex, int delta);
    int trackHeight() const;
    void setTrackHeight(int height);
    double scaleFactor() const;
    void setScaleFactor(double scale);
    bool isTransition(Mlt::Playlist& playlist, int clipIndex) const;
    void insertTrack(int trackIndex, TrackType type = VideoTrackType);
    void insertOrAdjustBlankAt(QList<int> tracks, int position, int length);

signals:
    void created();
    void loaded();
    void closed();
    void modified();
    void seeked(int position);
    void trackHeightChanged();
    void scaleFactorChanged();

public slots:
    void refreshTrackList();
    void setTrackName(int row, const QString &value);
    void setTrackMute(int row, bool mute);
    void setTrackHidden(int row, bool hidden);
    void setTrackComposite(int row, Qt::CheckState composite);
    void setTrackLock(int row, bool lock);
    int trimClipIn(int trackIndex, int clipIndex, int delta, bool ripple);
    void notifyClipIn(int trackIndex, int clipIndex);
    int trimClipOut(int trackIndex, int clipIndex, int delta, bool ripple);
    void notifyClipOut(int trackIndex, int clipIndex);
    bool moveClipValid(int fromTrack, int toTrack, int clipIndex, int position);
    bool moveClip(int fromTrack, int toTrack, int clipIndex, int position);
    int overwriteClip(int trackIndex, Mlt::Producer& clip, int position);
    QString overwrite(int trackIndex, Mlt::Producer& clip, int position);
    int insertClip(int trackIndex, Mlt::Producer& clip, int position);
    int appendClip(int trackIndex, Mlt::Producer &clip);
    void removeClip(int trackIndex, int clipIndex);
    void liftClip(int trackIndex, int clipIndex);
    void splitClip(int trackIndex, int clipIndex, int position);
    void joinClips(int trackIndex, int clipIndex);
    void appendFromPlaylist(Mlt::Playlist* playlist, int trackIndex);
    void overwriteFromPlaylist(Mlt::Playlist& playlist, int trackIndex, int position);
    void fadeIn(int trackIndex, int clipIndex, int duration);
    void fadeOut(int trackIndex, int clipIndex, int duration);
    bool addTransitionValid(int fromTrack, int toTrack, int clipIndex, int position);
    int addTransition(int trackIndex, int clipIndex, int position);
    void removeTransition(int trackIndex, int clipIndex);
    bool trimTransitionInValid(int trackIndex, int clipIndex, int delta);
    void trimTransitionIn(int trackIndex, int clipIndex, int delta);
    bool trimTransitionOutValid(int trackIndex, int clipIndex, int delta);
    void trimTransitionOut(int trackIndex, int clipIndex, int delta);
    bool addTransitionByTrimInValid(int trackIndex, int clipIndex, int delta);
    void addTransitionByTrimIn(int trackIndex, int clipIndex, int delta);
    bool addTransitionByTrimOutValid(int trackIndex, int clipIndex, int delta);
    void addTransitionByTrimOut(int trackIndex, int clipIndex, int delta);

private:
    Mlt::Tractor* m_tractor;
    TrackList m_trackList;
    bool m_isMakingTransition;

    bool moveClipToTrack(int fromTrack, int toTrack, int clipIndex, int position);
    void moveClipToEnd(Mlt::Playlist& playlist, int trackIndex, int clipIndex, int position);
    void relocateClip(Mlt::Playlist& playlist, int trackIndex, int clipIndex, int position);
    void moveClipInBlank(Mlt::Playlist& playlist, int trackIndex, int clipIndex, int position);
    void consolidateBlanks(Mlt::Playlist& playlist, int trackIndex);
    void consolidateBlanksAllTracks();
    void getAudioLevels();
    void addBlackTrackIfNeeded();
    void convertOldDoc();
    Mlt::Transition* getTransition(const QString& name, int trackIndex) const;
    Mlt::Filter* getFilter(const QString& name, int trackIndex) const;
    Mlt::Filter* getFilter(const QString& name, Mlt::Service* service) const;
    void removeBlankPlaceholder(Mlt::Playlist& playlist, int trackIndex);
    void retainPlaylist();
    void loadPlaylist();
    void removeRegion(int trackIndex, int position, int length);

    friend class UndoHelper;

private slots:
    void adjustBackgroundDuration();

};

#endif // MULTITRACKMODEL_H
