CONFIG   += link_prl

QT       += widgets opengl xml network printsupport qml quick sql webkitwidgets
QT       += multimedia websockets quickwidgets
QT       += qml-private core-private quick-private gui-private

TARGET = shotcut
TEMPLATE = app

win32:DEFINES += QT_STATIC

SOURCES += main.cpp\
    mainwindow.cpp \
    mltcontroller.cpp \
    scrubbar.cpp \
    openotherdialog.cpp \
    controllers/filtercontroller.cpp \
    widgets/plasmawidget.cpp \
    widgets/lissajouswidget.cpp \
    widgets/isingwidget.cpp \
    widgets/video4linuxwidget.cpp \
    widgets/colorproducerwidget.cpp \
    widgets/decklinkproducerwidget.cpp \
    widgets/networkproducerwidget.cpp \
    widgets/colorbarswidget.cpp \
    widgets/countproducerwidget.cpp \
    widgets/noisewidget.cpp \
    widgets/pulseaudiowidget.cpp \
    widgets/jackproducerwidget.cpp \
    widgets/toneproducerwidget.cpp \
    widgets/alsawidget.cpp \
    widgets/x11grabwidget.cpp \
    player.cpp \
    glwidget.cpp \
    widgets/servicepresetwidget.cpp \
    abstractproducerwidget.cpp \
    widgets/avformatproducerwidget.cpp \
    widgets/imageproducerwidget.cpp \
    widgets/timespinbox.cpp \
    widgets/audiometerwidget.cpp \
    docks/recentdock.cpp \
    docks/encodedock.cpp \
    dialogs/addencodepresetdialog.cpp \
    jobqueue.cpp \
    docks/jobsdock.cpp \
    dialogs/textviewerdialog.cpp \
    models/playlistmodel.cpp \
    docks/playlistdock.cpp \
    dialogs/durationdialog.cpp \
    widgets/colorwheel.cpp \
    models/attachedfiltersmodel.cpp \
    models/metadatamodel.cpp \
    docks/filtersdock.cpp \
    dialogs/customprofiledialog.cpp \
    qmltypes/colorpickeritem.cpp \
    qmltypes/colorwheelitem.cpp \
    qmltypes/qmlapplication.cpp \
    qmltypes/qmlfile.cpp \
    qmltypes/qmlfilter.cpp \
    qmltypes/qmlhtmleditor.cpp \
    qmltypes/qmlmetadata.cpp \
    qmltypes/timelineitems.cpp \
    qmltypes/qmlprofile.cpp \
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
    qmltypes/qmlview.cpp \
    qmltypes/thumbnailprovider.cpp \
    commands/timelinecommands.cpp \
    util.cpp \
    widgets/lumamixtransition.cpp \
    autosavefile.cpp \
    widgets/directshowvideowidget.cpp \
    jobs/abstractjob.cpp \
    jobs/meltjob.cpp \
    jobs/encodejob.cpp \
    jobs/postjobaction.cpp \
    jobs/videoqualityjob.cpp \
    commands/playlistcommands.cpp \
    docks/scopedock.cpp \
    controllers/scopecontroller.cpp \
    widgets/scopes/scopewidget.cpp \
    widgets/scopes/audioloudnessscopewidget.cpp \
    widgets/scopes/audiopeakmeterscopewidget.cpp \
    widgets/scopes/audiospectrumscopewidget.cpp \
    widgets/scopes/audiowaveformscopewidget.cpp \
    widgets/scopes/videohistogramscopewidget.cpp \
    widgets/scopes/videowaveformscopewidget.cpp \
    sharedframe.cpp \
    widgets/audioscale.cpp \
    widgets/playlisttable.cpp \
    widgets/playlisticonview.cpp \
    commands/undohelper.cpp \
    models/audiolevelstask.cpp \
    mltxmlchecker.cpp \
    widgets/avfoundationproducerwidget.cpp \
    widgets/gdigrabwidget.cpp \
    widgets/trackpropertieswidget.cpp \
    widgets/timelinepropertieswidget.cpp \
    jobs/ffprobejob.cpp \
    jobs/ffmpegjob.cpp \
    dialogs/unlinkedfilesdialog.cpp \
    dialogs/transcodedialog.cpp \
    docks/keyframesdock.cpp \
    qmltypes/qmlproducer.cpp \
    models/keyframesmodel.cpp \
    widgets/textproducerwidget.cpp \
    dialogs/listselectiondialog.cpp \
    widgets/newprojectfolder.cpp


