add_executable(shotcut WIN32 MACOSX_BUNDLE
  abstractproducerwidget.cpp abstractproducerwidget.h
  actions.cpp actions.h
  autosavefile.cpp autosavefile.h
  commands/markercommands.cpp commands/markercommands.h
  commands/playlistcommands.cpp commands/playlistcommands.h
  commands/timelinecommands.cpp commands/timelinecommands.h
  commands/undohelper.cpp commands/undohelper.h
  controllers/filtercontroller.cpp controllers/filtercontroller.h
  controllers/scopecontroller.cpp controllers/scopecontroller.h
  database.cpp database.h
  dialogs/actionsdialog.cpp dialogs/actionsdialog.h
  dialogs/addencodepresetdialog.cpp dialogs/addencodepresetdialog.h
  dialogs/addencodepresetdialog.ui
  dialogs/alignaudiodialog.cpp dialogs/alignaudiodialog.h
  dialogs/alignmentarray.cpp dialogs/alignmentarray.h
  dialogs/customprofiledialog.cpp dialogs/customprofiledialog.h
  dialogs/customprofiledialog.ui
  dialogs/durationdialog.cpp dialogs/durationdialog.h
  dialogs/durationdialog.ui
  dialogs/editmarkerdialog.cpp dialogs/editmarkerdialog.h
  dialogs/filedatedialog.cpp dialogs/filedatedialog.h
  dialogs/listselectiondialog.cpp dialogs/listselectiondialog.h
  dialogs/listselectiondialog.ui
  dialogs/longuitask.cpp dialogs/longuitask.h
  dialogs/multifileexportdialog.cpp dialogs/multifileexportdialog.h
  dialogs/resourcedialog.cpp dialogs/resourcedialog.h
  dialogs/saveimagedialog.cpp dialogs/saveimagedialog.h
  dialogs/slideshowgeneratordialog.cpp dialogs/slideshowgeneratordialog.h
  dialogs/systemsyncdialog.cpp dialogs/systemsyncdialog.h
  dialogs/systemsyncdialog.ui
  dialogs/textviewerdialog.cpp dialogs/textviewerdialog.h
  dialogs/textviewerdialog.ui
  dialogs/transcodedialog.cpp dialogs/transcodedialog.h
  dialogs/transcodedialog.ui
  dialogs/unlinkedfilesdialog.cpp dialogs/unlinkedfilesdialog.h
  dialogs/unlinkedfilesdialog.ui
  docks/encodedock.cpp docks/encodedock.h
  docks/encodedock.ui
  docks/filtersdock.cpp docks/filtersdock.h
  docks/jobsdock.cpp docks/jobsdock.h
  docks/jobsdock.ui
  docks/keyframesdock.cpp docks/keyframesdock.h
  docks/markersdock.cpp docks/markersdock.h
  docks/notesdock.cpp docks/notesdock.h
  docks/playlistdock.cpp docks/playlistdock.h
  docks/playlistdock.ui
  docks/recentdock.cpp docks/recentdock.h
  docks/recentdock.ui
  docks/scopedock.cpp docks/scopedock.h
  docks/timelinedock.cpp docks/timelinedock.h
  jobqueue.cpp jobqueue.h
  jobs/abstractjob.cpp jobs/abstractjob.h
  jobs/encodejob.cpp jobs/encodejob.h
  jobs/ffmpegjob.cpp jobs/ffmpegjob.h
  jobs/ffprobejob.cpp jobs/ffprobejob.h
  jobs/gopro2gpxjob.cpp jobs/gopro2gpxjob.h
  jobs/meltjob.cpp jobs/meltjob.h
  jobs/postjobaction.cpp jobs/postjobaction.h
  jobs/qimagejob.cpp jobs/qimagejob.h
  jobs/videoqualityjob.cpp jobs/videoqualityjob.h
  main.cpp
  mainwindow.cpp mainwindow.h
  mainwindow.ui
  mltcontroller.cpp mltcontroller.h
  mltxmlchecker.cpp mltxmlchecker.h
  models/actionsmodel.cpp models/actionsmodel.h
  models/alignclipsmodel.cpp models/alignclipsmodel.h
  models/attachedfiltersmodel.cpp models/attachedfiltersmodel.h
  models/audiolevelstask.cpp models/audiolevelstask.h
  models/keyframesmodel.cpp models/keyframesmodel.h
  models/markersmodel.cpp models/markersmodel.h
  models/metadatamodel.cpp models/metadatamodel.h
  models/motiontrackermodel.h models/motiontrackermodel.cpp
  models/multitrackmodel.cpp models/multitrackmodel.h
  models/playlistmodel.cpp models/playlistmodel.h
  models/resourcemodel.cpp models/resourcemodel.h
  openotherdialog.cpp openotherdialog.h
  openotherdialog.ui
  player.cpp player.h
  proxymanager.cpp proxymanager.h
  qmltypes/colordialog.h qmltypes/colordialog.cpp
  qmltypes/colorpickeritem.cpp qmltypes/colorpickeritem.h
  qmltypes/colorwheelitem.cpp qmltypes/colorwheelitem.h
  qmltypes/filedialog.h qmltypes/filedialog.cpp
  qmltypes/fontdialog.h qmltypes/fontdialog.cpp
  qmltypes/messagedialog.h qmltypes/messagedialog.cpp
  qmltypes/qmlapplication.cpp qmltypes/qmlapplication.h
  qmltypes/qmleditmenu.cpp qmltypes/qmleditmenu.h
  qmltypes/qmlfile.cpp qmltypes/qmlfile.h
  qmltypes/qmlfilter.cpp qmltypes/qmlfilter.h
  qmltypes/qmlmarkermenu.cpp qmltypes/qmlmarkermenu.h
  qmltypes/qmlmetadata.cpp qmltypes/qmlmetadata.h
  qmltypes/qmlproducer.cpp qmltypes/qmlproducer.h
  qmltypes/qmlprofile.cpp qmltypes/qmlprofile.h
  qmltypes/qmlrichtext.cpp qmltypes/qmlrichtext.h
  qmltypes/qmlrichtextmenu.cpp qmltypes/qmlrichtextmenu.h
  qmltypes/qmlutilities.cpp qmltypes/qmlutilities.h
  qmltypes/qmlview.cpp qmltypes/qmlview.h
  qmltypes/thumbnailprovider.cpp qmltypes/thumbnailprovider.h
  qmltypes/timelineitems.cpp qmltypes/timelineitems.h
  resources.qrc
  scrubbar.cpp scrubbar.h
  settings.cpp settings.h
  sharedframe.cpp sharedframe.h
  shotcut_mlt_properties.h
  spatialmedia/box.cpp spatialmedia/box.h
  spatialmedia/container.cpp spatialmedia/container.h
  spatialmedia/mpeg4_container.cpp spatialmedia/mpeg4_container.h
  spatialmedia/sa3d.cpp spatialmedia/sa3d.h
  spatialmedia/spatialmedia.cpp spatialmedia/spatialmedia.h
  transportcontrol.h
  util.cpp util.h
  videowidget.cpp videowidget.h
  widgets/alsawidget.cpp widgets/alsawidget.h
  widgets/alsawidget.ui
  widgets/audiometerwidget.cpp widgets/audiometerwidget.h
  widgets/audioscale.cpp widgets/audioscale.h
  widgets/avformatproducerwidget.cpp widgets/avformatproducerwidget.h
  widgets/avformatproducerwidget.ui
  widgets/avfoundationproducerwidget.cpp widgets/avfoundationproducerwidget.h
  widgets/avfoundationproducerwidget.ui
  widgets/blipproducerwidget.cpp widgets/blipproducerwidget.h
  widgets/blipproducerwidget.ui
  widgets/colorbarswidget.cpp widgets/colorbarswidget.h
  widgets/colorbarswidget.ui
  widgets/colorproducerwidget.cpp widgets/colorproducerwidget.h
  widgets/colorproducerwidget.ui
  widgets/colorwheel.cpp widgets/colorwheel.h
  widgets/countproducerwidget.cpp widgets/countproducerwidget.h
  widgets/countproducerwidget.ui
  widgets/decklinkproducerwidget.cpp widgets/decklinkproducerwidget.h
  widgets/decklinkproducerwidget.ui
  widgets/directshowvideowidget.cpp widgets/directshowvideowidget.h
  widgets/directshowvideowidget.ui
  widgets/docktoolbar.cpp widgets/docktoolbar.h
  widgets/editmarkerwidget.cpp widgets/editmarkerwidget.h
  widgets/exportpresetstreeview.cpp widgets/exportpresetstreeview.h
  widgets/frameratewidget.cpp widgets/frameratewidget.h
  widgets/gdigrabwidget.cpp widgets/gdigrabwidget.h
  widgets/gdigrabwidget.ui
  widgets/glaxnimateproducerwidget.cpp widgets/glaxnimateproducerwidget.h
  widgets/glaxnimateproducerwidget.ui
  widgets/imageproducerwidget.cpp widgets/imageproducerwidget.h
  widgets/imageproducerwidget.ui
  widgets/isingwidget.cpp widgets/isingwidget.h
  widgets/isingwidget.ui
  widgets/jackproducerwidget.cpp widgets/jackproducerwidget.h
  widgets/jackproducerwidget.ui
  widgets/lineeditclear.cpp widgets/lineeditclear.h
  widgets/lissajouswidget.cpp widgets/lissajouswidget.h
  widgets/lissajouswidget.ui
  widgets/lumamixtransition.cpp widgets/lumamixtransition.h
  widgets/lumamixtransition.ui
  widgets/networkproducerwidget.cpp widgets/networkproducerwidget.h
  widgets/networkproducerwidget.ui
  widgets/newprojectfolder.cpp widgets/newprojectfolder.h
  widgets/newprojectfolder.ui
  widgets/noisewidget.cpp widgets/noisewidget.h
  widgets/noisewidget.ui
  widgets/plasmawidget.cpp widgets/plasmawidget.h
  widgets/plasmawidget.ui
  widgets/playlisticonview.cpp widgets/playlisticonview.h
  widgets/playlistlistview.cpp widgets/playlistlistview.h
  widgets/playlisttable.cpp widgets/playlisttable.h
  widgets/producerpreviewwidget.cpp widgets/producerpreviewwidget.h
  widgets/pulseaudiowidget.cpp widgets/pulseaudiowidget.h
  widgets/pulseaudiowidget.ui
  widgets/resourcewidget.cpp widgets/resourcewidget.h
  widgets/scopes/audioloudnessscopewidget.cpp widgets/scopes/audioloudnessscopewidget.h
  widgets/scopes/audiopeakmeterscopewidget.cpp widgets/scopes/audiopeakmeterscopewidget.h
  widgets/scopes/audiospectrumscopewidget.cpp widgets/scopes/audiospectrumscopewidget.h
  widgets/scopes/audiowaveformscopewidget.cpp widgets/scopes/audiowaveformscopewidget.h
  widgets/scopes/scopewidget.cpp widgets/scopes/scopewidget.h
  widgets/scopes/videohistogramscopewidget.cpp widgets/scopes/videohistogramscopewidget.h
  widgets/scopes/videorgbparadescopewidget.cpp widgets/scopes/videorgbparadescopewidget.h
  widgets/scopes/videorgbwaveformscopewidget.cpp widgets/scopes/videorgbwaveformscopewidget.h
  widgets/scopes/videovectorscopewidget.cpp widgets/scopes/videovectorscopewidget.h
  widgets/scopes/videowaveformscopewidget.cpp widgets/scopes/videowaveformscopewidget.h
  widgets/scopes/videozoomscopewidget.cpp widgets/scopes/videozoomscopewidget.h
  widgets/scopes/videozoomwidget.cpp widgets/scopes/videozoomwidget.h
  widgets/screenselector.cpp widgets/screenselector.h
  widgets/servicepresetwidget.cpp widgets/servicepresetwidget.h
  widgets/servicepresetwidget.ui
  widgets/slideshowgeneratorwidget.cpp widgets/slideshowgeneratorwidget.h
  widgets/statuslabelwidget.cpp widgets/statuslabelwidget.h
  widgets/textproducerwidget.cpp widgets/textproducerwidget.h
  widgets/textproducerwidget.ui
  widgets/timelinepropertieswidget.cpp widgets/timelinepropertieswidget.h
  widgets/timelinepropertieswidget.ui
  widgets/timespinbox.cpp widgets/timespinbox.h
  widgets/toneproducerwidget.cpp widgets/toneproducerwidget.h
  widgets/toneproducerwidget.ui
  widgets/trackpropertieswidget.cpp widgets/trackpropertieswidget.h
  widgets/trackpropertieswidget.ui
  widgets/video4linuxwidget.cpp widgets/video4linuxwidget.h
  widgets/video4linuxwidget.ui
  widgets/x11grabwidget.cpp widgets/x11grabwidget.h
  widgets/x11grabwidget.ui
  ../icons/resources.qrc
)

