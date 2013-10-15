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

#ifndef DATAPROCESSOR_P_H
#define DATAPROCESSOR_P_H

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
#include <QByteArray>
#include <QString>
#include <QTextCodec>
#include "qwebsocketprotocol.h"

QT_BEGIN_NAMESPACE

class QIODevice;
class Frame;

/*!
    \internal
    The DataProcessor class reads and interprets incoming websocket messages, and emits appropriate signals.
 */
class DataProcessor: public QObject
{
    Q_OBJECT
public:
    explicit DataProcessor(QObject *parent = 0);
    virtual ~DataProcessor();

    static quint64 maxMessageSize();
    static quint64 maxFrameSize();

Q_SIGNALS:
    void pingReceived(QByteArray data);
    void pongReceived(QByteArray data);
    void closeReceived(QWebSocketProtocol::CloseCode closeCode, QString closeReason);
    void textFrameReceived(QString frame, bool lastFrame);
    void binaryFrameReceived(QByteArray frame, bool lastFrame);
    void textMessageReceived(QString message);
    void binaryMessageReceived(QByteArray message);
    void errorEncountered(QWebSocketProtocol::CloseCode code, QString description);

public Q_SLOTS:
    void process(QIODevice *pIoDevice);
    void clear();

private:
    Q_DISABLE_COPY(DataProcessor)
    enum
    {
        PS_READ_HEADER,
        PS_READ_PAYLOAD_LENGTH,
        PS_READ_BIG_PAYLOAD_LENGTH,
        PS_READ_MASK,
        PS_READ_PAYLOAD,
        PS_DISPATCH_RESULT
    } m_processingState;

    bool m_isFinalFrame;
    bool m_isFragmented;
    QWebSocketProtocol::OpCode m_opCode;
    bool m_isControlFrame;
    bool m_hasMask;
    quint32 m_mask;
    QByteArray m_binaryMessage;
    QString m_textMessage;
    quint64 m_payloadLength;
    QTextCodec::ConverterState *m_pConverterState;
    QTextCodec *m_pTextCodec;

    bool processControlFrame(const Frame &frame);
};

QT_END_NAMESPACE

#endif // DATAPROCESSOR_P_H
