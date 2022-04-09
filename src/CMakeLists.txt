add_executable(shotcut
  main.cpp
  dialogs/systemsyncdialog.cpp
  jobs/qimagejob.cpp
  mainwindow.cpp
  mltcontroller.cpp
  proxymanager.cpp
  qmltypes/qmlrichtext.cpp
  scrubbar.cpp
  openotherdialog.cpp
  controllers/filtercontroller.cpp
  spatialmedia/box.cpp
  spatialmedia/container.cpp
  spatialmedia/mpeg4_container.cpp
  spatialmedia/sa3d.cpp
  spatialmedia/spatialmedia.cpp
  widgets/exportpresetstreeview.cpp
  widgets/plasmawidget.cpp
  widgets/lissajouswidget.cpp
  widgets/isingwidget.cpp
  widgets/video4linuxwidget.cpp
  widgets/colorproducerwidget.cpp
  widgets/decklinkproducerwidget.cpp
  widgets/networkproducerwidget.cpp
  widgets/colorbarswidget.cpp
  widgets/countproducerwidget.cpp
  widgets/frameratewidget.cpp
  widgets/noisewidget.cpp
  widgets/producerpreviewwidget.cpp
  widgets/pulseaudiowidget.cpp
  widgets/screenselector.cpp
  widgets/jackproducerwidget.cpp
  widgets/toneproducerwidget.cpp
  widgets/alsawidget.cpp
  widgets/x11grabwidget.cpp
  widgets/blipproducerwidget.cpp
  player.cpp
  glwidget.cpp
  widgets/servicepresetwidget.cpp
  abstractproducerwidget.cpp
  widgets/avformatproducerwidget.cpp
  widgets/imageproducerwidget.cpp
  widgets/timespinbox.cpp
  widgets/audiometerwidget.cpp
  docks/recentdock.cpp
  docks/encodedock.cpp
  dialogs/addencodepresetdialog.cpp
  dialogs/filedatedialog.cpp
  jobqueue.cpp
  docks/jobsdock.cpp
  dialogs/multifileexportdialog.cpp
  dialogs/saveimagedialog.cpp
  dialogs/editmarkerdialog.cpp
  dialogs/slideshowgeneratordialog.cpp
  dialogs/textviewerdialog.cpp
  models/playlistmodel.cpp
  docks/playlistdock.cpp
  dialogs/durationdialog.cpp
  widgets/colorwheel.cpp
  models/attachedfiltersmodel.cpp
  models/metadatamodel.cpp
  docks/filtersdock.cpp
  dialogs/customprofiledialog.cpp
  qmltypes/colorpickeritem.cpp
  qmltypes/colorwheelitem.cpp
  qmltypes/qmlapplication.cpp
  qmltypes/qmlfile.cpp
  qmltypes/qmlfilter.cpp
  qmltypes/qmlmetadata.cpp
  qmltypes/timelineitems.cpp
  qmltypes/qmlprofile.cpp
  settings.cpp
  widgets/lineeditclear.cpp
  leapnetworklistener.cpp
  database.cpp
  widgets/gltestwidget.cpp
  models/multitrackmodel.cpp
  docks/timelinedock.cpp
  qmltypes/qmlutilities.cpp
  qmltypes/qmlview.cpp
  qmltypes/thumbnailprovider.cpp
  commands/markercommands.cpp
  commands/timelinecommands.cpp
  util.cpp
  widgets/lumamixtransition.cpp
  autosavefile.cpp
  widgets/directshowvideowidget.cpp
  jobs/abstractjob.cpp
  jobs/meltjob.cpp
  jobs/encodejob.cpp
  jobs/postjobaction.cpp
  jobs/videoqualityjob.cpp
  commands/playlistcommands.cpp
  docks/scopedock.cpp
  controllers/scopecontroller.cpp
  widgets/scopes/scopewidget.cpp
  widgets/scopes/audioloudnessscopewidget.cpp
  widgets/scopes/audiopeakmeterscopewidget.cpp
  widgets/scopes/audiospectrumscopewidget.cpp
  widgets/scopes/audiowaveformscopewidget.cpp
  widgets/scopes/videohistogramscopewidget.cpp
  widgets/scopes/videorgbparadescopewidget.cpp
  widgets/scopes/videorgbwaveformscopewidget.cpp
  widgets/scopes/videovectorscopewidget.cpp
  widgets/scopes/videowaveformscopewidget.cpp
  widgets/scopes/videozoomscopewidget.cpp
  widgets/scopes/videozoomwidget.cpp
  sharedframe.cpp
  widgets/audioscale.cpp
  widgets/playlisttable.cpp
  widgets/playlisticonview.cpp
  commands/undohelper.cpp
  models/audiolevelstask.cpp
  mltxmlchecker.cpp
  widgets/avfoundationproducerwidget.cpp
  widgets/gdigrabwidget.cpp
  widgets/trackpropertieswidget.cpp
  widgets/timelinepropertieswidget.cpp
  jobs/ffprobejob.cpp
  jobs/ffmpegjob.cpp
  dialogs/unlinkedfilesdialog.cpp
  dialogs/transcodedialog.cpp
  docks/keyframesdock.cpp
  docks/markersdock.cpp
  docks/notesdock.cpp
  qmltypes/qmlproducer.cpp
  models/keyframesmodel.cpp
  models/markersmodel.cpp
  widgets/editmarkerwidget.cpp
  widgets/slideshowgeneratorwidget.cpp
  widgets/textproducerwidget.cpp
  dialogs/listselectiondialog.cpp
  dialogs/longuitask.cpp
  widgets/newprojectfolder.cpp
  widgets/playlistlistview.cpp
  # header only
  shotcut_mlt_properties.h
  transportcontrol.h
  # UI
  mainwindow.ui
  dialogs/systemsyncdialog.ui
  openotherdialog.ui
  widgets/plasmawidget.ui
  widgets/lissajouswidget.ui
  widgets/isingwidget.ui
  widgets/video4linuxwidget.ui
  widgets/colorproducerwidget.ui
  widgets/decklinkproducerwidget.ui
  widgets/networkproducerwidget.ui
  widgets/colorbarswidget.ui
  widgets/countproducerwidget.ui
  widgets/noisewidget.ui
  widgets/pulseaudiowidget.ui
  widgets/jackproducerwidget.ui
  widgets/toneproducerwidget.ui
  widgets/alsawidget.ui
  widgets/x11grabwidget.ui
  widgets/servicepresetwidget.ui
  widgets/avformatproducerwidget.ui
  widgets/imageproducerwidget.ui
  widgets/blipproducerwidget.ui
  docks/recentdock.ui
  docks/encodedock.ui
  dialogs/addencodepresetdialog.ui
  docks/jobsdock.ui
  dialogs/textviewerdialog.ui
  docks/playlistdock.ui
  dialogs/durationdialog.ui
  dialogs/customprofiledialog.ui
  docks/timelinedock.ui
  widgets/lumamixtransition.ui
  widgets/directshowvideowidget.ui
  widgets/avfoundationproducerwidget.ui
  widgets/gdigrabwidget.ui
  widgets/trackpropertieswidget.ui
  widgets/timelinepropertieswidget.ui
  dialogs/unlinkedfilesdialog.ui
  dialogs/transcodedialog.ui
  widgets/textproducerwidget.ui
  dialogs/listselectiondialog.ui
  widgets/newprojectfolder.ui
  # QRC
  ../icons/resources.qrc
)

