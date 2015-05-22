/*
 * Copyright (c) 2012-2015 Meltytech, LLC
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

#include "meltedplaylistmodel.h"
#include "mltcontroller.h"
#include "util.h"

MeltedPlaylistModel::MeltedPlaylistModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_unit(255)
    , m_list(0)
    , m_response(0)
    , m_index(-1)
    , m_dropRow(-1)
    , m_tokeniser(0)
{
    //XXX qt5
//    setSupportedDragActions(Qt::MoveAction);
}

MeltedPlaylistModel::~MeltedPlaylistModel()
{
    mvcp_list_close(m_list);
    m_list = 0;
    mvcp_response_close(m_response);
    m_response = 0;
}

int MeltedPlaylistModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if (m_list)
        return mvcp_list_count(m_list);
    else
        return 0;
}

int MeltedPlaylistModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
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
            if (role == Qt::DisplayRole)
                result = Util::baseName(result);
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
        break;
    }
    return QVariant();
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

Qt::DropActions MeltedPlaylistModel::supportedDropActions() const
{
    return Qt::MoveAction | Qt::CopyAction;
}

bool MeltedPlaylistModel::insertRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(count)
    Q_UNUSED(parent)
    if (!m_list) return false;
    if (m_dropRow == -1)
        m_dropRow = row;
    return true;
}

bool MeltedPlaylistModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(count)
    Q_UNUSED(parent)
    if (!m_list) return false;
    if (row == m_dropRow) return false;
    emit moveClip(row, m_dropRow);
    m_dropRow = -1;
    return true;
}

Qt::ItemFlags MeltedPlaylistModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaults = QAbstractTableModel::flags(index);
    if (index.isValid())
        return Qt::ItemIsDragEnabled | defaults;
    else
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaults;
}

QStringList MeltedPlaylistModel::mimeTypes() const
{
    QStringList ls = QAbstractTableModel::mimeTypes();
    ls.append("application/mvcp+path");
    return ls;
}

bool MeltedPlaylistModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (data->hasFormat("application/mvcp+path")) {
        emit dropped(QString::fromUtf8(data->data("application/mvcp+path")), row);
        return true;
    }
    else if (action == Qt::MoveAction)
        return QAbstractTableModel::dropMimeData(data, action, row, column, parent);
    else
        return false;
}

void MeltedPlaylistModel::gotoClip(int index)
{
    m_commands << MVCP_GOTO;
    m_socket.write(QString("GOTO U%1 0 %2\r\n").arg(m_unit).arg(index).toLatin1());
    onClipIndexChanged(m_unit, index);
}

void MeltedPlaylistModel::append(const QString &clip, int in, int out, bool notify)
{
    m_commands.append(notify? MVCP_APND : MVCP_IGNORE);
    m_socket.write(QString("APND U%1 \"%2\" %3 %4\r\n").arg(m_unit).arg(clip).arg(in).arg(out).toUtf8());
}

void MeltedPlaylistModel::remove(int row, bool notify)
{
    m_commands.append(notify? MVCP_REMOVE : MVCP_IGNORE);
    m_socket.write(QString("REMOVE U%1 %2\r\n").arg(m_unit).arg(row).toLatin1());
}

void MeltedPlaylistModel::insert(const QString &clip, int row, int in, int out, bool notify)
{
    m_commands.append(notify? MVCP_INSERT : MVCP_IGNORE);
    m_socket.write(QString("INSERT U%1 \"%2\" %3 %4 %5\r\n").arg(m_unit).arg(clip).arg(row).arg(in).arg(out).toUtf8());
}

void MeltedPlaylistModel::move(int from, int to, bool notify)
{
    m_commands.append(notify? MVCP_MOVE : MVCP_IGNORE);
    m_socket.write(QString("MOVE U%1 %2 %3\r\n").arg(m_unit).arg(from).arg(to).toLatin1());
}

void MeltedPlaylistModel::wipe()
{
    m_commands << MVCP_IGNORE;
    m_socket.write(QString("WIPE U%1\r\n").arg(m_unit).toLatin1());
}

void MeltedPlaylistModel::clean()
{
    m_commands << MVCP_IGNORE;
    m_socket.write(QString("CLEAN U%1\r\n").arg(m_unit).toLatin1());
}

void MeltedPlaylistModel::clear()
{
    m_commands << MVCP_IGNORE;
    m_socket.write(QString("CLEAR U%1\r\n").arg(m_unit).toLatin1());
}

void MeltedPlaylistModel::play(double speed)
{
    m_commands << MVCP_IGNORE;
    m_socket.write(QString("PLAY U%1 %2\r\n").arg(m_unit).arg(1000 * speed).toLatin1());
}

void MeltedPlaylistModel::pause()
{
    m_commands << MVCP_IGNORE;
    m_socket.write(QString("PAUSE U%1\r\n").arg(m_unit).toLatin1());
}

void MeltedPlaylistModel::stop()
{
    m_commands << MVCP_IGNORE;
    m_socket.write(QString("STOP U%1\r\n").arg(m_unit).toLatin1());
}

void MeltedPlaylistModel::seek(int position)
{
    pause();
    m_commands << MVCP_IGNORE;
    m_socket.write(QString("GOTO U%1 %2\r\n").arg(m_unit).arg(position).toLatin1());
}

void MeltedPlaylistModel::rewind()
{
    m_commands << MVCP_IGNORE;
    m_socket.write(QString("REW U%1\r\n").arg(m_unit).toLatin1());
}

void MeltedPlaylistModel::fastForward()
{
    m_commands << MVCP_IGNORE;
    m_socket.write(QString("FF U%1\r\n").arg(m_unit).toLatin1());
}

void MeltedPlaylistModel::previous()
{
    m_commands << MVCP_IGNORE;
    m_socket.write(QString("GOTO U%1 0 -1\r\n").arg(m_unit).toLatin1());
}

void MeltedPlaylistModel::next()
{
    m_commands << MVCP_IGNORE;
    m_socket.write(QString("GOTO U%1 0 +1\r\n").arg(m_unit).toLatin1());
}

void MeltedPlaylistModel::setIn(int in)
{
    m_commands << MVCP_IGNORE;
    m_socket.write(QString("SIN U%1 %2\r\n").arg(m_unit).arg(in).toLatin1());
}

void MeltedPlaylistModel::setOut(int out)
{
    m_commands << MVCP_IGNORE;
    m_socket.write(QString("SOUT U%1 %2\r\n").arg(m_unit).arg(out).toLatin1());
}

void MeltedPlaylistModel::onConnected(const QString &address, quint16 port, quint8 unit)
{
    connect(&m_socket, SIGNAL(readyRead()), this, SLOT(readResponse()));
    m_unit = unit;
    m_commands << MVCP_IGNORE;
    m_response = mvcp_response_init();
    m_tokeniser = mvcp_tokeniser_init();
    m_socket.connectToHost(address, port);
}

void MeltedPlaylistModel::onDisconnected()
{
    m_socket.disconnect(this);
    m_socket.disconnectFromHost();
    beginResetModel();
    mvcp_list_close(m_list);
    m_list = 0;
    mvcp_response_close(m_response);
    m_response = 0;
    if (m_tokeniser) {
        mvcp_tokeniser_close(m_tokeniser);
        m_tokeniser = 0;
    }
    endResetModel();
}

void MeltedPlaylistModel::refresh()
{
    m_commands << MVCP_LIST;
    m_socket.write(QString("LIST U%1\r\n").arg(m_unit).toLatin1());
}

void MeltedPlaylistModel::onUnitChanged(quint8 unit)
{
    m_unit = unit;
    beginResetModel();
    mvcp_list_close(m_list);
    m_list = 0;
    endResetModel();
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
    if (data.size() > 0 && m_tokeniser) {
        m_data.append(data);
        if (!m_data.contains('\n'))
            return;
        // There can be multiple responses in the same data block, so we use a
        // tokeniser feeding into the mvcp_response.
        mvcp_tokeniser_parse_new(m_tokeniser, const_cast<char*>(m_data.constData()), "\n");
        m_data.clear();

        for (int i = 0; i < mvcp_tokeniser_count(m_tokeniser); i++) {
            char* line = mvcp_tokeniser_get_string(m_tokeniser, i);
            // mvcp_response_write() looks for line feeds
            if (line[strlen(line) - 1] == '\r')
                line[strlen(line) - 1] = '\n';
            mvcp_response_write(m_response, line, strlen(line));
            int last_line = mvcp_response_count(m_response) - 1;
            if (last_line < 0)
                continue;
            int terminated = 0;
            int status = mvcp_response_get_error_code(m_response);
            switch(status) {
            case 201:
            case 500:
                terminated = !strcmp(mvcp_response_get_line(m_response, last_line), "");
                break;
            case 202:
                terminated = last_line >= 1;
                break;
            default:
                terminated = 1;
                break;
            }
            if (terminated) {
                int command = m_commands.isEmpty()? MVCP_IGNORE : m_commands.takeFirst();
                switch (command) {
                case MVCP_LIST: {
                    m_list = (mvcp_list) calloc(1, sizeof(*m_list));
                    m_list->response = mvcp_response_clone(m_response);
                    if (mvcp_response_count(m_response ) >= 2)
                        m_list->generation = atoi(mvcp_response_get_line(m_response, 1));
                    beginInsertRows(QModelIndex(), 0, mvcp_list_count(m_list) - 1);
                    endInsertRows();
                    emit loaded();
                    break;
                }
                case MVCP_APND:
                case MVCP_REMOVE:
                case MVCP_INSERT:
                case MVCP_MOVE:
                    if (status == 200)
                        emit success();
                    break;
                default:
                    break;
                }
                mvcp_response_close(m_response);
                m_response = mvcp_response_init();
            }
        }
    }
}
