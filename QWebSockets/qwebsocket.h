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

#ifndef QWEBSOCKET_H
#define QWEBSOCKET_H

#include <QUrl>
#include <QAbstractSocket>
#include <QHostAddress>
#ifndef QT_NO_NETWORKPROXY
#include <QNetworkProxy>
#endif
#include <QTime>
#include "qwebsocketsglobal.h"
#include "qwebsocketprotocol.h"

QT_BEGIN_NAMESPACE

class QTcpSocket;
class QWebSocketPrivate;

class Q_WEBSOCKETS_EXPORT QWebSocket:public QObject
{
    Q_OBJECT

public:
    explicit QWebSocket(const QString &origin = QString(), QWebSocketProtocol::Version version = QWebSocketProtocol::V_LATEST, QObject *parent = 0);
    virtual ~QWebSocket();

    void abort();
    QAbstractSocket::SocketError error() const;
    QString errorString() const;
    bool flush();
    bool isValid() const;
    QHostAddress localAddress() const;
    quint16 localPort() const;
    QAbstractSocket::PauseModes pauseMode() const;
    QHostAddress peerAddress() const;
    QString peerName() const;
    quint16 peerPort() const;
#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy proxy() const;
    void setProxy(const QNetworkProxy &networkProxy);
#endif
    qint64 readBufferSize() const;
    void setReadBufferSize(qint64 size);

    void resume();
    void setPauseMode(QAbstractSocket::PauseModes pauseMode);

    void setSocketOption(QAbstractSocket::SocketOption option, const QVariant &value);
    QVariant socketOption(QAbstractSocket::SocketOption option);
    QAbstractSocket::SocketState state() const;

    bool waitForConnected(int msecs = 30000);
    bool waitForDisconnected(int msecs = 30000);

    QWebSocketProtocol::Version version() const;
    QString resourceName() const;
    QUrl requestUrl() const;
    QString origin() const;
    QString protocol() const;
    QString extension() const;

    qint64 write(const char *message);		//send data as text
    qint64 write(const char *message, qint64 maxSize);		//send data as text
    qint64 write(const QString &message);	//send data as text
    qint64 write(const QByteArray &data);	//send data as binary

public Q_SLOTS:
    void close(QWebSocketProtocol::CloseCode closeCode = QWebSocketProtocol::CC_NORMAL, const QString &reason = QString());
    void open(const QUrl &url, bool mask = true);
    void ping(const QByteArray &payload = QByteArray());

Q_SIGNALS:
    void aboutToClose();
    void connected();
    void disconnected();
    void stateChanged(QAbstractSocket::SocketState state);
#ifndef QT_NO_NETWORKPROXY
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *pAuthenticator);
#endif
    void readChannelFinished();
    void textFrameReceived(QString frame, bool isLastFrame);
    void binaryFrameReceived(QByteArray frame, bool isLastFrame);
    void textMessageReceived(QString message);
    void binaryMessageReceived(QByteArray message);
    void error(QAbstractSocket::SocketError error);
    void pong(quint64 elapsedTime, QByteArray payload);

private:
    Q_DISABLE_COPY(QWebSocket)
    Q_DECLARE_PRIVATE(QWebSocket)
    QWebSocket(QTcpSocket *pTcpSocket, QWebSocketProtocol::Version version, QObject *parent = 0);
    QWebSocketPrivate * const d_ptr;
};

QT_END_NAMESPACE

#endif // QWEBSOCKET_H
