/*
QWebSockets implements the WebSocket protocol as defined in RFC 6455.
Copyright (C) 2013 Kurt Pattyn (pattyn.kurt@gmail.com)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef QWEBSOCKETSERVER_H
#define QWEBSOCKETSERVER_H

#include <QObject>
#include <QString>
#include <QHostAddress>
#include "qwebsocketsglobal.h"
#include "qwebsocketprotocol.h"

QT_BEGIN_NAMESPACE

class QWebSocketServerPrivate;
class QWebSocket;
class QCorsAuthenticator;

class Q_WEBSOCKETS_EXPORT QWebSocketServer : public QObject
{
    Q_OBJECT

public:
    explicit QWebSocketServer(const QString &serverName, QObject *parent = 0);
    virtual ~QWebSocketServer();

    bool listen(const QHostAddress &address = QHostAddress::Any, quint16 port = 0);
    void close();

    bool isListening() const;

    void setMaxPendingConnections(int numConnections);
    int maxPendingConnections() const;

    quint16 serverPort() const;
    QHostAddress serverAddress() const;

    bool setSocketDescriptor(int socketDescriptor);
    int socketDescriptor() const;

    bool waitForNewConnection(int msec = 0, bool *timedOut = 0);
    bool hasPendingConnections() const;
    virtual QWebSocket *nextPendingConnection();

    QAbstractSocket::SocketError serverError() const;
    QString errorString() const;

    void pauseAccepting();
    void resumeAccepting();

    void setServerName(const QString &serverName);
    QString serverName() const;

#ifndef QT_NO_NETWORKPROXY
    void setProxy(const QNetworkProxy &networkProxy);
    QNetworkProxy proxy() const;
#endif

    QList<QWebSocketProtocol::Version> supportedVersions() const;
    QList<QString> supportedProtocols() const;
    QList<QString> supportedExtensions() const;

Q_SIGNALS:
    void acceptError(QAbstractSocket::SocketError socketError);
    void originAuthenticationRequired(QCorsAuthenticator *pAuthenticator);
    void newConnection();

private:
    Q_DISABLE_COPY(QWebSocketServer)
    Q_DECLARE_PRIVATE(QWebSocketServer)
    QWebSocketServerPrivate * const d_ptr;
};

QT_END_NAMESPACE

#endif // QWEBSOCKETSERVER_H