HEADERS  += mainwindow.h \
    mltcontroller.h \
    scrubbar.h \
    openotherdialog.h \
    controllers/filtercontroller.h \
    widgets/plasmawidget.h \
    abstractproducerwidget.h \
    widgets/lissajouswidget.h \
    widgets/isingwidget.h \
    widgets/video4linuxwidget.h \
    widgets/colorproducerwidget.h \
    widgets/decklinkproducerwidget.h \
    widgets/networkproducerwidget.h \
    widgets/colorbarswidget.h \
    widgets/countproducerwidget.h \
    widgets/noisewidget.h \
    widgets/pulseaudiowidget.h \
    widgets/jackproducerwidget.h \
    widgets/toneproducerwidget.h \
    widgets/alsawidget.h \
    widgets/x11grabwidget.h \
    player.h \
    glwidget.h \
    widgets/servicepresetwidget.h \
    widgets/avformatproducerwidget.h \
    widgets/imageproducerwidget.h \
    widgets/timespinbox.h \
    widgets/iecscale.h \
    widgets/audiometerwidget.h \
    docks/recentdock.h \
    docks/encodedock.h \
    dialogs/addencodepresetdialog.h \
    jobqueue.h \
    docks/jobsdock.h \
    dialogs/textviewerdialog.h \
    models/playlistmodel.h \
    docks/playlistdock.h \
    dialogs/durationdialog.h \
    transportcontrol.h \
    widgets/colorwheel.h \
    models/attachedfiltersmodel.h \
    models/metadatamodel.h \
    docks/filtersdock.h \
    dialogs/customprofiledialog.h \
    qmltypes/colorpickeritem.h \
    qmltypes/colorwheelitem.h \
    qmltypes/qmlapplication.h \
    qmltypes/qmlfile.h \
    qmltypes/qmlfilter.h \
    qmltypes/qmlhtmleditor.h \
    qmltypes/qmlmetadata.h \
    qmltypes/timelineitems.h \
    qmltypes/qmlprofile.h \
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
    qmltypes/qmlview.h \
    qmltypes/thumbnailprovider.h \
    commands/timelinecommands.h \
    util.h \
    widgets/lumamixtransition.h \
    autosavefile.h \
    widgets/directshowvideowidget.h \
    jobs/abstractjob.h \
    jobs/meltjob.h \
    jobs/encodejob.h \
    jobs/postjobaction.h \
    jobs/videoqualityjob.h \
    commands/playlistcommands.h \
    docks/scopedock.h \
    controllers/scopecontroller.h \
    widgets/scopes/scopewidget.h \
    widgets/scopes/audioloudnessscopewidget.h \
    widgets/scopes/audiopeakmeterscopewidget.h \
    widgets/scopes/audiospectrumscopewidget.h \
    widgets/scopes/audiowaveformscopewidget.h \
    widgets/scopes/videohistogramscopewidget.h \
    widgets/scopes/videowaveformscopewidget.h \
    dataqueue.h \
    sharedframe.h \
    widgets/audioscale.h \
    widgets/playlisttable.h \
    widgets/playlisticonview.h \
    commands/undohelper.h \
    models/audiolevelstask.h \
    shotcut_mlt_properties.h \
    mltxmlchecker.h \
    widgets/avfoundationproducerwidget.h \
    widgets/gdigrabwidget.h \
    widgets/trackpropertieswidget.h \
    widgets/timelinepropertieswidget.h \
    jobs/ffprobejob.h \
    jobs/ffmpegjob.h \
    dialogs/unlinkedfilesdialog.h \
    dialogs/transcodedialog.h \
    docks/keyframesdock.h \
    qmltypes/qmlproducer.h \
    models/keyframesmodel.h \
    widgets/textproducerwidget.h \
    dialogs/listselectiondialog.h \
    widgets/newprojectfolder.h

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
    widgets/countproducerwidget.ui \
    widgets/noisewidget.ui \
    widgets/pulseaudiowidget.ui \
    widgets/jackproducerwidget.ui \
    widgets/toneproducerwidget.ui \
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
    dialogs/customprofiledialog.ui \
    htmleditor/htmleditor.ui \
    htmleditor/inserthtmldialog.ui \
    widgets/webvfxproducer.ui \
    docks/timelinedock.ui \
    widgets/lumamixtransition.ui \
    widgets/directshowvideowidget.ui \
    widgets/avfoundationproducerwidget.ui \
    widgets/gdigrabwidget.ui \
    widgets/trackpropertieswidget.ui \
    widgets/timelinepropertieswidget.ui \
    dialogs/unlinkedfilesdialog.ui \
    dialogs/transcodedialog.ui \
    widgets/textproducerwidget.ui \
    dialogs/listselectiondialog.ui \
    widgets/newprojectfolder.ui

RESOURCES += \
    ../icons/resources.qrc \
    ../other-resources.qrc

