QT       *= network

SOURCES += \
    $$PWD/qwebsocket.cpp \
    $$PWD/qwebsocket_p.cpp \
    $$PWD/qwebsocketserver.cpp \
    $$PWD/qwebsocketserver_p.cpp \
    $$PWD/qwebsocketprotocol.cpp \
    $$PWD/handshakerequest_p.cpp \
    $$PWD/handshakeresponse_p.cpp \
    $$PWD/dataprocessor_p.cpp \
    $$PWD/qcorsauthenticator.cpp

HEADERS += \
    $$PWD/qwebsocket.h \
    $$PWD/qwebsocket_p.h \
    $$PWD/qwebsocketserver.h \
    $$PWD/qwebsocketserver_p.h \
    $$PWD/qwebsocketprotocol.h \
    $$PWD/handshakerequest_p.h \
    $$PWD/handshakeresponse_p.h \
    $$PWD/dataprocessor_p.h \
    $$PWD/qwebsocketsglobal.h \
    $$PWD/qcorsauthenticator.h \
    qcorsauthenticator_p.h

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
