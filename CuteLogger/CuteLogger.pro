QT       -= gui

TARGET = CuteLogger
TEMPLATE = lib

CONFIG += staticlib
CONFIG += create_prl

DEFINES += CUTELOGGER_LIBRARY

INCLUDEPATH += ./include

SOURCES += src/Logger.cpp \
           src/AbstractAppender.cpp \
           src/AbstractStringAppender.cpp \
           src/ConsoleAppender.cpp \
           src/FileAppender.cpp \
           src/RollingFileAppender.cpp

HEADERS += include/Logger.h \
           include/CuteLogger_global.h \
           include/AbstractAppender.h \
           include/AbstractStringAppender.h \
           include/ConsoleAppender.h \
           include/FileAppender.h \
           include/RollingFileAppender.h

win32 {
    SOURCES += src/OutputDebugAppender.cpp
    HEADERS += include/OutputDebugAppender.h
}

android {
    SOURCES += src/AndroidAppender.cpp
    HEADERS += include/AndroidAppender.h
}
