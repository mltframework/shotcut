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

#ifndef QWEBSOCKETSERVER_P_H
#define QWEBSOCKETSERVER_P_H
//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QObject>
#include <QQueue>
#include <QString>
#include <QHostAddress>
#include "qwebsocket.h"

QT_BEGIN_NAMESPACE

class QTcpServer;
class QWebSocketServer;

class QWebSocketServerPrivate : public QObject
{
    Q_OBJECT

public:
    explicit QWebSocketServerPrivate(const QString &serverName, QWebSocketServer * const pWebSocketServer, QObject *parent = 0);
    virtual ~QWebSocketServerPrivate();

    void close();
    QString errorString() const;
    bool hasPendingConnections() const;
    bool isListening() const;
    bool listen(const QHostAddress &address = QHostAddress::Any, quint16 port = 0);
    int maxPendingConnections() const;
    virtual QWebSocket *nextPendingConnection();
    void pauseAccepting();
#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy proxy() const;
    void setProxy(const QNetworkProxy &networkProxy);
#endif
    void resumeAccepting();
    QHostAddress serverAddress() const;
    QAbstractSocket::SocketError serverError() const;
    quint16 serverPort() const;
    void setMaxPendingConnections(int numConnections);
    bool setSocketDescriptor(int socketDescriptor);
    int socketDescriptor() const;
    bool waitForNewConnection(int msec = 0, bool *timedOut = 0);

    QList<QWebSocketProtocol::Version> supportedVersions() const;
    QList<QString> supportedProtocols() const;
    QList<QString> supportedExtensions() const;

    void setServerName(const QString &serverName);
    QString serverName() const;

Q_SIGNALS:
    void newConnection();

private Q_SLOTS:
    void onNewConnection();
    void onCloseConnection();
    void handshakeReceived();

private:
    Q_DECLARE_PUBLIC(QWebSocketServer)
    QWebSocketServer * const q_ptr;

    QTcpServer *m_pTcpServer;
    QString m_serverName;
    QQueue<QWebSocket *> m_pendingConnections;

    void addPendingConnection(QWebSocket *pWebSocket);
};

QT_END_NAMESPACE

#endif // QWEBSOCKETSERVER_P_H
