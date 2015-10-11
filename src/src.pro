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
    jobs/videoqualityjob.cpp \
    commands/playlistcommands.cpp \
    docks/scopedock.cpp \
    controllers/scopecontroller.cpp \
    widgets/scopes/scopewidget.cpp \
    widgets/scopes/audiopeakmeterscopewidget.cpp \
    widgets/scopes/audiospectrumscopewidget.cpp \
    widgets/scopes/audiowaveformscopewidget.cpp \
    widgets/scopes/videowaveformscopewidget.cpp \
    sharedframe.cpp \
    widgets/audioscale.cpp \
    widgets/playlisttable.cpp \
    commands/undohelper.cpp \
    models/audiolevelstask.cpp \
    mltxmlchecker.cpp


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
    jobs/videoqualityjob.h \
    commands/playlistcommands.h \
    docks/scopedock.h \
    controllers/scopecontroller.h \
    widgets/scopes/scopewidget.h \
    widgets/scopes/audiopeakmeterscopewidget.h \
    widgets/scopes/audiospectrumscopewidget.h \
    widgets/scopes/audiowaveformscopewidget.h \
    widgets/scopes/videowaveformscopewidget.h \
    dataqueue.h \
    sharedframe.h \
    widgets/audioscale.h \
    forwardingquickviewworkaround.h \
    widgets/playlisttable.h \
    commands/undohelper.h \
    models/audiolevelstask.h \
    shotcut_mlt_properties.h \
    mltxmlchecker.h

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
    mvcp/meltedserverdock.ui \
    mvcp/meltedplaylistdock.ui \
    dialogs/customprofiledialog.ui \
    htmleditor/htmleditor.ui \
    htmleditor/inserthtmldialog.ui \
    widgets/webvfxproducer.ui \
    docks/timelinedock.ui \
    widgets/lumamixtransition.ui \
    widgets/directshowvideowidget.ui

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
    qml/modules/Shotcut/Controls/ColorPicker.qml \
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
    qml/filters/mirror/meta.qml \
    qml/filters/mirror/meta_movit.qml \
    qml/filters/mirror/ui.qml \
    qml/filters/rotate/ui.qml \
    qml/filters/rotate/meta.qml \
    qml/filters/stabilize/meta.qml \
    qml/filters/stabilize/ui.qml \
    qml/filters/audio_mono/meta.qml \
    qml/filters/audio_swapchannels/meta.qml \
    qml/filters/audio_swapchannels/ui.qml \
    qml/filters/invert/meta.qml \
    qml/filters/invert/ui.qml \
    qml/filters/sepia/meta.qml \
    qml/filters/sepia/ui.qml \
    qml/filters/sharpen/meta_frei0r.qml \
    qml/filters/sharpen/ui_frei0r.qml \
    qml/filters/sharpen/meta_movit.qml \
    qml/filters/sharpen/ui_movit.qml \
    qml/modules/Shotcut/Controls/ToolTip.qml \
    qml/modules/Shotcut/Controls/UndoButton.qml \
    qml/timeline/timeline.qml \
    qml/timeline/TrackHead.qml \
    qml/timeline/Track.qml \
    qml/timeline/Clip.qml \
    qml/timeline/Ruler.qml \
    qml/timeline/Track.js \
    qml/views/filter/filterview.qml \
    qml/views/filter/AttachedFilters.qml \
    qml/views/filter/FilterMenu.qml \
    qml/views/filter/FilterMenu.js \
    qml/views/filter/FilterMenuDelegate.qml \
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
    qml/timeline/ToggleButton.qml \
    qml/filters/crop/meta_movit.qml \
    qml/filters/color/meta_movit.qml \
    qml/filters/color/meta_frei0r_coloradj.qml \
    qml/filters/color/ui_frei0r_coloradj.qml \
    qml/filters/wave/meta.qml \
    qml/filters/wave/ui.qml \
    qml/filters/webvfx/meta.qml \
    qml/filters/webvfx/ui.qml \
    qml/filters/white/meta_frei0r.qml \
    qml/filters/white/meta_movit.qml \
    qml/filters/white/ui.qml \
    qml/modules/Shotcut/Controls/SliderSpinner.qml \
    qml/modules/Shotcut/Controls/RectangleControl.qml \
    qml/modules/Shotcut/Controls/SimplePropertyUI.qml \
    qml/filters/size_position/SizePositionUI.qml \
    qml/filters/size_position/SizePositionVUI.qml \
    qml/filters/size_position/meta_affine.qml \
    qml/filters/size_position/vui_affine.qml \
    qml/filters/size_position/ui_affine.qml \
    qml/filters/size_position/meta_movit.qml \
    qml/filters/size_position/ui_movit.qml \
    qml/filters/size_position/vui_movit.qml \
    qml/filters/opacity/meta.qml \
    qml/filters/opacity/ui.qml \
    qml/filters/opacity/meta_movit.qml \
    qml/filters/dynamictext/meta.qml \
    qml/filters/dynamictext/ui.qml \
    qml/filters/dynamictext/vui.qml \
    qml/vui_droparea.qml \
    qml/filters/webvfx_threejs_text/meta.qml \
    qml/filters/webvfx_threejs_text/ui.qml \
    qml/filters/webvfx_threejs_text/threejs_text.html \
    qml/filters/webvfx_threejs_text/Detector.js \
    qml/filters/webvfx_threejs_text/GeometryUtils.js \
    qml/filters/webvfx_threejs_text/three.min.js \
    qml/filters/webvfx_threejs_text/fonts/droid_sans_bold.typeface.js \
    qml/filters/webvfx_threejs_text/fonts/droid_sans_regular.typeface.js \
    qml/filters/webvfx_threejs_text/fonts/droid_serif_bold.typeface.js \
    qml/filters/webvfx_threejs_text/fonts/droid_serif_regular.typeface.js \
    qml/filters/webvfx_threejs_text/fonts/gentilis_bold.typeface.js \
    qml/filters/webvfx_threejs_text/fonts/gentilis_regular.typeface.js \
    qml/filters/webvfx_threejs_text/fonts/helvetiker_bold.typeface.js \
    qml/filters/webvfx_threejs_text/fonts/helvetiker_regular.typeface.js \
    qml/filters/webvfx_threejs_text/fonts/optimer_bold.typeface.js \
    qml/filters/webvfx_threejs_text/fonts/optimer_regular.typeface.js \
    qml/modules/Shotcut/Controls/SaveDefaultButton.qml \
    qml/filters/dust/meta.qml \
    qml/filters/dust/ui.qml \
    qml/filters/lines/meta.qml \
    qml/filters/lines/ui.qml \
    qml/filters/grain/meta.qml \
    qml/filters/grain/ui.qml \
    qml/filters/tcolor/meta.qml \
    qml/filters/tcolor/ui.qml \
    qml/filters/oldfilm/meta.qml \
    qml/filters/oldfilm/ui.qml \
    qml/filters/audio_bandpass/meta.qml \
    qml/filters/audio_bandpass/ui.qml \
    qml/filters/audio_basstreble/meta.qml \
    qml/filters/audio_basstreble/ui.qml \
    qml/filters/audio_highpass/meta.qml \
    qml/filters/audio_highpass/ui.qml \
    qml/filters/audio_lowpass/meta.qml \
    qml/filters/audio_lowpass/ui.qml \
    qml/filters/audio_notch/meta.qml \
    qml/filters/audio_notch/ui.qml \
    qml/filters/audio_compressor/meta.qml \
    qml/filters/audio_compressor/ui.qml \
    qml/filters/audio_delay/meta.qml \
    qml/filters/audio_delay/ui.qml \
    qml/filters/audio_expander/meta.qml \
    qml/filters/audio_expander/ui.qml \
    qml/filters/audio_limiter/meta.qml \
    qml/filters/audio_limiter/ui.qml \
    qml/filters/audio_reverb/meta.qml \
    qml/filters/audio_reverb/ui.qml \
    qml/filters/audio_mute/meta.qml \
    qml/filters/audio_mute/ui.qml \
    qml/filters/webvfx_ruttetraizer/meta.qml \
    qml/filters/webvfx_ruttetraizer/ui.qml \
    qml/filters/webvfx_ruttetraizer/ruttetraizer.html \
    qml/filters/webvfx_ruttetraizer/three.js

