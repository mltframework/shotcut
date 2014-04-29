CONFIG   += link_prl

QT       += widgets opengl xml network qml quick webkitwidgets sql

TARGET = shotcut
TEMPLATE = app

win32:DEFINES += QT_STATIC
include (../QWebSockets/qwebsockets.pri)

SOURCES += main.cpp\
    mainwindow.cpp \
    mltcontroller.cpp \
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
    widgets/colorpickerwidget.cpp \
    filters/whitebalancefilter.cpp \
    dialogs/customprofiledialog.cpp \
    qmltypes/colorwheelitem.cpp \
    qmltypes/qmlfilter.cpp \
    qmltypes/qmlmetadata.cpp \
    qmltypes/qmlprofile.cpp \
    filters/webvfxfilter.cpp \
    htmleditor/htmleditor.cpp \
    htmleditor/highlighter.cpp \
    settings.cpp \
    widgets/lineeditclear.cpp \
    leapnetworklistener.cpp \
    widgets/webvfxproducer.cpp \
    database.cpp \
    widgets/gltestwidget.cpp \
    models/multitrackmodel.cpp \
    docks/timelinedock.cpp \
    qmltypes/qmlutilities.cpp \
    qmltypes/thumbnailprovider.cpp \
    commands/timelinecommands.cpp \
    util.cpp \
    widgets/lumamixtransition.cpp

