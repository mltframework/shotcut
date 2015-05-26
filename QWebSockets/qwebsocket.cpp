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

/*!
    \class QWebSocket

    \inmodule QWebSockets
    \brief Implements a TCP socket that talks the websocket protocol.

    WebSockets is a web technology providing full-duplex communications channels over a single TCP connection.
    The WebSocket protocol was standardized by the IETF as RFC 6455 in 2011 (see http://tools.ietf.org/html/rfc6455).
    It can both be used in a client application and server application.

    This class was modeled after QAbstractSocket.

    \sa QAbstractSocket, QTcpSocket

    \sa echoclient.html
*/

/*!
    \page echoclient.html example
    \title QWebSocket client example
    \brief A sample websocket client that sends a message and displays the message that it receives back.

    \section1 Description
    The EchoClient example implements a web socket client that sends a message to a websocket server and dumps the answer that it gets back.
    This example should ideally be used with the EchoServer example.
    \section1 Code
    We start by connecting to the `connected()` signal.
    \snippet echoclient.cpp constructor
    After the connection, we open the socket to the given \a url.

    \snippet echoclient.cpp onConnected
    When the client is connected successfully, we connect to the `onTextMessageReceived()` signal, and send out "Hello, world!".
    If connected with the EchoServer, we will receive the same message back.

    \snippet echoclient.cpp onTextMessageReceived
    Whenever a message is received, we write it out.
*/

/*!
  \fn void QWebSocket::connected()
  \brief Emitted when a connection is successfully established.
  \sa open(), disconnected()
*/
/*!
  \fn void QWebSocket::disconnected()
  \brief Emitted when the socket is disconnected.
  \sa close(), connected()
*/
/*!
    \fn void QWebSocket::aboutToClose()

    This signal is emitted when the socket is about to close.
    Connect this signal if you have operations that need to be performed before the socket closes
    (e.g., if you have data in a separate buffer that needs to be written to the device).

    \sa close()
 */
/*!
\fn void QWebSocket::proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)

This signal can be emitted when a \a proxy that requires
authentication is used. The \a authenticator object can then be
filled in with the required details to allow authentication and
continue the connection.

\note It is not possible to use a QueuedConnection to connect to
this signal, as the connection will fail if the authenticator has
not been filled in with new information when the signal returns.

\sa QAuthenticator, QNetworkProxy
*/
/*!
    \fn void QWebSocket::stateChanged(QAbstractSocket::SocketState state);

    This signal is emitted whenever QWebSocket's state changes.
    The \a state parameter is the new state.

    QAbstractSocket::SocketState is not a registered metatype, so for queued
    connections, you will have to register it with Q_REGISTER_METATYPE() and
    qRegisterMetaType().

    \sa state()
*/
/*!
    \fn void QWebSocket::readChannelFinished()

    This signal is emitted when the input (reading) stream is closed in this device. It is emitted as soon as the closing is detected.

    \sa close()
*/

/*!
    \fn void QWebSocket::textFrameReceived(QString frame, bool isLastFrame);

    This signal is emitted whenever a text frame is received. The \a frame contains the data and
    \a isLastFrame indicates whether this is the last frame of the complete message.

    This signal can be used to process large messages frame by frame, instead of waiting for the complete
    message to arrive.

    \sa binaryFrameReceived()
*/
/*!
    \fn void QWebSocket::binaryFrameReceived(QByteArray frame, bool isLastFrame);

    This signal is emitted whenever a binary frame is received. The \a frame contains the data and
    \a isLastFrame indicates whether this is the last frame of the complete message.

    This signal can be used to process large messages frame by frame, instead of waiting for the complete
    message to arrive.

    \sa textFrameReceived()
*/
/*!
    \fn void QWebSocket::textMessageReceived(QString message);

    This signal is emitted whenever a text message is received. The \a message contains the received text.

    \sa binaryMessageReceived()
*/
/*!
    \fn void QWebSocket::binaryMessageReceived(QByteArray message);

    This signal is emitted whenever a binary message is received. The \a message contains the received bytes.

    \sa textMessageReceived()
*/
/*!
    \fn void QWebSocket::error(QAbstractSocket::SocketError error);

    This signal is emitted after an error occurred. The \a error
    parameter describes the type of error that occurred.

    QAbstractSocket::SocketError is not a registered metatype, so for queued
    connections, you will have to register it with Q_DECLARE_METATYPE() and
    qRegisterMetaType().

    \sa error(), errorString()
*/
/*!
    \fn void QWebSocket::pong(quint64 elapsedTime, QByteArray payload)

    Emitted when a pong message is received in reply to a previous ping.
    \a elapsedTime contains the roundtrip time in milliseconds and \a payload contains an optional payload that was sent with the ping.

    \sa ping()
  */
