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
    \class QCorsAuthenticator

    \inmodule QWebSockets
    \brief The QCorsAuthenticator class provides an authenticator object for Cross Origin Requests (CORS).

    The QCorsAuthenticator class is used in the \l{QWebSocketServer::}{originAuthenticationRequired()} signal.
    The class provides a way to pass back the required information to the QWebSocketServer.
    It provides applications with fine-grained control over which origin URLs are allowed and which aren't.
    By default, every origin is accepted.
    To get fine grained control, an application connects the \l{QWebSocketServer::}{originAuthenticationRequired()} signal to
    a slot. When the origin (QCorsAuthenticator::origin()) is accepted, it calls QCorsAuthenticator::setAllowed(true)

    \note Checking on the origin does not make much sense when the server is accessed
    via a non-browser client, as that client can set whatever origin header it likes.
    In case of a browser client, the server SHOULD check the validity of the origin.
    \sa http://tools.ietf.org/html/rfc6455#section-10

    \sa QWebSocketServer
*/

#include "qcorsauthenticator.h"
#include "qcorsauthenticator_p.h"

QT_BEGIN_NAMESPACE

/*!
  \internal
 */
QCorsAuthenticatorPrivate::QCorsAuthenticatorPrivate(const QString &origin, bool allowed) :
    m_origin(origin),
    m_isAllowed(allowed)
{}

/*!
  \internal
 */
QCorsAuthenticatorPrivate::~QCorsAuthenticatorPrivate()
{}

/*!
  Constructs a new QCorsAuthencator object with the given \a origin.
  \note By default, allowed() returns true. This means that per default every origin is accepted.
 */
QCorsAuthenticator::QCorsAuthenticator(const QString &origin) :
    d_ptr(new QCorsAuthenticatorPrivate(origin, true))  //all origins are per default allowed
{
}

/*!
  Destructs the object
 */
QCorsAuthenticator::~QCorsAuthenticator()
{
    if (d_ptr)
    {
        delete d_ptr;
    }
}

/*!
  Constructs a coy of \a other
 */
QCorsAuthenticator::QCorsAuthenticator(const QCorsAuthenticator &other) :
    d_ptr(new QCorsAuthenticatorPrivate(other.d_ptr->m_origin, other.d_ptr->m_isAllowed))
{
}

/*!
  Assigns \a other to this authenticator object
 */
QCorsAuthenticator &QCorsAuthenticator::operator =(const QCorsAuthenticator &other)
{
    Q_D(QCorsAuthenticator);
    if (this != &other)
    {
        d->m_origin = other.d_ptr->m_origin;
        d->m_isAllowed = other.d_ptr->m_isAllowed;
    }
    return *this;
}

/*!
  Returns the origin this autenticator is handling about.
 */
QString QCorsAuthenticator::origin() const
{
    Q_D(const QCorsAuthenticator);
    return d->m_origin;
}

/*!
  Allows or disallows the origin. Setting \a allowed to true, will accept the connection request for the given origin.
  Setting \a allowed to false, will reject the connection request.

  \note By default, all origins are accepted.
 */
void QCorsAuthenticator::setAllowed(bool allowed)
{
    Q_D(QCorsAuthenticator);
    d->m_isAllowed = allowed;
}

/*!
  Returns true if the origin is allowed, otherwise returns false.

  \note By default, all origins are accepted.
 */
bool QCorsAuthenticator::allowed() const
{
    Q_D(const QCorsAuthenticator);
    return d->m_isAllowed;
}