add_custom_target(OTHER_FILES
  SOURCES
  ../.github/ISSUE_TEMPLATE.md
  ../.github/workflows/build-linux-unstable.yml
  ../.github/workflows/build-linux.yml
  ../.github/workflows/build-macos-unstable.yml
  ../.github/workflows/build-macos.yml
  ../.github/workflows/build-sdk-windows-unstable.yml
  ../.github/workflows/build-sdk-windows.yml
  ../.github/workflows/build-windows-unstable.yml
  ../.github/workflows/build-windows.yml
  ../COPYING
  ../icons/dark/index.theme
  ../icons/light/index.theme
  ../packaging/linux/appimage/appimage.yml
  ../packaging/linux/Makefile
  ../packaging/linux/org.shotcut.Shotcut.desktop
  ../packaging/linux/org.shotcut.Shotcut.metainfo.xml.in
  ../packaging/linux/org.shotcut.Shotcut.xml
  ../packaging/linux/shotcut.1
  ../packaging/linux/snapcraft.yaml.in
  ../packaging/macos/Info.plist.in
  ../packaging/macos/shotcut.icns
  ../packaging/windows/shotcut.nsi
  ../packaging/windows/shotcut.rc.in
  ../README.md
  ../scripts/build-shotcut-msys2.sh
  ../scripts/build-shotcut.sh
  ../scripts/codesign_and_notarize.sh
  ../scripts/notarize.sh
  ../scripts/staple.sh
)

