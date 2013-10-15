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
    \class QWebSocketServer

    \inmodule QWebSockets

    \brief Implements a websocket-based server.

    It is modeled after QTcpServer, and behaves the same. So, if you know how to use QTcpServer, you know how to use QWebSocketServer.
    This class makes it possible to accept incoming websocket connections.
    You can specify the port or have QWebSocketServer pick one automatically.
    You can listen on a specific address or on all the machine's addresses.
    Call listen() to have the server listen for incoming connections.

    The newConnection() signal is then emitted each time a client connects to the server.
    Call nextPendingConnection() to accept the pending connection as a connected QWebSocket.
    The function returns a pointer to a QWebSocket in QAbstractSocket::ConnectedState that you can use for communicating with the client.
    If an error occurs, serverError() returns the type of error, and errorString() can be called to get a human readable description of what happened.
    When listening for connections, the address and port on which the server is listening are available as serverAddress() and serverPort().
    Calling close() makes QWebSocketServer stop listening for incoming connections.
    Although QWebSocketServer is mostly designed for use with an event loop, it's possible to use it without one. In that case, you must use waitForNewConnection(), which blocks until either a connection is available or a timeout expires.

    \sa echoserver.html

    \sa QWebSocket
*/

/*!
  \page echoserver.html example
  \title WebSocket server example
  \brief A sample websocket server echoing back messages sent to it.

  \section1 Description
  The echoserver example implements a web socket server that echoes back everything that is sent to it.
  \section1 Code
  We start by creating a QWebSocketServer (`new QWebSocketServer()`). After the creation, we listen on all local network interfaces (`QHostAddress::Any`) on the specified \a port.
  \snippet echoserver.cpp constructor
  If listening is successful, we connect the `newConnection()` signal to the slot `onNewConnection()`.
  The `newConnection()` signal will be thrown whenever a new web socket client is connected to our server.

  \snippet echoserver.cpp onNewConnection
  When a new connection is received, the client QWebSocket is retrieved (`nextPendingConnection()`), and the signals we are interested in
  are connected to our slots (`textMessageReceived()`, `binaryMessageReceived()` and `disconnected()`).
  The client socket is remembered in a list, in case we would like to use it later (in this example, nothing is done with it).

  \snippet echoserver.cpp processMessage
  Whenever `processMessage()` is triggered, we retrieve the sender, and if valid, send back the original message (`send()`).
  The same is done with binary messages.
  \snippet echoserver.cpp processBinaryMessage
  The only difference is that the message now is a QByteArray instead of a QString.

  \snippet echoserver.cpp socketDisconnected
  Whenever a socket is disconnected, we remove it from the clients list and delete the socket.
  Note: it is best to use `deleteLater()` to delete the socket.
*/

/*!
    \fn void QWebSocketServer::acceptError(QAbstractSocket::SocketError socketError)
    This signal is emitted when accepting a new connection results in an error.
    The \a socketError parameter describes the type of error that occurred

    \sa pauseAccepting(), resumeAccepting()
*/

/*!
    \fn void QWebSocketServer::newConnection()
    This signal is emitted every time a new connection is available.

    \sa hasPendingConnections(), nextPendingConnection()
*/

/*!
    \fn void QWebSocketServer::originAuthenticationRequired(QCorsAuthenticator *authenticator)
    This signal is emitted when a new connection is requested.
    The slot connected to this signal should indicate whether the origin (which can be determined by the origin() call)
    is allowed in the \a authenticator object (by issuing \l{QCorsAuthenticator::}{setAllowed()})

    If no slot is connected to this signal, all origins will be accepted by default.

    \note It is not possible to use a QueuedConnection to connect to
    this signal, as the connection will always succeed.
*/

#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkProxy>
#include "qwebsocketprotocol.h"
#include "qwebsocket.h"
#include "qwebsocketserver.h"
#include "qwebsocketserver_p.h"

//TODO: CorsCheck: give list in constructor or use CorsAuthenticator object
//in QNetworkAccessManager the signal cannot be connected to a queued signal, because it waits for the signal to return

QT_BEGIN_NAMESPACE

