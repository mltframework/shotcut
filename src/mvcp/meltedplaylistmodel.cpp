/*
 * Copyright (c) 2012 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * GL shader based on BSD licensed code from Peter Bengtsson:
 * http://www.fourcc.org/source/YUV420P-OpenGL-GLSLang.c
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

#include "meltedplaylistmodel.h"
#include <QtCore/QFileInfo>
#include "mltcontroller.h"

MeltedPlaylistModel::MeltedPlaylistModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_unit(255)
    , m_list(0)
    , m_response(0)
    , m_index(-1)
{
}

MeltedPlaylistModel::~MeltedPlaylistModel()
{
    onDisconnected();
}

int MeltedPlaylistModel::rowCount(const QModelIndex &parent) const
{
    if (m_list)
        return mvcp_list_count(m_list);
    else
        return 0;
}

int MeltedPlaylistModel::columnCount(const QModelIndex &parent) const
{
    return COLUMN_COUNT;
}

QVariant MeltedPlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!m_list || index.row() >= mvcp_list_count(m_list))
        return QVariant();
    switch (role) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole: {
        mvcp_list_entry_t entry;
        mvcp_list_get(m_list, index.row(), &entry);
        switch (index.column()) {
        case COLUMN_INDEX:
            return QString::number(index.row() + 1);
        case COLUMN_RESOURCE: {
            QString result = QString::fromUtf8(entry.full);
            // Use basename for display
            if (role == Qt::DisplayRole && result.startsWith('/'))
                result = QFileInfo(result).fileName();
            return result;
        }
        case COLUMN_IN:
            return entry.in;
        case COLUMN_OUT:
            return entry.out;
        default:
            break;
        }
        break;
    }
    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case COLUMN_INDEX:
        case COLUMN_IN:
        case COLUMN_OUT:
            return Qt::AlignRight;
        default:
            return Qt::AlignLeft;
        }
        break;
    case Qt::CheckStateRole:
        if (index.row() == m_index && index.column() == COLUMN_ACTIVE)
            return Qt::Checked;
    default:
        return QVariant();
    }
}

QVariant MeltedPlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section) {
        case COLUMN_INDEX:
            return tr("#");
        case COLUMN_RESOURCE:
            return tr("Clip");
        case COLUMN_IN:
            return tr("In");
        case COLUMN_OUT:
            return tr("Out");
        default:
            break;
        }
    }
    else if (role == Qt::TextAlignmentRole) {
        switch (section) {
        case COLUMN_INDEX:
        case COLUMN_IN:
        case COLUMN_OUT:
            return Qt::AlignRight;
        default:
            return Qt::AlignLeft;
        }
    }
    return QVariant();
}

void MeltedPlaylistModel::onConnected(const QString &address, quint16 port, quint8 unit)
{
    connect(&m_socket, SIGNAL(readyRead()), this, SLOT(readResponse()));
    m_socket.connectToHost(address, port);
//    connect(&m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    m_unit = unit;
    m_response = mvcp_response_init();
}

void MeltedPlaylistModel::onDisconnected()
{
    m_socket.disconnect(this);
    m_socket.disconnectFromHost();
    bool update = m_list;
    if (update)
        beginRemoveRows(QModelIndex(), 0, mvcp_list_count(m_list) - 1);
    mvcp_list_close(m_list);
    m_list = 0;
    m_response = 0;
    if (update)
        endRemoveRows();
}

void MeltedPlaylistModel::refresh()
{
    if (m_response)
        mvcp_response_close(m_response);
    m_response = mvcp_response_init();
    m_socket.write(QString("LIST U%1\r\n").arg(m_unit).toAscii());
}

void MeltedPlaylistModel::onUnitChanged(quint8 unit)
{
    m_unit = unit;
    bool update = m_list;
    if (update)
        beginRemoveRows(QModelIndex(), 0, mvcp_list_count(m_list) - 1);
    mvcp_list_close(m_list);
    m_list = 0;
    m_response = 0;
    if (update)
        endRemoveRows();
    refresh();
}

void MeltedPlaylistModel::onClipIndexChanged(quint8 unit, int index)
{
    if (unit == m_unit) {
        if (m_index >= 0)
            emit dataChanged(createIndex(m_index, COLUMN_ACTIVE), createIndex(m_index, COLUMN_ACTIVE));
        m_index = index;
        emit dataChanged(createIndex(m_index, COLUMN_ACTIVE), createIndex(m_index, COLUMN_ACTIVE));
    }
}

void MeltedPlaylistModel::onGenerationChanged(quint8 unit)
{
    if (unit == m_unit)
        onUnitChanged(m_unit);
}

void MeltedPlaylistModel::readResponse()
{
    QByteArray data = m_socket.readAll();
    if (data.size() > 0)
    {
        mvcp_response_write(m_response, data.constData(), data.size());
        int position = mvcp_response_count(m_response) - 1;
        if (position < 0 || data[data.size() - 1] != '\n')
            return;
        int terminated = 0;
        switch(mvcp_response_get_error_code(m_response))
        {
        case 100:
            //refresh();
            break;
        case 201:
        case 500:
            terminated = !strcmp(mvcp_response_get_line(m_response, position), "");
            break;
        case 202:
            terminated = mvcp_response_count(m_response) >= 2;
            break;
        default:
            terminated = 1;
            break;
        }
        if (terminated) {
            m_list = (mvcp_list) calloc(1, sizeof(*m_list));
            m_list->response = m_response;
            if (mvcp_response_count(m_response ) >= 2)
                m_list->generation = atoi(mvcp_response_get_line(m_response, 1));
            beginInsertRows(QModelIndex(), 0, mvcp_list_count(m_list) - 1);
            endInsertRows();
            emit loaded();
        }
    }
}
