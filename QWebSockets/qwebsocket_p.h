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

#ifndef QWEBSOCKET_P_H
#define QWEBSOCKET_P_H
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

#include <QUrl>
#include <QAbstractSocket>
#include <QHostAddress>
#ifndef QT_NO_NETWORKPROXY
#include <QNetworkProxy>
#endif
#include <QTime>
#include "qwebsocketsglobal.h"
#include "qwebsocketprotocol.h"
#include "dataprocessor_p.h"

QT_BEGIN_NAMESPACE

class HandshakeRequest;
class HandshakeResponse;
class QTcpSocket;
class QWebSocket;

class QWebSocketPrivate:public QObject
{
    Q_OBJECT

public:
    explicit QWebSocketPrivate(const QString &origin,
                               QWebSocketProtocol::Version version,
                               QWebSocket * const pWebSocket,
                               QObject *parent = 0);
    virtual ~QWebSocketPrivate();

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
    void resume();
    void setPauseMode(QAbstractSocket::PauseModes pauseMode);
    void setReadBufferSize(qint64 size);
    void setSocketOption(QAbstractSocket::SocketOption option, const QVariant &value);
    QVariant socketOption(QAbstractSocket::SocketOption option);
    QAbstractSocket::SocketState state() const;

    bool waitForConnected(int msecs);
    bool waitForDisconnected(int msecs);

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
    void close(QWebSocketProtocol::CloseCode closeCode, QString reason);
    void open(const QUrl &url, bool mask);
    void ping(const QByteArray &payload);

private Q_SLOTS:
    void processData();
    void processPing(QByteArray data);
    void processPong(QByteArray data);
    void processClose(QWebSocketProtocol::CloseCode closeCode, QString closeReason);
    void processHandshake(QTcpSocket *pSocket);
    void processStateChanged(QAbstractSocket::SocketState socketState);

private:
    Q_DISABLE_COPY(QWebSocketPrivate)
    Q_DECLARE_PUBLIC(QWebSocket)

    QWebSocket * const q_ptr;

    QWebSocketPrivate(QTcpSocket *pTcpSocket, QWebSocketProtocol::Version version, QWebSocket *pWebSocket, QObject *parent = 0);
    void setVersion(QWebSocketProtocol::Version version);
    void setResourceName(const QString &resourceName);
    void setRequestUrl(const QUrl &requestUrl);
    void setOrigin(const QString &origin);
    void setProtocol(const QString &protocol);
    void setExtension(const QString &extension);
    void enableMasking(bool enable);
    void setSocketState(QAbstractSocket::SocketState state);
    void setErrorString(const QString &errorString);

    qint64 doWriteData(const QByteArray &data, bool isBinary);
    qint64 doWriteFrames(const QByteArray &data, bool isBinary);

    void makeConnections(const QTcpSocket *pTcpSocket);
    void releaseConnections(const QTcpSocket *pTcpSocket);

    QByteArray getFrameHeader(QWebSocketProtocol::OpCode opCode, quint64 payloadLength, quint32 maskingKey, bool lastFrame) const;
    QString calculateAcceptKey(const QString &key) const;
    QString createHandShakeRequest(QString resourceName,
                                   QString host,
                                   QString origin,
                                   QString extensions,
                                   QString protocols,
                                   QByteArray key);

    static QWebSocket *upgradeFrom(QTcpSocket *tcpSocket,
                                   const HandshakeRequest &request,
                                   const HandshakeResponse &response,
                                   QObject *parent = 0);

    quint32 generateMaskingKey() const;
    QByteArray generateKey() const;
    quint32 generateRandomNumber() const;
    qint64 writeFrames(const QList<QByteArray> &frames);
    qint64 writeFrame(const QByteArray &frame);

    QTcpSocket *m_pSocket;
    QString m_errorString;
    QWebSocketProtocol::Version m_version;
    QUrl m_resource;
    QString m_resourceName;
    QUrl m_requestUrl;
    QString m_origin;
    QString m_protocol;
    QString m_extension;
    QAbstractSocket::SocketState m_socketState;

    QByteArray m_key;	//identification key used in handshake requests

    bool m_mustMask;	//a server must not mask the frames it sends

    bool m_isClosingHandshakeSent;
    bool m_isClosingHandshakeReceived;

    QTime m_pingTimer;

    DataProcessor m_dataProcessor;

    friend class QWebSocketServerPrivate;
};

QT_END_NAMESPACE

#endif // QWEBSOCKET_H
