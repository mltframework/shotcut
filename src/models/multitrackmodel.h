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
    QString name;
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
        IsHiddenRole,   /// track only
        IsAudioRole
    };

    explicit MultitrackModel(QObject *parent = 0);
    ~MultitrackModel();

    Mlt::Tractor* tractor() const { return m_tractor; }

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
//    Qt::ItemFlags flags(const QModelIndex &index) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    QHash<int, QByteArray> roleNames() const;

signals:
    void loaded();

public slots:
    void load();
    void refreshTrackList();

private:
    Mlt::Tractor* m_tractor;
    TrackList m_trackList;

    void addBlackTrackIfNeeded();
    void addMissingTransitions();
};

#endif // MULTITRACKMODEL_H
