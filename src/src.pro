CONFIG   += link_prl

QT       += widgets opengl xml network qml quick webkitwidgets sql

TARGET = shotcut
TEMPLATE = app

include (../QWebSockets/qwebsockets.pri)

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
    filters/movitsharpenfilter.cpp \
    filters/frei0rsharpnessfilter.cpp \
    widgets/colorpickerwidget.cpp \
    filters/whitebalancefilter.cpp \
    dialogs/customprofiledialog.cpp \
    qmltypes/qmlfilter.cpp \
    qmltypes/qmlmetadata.cpp \
    filters/webvfxfilter.cpp \
    htmleditor/htmleditor.cpp \
    htmleditor/highlighter.cpp \
    settings.cpp \
    widgets/lineeditclear.cpp \
    leapnetworklistener.cpp \
    widgets/webvfxproducer.cpp \
    filters/normalize.cpp \
    database.cpp \
    widgets/gltestwidget.cpp \
    models/multitrackmodel.cpp \
    docks/timelinedock.cpp \
    qmltypes/qmlutilities.cpp

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
    filters/movitsharpenfilter.h \
    filters/frei0rsharpnessfilter.h \
    widgets/colorpickerwidget.h \
    filters/whitebalancefilter.h \
    dialogs/customprofiledialog.h \
    qmltypes/qmlfilter.h \
    qmltypes/qmlmetadata.h \
    filters/webvfxfilter.h \
    htmleditor/htmleditor.h \
    htmleditor/highlighter.h \
    settings.h \
    widgets/lineeditclear.h \
    leapnetworklistener.h \
    widgets/webvfxproducer.h \
    filters/normalize.h \
    database.h \
    widgets/gltestwidget.h \
    models/multitrackmodel.h \
    docks/timelinedock.h \
    qmltypes/qmlutilities.h

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
    filters/movitsharpenfilter.ui \
    filters/frei0rsharpnessfilter.ui \
    filters/whitebalancefilter.ui \
    dialogs/customprofiledialog.ui \
    filters/webvfxfilter.ui \
    htmleditor/htmleditor.ui \
    htmleditor/inserthtmldialog.ui \
    widgets/webvfxproducer.ui \
    filters/normalize.ui \
    docks/timelinedock.ui

RESOURCES += \
    ../icons/resources.qrc \
    ../other-resources.qrc

OTHER_FILES += \
    ../COPYING \
    shotcut.rc \
    ../scripts/build-shotcut.sh \
    ../icons/shotcut.icns \
    ../scripts/shotcut.nsi \
    ../Info.plist \
    ../icons/dark/index.theme \
    ../icons/light/index.theme \
    qml/filters/saturation/meta_frei0r.qml \
    qml/filters/saturation/ui_frei0r.qml \
    qml/filters/saturation/meta_movit.qml \
    qml/filters/saturation/ui_movit.qml \
    qml/filters/webvfx_circular_frame/filter-demo.html \
    qml/filters/webvfx_circular_frame/ui.qml \
    qml/filters/webvfx_circular_frame/meta.qml \
    qml/modules/Shotcut/Controls/qmldir \
    qml/modules/Shotcut/Controls/Preset.qml \
    qml/htmleditor/text_outline.qml \
    qml/htmleditor/text_shadow.qml \
    qml/filters/audio_channelcopy/meta.qml \
    qml/filters/audio_channelcopy/ui.qml \
    qml/filters/audio_gain/meta.qml \
    qml/filters/audio_gain/ui.qml \
    qml/filters/audio_pan/meta.qml \
    qml/filters/audio_pan/ui.qml \
    qml/filters/audio_balance/meta.qml \
    qml/filters/audio_balance/ui.qml \
    qml/filters/rotate/ui.qml \
    qml/filters/rotate/meta.qml \
    qml/filters/stabilize/meta.qml \
    qml/filters/stabilize/ui.qml \
    qml/filters/audio_mono/meta.qml \
    qml/filters/audio_swapchannels/meta.qml \
    qml/filters/audio_swapchannels/ui.qml \
    qml/filters/invert/meta.qml \
    qml/filters/sepia/meta.qml \
    qml/filters/sepia/ui.qml \
    qml/modules/Shotcut/Controls/UndoButton.qml \
    qml/timeline/timeline.qml \
    qml/timeline/TrackHead.qml \
    qml/timeline/Track.qml \
    qml/timeline/Clip.qml \
    qml/timeline/Ruler.qml

