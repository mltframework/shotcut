CONFIG   += link_prl

QT       += widgets opengl xml network qml quick sql
QT       += multimedia websockets quickwidgets quickcontrols2

TARGET = shotcut
TEMPLATE = app

win32:DEFINES += QT_STATIC

SOURCES += main.cpp\
    dialogs/systemsyncdialog.cpp \
    jobs/qimagejob.cpp \
    mainwindow.cpp \
    mltcontroller.cpp \
    proxymanager.cpp \
    qmltypes/qmlrichtext.cpp \
    scrubbar.cpp \
    openotherdialog.cpp \
    controllers/filtercontroller.cpp \
    spatialmedia/box.cpp \
    spatialmedia/container.cpp \
    spatialmedia/mpeg4_container.cpp \
    spatialmedia/sa3d.cpp \
    spatialmedia/spatialmedia.cpp \
    widgets/exportpresetstreeview.cpp \
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
    widgets/producerpreviewwidget.cpp \
    widgets/pulseaudiowidget.cpp \
    widgets/screenselector.cpp \
    widgets/jackproducerwidget.cpp \
    widgets/toneproducerwidget.cpp \
    widgets/alsawidget.cpp \
    widgets/x11grabwidget.cpp \
    widgets/blipproducerwidget.cpp \
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
    dialogs/alignaudiodialog.cpp \
    dialogs/alignmentarray.cpp \
    dialogs/filedatedialog.cpp \
    jobqueue.cpp \
    docks/jobsdock.cpp \
    dialogs/multifileexportdialog.cpp \
    dialogs/saveimagedialog.cpp \
    dialogs/editmarkerdialog.cpp \
    dialogs/slideshowgeneratordialog.cpp \
    dialogs/textviewerdialog.cpp \
    models/playlistmodel.cpp \
    docks/playlistdock.cpp \
    dialogs/durationdialog.cpp \
    widgets/colorwheel.cpp \
    models/alignclipsmodel.cpp \
    models/attachedfiltersmodel.cpp \
    models/metadatamodel.cpp \
    docks/filtersdock.cpp \
    dialogs/customprofiledialog.cpp \
    qmltypes/colorpickeritem.cpp \
    qmltypes/colorwheelitem.cpp \
    qmltypes/qmlapplication.cpp \
    qmltypes/qmlfile.cpp \
    qmltypes/qmlfilter.cpp \
    qmltypes/qmlmetadata.cpp \
    qmltypes/timelineitems.cpp \
    qmltypes/qmlprofile.cpp \
    settings.cpp \
    widgets/lineeditclear.cpp \
    leapnetworklistener.cpp \
    database.cpp \
    widgets/gltestwidget.cpp \
    models/multitrackmodel.cpp \
    docks/timelinedock.cpp \
    qmltypes/qmlutilities.cpp \
    qmltypes/qmlview.cpp \
    qmltypes/thumbnailprovider.cpp \
    commands/markercommands.cpp \
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
    widgets/scopes/videorgbparadescopewidget.cpp \
    widgets/scopes/videorgbwaveformscopewidget.cpp \
    widgets/scopes/videovectorscopewidget.cpp \
    widgets/scopes/videowaveformscopewidget.cpp \
    widgets/scopes/videozoomscopewidget.cpp \
    widgets/scopes/videozoomwidget.cpp \
    sharedframe.cpp \
    widgets/audioscale.cpp \
    widgets/playlisttable.cpp \
    widgets/playlisticonview.cpp \
    commands/undohelper.cpp \
    models/audiolevelstask.cpp \
    mltxmlchecker.cpp \
    widgets/avfoundationproducerwidget.cpp \
    widgets/frameratewidget.cpp \
    widgets/gdigrabwidget.cpp \
    widgets/trackpropertieswidget.cpp \
    widgets/timelinepropertieswidget.cpp \
    jobs/ffprobejob.cpp \
    jobs/ffmpegjob.cpp \
    dialogs/unlinkedfilesdialog.cpp \
    dialogs/transcodedialog.cpp \
    docks/keyframesdock.cpp \
    docks/markersdock.cpp \
    docks/notesdock.cpp \
    qmltypes/qmlproducer.cpp \
    models/keyframesmodel.cpp \
    models/markersmodel.cpp \
    widgets/editmarkerwidget.cpp \
    widgets/slideshowgeneratorwidget.cpp \
    widgets/textproducerwidget.cpp \
    dialogs/listselectiondialog.cpp \
    dialogs/longuitask.cpp \
    widgets/newprojectfolder.cpp \
    widgets/playlistlistview.cpp

mac: OBJECTIVE_SOURCES = macos.mm

