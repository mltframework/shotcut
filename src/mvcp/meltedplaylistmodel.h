/*
 * Copyright (c) 2012-2013 Meltytech, LLC
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

#ifndef MELTEDPLAYLISTMODEL_H
#define MELTEDPLAYLISTMODEL_H

#include <QAbstractTableModel>
#include <QTcpSocket>
#include <QStringList>
#include <QMimeData>
#include <mvcp.h>
#include <mvcp_tokeniser.h>

class MeltedPlaylistModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Columns {
        COLUMN_ACTIVE = 0,
        COLUMN_INDEX,
        COLUMN_RESOURCE,
        COLUMN_IN,
        COLUMN_OUT,
        COLUMN_COUNT
    };

    enum MvcpCommands {
        MVCP_IGNORE,
        MVCP_LIST,
        MVCP_GOTO,
        MVCP_APND,
        MVCP_REMOVE,
        MVCP_INSERT,
        MVCP_MOVE
    };

    explicit MeltedPlaylistModel(QObject *parent = 0);
    ~MeltedPlaylistModel();

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::DropActions supportedDropActions() const;
    bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex());
    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QStringList mimeTypes() const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

    void gotoClip(int);
    void append(const QString& clip, int in = -1, int out = -1, bool notify = true);
    void remove(int row, bool notify = true);
    void insert(const QString& clip, int row, int in = -1, int out = -1, bool notify = true);
    void move(int from, int to, bool notify = true);
    void wipe();
    void clean();
    void clear();
    void play(double speed);
    void pause();
    void stop();
    void seek(int position);
    void rewind();
    void fastForward();
    void previous();
    void next();
    void setIn(int in);
    void setOut(int out);

signals:
    void loaded();
    void dropped(QString clip, int row);
    void moveClip(int from, int to);
    void success();

public slots:
    void onConnected(const QString& address, quint16 port = 5250, quint8 unit = 0);
    void onDisconnected();
    void refresh();
    void onUnitChanged(quint8 unit);
    void onClipIndexChanged(quint8 unit, int index);
    void onGenerationChanged(quint8 unit);

private slots:
    void readResponse();
//    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket m_socket;
    quint8 m_unit;
    mvcp_list m_list;
    mvcp_response m_response;
    int m_index;
    QList<int> m_commands;
    int m_dropRow;
    QByteArray m_data;
    mvcp_tokeniser m_tokeniser;
};

#endif // MELTEDPLAYLISTMODEL_H