add_custom_target(OTHER_FILES
  SOURCES
  ../.github/workflows/build-linux.yml
  ../.github/workflows/build-linux-unstable.yml
  ../.github/workflows/build-macos.yml
  ../.github/workflows/build-macos-unstable.yml
  ../.github/workflows/build-sdk-windows.yml
  ../.github/workflows/build-sdk-windows-unstable.yml
  ../.github/workflows/build-windows.yml
  ../.github/workflows/build-windows-unstable.yml
  ../COPYING
  ../README.md
  ../packaging/windows/shotcut.rc
  ../scripts/build-shotcut.sh
  ../scripts/build-shotcut-msys2.sh
  ../packaging/macos/shotcut.icns
  ../packaging/windows/shotcut.nsi
  ../packaging/macos/Info.plist
  ../icons/dark/index.theme
  ../icons/light/index.theme
  ../packaging/linux/Makefile
  ../packaging/linux/appimage/appimage.yml
  ../packaging/linux/snapcraft.yaml.in
  ../packaging/linux/org.shotcut.Shotcut.metainfo.xml
  ../packaging/linux/org.shotcut.Shotcut.desktop
  ../packaging/linux/org.shotcut.Shotcut.xml
  ../packaging/linux/shotcut.1
  ../.github/ISSUE_TEMPLATE.md
  ../scripts/codesign_and_notarize.sh
  ../scripts/notarize.sh
  ../scripts/staple.sh
)

target_link_libraries(shotcut
  PRIVATE
  Qt5::Widgets
  Qt5::Network
  Qt5::QuickControls2
  Qt5::QuickWidgets
  Qt5::Xml
  Qt5::WebSockets
  Qt5::OpenGL
  Qt5::Sql
  Qt5::Multimedia
  PkgConfig::mlt++
  CuteLogger
)

target_include_directories(shotcut PRIVATE ${CMAKE_SOURCE_DIR}/CuteLogger/include)
target_compile_definitions(shotcut PRIVATE SHOTCUT_VERSION="${SHOTCUT_VERSION}")

if(WIN32)
  # Windows resource
  string(REPLACE . , PRODUCT_VERSION ${PROJECT_VERSION})
  configure_file(${CMAKE_SOURCE_DIR}/packaging/windows/shotcut.rc.in ${CMAKE_SOURCE_DIR}/packaging/windows/shotcut.rc)
  target_sources(shotcut PRIVATE ${CMAKE_SOURCE_DIR}/packaging/windows/shotcut.rc)

  # Windows integration features
  find_package(Qt5 5.15 REQUIRED COMPONENTS WinExtras)
  target_sources(shotcut PRIVATE windowstools.cpp)
  target_link_libraries(shotcut PRIVATE Qt5::WinExtras)

  # Runtime exception handler for debug only
  target_include_directories(shotcut PRIVATE ${CMAKE_SOURCE_DIR}/drmingw/include)
  target_link_directories(shotcut PRIVATE ${CMAKE_SOURCE_DIR}/drmingw/x64/lib)
  target_link_libraries(shotcut PRIVATE debug exchndl)

  install(TARGETS shotcut RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
  install(DIRECTORY qml/ DESTINATION ${CMAKE_INSTALL_PREFIX}/share/shotcut/qml)
endif()

if(APPLE)
  target_sources(shotcut macos.mm)
  set_target_properties(shotcut PROPERTIES OUTPUT_NAME "Shotcut")
  install(TARGETS shotcut RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(DIRECTORY qml/ DESTINATION ${CMAKE_INSTALL_DATADIR}/shotcut/qml)
endif()

if(UNIX AND NOT APPLE)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/share/shotcut)
  file(CREATE_LINK ${CMAKE_SOURCE_DIR}/src/qml ${CMAKE_BINARY_DIR}/share/shotcut/qml SYMBOLIC)
  install(TARGETS shotcut RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(DIRECTORY qml/ DESTINATION ${CMAKE_INSTALL_DATADIR}/shotcut/qml)
endif()
