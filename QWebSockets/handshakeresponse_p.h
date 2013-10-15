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

#ifndef HANDSHAKERESPONSE_P_H
#define HANDSHAKERESPONSE_P_H
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
#include "qwebsocketprotocol.h"

QT_BEGIN_NAMESPACE

class HandshakeRequest;
class QString;
class QTextStream;

class HandshakeResponse:public QObject
{
    Q_OBJECT
public:
    HandshakeResponse(const HandshakeRequest &request,
                      const QString &serverName,
                      bool isOriginAllowed,
                      const QList<QWebSocketProtocol::Version> &supportedVersions,
                      const QList<QString> &supportedProtocols,
                      const QList<QString> &supportedExtensions);

    virtual ~HandshakeResponse();

    bool isValid() const;
    bool canUpgrade() const;
    QString getAcceptedProtocol() const;
    QString getAcceptedExtension() const;
    QWebSocketProtocol::Version getAcceptedVersion() const;

public Q_SLOTS:

Q_SIGNALS:

private:
    Q_DISABLE_COPY(HandshakeResponse)
    bool m_isValid;
    bool m_canUpgrade;
    QString m_response;
    QString m_acceptedProtocol;
    QString m_acceptedExtension;
    QWebSocketProtocol::Version m_acceptedVersion;

    QString calculateAcceptKey(const QString &key) const;
    QString getHandshakeResponse(const HandshakeRequest &request,
                                 const QString &serverName,
                                 bool isOriginAllowed,
                                 const QList<QWebSocketProtocol::Version> &supportedVersions,
                                 const QList<QString> &supportedProtocols,
                                 const QList<QString> &supportedExtensions);

    QTextStream &writeToStream(QTextStream &textStream) const;
    friend QTextStream &operator <<(QTextStream &stream, const HandshakeResponse &response);
};

QT_END_NAMESPACE

#endif // HANDSHAKERESPONSE_P_H
