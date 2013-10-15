QMAKE_DOCS = $$PWD/doc/qwebsockets.qdocconfig

include($$PWD/qwebsockets.pri)

TEMPLATE = lib
VERSION = 0.9
TARGET = QWebSockets

mac:QMAKE_FRAMEWORK_BUNDLE_NAME = $$TARGET
mac:QMAKE_CXXFLAGS += -Wall -Werror -Wextra
