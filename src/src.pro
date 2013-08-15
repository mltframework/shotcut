CONFIG   += link_prl

QT       += widgets opengl xml network

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
    widgets/isingwidget.cpp \
    widgets/video4linuxwidget.cpp \
    widgets/colorproducerwidget.cpp \
    widgets/decklinkproducerwidget.cpp \
    widgets/networkproducerwidget.cpp \
    widgets/colorbarswidget.cpp \
    widgets/noisewidget.cpp \
    widgets/pulseaudiowidget.cpp \
    widgets/jackproducerwidget.cpp \
    widgets/alsawidget.cpp \
    widgets/x11grabwidget.cpp \
    player.cpp \
    glwidget.cpp \
    widgets/servicepresetwidget.cpp \
    abstractproducerwidget.cpp \
    widgets/avformatproducerwidget.cpp \
    widgets/imageproducerwidget.cpp \
    widgets/timespinbox.cpp \
    widgets/audiosignal.cpp \
    docks/recentdock.cpp \
    docks/encodedock.cpp \
    dialogs/addencodepresetdialog.cpp \
    jobqueue.cpp \
    docks/jobsdock.cpp \
    dialogs/textviewerdialog.cpp \
    models/playlistmodel.cpp \
    docks/playlistdock.cpp \
    dialogs/durationdialog.cpp \
    mvcp/qconsole.cpp \
    mvcp/mvcp_socket.cpp \
    mvcp/meltedclipsmodel.cpp \
    mvcp/meltedunitsmodel.cpp \
    mvcp/mvcpthread.cpp \
    mvcp/meltedplaylistmodel.cpp \
    mvcp/meltedplaylistdock.cpp \
    mvcp/meltedserverdock.cpp \
    widgets/colorwheel.cpp \
    models/attachedfiltersmodel.cpp \
    docks/filtersdock.cpp \
    filters/movitblurfilter.cpp \
    filters/movitglowfilter.cpp \
    filters/movitcolorfilter.cpp \
    filters/frei0rcoloradjwidget.cpp \
    filters/boxblurfilter.cpp \
    filters/frei0rglowfilter.cpp \
    filters/cropfilter.cpp \
    filters/saturationfilter.cpp \
    filters/movitsharpenfilter.cpp \
    filters/frei0rsharpnessfilter.cpp \
    widgets/colorpickerwidget.cpp \
    filters/whitebalancefilter.cpp \
    dialogs/customprofiledialog.cpp

HEADERS  += mainwindow.h \
    mltcontroller.h \
    sdlwidget.h \
    scrubbar.h \
    openotherdialog.h \
    widgets/plasmawidget.h \
    abstractproducerwidget.h \
    widgets/lissajouswidget.h \
    widgets/isingwidget.h \
    widgets/video4linuxwidget.h \
    widgets/colorproducerwidget.h \
    widgets/decklinkproducerwidget.h \
    widgets/networkproducerwidget.h \
    widgets/colorbarswidget.h \
    widgets/noisewidget.h \
    widgets/pulseaudiowidget.h \
    widgets/jackproducerwidget.h \
    widgets/alsawidget.h \
    widgets/x11grabwidget.h \
    player.h \
    glwidget.h \
    widgets/servicepresetwidget.h \
    widgets/avformatproducerwidget.h \
    widgets/imageproducerwidget.h \
    widgets/timespinbox.h \
    widgets/audiosignal.h \
    docks/recentdock.h \
    docks/encodedock.h \
    dialogs/addencodepresetdialog.h \
    jobqueue.h \
    docks/jobsdock.h \
    dialogs/textviewerdialog.h \
    models/playlistmodel.h \
    docks/playlistdock.h \
    dialogs/durationdialog.h \
    mvcp/qconsole.h \
    mvcp/meltedclipsmodel.h \
    mvcp/meltedunitsmodel.h \
    mvcp/mvcpthread.h \
    mvcp/meltedplaylistmodel.h \
    mvcp/meltedplaylistdock.h \
    mvcp/meltedserverdock.h \
    transportcontrol.h \
    widgets/colorwheel.h \
    models/attachedfiltersmodel.h \
    docks/filtersdock.h \
    filters/movitblurfilter.h \
    filters/movitglowfilter.h \
    filters/movitcolorfilter.h \
    filters/frei0rcoloradjwidget.h \
    filters/boxblurfilter.h \
    filters/frei0rglowfilter.h \
    filters/cropfilter.h \
    filters/saturationfilter.h \
    filters/movitsharpenfilter.h \
    filters/frei0rsharpnessfilter.h \
    widgets/colorpickerwidget.h \
    filters/whitebalancefilter.h \
    dialogs/customprofiledialog.h

