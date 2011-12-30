QT       += core gui opengl

TARGET = shotcut
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    mltcontroller.cpp \
    sdlwidget.cpp \
    scrubbar.cpp

HEADERS  += mainwindow.h \
    mltcontroller.h \
    sdlwidget.h \
    scrubbar.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc

OTHER_FILES += \
    COPYING \
    deploy-linux.sh \
    deploy-osx.sh \
    deploy-win32.sh \
    mlt.icns \
    shotcut.rc

mac {
    TARGET = Shotcut
    ICON = mlt.icns
}
win32 {
    INCLUDEPATH += include/mlt++ include/mlt
    LIBS += -Llib -lmlt++ -lmlt
	RC_FILE = shotcut.rc
} else {
    CONFIG += link_pkgconfig
    SOURCES += glwidget.cpp
    HEADERS += glwidget.h
    PKGCONFIG += mlt++
}
