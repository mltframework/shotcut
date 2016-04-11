/*
 * Copyright (c) 2012-2016 Meltytech, LLC
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

#include "meltedunitsmodel.h"
#include <QTimer>
#include <mvcp_util.h>
#include <Logger.h>

MeltedUnitsModel::MeltedUnitsModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_mvcp(0)
    , m_tokeniser(0)
    , m_statusSent(false)
{
}

MeltedUnitsModel::~MeltedUnitsModel()
{
    onDisconnected();
}

int MeltedUnitsModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return m_units.size();
}

int MeltedUnitsModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 2;
}

QVariant MeltedUnitsModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    if (!m_mvcp || role != Qt::DisplayRole || index.column() > 1)
        return result;
    switch (index.column()) {
        case 0:
            result = QString("U%1: %2").arg(index.row()).arg(m_units[index.row()]->objectName());
            break;
        case 1:
            result = m_units[index.row()]->property("status");
            break;
        default:
            break;
    }
    return result;
}

QVariant MeltedUnitsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        return (section == 0)? tr("Unit") : tr("Status");
    else
        return QVariant();
}

void MeltedUnitsModel::onConnected(MvcpThread *a_mvcp)
{
    m_mvcp = a_mvcp;
    connect(m_mvcp, SIGNAL(ulsResult(QStringList)), this, SLOT(onUlsResult(QStringList)));
    m_mvcp->uls();
}

void MeltedUnitsModel::onConnected(const QString &address, quint16 port, quint8 /*unit*/)
{
    m_tokeniser = mvcp_tokeniser_init();
    m_statusSent = false;
    connect(&m_socket, SIGNAL(readyRead()), this, SLOT(readResponse()));
    m_socket.connectToHost(address, port);
}

void MeltedUnitsModel::onDisconnected()
{
    m_socket.disconnect(this);
    m_socket.disconnectFromHost();
    m_mvcp = 0;
    if (m_tokeniser) {
        mvcp_tokeniser_close(m_tokeniser);
        m_tokeniser = 0;
    }
    if (rowCount() > 0) {
        emit beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
        foreach (QObject* o, m_units)
            delete o;
        m_units.clear();
        emit endRemoveRows();
    }
}

QString MeltedUnitsModel::decodeStatus(unit_status status)
{
    switch (status) {
    case unit_unknown:      return tr("unknown");
    case unit_undefined:    return tr("undefined");
    case unit_offline:      return tr("offline");
    case unit_not_loaded:   return tr("unloaded");
    case unit_stopped:      return tr("stopped");
    case unit_playing:      return tr("playing");
    case unit_paused:       return tr("paused");
    case unit_disconnected: return tr("disconnected");
    }
    return QString();
}

void MeltedUnitsModel::readResponse()
{
    QByteArray data = m_socket.readAll();
    if (data.size() > 0 && m_tokeniser)
    {
        mvcp_status_t status;
        m_data.append(data);
        if (m_data.contains('\n')) {
            mvcp_tokeniser_parse_new(m_tokeniser, const_cast<char*>(m_data.constData()), "\n");
            m_data.clear();
            for (int i = 0; i < mvcp_tokeniser_count(m_tokeniser); i++) {
                char* line = mvcp_tokeniser_get_string(m_tokeniser, i);
                if (!line || strlen(line) <= 1 || !strcmp(line, "100 VTR Ready\r"))
                    continue;
                if (line[strlen(line) - 1] == '\r') {
                    mvcp_util_chomp(line);
                    mvcp_status_parse(&status, line);
                    if (status.status != unit_unknown && status.unit < m_units.size()) {
                        // Refresh the status table cell if changed
                        if (!m_units[status.unit]->property("unit_status").isValid() ||
                                m_units[status.unit]->property("unit_status").toInt() != status.status) {
                            m_units[status.unit]->setProperty("unit_status", status.status);
                            m_units[status.unit]->setProperty("status", decodeStatus(status.status));
                            emit dataChanged(createIndex(status.unit, 1), createIndex(status.unit, 1));
                        }
                        // Inform others like the MeltedPlaylistModel when the currently playing clip has changed.
                        if (!m_units[status.unit]->property("clip_index").isValid() ||
                                m_units[status.unit]->property("clip_index").toInt() != status.clip_index) {
                            m_units[status.unit]->setProperty("clip_index", status.clip_index);
                            emit clipIndexChanged(status.unit, status.clip_index);
                        }
                        // Inform others like the MeltedPlaylistModel when the playlist has changed.
                        if (!m_units[status.unit]->property("generation").isValid() ||
                                m_units[status.unit]->property("generation").toInt() != status.generation) {
                            m_units[status.unit]->setProperty("generation", status.generation);
                            emit generationChanged(status.unit);
                        }
                        emit positionUpdated(status.unit, status.position, status.fps,
                            status.in, status.out, status.length, status.status == unit_playing);
                    }
                    else if (status.status != unit_unknown && status.unit >= m_units.size()) {
                        m_mvcp->uls();
                    }
                }
                else {
                    m_data.append(line, strlen(line));
                }
            }
        }
    }
}

void MeltedUnitsModel::onUlsResult(QStringList units)
{
    for (quint8 i = 0; i < units.size(); i++) {
        if (i >= m_units.size()) {
            emit beginInsertRows(QModelIndex(), i, i);
            m_units.append(new QObject);
            emit endInsertRows();
        }
        m_units[i]->setObjectName(units[i]);
        emit dataChanged(createIndex(i, 0), createIndex(i, 0));
    }
    if (!m_statusSent) {
        m_socket.write(QString("STATUS\r\n").toLatin1());
        m_statusSent = true;
    }
}
