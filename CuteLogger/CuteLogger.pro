QT       -= gui

TARGET = Logger
TEMPLATE = lib

CONFIG += staticlib
CONFIG += create_prl

DEFINES += CUTELOGGER_LIBRARY

INCLUDEPATH += ./include

SOURCES += src/Logger.cpp \
           src/AbstractAppender.cpp \
           src/AbstractStringAppender.cpp \
           src/ConsoleAppender.cpp \
           src/FileAppender.cpp

HEADERS += include/Logger.h \
           include/CuteLogger_global.h \
           include/AbstractAppender.h \
           include/AbstractStringAppender.h \
           include/ConsoleAppender.h \
           include/FileAppender.h

win32 {
    SOURCES += src/OutputDebugAppender.cpp
    HEADERS += include/OutputDebugAppender.h
}

symbian {
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xE8FB3D8D
    TARGET.CAPABILITY = 
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = CuteLogger.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
}

OTHER_FILES += \
    Doxyfile \
    LICENSE.LGPL \
    CMakeLists.txt