OTHER_FILES += \
    ../COPYING \
    ../packaging/windows/shotcut.rc \
    ../scripts/build-shotcut.sh \
    ../packaging/macos/shotcut.icns \
    ../packaging/windows/shotcut.nsi \
    ../packaging/macos/Info.plist \
    ../icons/dark/index.theme \
    ../icons/light/index.theme \
    ../packaging/linux/appimage/appimage.yml \
    ../packaging/linux/snap/snapcraft.yaml \
    ../packaging/linux/snap/package.mak \
    ../packaging/linux/org.shotcut.Shotcut.appdata.xml \
    ../packaging/linux/org.shotcut.Shotcut.desktop \
    ../packaging/linux/org.shotcut.Shotcut.xml \
    ../packaging/linux/shotcut.1

INCLUDEPATH = ../CuteLogger/include

debug_and_release {
    build_pass:CONFIG(debug, debug|release) {
        LIBS += -L../CuteLogger/debug
    } else {
        LIBS += -L../CuteLogger/release
    }
} else {
    LIBS += -L../CuteLogger
}
LIBS += -lCuteLogger

isEmpty(SHOTCUT_VERSION) {
    !win32:SHOTCUT_VERSION = $$system(date "+%y.%m.%d")
     win32:SHOTCUT_VERSION = adhoc
}
DEFINES += SHOTCUT_VERSION=\\\"$$SHOTCUT_VERSION\\\"
VERSION = $$SHOTCUT_VERSION

mac {
    TARGET = Shotcut
    ICON = ../packaging/macos/shotcut.icns
    QMAKE_INFO_PLIST = ../packaging/macos/Info.plist
    INCLUDEPATH += $$[QT_INSTALL_HEADERS]

    # QMake from Qt 5.1.0 on OSX is messing with the environment in which it runs
    # pkg-config such that the PKG_CONFIG_PATH env var is not set.
    isEmpty(MLT_PREFIX) {
        MLT_PREFIX = /opt/local
    }
    isEmpty(PREFIX) {
        INCLUDEPATH += $$MLT_PREFIX/include/mlt++
        INCLUDEPATH += $$MLT_PREFIX/include/mlt
        LIBS += -L$$MLT_PREFIX/lib -lmlt++ -lmlt
    } else {
        INCLUDEPATH += $$PREFIX/Contents/Frameworks/include/mlt++
        INCLUDEPATH += $$PREFIX/Contents/Frameworks/include/mlt
        LIBS += -L$$PREFIX/Contents/Frameworks -lmlt++ -lmlt
    }
}
win32 {
    CONFIG += windows rtti
    isEmpty(MLT_PATH) {
        message("MLT_PATH not set; using ..\\..\\... You can change this with 'qmake MLT_PATH=...'")
        MLT_PATH = ..\\..\\..
    }
    INCLUDEPATH += $$MLT_PATH\\include\\mlt++ $$MLT_PATH\\include\\mlt
    LIBS += -L$$MLT_PATH\\lib -lmlt++ -lmlt -lopengl32
    CONFIG(debug, debug|release) {
        INCLUDEPATH += $$PWD/../drmingw/include
        LIBS += -L$$PWD/../drmingw/x64/lib -lexchndl
    }
    RC_FILE = ../packaging/windows/shotcut.rc
}
unix:!mac {
    QT += x11extras
    CONFIG += link_pkgconfig
    PKGCONFIG += mlt++
    LIBS += -lX11
}

unix:!mac:isEmpty(PREFIX) {
    message("Install PREFIX not set; using /usr/local. You can change this with 'qmake PREFIX=...'")
    PREFIX = /usr/local
}
win32:isEmpty(PREFIX) {
    message("Install PREFIX not set; using C:\\Projects\\Shotcut. You can change this with 'qmake PREFIX=...'")
    PREFIX = C:\\Projects\\Shotcut
}
unix:!mac:target.path = $$PREFIX/bin
win32:target.path = $$PREFIX
INSTALLS += target

qmlfiles.files = $$PWD/qml
!mac:qmlfiles.path = $$PREFIX/share/shotcut
mac:qmlfiles.path = $$PREFIX/Contents/Resources/shotcut
INSTALLS += qmlfiles

unix:!mac {
    metainfo.files = $$PWD/../packaging/linux/org.shotcut.Shotcut.appdata.xml
    metainfo.path = $$PREFIX/share/metainfo
    desktop.files = $$PWD/../packaging/linux/org.shotcut.Shotcut.desktop
    desktop.path = $$PREFIX/share/applications
    mime.files = $$PWD/../packaging/linux/org.shotcut.Shotcut.xml
    mime.path = $$PREFIX/share/mime/packages
    icons.files = $$PWD/../packaging/linux/org.shotcut.Shotcut.png
    icons.path = $$PREFIX/share/icons/hicolor/64x64/apps
    man.files = $$PWD/../packaging/linux/shotcut.1
    man.path = $$PREFIX/share/man/man1
    INSTALLS += metainfo desktop mime icons man
}