/*!
    Constructs a new WebSocketServer with the given \a serverName.
    The \a serverName will be used in the http handshake phase to identify the server.

    \a parent is passed to the QObject constructor.
 */
QWebSocketServer::QWebSocketServer(const QString &serverName, QObject *parent) :
    QObject(parent),
    d_ptr(new QWebSocketServerPrivate(serverName, this, this))
{
}

/*!
    Destroys the WebSocketServer object. If the server is listening for connections, the socket is automatically closed.
    Any client WebSockets that are still connected are closed and deleted.

    \sa close()
 */
QWebSocketServer::~QWebSocketServer()
{
    delete d_ptr;
}

/*!
  Closes the server. The server will no longer listen for incoming connections.
 */
void QWebSocketServer::close()
{
    Q_D(QWebSocketServer);
    d->close();
}

/*!
    Returns a human readable description of the last error that occurred.

    \sa serverError()
*/
QString QWebSocketServer::errorString() const
{
    Q_D(const QWebSocketServer);
    return d->errorString();
}

/*!
    Returns true if the server has pending connections; otherwise returns false.

    \sa nextPendingConnection(), setMaxPendingConnections()
 */
bool QWebSocketServer::hasPendingConnections() const
{
    Q_D(const QWebSocketServer);
    return d->hasPendingConnections();
}

/*!
    Returns true if the server is currently listening for incoming connections; otherwise returns false.

    \sa listen()
 */
bool QWebSocketServer::isListening() const
{
    Q_D(const QWebSocketServer);
    return d->isListening();
}

/*!
    Tells the server to listen for incoming connections on address \a address and port \a port.
    If \a port is 0, a port is chosen automatically.
    If \a address is QHostAddress::Any, the server will listen on all network interfaces.

    Returns true on success; otherwise returns false.

    \sa isListening()
 */
bool QWebSocketServer::listen(const QHostAddress &address, quint16 port)
{
    Q_D(QWebSocketServer);
    return d->listen(address, port);
}

/*!
    Returns the maximum number of pending accepted connections. The default is 30.

    \sa setMaxPendingConnections(), hasPendingConnections()
 */
int QWebSocketServer::maxPendingConnections() const
{
    Q_D(const QWebSocketServer);
    return d->maxPendingConnections();
}

/*!
    Returns the next pending connection as a connected WebSocket object.
    The socket is created as a child of the server, which means that it is automatically deleted when the WebSocketServer object is destroyed. It is still a good idea to delete the object explicitly when you are done with it, to avoid wasting memory.
    0 is returned if this function is called when there are no pending connections.

    Note: The returned WebSocket object cannot be used from another thread..

    \sa hasPendingConnections()
*/
QWebSocket *QWebSocketServer::nextPendingConnection()
{
    Q_D(QWebSocketServer);
    return d->nextPendingConnection();
}

/*!
    Pauses incoming new connections. Queued connections will remain in queue.
    \sa resumeAccepting()
 */
void QWebSocketServer::pauseAccepting()
{
    Q_D(QWebSocketServer);
    d->pauseAccepting();
}

#ifndef QT_NO_NETWORKPROXY
/*!
    Returns the network proxy for this socket. By default QNetworkProxy::DefaultProxy is used.

    \sa setProxy()
*/
QNetworkProxy QWebSocketServer::proxy() const
{
    Q_D(const QWebSocketServer);
    return d->proxy();
}

/*!
    \brief Sets the explicit network proxy for this socket to \a networkProxy.

    To disable the use of a proxy for this socket, use the QNetworkProxy::NoProxy proxy type:

    \code
        server->setProxy(QNetworkProxy::NoProxy);
    \endcode

    \sa proxy()
*/
void QWebSocketServer::setProxy(const QNetworkProxy &networkProxy)
{
    Q_D(QWebSocketServer);
    d->setProxy(networkProxy);
}
#endif
/*!
    Resumes accepting new connections.
    \sa pauseAccepting()
 */
void QWebSocketServer::resumeAccepting()
{
    Q_D(QWebSocketServer);
    d->resumeAccepting();
}

/*!
    Sets the server name that will be used during the http handshake phase to the given \a serverName.
    Existing connected clients will not be notified of this change, only newly connecting clients
    will see this new name.
 */