target_link_libraries(shotcut
  PRIVATE
  CuteLogger
  PkgConfig::mlt++
  PkgConfig::FFTW
  Qt6::Multimedia
  Qt6::Network
  Qt6::OpenGL
  Qt6::OpenGLWidgets
  Qt6::QuickControls2
  Qt6::QuickWidgets
  Qt6::Sql
  Qt6::Widgets
  Qt6::Xml
)

file(GLOB_RECURSE QML_SRC "qml/*")
target_sources(shotcut PRIVATE ${QML_SRC})

target_include_directories(shotcut PRIVATE ${CMAKE_SOURCE_DIR}/CuteLogger/include)
target_compile_definitions(shotcut PRIVATE SHOTCUT_VERSION="${SHOTCUT_VERSION}")

if(WIN32)
  # Windows resource
  string(REPLACE . , PRODUCT_VERSION ${PROJECT_VERSION})
  configure_file(${CMAKE_SOURCE_DIR}/packaging/windows/shotcut.rc.in ${CMAKE_SOURCE_DIR}/packaging/windows/shotcut.rc)
  target_sources(shotcut PRIVATE ${CMAKE_SOURCE_DIR}/packaging/windows/shotcut.rc)

  # Windows integration features
  # These are not yet available in Qt 6. They plan to make a cross-platform API planned but not yet implemented.
