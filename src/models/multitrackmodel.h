/*
 * Copyright (c) 2013-2025 Meltytech, LLC
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

#include <MltPlaylist.h>
#include <MltTractor.h>
#include <QAbstractItemModel>
#include <QList>
#include <QString>

#include <memory>

typedef enum {
    PlaylistTrackType = 0,
    BlackTrackType,
    SilentTrackType,
    AudioTrackType,
    VideoTrackType
} TrackType;

typedef struct
{
    TrackType type;
    int number;
    int mlt_index;
} Track;

typedef QList<Track> TrackList;

class MultitrackModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(int trackHeight READ trackHeight WRITE setTrackHeight NOTIFY trackHeightChanged)
    Q_PROPERTY(int trackHeaderWidth READ trackHeaderWidth WRITE setTrackHeaderWidth NOTIFY
                   trackHeaderWidthChanged FINAL)
    Q_PROPERTY(double scaleFactor READ scaleFactor WRITE setScaleFactor NOTIFY scaleFactorChanged)
    Q_PROPERTY(bool filtered READ isFiltered NOTIFY filteredChanged)

public:
    /// Two level model: tracks and clips on track
    enum {
        NameRole = Qt::UserRole + 1,
        CommentRole,  /// clip only
        ResourceRole, /// clip only
        ServiceRole,  /// clip only
        IsBlankRole,  /// clip only
        StartRole,    /// clip only
        DurationRole,
        InPointRole,   /// clip only
        OutPointRole,  /// clip only
        FramerateRole, /// clip only
        IsMuteRole,    /// track only
        IsHiddenRole,  /// track only
        IsAudioRole,
        AudioLevelsRole,  /// clip only
        IsCompositeRole,  /// track only
        IsLockedRole,     /// track only
        FadeInRole,       /// clip only
        FadeOutRole,      /// clip only
        IsTransitionRole, /// clip only
        FileHashRole,     /// clip only
        SpeedRole,        /// clip only
        IsFilteredRole,
        IsTopVideoRole,    /// track only
        IsBottomVideoRole, /// track only
        IsTopAudioRole,    /// track only
        IsBottomAudioRole, /// track only
        AudioIndexRole,    /// clip only
        GroupRole,         /// clip only
        GainRole,          /// clip only
        GainEnabledRole,   /// clip only
    };

    explicit MultitrackModel(QObject *parent = 0);
    ~MultitrackModel();

    Mlt::Tractor *tractor() const { return m_tractor; }
    const TrackList &trackList() const { return m_trackList; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex makeIndex(int trackIndex, int clipIndex) const;
    QModelIndex parent(const QModelIndex &index) const;
    QHash<int, QByteArray> roleNames() const;
    Q_INVOKABLE void audioLevelsReady(const QPersistentModelIndex &index);
    bool createIfNeeded();
    void addBackgroundTrack();
    int addAudioTrack();
    int addVideoTrack();
    void removeTrack(int trackIndex);
    void load();
    void close();
    int clipIndex(int trackIndex, int position);
    bool trimClipInValid(int trackIndex, int clipIndex, int delta, bool ripple);
    bool trimClipOutValid(int trackIndex, int clipIndex, int delta, bool ripple);
    int trackHeight() const;
    void setTrackHeight(int height);
    int trackHeaderWidth() const;
    void setTrackHeaderWidth(int width);
    double scaleFactor() const;
    void setScaleFactor(double scale);
    bool isTransition(Mlt::Playlist &playlist, int clipIndex) const;
    void insertTrack(int trackIndex, TrackType type = VideoTrackType);
    void moveTrack(int fromTrackIndex, int toTrackIndex);
    void insertOrAdjustBlankAt(QList<int> tracks, int position, int length);
    bool mergeClipWithNext(int trackIndex, int clipIndex, bool dryrun);
    std::unique_ptr<Mlt::ClipInfo> findClipByUuid(const QUuid &uuid,
                                                  int &trackIndex,
                                                  int &clipIndex);
    std::unique_ptr<Mlt::ClipInfo> getClipInfo(int trackIndex, int clipIndex);
    QString getTrackName(int trackIndex);
    int bottomVideoTrackIndex() const;
    int mltIndexForTrack(int trackIndex) const;
    bool checkForEmptyTracks(int trackIndex);

signals:
    void created();
    void aboutToClose();
    void closed();
    void modified();
    void seeked(int position, bool seekPlayer = true);
    void trackHeightChanged();
    void trackHeaderWidthChanged();
    void scaleFactorChanged();
    void showStatusMessage(QString);
    void durationChanged();
    void filteredChanged();
    void reloadRequested();
    void appended(int trackIndex, int clipIndex);
    void inserted(int trackIndex, int clipIndex);
    void overWritten(int trackIndex, int clipIndex);
    void removing(Mlt::Service *service);
    void noMoreEmptyTracks(bool isAudio);

public slots:
    void refreshTrackList();
    void setTrackName(int row, const QString &value);
    void setTrackMute(int row, bool mute);
    void setTrackHidden(int row, bool hidden);
    void setTrackComposite(int row, bool composite);
    void setTrackLock(int row, bool lock);
    int trimClipIn(int trackIndex, int clipIndex, int delta, bool ripple, bool rippleAllTracks);
    void notifyClipIn(int trackIndex, int clipIndex);
    int trimClipOut(int trackIndex, int clipIndex, int delta, bool ripple, bool rippleAllTracks);
    void notifyClipOut(int trackIndex, int clipIndex);
    bool moveClip(
        int fromTrack, int toTrack, int clipIndex, int position, bool ripple, bool rippleAllTracks);
    int overwriteClip(int trackIndex, Mlt::Producer &clip, int position, bool seek = true);
    QString overwrite(
        int trackIndex, Mlt::Producer &clip, int position, bool seek = true, bool notify = true);
    int insertClip(int trackIndex,
                   Mlt::Producer &clip,
                   int position,
                   bool rippleAllTracks,
                   bool seek = true,
                   bool notify = true);
    int appendClip(int trackIndex, Mlt::Producer &clip, bool seek = true, bool notify = true);
    void removeClip(int trackIndex, int clipIndex, bool rippleAllTracks);
    void liftClip(int trackIndex, int clipIndex);
    void splitClip(int trackIndex, int clipIndex, int position);
    void joinClips(int trackIndex, int clipIndex);
    void changeGain(int trackIndex, int clipIndex, double gain);
    void fadeIn(int trackIndex, int clipIndex, int duration);
    void fadeOut(int trackIndex, int clipIndex, int duration);
    bool addTransitionValid(int fromTrack, int toTrack, int clipIndex, int position, bool ripple);
    int addTransition(int trackIndex, int clipIndex, int position, bool ripple, bool rippleAllTracks);
    void removeTransition(int trackIndex, int clipIndex);
    void removeTransitionByTrimIn(int trackIndex, int clipIndex, int delta);
    void removeTransitionByTrimOut(int trackIndex, int clipIndex, int delta);
    bool trimTransitionInValid(int trackIndex, int clipIndex, int delta);
    void trimTransitionIn(int trackIndex, int clipIndex, int delta, bool slip = false);
    bool trimTransitionOutValid(int trackIndex, int clipIndex, int delta);
    void trimTransitionOut(int trackIndex, int clipIndex, int delta, bool slip = false);
    bool addTransitionByTrimInValid(int trackIndex, int clipIndex, int delta);
    int addTransitionByTrimIn(int trackIndex, int clipIndex, int delta);
    bool addTransitionByTrimOutValid(int trackIndex, int clipIndex, int delta);
    void addTransitionByTrimOut(int trackIndex, int clipIndex, int delta);
    bool removeTransitionByTrimInValid(int trackIndex, int clipIndex, int delta);
    bool removeTransitionByTrimOutValid(int trackIndex, int clipIndex, int delta);
    void filterAddedOrRemoved(Mlt::Producer *producer);
    void onFilterChanged(Mlt::Service *service);
    void reload(bool asynchronous = false);
    void replace(int trackIndex, int clipIndex, Mlt::Producer &clip, bool copyFilters = true);

private:
    Mlt::Tractor *m_tractor;
    TrackList m_trackList;
    bool m_isMakingTransition;

    void moveClipToEnd(Mlt::Playlist &playlist,
                       int trackIndex,
                       int clipIndex,
                       int position,
                       bool ripple,
                       bool rippleAllTracks);
    void moveClipInBlank(Mlt::Playlist &playlist,
                         int trackIndex,
                         int clipIndex,
                         int position,
                         bool ripple,
                         bool rippleAllTracks,
                         int duration = 0);
    void consolidateBlanks(Mlt::Playlist &playlist, int trackIndex);
    void consolidateBlanksAllTracks();
    void getAudioLevels();
    void addBlackTrackIfNeeded();
    void convertOldDoc();
    Mlt::Transition *getTransition(const QString &name, int trackIndex) const;
    Mlt::Filter *getFilter(const QString &name, int trackIndex) const;
    Mlt::Filter *getFilter(const QString &name, Mlt::Service *service) const;
    void removeBlankPlaceholder(Mlt::Playlist &playlist, int trackIndex);
    void retainPlaylist();
    void loadPlaylist();
    void removeRegion(int trackIndex, int position, int length);
    void clearMixReferences(int trackIndex, int clipIndex);
    bool isFiltered(Mlt::Producer *producer = 0) const;
    int getDuration();
    void adjustServiceFilterDurations(Mlt::Service &service, int duration);
    bool warnIfInvalid(Mlt::Service &service);
    Mlt::Transition *getVideoBlendTransition(int trackIndex) const;
    void refreshVideoBlendTransitions();
    int bottomVideoTrackMltIndex() const;
    bool hasEmptyTrack(TrackType trackType) const;

    friend class UndoHelper;

private slots:
    void adjustBackgroundDuration();
    void adjustTrackFilters();
};

#endif // MULTITRACKMODEL_H
