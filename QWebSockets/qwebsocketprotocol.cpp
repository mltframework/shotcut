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

#include "qwebsocketprotocol.h"
#include <QString>
#include <QSet>
#include <QtEndian>

/*!
    \enum WebSocketProtocol::CloseCode

    The close codes supported by WebSockets V13

    \value CC_NORMAL                    Normal closure
    \value CC_GOING_AWAY                Going away
    \value CC_PROTOCOL_ERROR            Protocol error
    \value CC_DATATYPE_NOT_SUPPORTED    Unsupported data
    \value CC_RESERVED_1004             Reserved
    \value CC_MISSING_STATUS_CODE       No status received
    \value CC_ABNORMAL_DISCONNECTION    Abnormal closure
    \value CC_WRONG_DATATYPE            Invalid frame payload data
    \value CC_POLICY_VIOLATED           Policy violation
    \value CC_TOO_MUCH_DATA             Message too big
    \value CC_MISSING_EXTENSION         Mandatory extension missing
    \value CC_BAD_OPERATION             Internal server error
    \value CC_TLS_HANDSHAKE_FAILED      TLS handshake failed

    \sa \l{QWebSocket::} {close()}
*/
/*!
    \enum WebSocketProtocol::Version

    \brief The different defined versions of the Websocket protocol.

    For an overview of the differences between the different protocols, see
    <http://code.google.com/p/pywebsocket/wiki/WebSocketProtocolSpec>

    \value V_Unknow
    \value V_0          hixie76: http://tools.ietf.org/html/draft-hixie-thewebsocketprotocol-76 & hybi-00: http://tools.ietf.org/html/draft-ietf-hybi-thewebsocketprotocol-00.
                        Works with key1, key2 and a key in the payload.
                        Attribute: Sec-WebSocket-Draft value 0.
    \value V_4          hybi-04: http://tools.ietf.org/id/draft-ietf-hybi-thewebsocketprotocol-04.txt.
                        Changed handshake: key1, key2, key3 ==> Sec-WebSocket-Key, Sec-WebSocket-Nonce, Sec-WebSocket-Accept
                        Sec-WebSocket-Draft renamed to Sec-WebSocket-Version
                        Sec-WebSocket-Version = 4
    \value V_5          hybi-05: http://tools.ietf.org/id/draft-ietf-hybi-thewebsocketprotocol-05.txt.
                        Sec-WebSocket-Version = 5
                        Removed Sec-WebSocket-Nonce
                        Added Sec-WebSocket-Accept
    \value V_6          Sec-WebSocket-Version = 6.
    \value V_7          hybi-07: http://tools.ietf.org/html/draft-ietf-hybi-thewebsocketprotocol-07.
                        Sec-WebSocket-Version = 7
    \value V_8          hybi-8, hybi-9, hybi-10, hybi-11 and hybi-12.
                        Status codes 1005 and 1006 are added and all codes are now unsigned
                        Internal error results in 1006
    \value V_13         hybi-13, hybi14, hybi-15, hybi-16, hybi-17 and RFC 6455.
                        Sec-WebSocket-Version = 13
                        Status code 1004 is now reserved
                        Added 1008, 1009 and 1010
                        Must support TLS
                        Clarify multiple version support
    \value V_LATEST     Refers to the latest know version to QWebSockets.
*/

/*!
  \fn WebSocketProtocol::isOpCodeReserved(OpCode code)
  Checks if \a code is a valid OpCode
  \internal
*/

/*!
  \fn WebSocketProtocol::isCloseCodeValid(int closeCode)
  Checks if \a closeCode is a valid web socket close code
  \internal
*/

/*!
  \fn WebSocketProtocol::getCurrentVersion()
  Returns the latest version that WebSocket is supporting
  \internal
*/

QT_BEGIN_NAMESPACE

/**
 * @brief Contains constants related to the WebSocket standard.
 */
namespace QWebSocketProtocol
{
/*!
        Parses the \a versionString and converts it to a Version value
        \internal
    */
Version versionFromString(const QString &versionString)
{
    bool ok = false;
    Version version = V_Unknow;
    int ver = versionString.toInt(&ok);
    QSet<Version> supportedVersions;
    supportedVersions << V_0 << V_4 << V_5 << V_6 << V_7 << V_8 << V_13;
    if (ok)
    {
        if (supportedVersions.contains(static_cast<Version>(ver)))
        {
            version = static_cast<Version>(ver);
        }
    }
    return version;
}

/*!
      Mask the \a payload with the given \a maskingKey and stores the result back in \a payload.
      \internal
    */
void mask(QByteArray *payload, quint32 maskingKey)
{
    quint32 *payloadData = reinterpret_cast<quint32 *>(payload->data());
    quint32 numIterations = static_cast<quint32>(payload->size()) / sizeof(quint32);
    quint32 remainder = static_cast<quint32>(payload->size()) % sizeof(quint32);
    quint32 i;
    for (i = 0; i < numIterations; ++i)
    {
        *(payloadData + i) ^= maskingKey;
    }
    if (remainder)
    {
        const quint32 offset = i * static_cast<quint32>(sizeof(quint32));
        char *payloadBytes = payload->data();
        uchar *mask = reinterpret_cast<uchar *>(&maskingKey);
        for (quint32 i = 0; i < remainder; ++i)
        {
            *(payloadBytes + offset + i) ^= static_cast<char>(mask[(i + offset) % 4]);
        }
    }
}

/*!
      Masks the \a payload of length \a size with the given \a maskingKey and stores the result back in \a payload.
      \internal
    */
void mask(char *payload, quint64 size, quint32 maskingKey)
{
    quint32 *payloadData = reinterpret_cast<quint32 *>(payload);
    quint32 numIterations = static_cast<quint32>(size / sizeof(quint32));
    quint32 remainder = size % sizeof(quint32);
    quint32 i;
    for (i = 0; i < numIterations; ++i)
    {
        *(payloadData + i) ^= maskingKey;
    }
    if (remainder)
    {
        const quint32 offset = i * static_cast<quint32>(sizeof(quint32));
        uchar *mask = reinterpret_cast<uchar *>(&maskingKey);
        for (quint32 i = 0; i < remainder; ++i)
        {
            *(payload + offset + i) ^= static_cast<char>(mask[(i + offset) % 4]);
        }
    }
}
}	//end namespace WebSocketProtocol

QT_END_NAMESPACE
