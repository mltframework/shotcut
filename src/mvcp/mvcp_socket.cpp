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

#include <mvcp_socket.h>
#include <QTcpSocket>

extern "C" {

/** Initialise the socket.
*/

mvcp_socket mvcp_socket_init( char *server, int port )
{
    if (QString(server).isEmpty())
        return NULL;
    QTcpSocket* socket = new QTcpSocket();
    socket->setProperty("server", server);
    socket->setProperty("port", port);
    return socket;
}

/** Connect to the server.
*/

int mvcp_socket_connect( mvcp_socket connection )
{
    QTcpSocket* socket = (QTcpSocket*) connection;
    int ret = 0;

    if (socket) {
        socket->connectToHost(socket->property("server").toString(),
                              socket->property("port").toInt());
        if (!socket->waitForConnected(5000))
            ret = -1;
    } else {
        ret = -1;
    }
    return ret;
}

/** Convenience constructor for a connected file descriptor.
*/

mvcp_socket mvcp_socket_init_fd( int /*fd*/ )
{
    return NULL;
}

/** Read an arbitrarily formatted block of data from the server.
*/

int mvcp_socket_read_data( mvcp_socket connection, char *data, int length )
{
    QTcpSocket* socket = (QTcpSocket*) connection;
    int used = -1;
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        if (socket->bytesAvailable() > 0 || socket->waitForReadyRead(1000))
            used = socket->read(data, length);
        else
            used = 0;
    }
    return used;
}

/** Write an arbitrarily formatted block of data to the server.
*/

int mvcp_socket_write_data( mvcp_socket connection, const char *data, int length )
{
    QTcpSocket* socket = (QTcpSocket*) connection;
    int used = -1;
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        used = socket->write(data, length);
        socket->waitForBytesWritten(1000);
    }
    return used;
}

/** Close the socket.
*/

void mvcp_socket_close( mvcp_socket connection )
{
    QTcpSocket* socket = (QTcpSocket*) connection;
    delete socket;
}

}