void QWebSocketServer::setServerName(const QString &serverName)
{
    Q_D(QWebSocketServer);
    d->setServerName(serverName);
}

/*!
    Returns the server name that is used during the http handshake phase.
 */
QString QWebSocketServer::serverName() const
{
    Q_D(const QWebSocketServer);
    return d->serverName();
}

/*!
    Returns the server's address if the server is listening for connections; otherwise returns QHostAddress::Null.

    \sa serverPort(), listen()
 */
QHostAddress QWebSocketServer::serverAddress() const
{
    Q_D(const QWebSocketServer);
    return d->serverAddress();
}

/*!
    Returns an error code for the last error that occurred.
    \sa errorString()
 */
QAbstractSocket::SocketError QWebSocketServer::serverError() const
{
    Q_D(const QWebSocketServer);
    return d->serverError();
}

/*!
    Returns the server's port if the server is listening for connections; otherwise returns 0.
    \sa serverAddress(), listen()
 */
quint16 QWebSocketServer::serverPort() const
{
    Q_D(const QWebSocketServer);
    return d->serverPort();
}

/*!
    Sets the maximum number of pending accepted connections to \a numConnections.
    WebSocketServer will accept no more than \a numConnections incoming connections before nextPendingConnection() is called.
    By default, the limit is 30 pending connections.

    Clients may still able to connect after the server has reached its maximum number of pending connections (i.e., WebSocket can still emit the connected() signal). WebSocketServer will stop accepting the new connections, but the operating system may still keep them in queue.
    \sa maxPendingConnections(), hasPendingConnections()
 */
void QWebSocketServer::setMaxPendingConnections(int numConnections)
{
    Q_D(QWebSocketServer);
    d->setMaxPendingConnections(numConnections);
}

/*!
    Sets the socket descriptor this server should use when listening for incoming connections to \a socketDescriptor.

    Returns true if the socket is set successfully; otherwise returns false.
    The socket is assumed to be in listening state.

    \sa socketDescriptor(), isListening()
 */
bool QWebSocketServer::setSocketDescriptor(int socketDescriptor)
{
    Q_D(QWebSocketServer);
    return d->setSocketDescriptor(socketDescriptor);
}

/*!
    Returns the native socket descriptor the server uses to listen for incoming instructions, or -1 if the server is not listening.
    If the server is using QNetworkProxy, the returned descriptor may not be usable with native socket functions.

    \sa setSocketDescriptor(), isListening()
 */
int QWebSocketServer::socketDescriptor() const
{
    Q_D(const QWebSocketServer);
    return d->socketDescriptor();
}

/*!
    Waits for at most \a msec milliseconds or until an incoming connection is available.
    Returns true if a connection is available; otherwise returns false.
    If the operation timed out and \a timedOut is not 0, \a timedOut will be set to true.

    \note This is a blocking function call.
    \note Its use is disadvised in a single-threaded GUI application, since the whole application will stop responding until the function returns. waitForNewConnection() is mostly useful when there is no event loop available.
    \note The non-blocking alternative is to connect to the newConnection() signal.

    If \a msec is -1, this function will not time out.

    \sa hasPendingConnections(), nextPendingConnection()
*/
bool QWebSocketServer::waitForNewConnection(int msec, bool *timedOut)
{
    Q_D(QWebSocketServer);
    return d->waitForNewConnection(msec, timedOut);
}

/*!
  Returns a list of websocket versions that this server is supporting.
 */
QList<QWebSocketProtocol::Version> QWebSocketServer::supportedVersions() const
{
    Q_D(const QWebSocketServer);
    return d->supportedVersions();
}

/*!
  Returns a list of websocket subprotocols that this server supports.
 */
QList<QString> QWebSocketServer::supportedProtocols() const
{
    Q_D(const QWebSocketServer);
    return d->supportedProtocols();
}

/*!
  Returns a list of websocket extensions that this server supports.
 */
QList<QString> QWebSocketServer::supportedExtensions() const
{
    Q_D(const QWebSocketServer);
    return d->supportedExtensions();
}

QT_END_NAMESPACE