#include "qwebsocket.h"
#include "qwebsocket_p.h"
#include <QUrl>
#include <QTcpSocket>
#include <QByteArray>
#include <QHostAddress>

#include <QDebug>

#include <limits>

QT_BEGIN_NAMESPACE

/*!
 * \brief Creates a new QWebSocket with the given \a origin, the \a version of the protocol to use and \a parent.
 *
 * The \a origin of the client is as specified in http://tools.ietf.org/html/rfc6454.
 * (The \a origin is not required for non-web browser clients (see RFC 6455)).
 * \note Currently only V13 (RFC 6455) is supported
 */
QWebSocket::QWebSocket(const QString &origin, QWebSocketProtocol::Version version, QObject *parent) :
    QObject(parent),
    d_ptr(new QWebSocketPrivate(origin, version, this, this))
{
}

/*!
 * \brief Destroys the QWebSocket. Closes the socket if it is still open, and releases any used resources.
 */
QWebSocket::~QWebSocket()
{
    delete d_ptr;
    //d_ptr = 0;
}

/*!
 * \brief Aborts the current socket and resets the socket. Unlike close(), this function immediately closes the socket, discarding any pending data in the write buffer.
 */
void QWebSocket::abort()
{
    Q_D(QWebSocket);
    d->abort();
}

/*!
 * Returns the type of error that last occurred
 * \sa errorString()
 */
QAbstractSocket::SocketError QWebSocket::error() const
{
    Q_D(const QWebSocket);
    return d->error();
}

//only called by QWebSocketPrivate::upgradeFrom
/*!
  \internal
 */
QWebSocket::QWebSocket(QTcpSocket *pTcpSocket, QWebSocketProtocol::Version version, QObject *parent) :
    QObject(parent),
    d_ptr(new QWebSocketPrivate(pTcpSocket, version, this, this))
{
}

/*!
 * Returns a human-readable description of the last error that occurred
 *
 * \sa error()
 */
QString QWebSocket::errorString() const
{
    Q_D(const QWebSocket);
    return d->errorString();
}

/*!
    This function writes as much as possible from the internal write buffer to the underlying network socket, without blocking.
    If any data was written, this function returns true; otherwise false is returned.
    Call this function if you need QWebSocket to start sending buffered data immediately.
    The number of bytes successfully written depends on the operating system.
    In most cases, you do not need to call this function, because QWebSocket will start sending data automatically once control goes back to the event loop.
    In the absence of an event loop, call waitForBytesWritten() instead.
*/
bool QWebSocket::flush()
{
    Q_D(QWebSocket);
    return d->flush();
}

/*!
    Sends the given \a message over the socket as a text message and returns the number of bytes actually sent.
    \a message must be '\\0' terminated.
 */
qint64 QWebSocket::write(const char *message)
{
    Q_D(QWebSocket);
    return d->write(message);
}

/*!
    Sends the most \a maxSize bytes of the given \a message over the socket as a text message and returns the number of bytes actually sent.
 */
qint64 QWebSocket::write(const char *message, qint64 maxSize)
{
    Q_D(QWebSocket);
    return d->write(message, maxSize);
}

/*!
    \brief Sends the given \a message over the socket as a text message and returns the number of bytes actually sent.
 */
qint64 QWebSocket::write(const QString &message)
{
    Q_D(QWebSocket);
    return d->write(message);
}

