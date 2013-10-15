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

#include "handshakeresponse_p.h"
#include "handshakerequest_p.h"
#include <QString>
#include <QTextStream>
#include <QByteArray>
#include <QStringList>
#include <QDateTime>
#include <QCryptographicHash>
#include <QSet>
#include <QList>
#include <QStringBuilder>   //for more efficient string concatenation

QT_BEGIN_NAMESPACE

/*!
    \internal
 */
HandshakeResponse::HandshakeResponse(const HandshakeRequest &request,
                                     const QString &serverName,
                                     bool isOriginAllowed,
                                     const QList<QWebSocketProtocol::Version> &supportedVersions,
                                     const QList<QString> &supportedProtocols,
                                     const QList<QString> &supportedExtensions) :
    m_isValid(false),
    m_canUpgrade(false),
    m_response(),
    m_acceptedProtocol(),
    m_acceptedExtension(),
    m_acceptedVersion(QWebSocketProtocol::V_Unknow)
{
    m_response = getHandshakeResponse(request, serverName, isOriginAllowed, supportedVersions, supportedProtocols, supportedExtensions);
    m_isValid = true;
}

/*!
    \internal
 */
HandshakeResponse::~HandshakeResponse()
{
}

/*!
    \internal
 */
bool HandshakeResponse::isValid() const
{
    return m_isValid;
}

/*!
    \internal
 */
bool HandshakeResponse::canUpgrade() const
{
    return m_isValid && m_canUpgrade;
}

/*!
    \internal
 */
QString HandshakeResponse::getAcceptedProtocol() const
{
    return m_acceptedProtocol;
}

/*!
    \internal
 */
QString HandshakeResponse::calculateAcceptKey(const QString &key) const
{
    QString tmpKey = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";	//the UID comes from RFC6455
    QByteArray hash = QCryptographicHash::hash(tmpKey.toLatin1(), QCryptographicHash::Sha1);
    return QString(hash.toBase64());
}

/*!
    \internal
 */
QString HandshakeResponse::getHandshakeResponse(const HandshakeRequest &request,
                                                const QString &serverName,
                                                bool isOriginAllowed,
                                                const QList<QWebSocketProtocol::Version> &supportedVersions,
                                                const QList<QString> &supportedProtocols,
                                                const QList<QString> &supportedExtensions)
{
    QStringList response;
    m_canUpgrade = false;

    if (!isOriginAllowed)
    {
        if (!m_canUpgrade)
        {
            response << "HTTP/1.1 403 Access Forbidden";
        }
    }
    else
    {
        if (request.isValid())
        {
            QString acceptKey = calculateAcceptKey(request.getKey());
            QList<QString> matchingProtocols = supportedProtocols.toSet().intersect(request.getProtocols().toSet()).toList();
            QList<QString> matchingExtensions = supportedExtensions.toSet().intersect(request.getExtensions().toSet()).toList();
            QList<QWebSocketProtocol::Version> matchingVersions = request.getVersions().toSet().intersect(supportedVersions.toSet()).toList();
            qStableSort(matchingVersions.begin(), matchingVersions.end(), qGreater<QWebSocketProtocol::Version>());	//sort in descending order

            if (matchingVersions.isEmpty())
            {
                m_canUpgrade = false;
            }
            else
            {
                response << "HTTP/1.1 101 Switching Protocols" <<
                            "Upgrade: websocket" <<
                            "Connection: Upgrade" <<
                            "Sec-WebSocket-Accept: " % acceptKey;
                if (!matchingProtocols.isEmpty())
                {
                    m_acceptedProtocol = matchingProtocols.first();
                    response << "Sec-WebSocket-Protocol: " % m_acceptedProtocol;
                }
                if (!matchingExtensions.isEmpty())
                {
                    m_acceptedExtension = matchingExtensions.first();
                    response << "Sec-WebSocket-Extensions: " % m_acceptedExtension;
                }
                QString origin = request.getOrigin().trimmed();
                if (origin.isEmpty())
                {
                    origin = "*";
                }
                response << "Server: " + serverName <<
                            "Access-Control-Allow-Credentials: false"		<<	//do not allow credentialed request (containing cookies)
                            "Access-Control-Allow-Methods: GET"				<<	//only GET is allowed during handshaking
                            "Access-Control-Allow-Headers: content-type"	<<	//this is OK; only the content-type header is allowed, no other headers are accepted
                            "Access-Control-Allow-Origin: " % origin		<<
                            "Date: " % QDateTime::currentDateTimeUtc().toString("ddd, dd MMM yyyy hh:mm:ss 'GMT'");

                m_acceptedVersion = QWebSocketProtocol::currentVersion();
                m_canUpgrade = true;
            }
        }
        else
        {
            m_canUpgrade = false;
        }
        if (!m_canUpgrade)
        {
            response << "HTTP/1.1 400 Bad Request";
            QStringList versions;
            Q_FOREACH(QWebSocketProtocol::Version version, supportedVersions)
            {
                versions << QString::number(static_cast<int>(version));
            }
            response << "Sec-WebSocket-Version: " % versions.join(", ");
        }
    }
    response << "\r\n";	//append empty line at end of header
    return response.join("\r\n");
}

/*!
    \internal
 */
QTextStream &HandshakeResponse::writeToStream(QTextStream &textStream) const
{
    if (!m_response.isEmpty())
    {
        textStream << m_response.toLatin1().constData();
    }
    else
    {
        textStream.setStatus(QTextStream::WriteFailed);
    }
    return textStream;
}

/*!
    \internal
 */
QTextStream &operator <<(QTextStream &stream, const HandshakeResponse &response)
{
    return response.writeToStream(stream);
}

/*!
    \internal
 */
QWebSocketProtocol::Version HandshakeResponse::getAcceptedVersion() const
{
    return m_acceptedVersion;
}

/*!
    \internal
 */
QString HandshakeResponse::getAcceptedExtension() const
{
    return m_acceptedExtension;
}

QT_END_NAMESPACE