HEADERS  += mainwindow.h \
    mltcontroller.h \
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
    widgets/colorpickerwidget.h \
    filters/whitebalancefilter.h \
    dialogs/customprofiledialog.h \
    qmltypes/colorwheelitem.h \
    qmltypes/qmlfilter.h \
    qmltypes/qmlmetadata.h \
    qmltypes/qmlprofile.h \
    filters/webvfxfilter.h \
    htmleditor/htmleditor.h \
    htmleditor/highlighter.h \
    settings.h \
    widgets/lineeditclear.h \
    leapnetworklistener.h \
    widgets/webvfxproducer.h \
    database.h \
    widgets/gltestwidget.h \
    models/multitrackmodel.h \
    docks/timelinedock.h \
    qmltypes/qmlutilities.h \
    qmltypes/thumbnailprovider.h \
    commands/timelinecommands.h \
    util.h \
    widgets/lumamixtransition.h

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
    filters/whitebalancefilter.ui \
    dialogs/customprofiledialog.ui \
    filters/webvfxfilter.ui \
    htmleditor/htmleditor.ui \
    htmleditor/inserthtmldialog.ui \
    widgets/webvfxproducer.ui \
    docks/timelinedock.ui \
    widgets/lumamixtransition.ui

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
    qml/filters/audio_loudness/meta.qml \
    qml/filters/audio_loudness/ui.qml \
    qml/filters/audio_pan/meta.qml \
    qml/filters/audio_pan/ui.qml \
    qml/filters/audio_balance/meta.qml \
    qml/filters/audio_balance/ui.qml \
    qml/filters/blur/meta_boxblur.qml \
    qml/filters/blur/ui_boxblur.qml \
    qml/filters/blur/meta_movit.qml \
    qml/filters/blur/ui_movit.qml \
    qml/filters/color/meta.qml \
    qml/filters/color/ui.qml \
    qml/filters/crop/meta.qml \
    qml/filters/crop/ui.qml \
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
    qml/filters/sharpen/meta_frei0r.qml \
    qml/filters/sharpen/ui_frei0r.qml \
    qml/filters/sharpen/meta_movit.qml \
    qml/filters/sharpen/ui_movit.qml \
    qml/modules/Shotcut/Controls/UndoButton.qml \
    qml/timeline/timeline.qml \
    qml/timeline/TrackHead.qml \
    qml/timeline/Track.qml \
    qml/timeline/Clip.qml \
    qml/timeline/Ruler.qml \
    qml/timeline/Track.js \
    qml/filters/movit_diffusion/meta.qml \
    qml/filters/movit_diffusion/ui.qml \
    qml/filters/vignette/meta_movit.qml \
    qml/filters/vignette/ui_movit.qml \
    qml/filters/vignette/meta_oldfilm.qml \
    qml/filters/vignette/ui_oldfilm.qml \
    qml/timeline/TimelineToolbar.qml \
    qml/timeline/ZoomSlider.qml \
    qml/timeline/Timeline.js \
    qml/modules/Shotcut/Controls/TimeSpinner.qml \
    qml/filters/fadein_brightness/meta.qml \
    qml/filters/fadein_brightness/ui.qml \
    qml/filters/audio_fadein/meta.qml \
    qml/filters/audio_fadein/ui.qml \
    qml/filters/fadein_movit/meta.qml \
    qml/filters/fadein_movit/ui.qml \
    qml/filters/fadeout_brightness/meta.qml \
    qml/filters/fadeout_brightness/ui.qml \
    qml/filters/audio_fadeout/meta.qml \
    qml/filters/audio_fadeout/ui.qml \
    qml/filters/fadeout_movit/meta.qml \
    qml/filters/fadeout_movit/ui.qml \
    qml/filters/glow/meta_frei0r.qml \
    qml/filters/glow/ui_frei0r.qml \
    qml/filters/glow/meta_movit.qml \
    qml/filters/glow/ui_movit.qml \
    qml/timeline/ToolTip.qml \
    qml/timeline/ToggleButton.qml \
    qml/filters/crop/meta_movit.qml \
    qml/filters/color/meta_movit.qml \
    qml/filters/color/meta_frei0r_coloradj.qml \
    qml/filters/color/ui_frei0r_coloradj.qml \
    qml/filters/wave/meta.qml \
    qml/filters/wave/ui.qml

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
    qml/filters/audio_loudness/meta.qml \
    qml/filters/audio_loudness/ui.qml \
    qml/filters/audio_pan/meta.qml \
    qml/filters/audio_pan/ui.qml \
    qml/filters/audio_balance/meta.qml \
    qml/filters/audio_balance/ui.qml \
    qml/filters/blur/meta_boxblur.qml \
    qml/filters/blur/ui_boxblur.qml \
    qml/filters/blur/meta_movit.qml \
    qml/filters/blur/ui_movit.qml \
    qml/filters/color/meta.qml \
    qml/filters/color/ui.qml \
    qml/filters/crop/meta.qml \
    qml/filters/crop/ui.qml \
    qml/filters/glow/meta_frei0r.qml \
    qml/filters/glow/ui_frei0r.qml \
    qml/filters/glow/meta_movit.qml \
    qml/filters/glow/ui_movit.qml \
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
    qml/filters/sharpen/meta_frei0r.qml \
    qml/filters/sharpen/ui_frei0r.qml \
    qml/filters/sharpen/meta_movit.qml \
    qml/filters/sharpen/ui_movit.qml \
    qml/modules/Shotcut/Controls/UndoButton.qml \
    qml/timeline/timeline.qml \
    qml/timeline/TimelineToolbar.qml \
    qml/timeline/TrackHead.qml \
    qml/timeline/Track.qml \
    qml/timeline/Clip.qml \
    qml/timeline/Ruler.qml \
    qml/filters/movit_diffusion/meta.qml \
    qml/filters/movit_diffusion/ui.qml \
    qml/filters/vignette/meta_movit.qml \
    qml/filters/vignette/ui_movit.qml \
    qml/filters/vignette/meta_oldfilm.qml \
    qml/filters/vignette/ui_oldfilm.qml \
    qml/modules/Shotcut/Controls/TimeSpinner.qml \
    qml/filters/fadein_brightness/meta.qml \
    qml/filters/fadein_brightness/ui.qml \
    qml/filters/audio_fadein/meta.qml \
    qml/filters/audio_fadein/ui.qml \
    qml/filters/fadein_movit/meta.qml \
    qml/filters/fadein_movit/ui.qml \
    qml/filters/fadeout_brightness/meta.qml \
    qml/filters/fadeout_brightness/ui.qml \
    qml/filters/audio_fadeout/meta.qml \
    qml/filters/audio_fadeout/ui.qml \
    qml/filters/fadeout_movit/meta.qml \
    qml/filters/fadeout_movit/ui.qml \
    qml/filters/crop/meta_movit.qml \
    qml/filters/color/meta_movit.qml \
    qml/filters/color/meta_frei0r_coloradj.qml \
    qml/filters/color/ui_frei0r_coloradj.qml \
    qml/filters/wave/meta.qml \
    qml/filters/wave/ui.qml
}

TRANSLATIONS += \
    ../translations/shotcut_ca.ts \
    ../translations/shotcut_cs.ts \
    ../translations/shotcut_da.ts \
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
    CONFIG -= debug_and_release
    isEmpty(MLT_PATH) {
        message("MLT_PATH not set; using ..\\..\\... You can change this with 'qmake MLT_PATH=...'")
        MLT_PATH = ..\\..\\..
    }
    INCLUDEPATH += $$MLT_PATH\\include\\mlt++ $$MLT_PATH\\include\\mlt
    LIBS += -L$$MLT_PATH\\lib -lmlt++ -lmlt -lopengl32
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
     win32:SHOTCUT_VERSION = adhoc
}
DEFINES += SHOTCUT_VERSION=\\\"$$SHOTCUT_VERSION\\\"

unix:!mac:isEmpty(PREFIX) {
    message("Install PREFIX not set; using /usr/local. You can change this with 'qmake PREFIX=...'")
    PREFIX = /usr/local
}
win32:isEmpty(PREFIX) {
    message("Install PREFIX not set; using C:\\Projects\\Shotcut. You can change this with 'qmake PREFIX=...'")
    PREFIX = C:\\Projects\\Shotcut
}
unix:target.path = $$PREFIX/bin
win32:target.path = $$PREFIX
INSTALLS += target
