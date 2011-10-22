QT       += core gui

TARGET = shotcut
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    mltcontroller.cpp

HEADERS  += mainwindow.h \
    mltcontroller.h

FORMS    += mainwindow.ui

mac {
    QT += opengl
    SOURCES += glwidget.cpp
    HEADERS += glwidget.h
}
win32 {
    INCLUDEPATH += include/mlt++ include/mlt
    LIBS += -Llib -lmlt++ -lmlt
} else {
    CONFIG += link_pkgconfig
    PKGCONFIG += mlt++
}

RESOURCES += \
    resources.qrc
