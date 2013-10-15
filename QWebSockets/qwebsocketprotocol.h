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

#ifndef QWEBSOCKETPROTOCOL_H
#define QWEBSOCKETPROTOCOL_H

#include <qglobal.h>

QT_BEGIN_NAMESPACE

class QString;
class QByteArray;

namespace QWebSocketProtocol
{
enum Version
{
    V_Unknow = -1,
    V_0 = 0,
    //hybi-01, hybi-02 and hybi-03 not supported
    V_4 = 4,
    V_5 = 5,
    V_6 = 6,
    V_7 = 7,
    V_8 = 8,
    V_13 = 13,
    V_LATEST = V_13
};

Version versionFromString(const QString &versionString);

enum CloseCode
{
    CC_NORMAL					= 1000,
    CC_GOING_AWAY				= 1001,
    CC_PROTOCOL_ERROR			= 1002,
    CC_DATATYPE_NOT_SUPPORTED	= 1003,
    CC_RESERVED_1004			= 1004,
    CC_MISSING_STATUS_CODE		= 1005,
    CC_ABNORMAL_DISCONNECTION	= 1006,
    CC_WRONG_DATATYPE			= 1007,
    CC_POLICY_VIOLATED			= 1008,
    CC_TOO_MUCH_DATA			= 1009,
    CC_MISSING_EXTENSION		= 1010,
    CC_BAD_OPERATION			= 1011,
    CC_TLS_HANDSHAKE_FAILED		= 1015
};

enum OpCode
{
    OC_CONTINUE		= 0x0,
    OC_TEXT			= 0x1,
    OC_BINARY		= 0x2,
    OC_RESERVED_3	= 0x3,
    OC_RESERVED_4	= 0x4,
    OC_RESERVED_5	= 0x5,
    OC_RESERVED_6	= 0x6,
    OC_RESERVED_7	= 0x7,
    OC_CLOSE		= 0x8,
    OC_PING			= 0x9,
    OC_PONG			= 0xA,
    OC_RESERVED_B	= 0xB,
    OC_RESERVED_C	= 0xC,
    OC_RESERVED_D	= 0xD,
    OC_RESERVED_E	= 0xE,
    OC_RESERVED_F	= 0xF
};


inline bool isOpCodeReserved(OpCode code)
{
    return ((code > OC_BINARY) && (code < OC_CLOSE)) || (code > OC_PONG);
}
inline bool isCloseCodeValid(int closeCode)
{
    return  (closeCode > 999) && (closeCode < 5000) &&
            (closeCode != CC_RESERVED_1004) &&          //see RFC6455 7.4.1
            (closeCode != CC_MISSING_STATUS_CODE) &&
            (closeCode != CC_ABNORMAL_DISCONNECTION) &&
            ((closeCode >= 3000) || (closeCode < 1012));
}

void mask(QByteArray *payload, quint32 maskingKey);
void mask(char *payload, quint64 size, quint32 maskingKey);

inline Version currentVersion() { return V_LATEST; }

}	//end namespace QWebSocketProtocol

QT_END_NAMESPACE

#endif // QWEBSOCKETPROTOCOL_H