HEADERS  += mainwindow.h \
    defaultlayouts.h \
    dialogs/systemsyncdialog.h \
    jobs/qimagejob.h \
    mltcontroller.h \
    proxymanager.h \
    qmltypes/qmlrichtext.h \
    scrubbar.h \
    openotherdialog.h \
    controllers/filtercontroller.h \
    spatialmedia/box.h \
    spatialmedia/constants.h \
    spatialmedia/container.h \
    spatialmedia/mpeg4_container.h \
    spatialmedia/sa3d.h \
    spatialmedia/spatialmedia.h \
    widgets/exportpresetstreeview.h \
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
    widgets/producerpreviewwidget.h \
    widgets/pulseaudiowidget.h \
    widgets/screenselector.h \
    widgets/jackproducerwidget.h \
    widgets/toneproducerwidget.h \
    widgets/alsawidget.h \
    widgets/x11grabwidget.h \
    widgets/blipproducerwidget.h \
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
    dialogs/alignaudiodialog.h \
    dialogs/alignmentarray.h \
    dialogs/filedatedialog.h \
    jobqueue.h \
    docks/jobsdock.h \
    dialogs/multifileexportdialog.h \
    dialogs/saveimagedialog.h \
    dialogs/editmarkerdialog.h \
    dialogs/slideshowgeneratordialog.h \
    dialogs/textviewerdialog.h \
    models/playlistmodel.h \
    docks/playlistdock.h \
    dialogs/durationdialog.h \
    transportcontrol.h \
    widgets/colorwheel.h \
    models/alignclipsmodel.h \
    models/attachedfiltersmodel.h \
    models/metadatamodel.h \
    docks/filtersdock.h \
    dialogs/customprofiledialog.h \
    qmltypes/colorpickeritem.h \
    qmltypes/colorwheelitem.h \
    qmltypes/qmlapplication.h \
    qmltypes/qmlfile.h \
    qmltypes/qmlfilter.h \
    qmltypes/qmlmetadata.h \
    qmltypes/timelineitems.h \
    qmltypes/qmlprofile.h \
    settings.h \
    widgets/lineeditclear.h \
    leapnetworklistener.h \
    database.h \
    widgets/gltestwidget.h \
    models/multitrackmodel.h \
    docks/timelinedock.h \
    qmltypes/qmlutilities.h \
    qmltypes/qmlview.h \
    qmltypes/thumbnailprovider.h \
    commands/markercommands.h \
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
    widgets/scopes/videorgbparadescopewidget.h \
    widgets/scopes/videorgbwaveformscopewidget.h \
    widgets/scopes/videovectorscopewidget.h \
    widgets/scopes/videowaveformscopewidget.h \
    widgets/scopes/videozoomscopewidget.h \
    widgets/scopes/videozoomwidget.h \
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
    widgets/frameratewidget.h \
    widgets/gdigrabwidget.h \
    widgets/trackpropertieswidget.h \
    widgets/timelinepropertieswidget.h \
    jobs/ffprobejob.h \
    jobs/ffmpegjob.h \
    dialogs/unlinkedfilesdialog.h \
    dialogs/transcodedialog.h \
    docks/keyframesdock.h \
    docks/markersdock.h \
    docks/notesdock.h \
    qmltypes/qmlproducer.h \
    models/keyframesmodel.h \
    models/markersmodel.h \
    widgets/editmarkerwidget.h \
    widgets/slideshowgeneratorwidget.h \
    widgets/textproducerwidget.h \
    dialogs/listselectiondialog.h \
    dialogs/longuitask.h \
    widgets/newprojectfolder.h \
    widgets/playlistlistview.h

FORMS    += mainwindow.ui \
    dialogs/systemsyncdialog.ui \
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
    widgets/blipproducerwidget.ui \
    docks/recentdock.ui \
    docks/encodedock.ui \
    dialogs/addencodepresetdialog.ui \
    docks/jobsdock.ui \
    dialogs/textviewerdialog.ui \
    docks/playlistdock.ui \
    dialogs/durationdialog.ui \
    dialogs/customprofiledialog.ui \
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
    ../icons/resources.qrc

OTHER_FILES += \
    ../.github/workflows/build-linux.yml \
    ../.github/workflows/build-linux-unstable.yml \
    ../.github/workflows/build-macos.yml \
    ../.github/workflows/build-macos-unstable.yml \
    ../.github/workflows/build-sdk-windows.yml \
    ../.github/workflows/build-sdk-windows-unstable.yml \
    ../.github/workflows/build-windows.yml \
    ../.github/workflows/build-windows-unstable.yml \
    ../COPYING \
    ../README.md \
    ../packaging/macos/Info.plist.in \
    ../scripts/build-shotcut.sh \
    ../scripts/build-shotcut-msys2.sh \
    ../packaging/macos/shotcut.icns \
    ../packaging/windows/shotcut.nsi \
    ../icons/dark/index.theme \
    ../icons/light/index.theme \
    ../packaging/linux/Makefile \
    ../packaging/linux/appimage/appimage.yml \
    ../packaging/linux/snapcraft.yaml.in \
    ../packaging/linux/org.shotcut.Shotcut.metainfo.xml \
    ../packaging/linux/org.shotcut.Shotcut.desktop \
    ../packaging/linux/org.shotcut.Shotcut.xml \
    ../packaging/linux/shotcut.1 \
    ../.github/ISSUE_TEMPLATE.md \
    ../scripts/codesign_and_notarize.sh \
    ../scripts/notarize.sh \
    ../scripts/staple.sh

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

