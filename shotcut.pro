QT       += core gui

TARGET = shotcut
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    mltcontroller.cpp

HEADERS  += mainwindow.h \
    mltcontroller.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc

OTHER_FILES += \
    COPYING

mac {
    TARGET = Shotcut
    QT += opengl
    SOURCES += glwidget.cpp
    HEADERS += glwidget.h
    ICON = mlt.icns
    OTHER_FILES += deploy-osx.sh \
        mlt.icns
}
win32 {
    INCLUDEPATH += include/mlt++ include/mlt
    LIBS += -Llib -lmlt++ -lmlt
	RC_FILE = shotcut.rc
	OTHER_FILES += shotcut.rc
} else {
    CONFIG += link_pkgconfig
    PKGCONFIG += mlt++
}