#  find_package(Qt 6.2 REQUIRED COMPONENTS WinExtras)
#  target_sources(shotcut PRIVATE windowstools.cpp windowstools.h)
#  target_link_libraries(shotcut PRIVATE Qt6::WinExtras)
  target_sources(shotcut PRIVATE widgets/d3dvideowidget.h widgets/d3dvideowidget.cpp)
  target_link_libraries(shotcut PRIVATE d3d11 d3dcompiler)

  # Runtime exception handler for debug only
  target_include_directories(shotcut PRIVATE ${CMAKE_SOURCE_DIR}/drmingw/include)
  target_link_directories(shotcut PRIVATE ${CMAKE_SOURCE_DIR}/drmingw/x64/lib)
  target_link_libraries(shotcut PRIVATE debug exchndl)

  if(WINDOWS_DEPLOY)
    install(TARGETS shotcut RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
  else()
    target_compile_definitions(shotcut PRIVATE NODEPLOY)
    install(TARGETS shotcut RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

  install(DIRECTORY qml DESTINATION ${CMAKE_INSTALL_PREFIX}/share/shotcut/)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/filter-sets DESTINATION ${CMAKE_INSTALL_PREFIX}/share/shotcut/)
else()
    target_sources(shotcut PRIVATE widgets/openglvideowidget.h widgets/openglvideowidget.cpp)
endif()

if(APPLE)
  target_sources(shotcut PRIVATE macos.mm macos.h
    widgets/metalvideowidget.h widgets/metalvideowidget.mm)
  string(SUBSTRING ${SHOTCUT_VERSION} 0, 5, SHORT_VERSION)
  set_target_properties(shotcut PROPERTIES
    OUTPUT_NAME "Shotcut"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/packaging/macos/Info.plist.in)
  find_library(FOUNDATION Foundation)
  find_library(COCOA Cocoa)
  target_link_libraries(shotcut PRIVATE ${FOUNDATION} ${COCOA})
  set(APP_ICON ${CMAKE_SOURCE_DIR}/packaging/macos/shotcut.icns)
  target_sources(shotcut PRIVATE ${APP_ICON})
  set_source_files_properties(${APP_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

  # Create a symlink to the qml folder after building
  # These are skipped for release because it breaks the app bundling and is only for development.
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Shotcut.app/Contents/Resources/shotcut)
    file(CREATE_LINK ${CMAKE_SOURCE_DIR}/src/qml ${CMAKE_CURRENT_BINARY_DIR}/Shotcut.app/Contents/Resources/shotcut/qml SYMBOLIC)
    file(CREATE_LINK ${CMAKE_SOURCE_DIR}/filter-sets ${CMAKE_CURRENT_BINARY_DIR}/Shotcut.app/Contents/Resources/shotcut/filter-sets SYMBOLIC)
  endif()

  install(TARGETS shotcut BUNDLE DESTINATION ${CMAKE_INSTALL_PREFIX})
  install(DIRECTORY qml DESTINATION ${CMAKE_INSTALL_PREFIX}/Shotcut.app/Contents/Resources/shotcut/)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/filter-sets DESTINATION ${CMAKE_INSTALL_PREFIX}/Shotcut.app/Contents/Resources/shotcut/)
endif()

if(UNIX AND NOT APPLE)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/share/shotcut)
  file(CREATE_LINK ${CMAKE_SOURCE_DIR}/src/qml ${CMAKE_BINARY_DIR}/share/shotcut/qml SYMBOLIC)
  file(CREATE_LINK ${CMAKE_SOURCE_DIR}/filter-sets ${CMAKE_BINARY_DIR}/share/shotcut/filter-sets SYMBOLIC)
  include(GNUInstallDirs)
  install(TARGETS shotcut RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(DIRECTORY qml DESTINATION ${CMAKE_INSTALL_DATADIR}/shotcut/)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/filter-sets DESTINATION ${CMAKE_INSTALL_DATADIR}/shotcut/)

  string(REPLACE . - SHOTCUT_DATE  ${SHOTCUT_VERSION})
  string(PREPEND SHOTCUT_DATE "20")
  configure_file(${CMAKE_SOURCE_DIR}/packaging/linux/org.shotcut.Shotcut.metainfo.xml.in
    ${CMAKE_BINARY_DIR}/packaging/linux/org.shotcut.Shotcut.metainfo.xml)
  install(FILES ${CMAKE_BINARY_DIR}/packaging/linux/org.shotcut.Shotcut.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo/)
  install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/org.shotcut.Shotcut.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications/)
  install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/org.shotcut.Shotcut.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages/)
  install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/icons/64x64/org.shotcut.Shotcut.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps/)
  install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/icons/128x128/org.shotcut.Shotcut.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps/)
  install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/shotcut.1
    DESTINATION ${CMAKE_INSTALL_DATADIR}/man/man1/)
endif()