lupdate_hack {
    SOURCES += \
    qml/filters/saturation/meta_frei0r.qml \
    qml/filters/saturation/ui_frei0r.qml \
    qml/filters/saturation/meta_movit.qml \
    qml/filters/saturation/ui_movit.qml \
    qml/filters/webvfx_circular_frame/ui.qml \
    qml/filters/webvfx_circular_frame/meta.qml \
    qml/modules/Shotcut/Controls/qmldir \
    qml/modules/Shotcut/Controls/ColorPicker.qml \
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
    qml/filters/mirror/meta.qml \
    qml/filters/mirror/meta_movit.qml \
    qml/filters/mirror/ui.qml \
    qml/filters/rotate/ui.qml \
    qml/filters/rotate/meta.qml \
    qml/filters/stabilize/meta.qml \
    qml/filters/stabilize/ui.qml \
    qml/filters/audio_mono/meta.qml \
    qml/filters/audio_swapchannels/meta.qml \
    qml/filters/audio_swapchannels/ui.qml \
    qml/filters/invert/meta.qml \
    qml/filters/invert/ui.qml \
    qml/filters/sepia/meta.qml \
    qml/filters/sepia/ui.qml \
    qml/filters/sharpen/meta_frei0r.qml \
    qml/filters/sharpen/ui_frei0r.qml \
    qml/filters/sharpen/meta_movit.qml \
    qml/filters/sharpen/ui_movit.qml \
    qml/modules/Shotcut/Controls/ToolTip.qml \
    qml/modules/Shotcut/Controls/UndoButton.qml \
    qml/timeline/timeline.qml \
    qml/timeline/TimelineToolbar.qml \
    qml/timeline/TrackHead.qml \
    qml/timeline/Track.qml \
    qml/timeline/Clip.qml \
    qml/timeline/Ruler.qml \
    qml/views/filter/filterview.qml \
    qml/views/filter/AttachedFilters.qml \
    qml/views/filter/FilterMenu.qml \
    qml/views/filter/FilterMenu.js \
    qml/views/filter/FilterMenuDelegate.qml \
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
    qml/filters/wave/ui.qml \
    qml/filters/webvfx/meta.qml \
    qml/filters/webvfx/ui.qml \
    qml/filters/white/meta_frei0r.qml \
    qml/filters/white/meta_movit.qml \
    qml/filters/white/ui.qml \
    qml/filters/size_position/SizePositionUI.qml \
    qml/filters/size_position/SizePositionVUI.qml \
    qml/filters/size_position/meta_affine.qml \
    qml/filters/size_position/vui_affine.qml \
    qml/filters/size_position/ui_affine.qml \
    qml/filters/size_position/meta_movit.qml \
    qml/filters/size_position/ui_movit.qml \
    qml/filters/size_position/vui_movit.qml \
    qml/filters/opacity/meta.qml \
    qml/filters/opacity/ui.qml \
    qml/filters/opacity/meta_movit.qml \
    qml/filters/dynamictext/meta.qml \
    qml/filters/dynamictext/ui.qml \
    qml/filters/dynamictext/vui.qml \
    qml/filters/webvfx_threejs_text/meta.qml \
    qml/filters/webvfx_threejs_text/ui.qml \
    qml/modules/Shotcut/Controls/SaveDefaultButton.qml \
    qml/filters/dust/meta.qml \
    qml/filters/dust/ui.qml \
    qml/filters/lines/meta.qml \
    qml/filters/lines/ui.qml \
    qml/filters/grain/meta.qml \
    qml/filters/grain/ui.qml \
    qml/filters/tcolor/meta.qml \
    qml/filters/tcolor/ui.qml \
    qml/filters/oldfilm/meta.qml \
    qml/filters/oldfilm/ui.qml \
    qml/filters/audio_bandpass/meta.qml \
    qml/filters/audio_bandpass/ui.qml \
    qml/filters/audio_basstreble/meta.qml \
    qml/filters/audio_basstreble/ui.qml \
    qml/filters/audio_highpass/meta.qml \
    qml/filters/audio_highpass/ui.qml \
    qml/filters/audio_lowpass/meta.qml \
    qml/filters/audio_lowpass/ui.qml \
    qml/filters/audio_notch/meta.qml \
    qml/filters/audio_notch/ui.qml \
    qml/filters/audio_compressor/meta.qml \
    qml/filters/audio_compressor/ui.qml \
    qml/filters/audio_delay/meta.qml \
    qml/filters/audio_delay/ui.qml \
    qml/filters/audio_expander/meta.qml \
    qml/filters/audio_expander/ui.qml \
    qml/filters/audio_limiter/meta.qml \
    qml/filters/audio_limiter/ui.qml \
    qml/filters/audio_reverb/meta.qml \
    qml/filters/audio_reverb/ui.qml \
    qml/filters/audio_mute/meta.qml \
    qml/filters/audio_mute/ui.qml \
    qml/filters/webvfx_ruttetraizer/ui.qml \
}

TRANSLATIONS += \
    ../translations/shotcut_ca.ts \
    ../translations/shotcut_cs.ts \
    ../translations/shotcut_da.ts \
    ../translations/shotcut_de.ts \
    ../translations/shotcut_el.ts \
    ../translations/shotcut_en.ts \
    ../translations/shotcut_es.ts \
    ../translations/shotcut_fr.ts \
    ../translations/shotcut_it.ts \
    ../translations/shotcut_nl.ts \
    ../translations/shotcut_pl.ts \
    ../translations/shotcut_pt_BR.ts \
    ../translations/shotcut_pt_PT.ts \
    ../translations/shotcut_ru.ts \
    ../translations/shotcut_zh.ts

INCLUDEPATH = ../CuteLogger/include ../mvcp

debug_and_release {
    build_pass:CONFIG(debug, debug|release) {
        LIBS += -L../CuteLogger/debug -L../mvcp/debug
    } else {
        LIBS += -L../CuteLogger/release -L../mvcp/release
    }
} else {
    LIBS += -L../CuteLogger -L../mvcp
}
LIBS += -lLogger -lmvcp -lpthread

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

qmlfiles.files = $$PWD/qml
qmlfiles.path = $$PREFIX/share/shotcut
INSTALLS += qmlfiles
