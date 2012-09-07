/*
 * Copyright (c) 2012 Meltytech, LLC
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

#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QAbstractTableModel>
#include "mltcontroller.h"

class PlaylistModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Columns {
        COLUMN_RESOURCE = 0,
        COLUMN_COUNT
    };
    explicit PlaylistModel(QObject *parent = 0);
    ~PlaylistModel();
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::DropActions supportedDropActions() const;
    bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex());
    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());
    Qt::ItemFlags flags(const QModelIndex &index) const;
    Mlt::Playlist* playlist()
    {
        return m_playlist;
    }

signals:

public slots:
    void clear();
    void load();
    void append(Mlt::Producer*);
    void insert(Mlt::Producer*, int row);
    void remove(int row);
    void update(int row, int in, int out);
    void appendBlank(int frames);
    void insertBlank(int frames, int row);

private:
    Mlt::Playlist* m_playlist;
    Mlt::ClipInfo m_clipInfo;
    void createIfNeeded();
    int m_dropRow;
};

#endif // PLAYLISTMODEL_H