FORMS    += mainwindow.ui \
    openotherdialog.ui \
    widgets/plasmawidget.ui \
    widgets/lissajouswidget.ui \
    widgets/isingwidget.ui \
    widgets/video4linuxwidget.ui \
    widgets/colorproducerwidget.ui \
    widgets/decklinkproducerwidget.ui \
    widgets/networkproducerwidget.ui \
    widgets/colorbarswidget.ui \
    widgets/noisewidget.ui \
    widgets/pulseaudiowidget.ui \
    widgets/jackproducerwidget.ui \
    widgets/alsawidget.ui \
    widgets/x11grabwidget.ui \
    widgets/servicepresetwidget.ui \
    widgets/avformatproducerwidget.ui \
    widgets/imageproducerwidget.ui \
    docks/recentdock.ui \
    docks/encodedock.ui \
    dialogs/addencodepresetdialog.ui \
    docks/jobsdock.ui \
    dialogs/textviewerdialog.ui \
    docks/playlistdock.ui \
    dialogs/durationdialog.ui \
    mvcp/meltedserverdock.ui \
    mvcp/meltedplaylistdock.ui \
    docks/filtersdock.ui \
    filters/movitblurfilter.ui \
    filters/movitglowfilter.ui \
    filters/movitcolorfilter.ui \
    filters/frei0rcoloradjwidget.ui \
    filters/boxblurfilter.ui \
    filters/frei0rglowfilter.ui \
    filters/cropfilter.ui \
    filters/saturationfilter.ui \
    filters/movitsharpenfilter.ui \
    filters/frei0rsharpnessfilter.ui \
    filters/whitebalancefilter.ui \
    dialogs/customprofiledialog.ui

RESOURCES += \
    resources.qrc

OTHER_FILES += \
    ../COPYING \
    shotcut.rc \
    ../scripts/build-shotcut.sh \
    ../icons/shotcut.icns \
    ../scripts/shotcut.nsi \
    ../Info.plist

TRANSLATIONS += \
    ../translations/shotcut_cs.ts \
    ../translations/shotcut_de.ts \
    ../translations/shotcut_en.ts \
    ../translations/shotcut_es.ts \
    ../translations/shotcut_fr.ts \
    ../translations/shotcut_pt.ts \
    ../translations/shotcut_zh.ts

INCLUDEPATH = ../mvcp

LIBS += -L../mvcp -lmvcp -lpthread

mac {
    TARGET = Shotcut
    ICON = ../icons/shotcut.icns
    QMAKE_INFO_PLIST = ../Info.plist

    # QMake from Qt 5.1.0 on OSX is messing with the environment in which it runs
    # pkg-config such that the PKG_CONFIG_PATH env var is not set.
    INCLUDEPATH += /opt/local/include/mlt++
    INCLUDEPATH += /opt/local/include/mlt
    LIBS += -L/opt/local/lib -lmlt++ -lmlt
}
win32 {
    CONFIG += windows rtti
    INCLUDEPATH += include/mlt++ include/mlt
    LIBS += -Llib -lmlt++ -lmlt -lglew32 -lopengl32
    RC_FILE = shotcut.rc
}
unix:!mac {
    QT += x11extras
    CONFIG += link_pkgconfig
    PKGCONFIG += mlt++ glew
    LIBS += -lX11
}

isEmpty(SHOTCUT_VERSION) {
    SHOTCUT_VERSION = $$system(date "+%y.%m.%d")
}
DEFINES += SHOTCUT_VERSION=\\\"$$SHOTCUT_VERSION\\\"