lupdate_hack {
    SOURCES += \
    qml/filters/saturation/meta_frei0r.qml \
    qml/filters/saturation/ui_frei0r.qml \
    qml/filters/saturation/meta_movit.qml \
    qml/filters/saturation/ui_movit.qml \
    qml/filters/webvfx_circular_frame/filter-demo.html \
    qml/filters/webvfx_circular_frame/ui.qml \
    qml/filters/webvfx_circular_frame/meta.qml \
    qml/modules/Shotcut/Controls/qmldir \
    qml/modules/Shotcut/Controls/Preset.qml \
    qml/htmleditor/text_outline.qml \
    qml/htmleditor/text_shadow.qml \
    qml/filters/audio_channelcopy/meta.qml \
    qml/filters/audio_channelcopy/ui.qml \
    qml/filters/audio_gain/meta.qml \
    qml/filters/audio_gain/ui.qml \
    qml/filters/audio_pan/meta.qml \
    qml/filters/audio_pan/ui.qml \
    qml/filters/audio_balance/meta.qml \
    qml/filters/audio_balance/ui.qml \
    qml/filters/rotate/ui.qml \
    qml/filters/rotate/meta.qml \
    qml/filters/stabilize/meta.qml \
    qml/filters/stabilize/ui.qml \
    qml/filters/audio_mono/meta.qml \
    qml/filters/audio_swapchannels/meta.qml \
    qml/filters/audio_swapchannels/ui.qml \
    qml/filters/invert/meta.qml \
    qml/filters/sepia/meta.qml \
    qml/filters/sepia/ui.qml \
    qml/modules/Shotcut/Controls/UndoButton.qml \
    qml/timeline/timeline.qml \
    qml/timeline/TrackHead.qml \
    qml/timeline/Track.qml \
    qml/timeline/Clip.qml \
    qml/timeline/Ruler.qml
}

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
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6

    # QMake from Qt 5.1.0 on OSX is messing with the environment in which it runs
    # pkg-config such that the PKG_CONFIG_PATH env var is not set.
    isEmpty(MLT_PREFIX) {
        MLT_PREFIX = /opt/local
    }
    INCLUDEPATH += $$MLT_PREFIX/include/mlt++
    INCLUDEPATH += $$MLT_PREFIX/include/mlt
    LIBS += -L$$MLT_PREFIX/lib -lmlt++ -lmlt
}
win32 {
    CONFIG += windows rtti
    INCLUDEPATH += include/mlt++ include/mlt
    LIBS += -Llib -lmlt++ -lmlt -lopengl32
    RC_FILE = shotcut.rc
}
unix:!mac {
    QT += x11extras
    CONFIG += link_pkgconfig
    PKGCONFIG += mlt++
    LIBS += -lX11
}

# Add CONFIG+=leap to include support for Leap Motion via its library.
CONFIG(leap) {
    DEFINES += WITH_LIBLEAP
    SOURCES += leaplistener.cpp
    HEADERS += leaplistener.h
    unix {
        isEmpty(LEAP_PREFIX) {
            LEAP_PREFIX = /usr/local
        }
        INCLUDEPATH += $$LEAP_PREFIX/include
        LIBS += -L$$LEAP_PREFIX/lib
    }
    LIBS += -lLeap
}

isEmpty(SHOTCUT_VERSION) {
    !win32:SHOTCUT_VERSION = $$system(date "+%y.%m.%d")
}
DEFINES += SHOTCUT_VERSION=\\\"$$SHOTCUT_VERSION\\\"

unix:!mac:isEmpty(PREFIX) {
    message("Install PREFIX not set; using /usr/local. You can change this with 'qmake PREFIX=...'")
    PREFIX = /usr/local
}
unix:target.path = $$PREFIX/bin
win32:target.path = $$PREFIX
INSTALLS += target