/*!
    \brief Sends the given \a data over the socket as a binary message and returns the number of bytes actually sent.
 */
qint64 QWebSocket::write(const QByteArray &data)
{
    Q_D(QWebSocket);
    return d->write(data);
}

/*!
    \brief Gracefully closes the socket with the given \a closeCode and \a reason. Any data in the write buffer is flushed before the socket is closed.
    The \a closeCode is a QWebSocketProtocol::CloseCode indicating the reason to close, and
    \a reason describes the reason of the closure more in detail
 */
void QWebSocket::close(QWebSocketProtocol::CloseCode closeCode, const QString &reason)
{
    Q_D(QWebSocket);
    d->close(closeCode, reason);
}

/*!
    \brief Opens a websocket connection using the given \a url.
    If \a mask is true, all frames will be masked; this is only necessary for client side sockets; servers should never mask
    \note A client socket must *always* mask its frames; servers may *never* mask its frames
 */
void QWebSocket::open(const QUrl &url, bool mask)
{
    Q_D(QWebSocket);
    d->open(url, mask);
}

/*!
    \brief Pings the server to indicate that the connection is still alive.
    Additional \a payload can be sent along the ping message.

    The size of the \a payload cannot be bigger than 125. If it is larger, the \a payload is clipped to 125 bytes.

    \sa pong()
 */
void QWebSocket::ping(const QByteArray &payload)
{
    Q_D(QWebSocket);
    if (payload.length() > 125)
    {
        payload.left(125);
    }
    d->ping(payload);
}

/*!
    \brief Returns the version the socket is currently using
 */
QWebSocketProtocol::Version QWebSocket::version() const
{
    Q_D(const QWebSocket);
    return d->version();
}

/*!
    \brief Returns the name of the resource currently accessed.
 */
QString QWebSocket::resourceName() const
{
    Q_D(const QWebSocket);
    return d->resourceName();
}

/*!
    \brief Returns the url the socket is connected to or will connect to.
 */
QUrl QWebSocket::requestUrl() const
{
    Q_D(const QWebSocket);
    return d->requestUrl();
}

/*!
    \brief Returns the current origin
 */
QString QWebSocket::origin() const
{
    Q_D(const QWebSocket);
    return d->origin();
}

/*!
    \brief Returns the currently used protocol.
 */
QString QWebSocket::protocol() const
{
    Q_D(const QWebSocket);
    return d->protocol();
}

/*!
    \brief Returns the currently used extension.
 */
QString QWebSocket::extension() const
{
    Q_D(const QWebSocket);
    return d->extension();
}

/*!
    \brief Returns the current state of the socket
 */
QAbstractSocket::SocketState QWebSocket::state() const
{
    Q_D(const QWebSocket);
    return d->state();
}

/*!
    \brief Waits until the socket is connected, up to \a msecs milliseconds. If the connection has been established, this function returns true; otherwise it returns false. In the case where it returns false, you can call error() to determine the cause of the error.
    The following example waits up to one second for a connection to be established:

    ~~~{.cpp}
    socket->open("ws://localhost:1234", false);
    if (socket->waitForConnected(1000))
    {
        qDebug("Connected!");
    }
    ~~~

    If \a msecs is -1, this function will not time out.
    @note This function may wait slightly longer than msecs, depending on the time it takes to complete the host lookup.
    @note Multiple calls to this functions do not accumulate the time. If the function times out, the connecting process will be aborted.

    \sa connected(), open(), state()
 */
bool QWebSocket::waitForConnected(int msecs)
{
    Q_D(QWebSocket);
    return d->waitForConnected(msecs);
}

/*!
  Waits \a msecs for the socket to be disconnected.
  If the socket was successfully disconnected within time, this method returns true.
  Otherwise false is returned.
  When \a msecs is -1, this function will block until the socket is disconnected.

  \sa close(), state()
*/
bool QWebSocket::waitForDisconnected(int msecs)
{
    Q_D(QWebSocket);
    return d->waitForDisconnected(msecs);
}