LIBS += -lfftw3

isEmpty(SHOTCUT_VERSION) {
    !win32:SHOTCUT_VERSION = $$system(date -u -d "@${SOURCE_DATE_EPOCH:-$(date +%s)}" "+%y.%m.%d" 2>/dev/null || date -u -r "${SOURCE_DATE_EPOCH:-$(date +%s)}" "+%y.%m.%d")
     win32:SHOTCUT_VERSION = $$system(echo "%date:~12,2%.%date:~4,2%.%date:~7,2%")
}
DEFINES += SHOTCUT_VERSION=\\\"$$SHOTCUT_VERSION\\\"
VERSION = $$SHOTCUT_VERSION

mac {
    TARGET = Shotcut
    ICON = ../packaging/macos/shotcut.icns
    QMAKE_INFO_PLIST = \
    ../packaging/macos/Info.plist.in
    INCLUDEPATH += $$[QT_INSTALL_HEADERS]
    LIBS += -framework Foundation -framework Cocoa

    # QMake from Qt 5.1.0 on OSX is messing with the environment in which it runs
    # pkg-config such that the PKG_CONFIG_PATH env var is not set.
    isEmpty(MLT_PREFIX) {
        MLT_PREFIX = /opt/local
        INCLUDEPATH += $$PREFIX/Contents/Frameworks/include/mlt-7/mlt++
        INCLUDEPATH += $$PREFIX/Contents/Frameworks/include/mlt-7
        LIBS += -L$$PREFIX/Contents/Frameworks -lmlt++-7 -lmlt-7
    } else {
        INCLUDEPATH += $$MLT_PREFIX/include/mlt-7/mlt++
        INCLUDEPATH += $$MLT_PREFIX/include/mlt-7
        LIBS += -L$$MLT_PREFIX/lib -lmlt++-7 -lmlt-7
    }
}
win32 {
    CONFIG += windows rtti
    isEmpty(MLT_PATH) {
        message("MLT_PATH not set; using ..\\..\\... You can change this with 'qmake MLT_PATH=...'")
        MLT_PATH = ..\\..\\..
    }
    INCLUDEPATH += $$MLT_PATH\\include\\mlt-7\\mlt++ $$MLT_PATH\\include\\mlt-7
    LIBS += -L$$MLT_PATH\\lib -lmlt++-7 -lmlt-7 -lopengl32
    CONFIG(debug, debug|release) {
        INCLUDEPATH += $$PWD/../drmingw/include
        LIBS += -L$$PWD/../drmingw/x64/lib -lexchndl
    }
    QMAKE_TARGET_COMPANY = Meltytech, LLC
    QMAKE_TARGET_COPYRIGHT = Copyright @ 2011-2022 Meltytech, LLC. All rights reserved.
    QMAKE_TARGET_DESCRIPTION = Shotcut video editor
    QMAKE_TARGET_PRODUCT = Shotcut
    RC_ICONS = ../packaging/windows/shotcut-logo-64.ico
    QT += winextras
    HEADERS += \
    windowstools.h
    SOURCES += \
    windowstools.cpp
}
unix:!mac {
    CONFIG += link_pkgconfig
    PKGCONFIG += mlt++-7
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
    isEmpty(SHOTCUT_DATE) {
        SHOTCUT_DATE = 20$$replace(SHOTCUT_VERSION, \., -)
    }
    appdata = $$cat($$PWD/../packaging/linux/org.shotcut.Shotcut.metainfo.xml.in, blob)
    appdata = $$replace(appdata, @METAINFO_RELEASE_VERSION@, $$SHOTCUT_VERSION)
    appdata = $$replace(appdata, @METAINFO_RELEASE_DATE@, $$SHOTCUT_DATE)
    write_file($$OUT_PWD/../packaging/linux/org.shotcut.Shotcut.metainfo.xml, appdata)

    metainfo.files = $$OUT_PWD/../packaging/linux/org.shotcut.Shotcut.metainfo.xml
    metainfo.path = $$PREFIX/share/metainfo
    desktop.files = $$PWD/../packaging/linux/org.shotcut.Shotcut.desktop
    desktop.path = $$PREFIX/share/applications
    mime.files = $$PWD/../packaging/linux/org.shotcut.Shotcut.xml
    mime.path = $$PREFIX/share/mime/packages
    icon64.files = $$PWD/../packaging/linux/icons/64x64/org.shotcut.Shotcut.png
    icon64.path = $$PREFIX/share/icons/hicolor/64x64/apps
    icon128.files = $$PWD/../packaging/linux/icons/128x128/org.shotcut.Shotcut.png
    icon128.path = $$PREFIX/share/icons/hicolor/128x128/apps
    man.files = $$PWD/../packaging/linux/shotcut.1
    man.path = $$PREFIX/share/man/man1
    INSTALLS += metainfo desktop mime icon64 icon128 man
}

