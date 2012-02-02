QT       += core gui opengl

TARGET = shotcut
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    mltcontroller.cpp \
    sdlwidget.cpp \
    scrubbar.cpp \
    openotherdialog.cpp \
    widgets/plasmawidget.cpp \
    widgets/lissajouswidget.cpp \
    widgets/isingwidget.cpp

HEADERS  += mainwindow.h \
    mltcontroller.h \
    sdlwidget.h \
    scrubbar.h \
    openotherdialog.h \
    widgets/plasmawidget.h \
    abstractproducerwidget.h \
    widgets/lissajouswidget.h \
    widgets/isingwidget.h

FORMS    += mainwindow.ui \
    openotherdialog.ui \
    widgets/plasmawidget.ui \
    widgets/lissajouswidget.ui \
    widgets/isingwidget.ui

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

