/*!
    Returns the local address
 */
QHostAddress QWebSocket::localAddress() const
{
    Q_D(const QWebSocket);
    return d->localAddress();
}

/*!
    Returns the local port
 */
quint16 QWebSocket::localPort() const
{
    Q_D(const QWebSocket);
    return d->localPort();
}

/*!
    Returns the pause mode of this socket
 */
QAbstractSocket::PauseModes QWebSocket::pauseMode() const
{
    Q_D(const QWebSocket);
    return d->pauseMode();
}

/*!
    Returns the peer address
 */
QHostAddress QWebSocket::peerAddress() const
{
    Q_D(const QWebSocket);
    return d->peerAddress();
}

/*!
    Returns the peerName
 */
QString QWebSocket::peerName() const
{
    Q_D(const QWebSocket);
    return d->peerName();
}

/*!
    Returns the peerport
 */
quint16 QWebSocket::peerPort() const
{
    Q_D(const QWebSocket);
    return d->peerPort();
}

#ifndef QT_NO_NETWORKPROXY
/*!
    Returns the currently configured proxy
 */
QNetworkProxy QWebSocket::proxy() const
{
    Q_D(const QWebSocket);
    return d->proxy();
}

/*!
    Sets the proxy to \a networkProxy
 */
void QWebSocket::setProxy(const QNetworkProxy &networkProxy)
{
    Q_D(QWebSocket);
    d->setProxy(networkProxy);
}
#endif

/*!
    Returns the size in bytes of the readbuffer that is used by the socket.
 */
qint64 QWebSocket::readBufferSize() const
{
    Q_D(const QWebSocket);
    return d->readBufferSize();
}

/*!
    Continues data transfer on the socket. This method should only be used after the socket
    has been set to pause upon notifications and a notification has been received.
    The only notification currently supported is sslErrors().
    Calling this method if the socket is not paused results in undefined behavior.

    \sa pauseMode(), setPauseMode()
 */
void QWebSocket::resume()
{
    Q_D(QWebSocket);
    d->resume();
}

/*!
    Controls whether to pause upon receiving a notification. The \a pauseMode parameter specifies
    the conditions in which the socket should be paused.
    The only notification currently supported is sslErrors().
    If set to PauseOnSslErrors, data transfer on the socket will be paused
    and needs to be enabled explicitly again by calling resume().
    By default, this option is set to PauseNever. This option must be called
    before connecting to the server, otherwise it will result in undefined behavior.

    \sa pauseMode(), resume()
 */
void QWebSocket::setPauseMode(QAbstractSocket::PauseModes pauseMode)
{
    Q_D(QWebSocket);
    d->setPauseMode(pauseMode);
}

/*!
    Sets the size of QWebSocket's internal read buffer to be \a size bytes.
    If the buffer size is limited to a certain size, QWebSocket won't buffer more than this size of data.
    Exceptionally, a buffer size of 0 means that the read buffer is unlimited and all incoming data is buffered. This is the default.
    This option is useful if you only read the data at certain points in time (e.g., in a real-time streaming application) or if you want to protect your socket against receiving too much data, which may eventually cause your application to run out of memory.
    \sa readBufferSize()
*/
void QWebSocket::setReadBufferSize(qint64 size)
{
    Q_D(QWebSocket);
    d->setReadBufferSize(size);
}

/*!
    Sets the given \a option to the value described by \a value.
    \sa socketOption()
*/
void QWebSocket::setSocketOption(QAbstractSocket::SocketOption option, const QVariant &value)
{
    Q_D(QWebSocket);
    d->setSocketOption(option, value);
}

/*!
    Returns the value of the option \a option.
    \sa setSocketOption()
*/
QVariant QWebSocket::socketOption(QAbstractSocket::SocketOption option)
{
    Q_D(QWebSocket);
    return d->socketOption(option);
}

/*!
    Returns true if the QWebSocket is valid.
 */
bool QWebSocket::isValid() const
{
    Q_D(const QWebSocket);
    return d->isValid();
}

QT_END_NAMESPACE
