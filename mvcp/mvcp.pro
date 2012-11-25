TEMPLATE = lib

CONFIG += staticlib
CONFIG += create_prl

DEFINES += MVCP_EMBEDDED

HEADERS += \
    mvcp.h \
    mvcp_util.h \
    mvcp_tokeniser.h \
    mvcp_status.h \
    mvcp_socket.h \
    mvcp_response.h \
    mvcp_remote.h \
    mvcp_parser.h \
    mvcp_notifier.h

SOURCES += \
    mvcp.c \
    mvcp_util.c \
    mvcp_tokeniser.c \
    mvcp_status.c \
    mvcp_response.c \
    mvcp_remote.c \
    mvcp_parser.c \
    mvcp_notifier.c

