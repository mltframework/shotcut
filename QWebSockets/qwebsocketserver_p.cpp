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

#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkProxy>
#include "qwebsocketserver.h"
#include "qwebsocketserver_p.h"
#include "qwebsocketprotocol.h"
#include "handshakerequest_p.h"
#include "handshakeresponse_p.h"
#include "qwebsocket.h"
#include "qwebsocket_p.h"
#include "qcorsauthenticator.h"

QT_BEGIN_NAMESPACE

/*!
    \internal
 */
QWebSocketServerPrivate::QWebSocketServerPrivate(const QString &serverName, QWebSocketServer * const pWebSocketServer, QObject *parent) :
    QObject(parent),
    q_ptr(pWebSocketServer),
    m_pTcpServer(0),
    m_serverName(serverName),
    m_pendingConnections()
{
    Q_ASSERT(pWebSocketServer != 0);
    m_pTcpServer = new QTcpServer(this);
    connect(m_pTcpServer, SIGNAL(acceptError(QAbstractSocket::SocketError)), q_ptr, SIGNAL(acceptError(QAbstractSocket::SocketError)));
    connect(m_pTcpServer, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
}

/*!
    \internal
 */
QWebSocketServerPrivate::~QWebSocketServerPrivate()
{
    while (!m_pendingConnections.isEmpty())
    {
        QWebSocket *pWebSocket = m_pendingConnections.dequeue();
        pWebSocket->close(QWebSocketProtocol::CC_GOING_AWAY, tr("Server closed."));
        pWebSocket->deleteLater();
    }
    m_pTcpServer->deleteLater();
}

/*!
    \internal
 */
void QWebSocketServerPrivate::close()
{
    m_pTcpServer->close();
}

/*!
    \internal
 */
QString QWebSocketServerPrivate::errorString() const
{
    return m_pTcpServer->errorString();
}

/*!
    \internal
 */
bool QWebSocketServerPrivate::hasPendingConnections() const
{
    return !m_pendingConnections.isEmpty();
}

/*!
    \internal
 */
bool QWebSocketServerPrivate::isListening() const
{
    return m_pTcpServer->isListening();
}

/*!
    \internal
 */
bool QWebSocketServerPrivate::listen(const QHostAddress &address, quint16 port)
{
    return m_pTcpServer->listen(address, port);
}

/*!
    \internal
 */
int QWebSocketServerPrivate::maxPendingConnections() const
{
    return m_pTcpServer->maxPendingConnections();
}

/*!
    \internal
 */
void QWebSocketServerPrivate::addPendingConnection(QWebSocket *pWebSocket)
{
    if (m_pendingConnections.size() < maxPendingConnections())
    {
        m_pendingConnections.enqueue(pWebSocket);
    }
}

/*!
    \internal
 */
QWebSocket *QWebSocketServerPrivate::nextPendingConnection()
{
    QWebSocket *pWebSocket = 0;
    if (!m_pendingConnections.isEmpty())
    {
        pWebSocket = m_pendingConnections.dequeue();
    }
    return pWebSocket;
}

/*!
    \internal
 */
void QWebSocketServerPrivate::pauseAccepting()
{
    m_pTcpServer->pauseAccepting();
}

#ifndef QT_NO_NETWORKPROXY
/*!
    \internal
 */
QNetworkProxy QWebSocketServerPrivate::proxy() const
{
    return m_pTcpServer->proxy();
}

/*!
    \internal
 */
void QWebSocketServerPrivate::setProxy(const QNetworkProxy &networkProxy)
{
    m_pTcpServer->setProxy(networkProxy);
}
#endif
/*!
    \internal
 */
void QWebSocketServerPrivate::resumeAccepting()
{
    m_pTcpServer->resumeAccepting();
}

/*!
    \internal
 */
QHostAddress QWebSocketServerPrivate::serverAddress() const
{
    return m_pTcpServer->serverAddress();
}

/*!
    \internal
 */
QAbstractSocket::SocketError QWebSocketServerPrivate::serverError() const
{
    return m_pTcpServer->serverError();
}

/*!
    \internal
 */
quint16 QWebSocketServerPrivate::serverPort() const
{
    return m_pTcpServer->serverPort();
}

/*!
    \internal
 */
void QWebSocketServerPrivate::setMaxPendingConnections(int numConnections)
{
    m_pTcpServer->setMaxPendingConnections(numConnections);
}

/*!
    \internal
 */
bool QWebSocketServerPrivate::setSocketDescriptor(int socketDescriptor)
{
    return m_pTcpServer->setSocketDescriptor(socketDescriptor);
}

/*!
    \internal
 */
int QWebSocketServerPrivate::socketDescriptor() const
{
    return m_pTcpServer->socketDescriptor();
}

/*!
    \internal
 */
bool QWebSocketServerPrivate::waitForNewConnection(int msec, bool *timedOut)
{
    return m_pTcpServer->waitForNewConnection(msec, timedOut);
}

/*!
    \internal
 */
QList<QWebSocketProtocol::Version> QWebSocketServerPrivate::supportedVersions() const
{
    QList<QWebSocketProtocol::Version> supportedVersions;
    supportedVersions << QWebSocketProtocol::currentVersion();	//we only support V13
    return supportedVersions;
}

/*!
    \internal
 */
QList<QString> QWebSocketServerPrivate::supportedProtocols() const
{
    QList<QString> supportedProtocols;
    return supportedProtocols;	//no protocols are currently supported
}

/*!
    \internal
 */
QList<QString> QWebSocketServerPrivate::supportedExtensions() const
{
    QList<QString> supportedExtensions;
    return supportedExtensions;	//no extensions are currently supported
}

/*!
  \internal
 */
void QWebSocketServerPrivate::setServerName(const QString &serverName)
{
    m_serverName = serverName;
}

/*!
  \internal
 */
QString QWebSocketServerPrivate::serverName() const
{
    return m_serverName;
}

/*!
    \internal
 */
void QWebSocketServerPrivate::onNewConnection()
{
    QTcpSocket *pTcpSocket = m_pTcpServer->nextPendingConnection();
    connect(pTcpSocket, SIGNAL(readyRead()), this, SLOT(handshakeReceived()));
}

/*!
    \internal
 */
void QWebSocketServerPrivate::onCloseConnection()
{
    QTcpSocket *pTcpSocket = qobject_cast<QTcpSocket*>(sender());
    if (pTcpSocket != 0)
    {
        pTcpSocket->close();
    }
}

/*!
    \internal
 */
void QWebSocketServerPrivate::handshakeReceived()
{
    Q_Q(QWebSocketServer);
    QTcpSocket *pTcpSocket = qobject_cast<QTcpSocket*>(sender());
    if (pTcpSocket != 0)
    {
        bool success = false;
        bool isSecure = false;
        HandshakeRequest request(pTcpSocket->peerPort(), isSecure);
        QTextStream textStream(pTcpSocket);
        textStream >> request;

        QCorsAuthenticator corsAuthenticator(request.getOrigin());
        Q_EMIT q->originAuthenticationRequired(&corsAuthenticator);

        HandshakeResponse response(request,
                                   m_serverName,
                                   corsAuthenticator.allowed(),
                                   supportedVersions(),
                                   supportedProtocols(),
                                   supportedExtensions());
        disconnect(pTcpSocket, SIGNAL(readyRead()), this, SLOT(handshakeReceived()));

        if (response.isValid())
        {
            QTextStream httpStream(pTcpSocket);
            httpStream << response;
            httpStream.flush();

            if (response.canUpgrade())
            {
                QWebSocket *pWebSocket = QWebSocketPrivate::upgradeFrom(pTcpSocket, request, response);
                if (pWebSocket)
                {
                    pWebSocket->setParent(this);
                    addPendingConnection(pWebSocket);
                    Q_EMIT q->newConnection();
                    success = true;
                }
                else
                {
                    //TODO: should set or emit error
                    qDebug() << tr("Upgrading to websocket failed.");
                }
            }
            else
            {
                //TODO: should set or emit error
                qDebug() << tr("Cannot upgrade to websocket.");
            }
        }
        else
        {
            //TODO: should set or emit error
            qDebug() << tr("Invalid response received.");
        }
        if (!success)
        {
            //TODO: should set or emit error
            qDebug() << tr("Closing socket because of invalid or unsupported request.");
            pTcpSocket->close();
        }
    }
    else
    {
        qWarning() << "Sender socket is NULL. This should not happen, otherwise it is a Qt bug!!!";
    }
}

QT_END_NAMESPACE
