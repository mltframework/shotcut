find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(SQLite3 REQUIRED)

add_executable(snapflow WIN32 MACOSX_BUNDLE
  abstractproducerwidget.cpp abstractproducerwidget.h
  actions.cpp actions.h
  autosavefile.cpp autosavefile.h
  commands/filtercommands.cpp commands/filtercommands.h
  commands/markercommands.cpp commands/markercommands.h
  commands/playlistcommands.cpp commands/playlistcommands.h
  commands/subtitlecommands.cpp commands/subtitlecommands.h
  commands/timelinecommands.cpp commands/timelinecommands.h
  commands/undohelper.cpp commands/undohelper.h
  controllers/addonmetadataparser.cpp controllers/addonmetadataparser.h
  controllers/addonqmlgenerator.cpp controllers/addonqmlgenerator.h
  controllers/filtercontroller.cpp controllers/filtercontroller.h
  controllers/scopecontroller.cpp controllers/scopecontroller.h
  database.cpp database.h
  dialogs/actionsdialog.cpp dialogs/actionsdialog.h
  dialogs/addencodepresetdialog.cpp dialogs/addencodepresetdialog.h
  dialogs/addencodepresetdialog.ui
  dialogs/alignaudiodialog.cpp dialogs/alignaudiodialog.h
  dialogs/alignmentarray.cpp dialogs/alignmentarray.h
  dialogs/addonfiltersdialog.cpp dialogs/addonfiltersdialog.h
  dialogs/addonmetadatahelpdialog.cpp dialogs/addonmetadatahelpdialog.h
  dialogs/bitratedialog.h dialogs/bitratedialog.cpp
  dialogs/customprofiledialog.cpp dialogs/customprofiledialog.h
  dialogs/customprofiledialog.ui
  dialogs/durationdialog.cpp dialogs/durationdialog.h
  dialogs/durationdialog.ui
  dialogs/editmarkerdialog.cpp dialogs/editmarkerdialog.h
  dialogs/filedatedialog.cpp dialogs/filedatedialog.h
  dialogs/filedownloaddialog.cpp dialogs/filedownloaddialog.h
  dialogs/listselectiondialog.cpp dialogs/listselectiondialog.h
  dialogs/listselectiondialog.ui
  dialogs/longuitask.cpp dialogs/longuitask.h
  dialogs/multifileexportdialog.cpp dialogs/multifileexportdialog.h
  dialogs/resourcedialog.cpp dialogs/resourcedialog.h
  dialogs/saveimagedialog.cpp dialogs/saveimagedialog.h
  dialogs/slideshowgeneratordialog.cpp dialogs/slideshowgeneratordialog.h
  dialogs/speechdialog.h dialogs/speechdialog.cpp
  dialogs/subtitletrackdialog.cpp dialogs/subtitletrackdialog.h
  dialogs/systemsyncdialog.cpp dialogs/systemsyncdialog.h
  dialogs/systemsyncdialog.ui
  dialogs/textviewerdialog.cpp dialogs/textviewerdialog.h
  dialogs/textviewerdialog.ui
  dialogs/transcodedialog.cpp dialogs/transcodedialog.h
  dialogs/transcodedialog.ui
  dialogs/transcribeaudiodialog.cpp dialogs/transcribeaudiodialog.h
  dialogs/unlinkedfilesdialog.cpp dialogs/unlinkedfilesdialog.h
  dialogs/unlinkedfilesdialog.ui
  docks/elementsdock.cpp docks/elementsdock.h
  docks/encodedock.cpp docks/encodedock.h
  docks/encodedock.ui
  docks/filesdock.cpp docks/filesdock.h
  docks/filesdock.ui
  docks/chatrustdock.cpp docks/chatrustdock.h
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
  docks/subtitlesdock.cpp docks/subtitlesdock.h
  docks/timelinedock.cpp docks/timelinedock.h
  FlatpakWrapperGenerator.cpp FlatpakWrapperGenerator.h
  gpuinfo.cpp gpuinfo.h
  htmlgenerator.h htmlgenerator.cpp
  jobqueue.cpp jobqueue.h
  jobs/abstractjob.cpp jobs/abstractjob.h
  jobs/bitrateviewerjob.h jobs/bitrateviewerjob.cpp
  jobs/dockerpulljob.h jobs/dockerpulljob.cpp
  jobs/encodejob.cpp jobs/encodejob.h
  jobs/ffmpegjob.cpp jobs/ffmpegjob.h
  jobs/ffprobejob.cpp jobs/ffprobejob.h
  jobs/gopro2gpxjob.cpp jobs/gopro2gpxjob.h
  jobs/htmlgeneratorjob.cpp jobs/htmlgeneratorjob.h
  jobs/kokorodokijob.cpp jobs/kokorodokijob.h
  jobs/meltjob.cpp jobs/meltjob.h
  jobs/postjobaction.cpp jobs/postjobaction.h
  jobs/qimagejob.cpp jobs/qimagejob.h
  jobs/screencapturejob.cpp jobs/screencapturejob.h
  jobs/videoqualityjob.cpp jobs/videoqualityjob.h
  jobs/whisperjob.cpp jobs/whisperjob.h
  main.cpp
  mainwindow.cpp mainwindow.h
  mainwindow.ui
  mltcontroller.cpp mltcontroller.h
  mltxmlchecker.cpp mltxmlchecker.h
  models/actionsmodel.cpp models/actionsmodel.h
  models/addonservicemodel.cpp models/addonservicemodel.h
  models/alignclipsmodel.cpp models/alignclipsmodel.h
  models/attachedfiltersmodel.cpp models/attachedfiltersmodel.h
  models/audiolevelstask.cpp models/audiolevelstask.h
  models/elementsmodel.cpp models/elementsmodel.h
  models/extensionmodel.cpp models/extensionmodel.h
  models/keyframesmodel.cpp models/keyframesmodel.h
  models/markersmodel.cpp models/markersmodel.h
  models/metadatamodel.cpp models/metadatamodel.h
  models/motiontrackermodel.h models/motiontrackermodel.cpp
  models/multitrackmodel.cpp models/multitrackmodel.h
  models/playlistmodel.cpp models/playlistmodel.h
  models/resourcemodel.cpp models/resourcemodel.h
  models/subtitles.cpp models/subtitles.h
  models/subtitlesmodel.cpp models/subtitlesmodel.h
  models/subtitlesselectionmodel.cpp models/subtitlesselectionmodel.h
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
  qmltypes/qmlextension.cpp qmltypes/qmlextension.h
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
  qmltypes/rustpanelitem.cpp qmltypes/rustpanelitem.h
  qmltypes/thumbnailprovider.cpp qmltypes/thumbnailprovider.h
  qmltypes/timelineitems.cpp qmltypes/timelineitems.h
  resources.qrc
  rustbridge/sap_ffi.cpp rustbridge/sap_ffi.h
  scrubbar.cpp scrubbar.h
  settings.cpp settings.h
  sharedframe.cpp sharedframe.h
  snapflow_mlt_properties.h
  transcoder.cpp transcoder.h
  screencapture/rectangleselector.cpp
  screencapture/rectangleselector.h
  screencapture/screencapture.cpp
  screencapture/screencapture.h
  screencapture/toolbarwidget.cpp
  screencapture/toolbarwidget.h
  screencapture/windowpicker.cpp
  screencapture/windowpicker.h
  spatialmedia/box.cpp spatialmedia/box.h
  spatialmedia/container.cpp spatialmedia/container.h
  spatialmedia/mpeg4_container.cpp spatialmedia/mpeg4_container.h
  spatialmedia/sa3d.cpp spatialmedia/sa3d.h
  spatialmedia/spatialmedia.cpp spatialmedia/spatialmedia.h
  transportcontrol.h
  urls.h
  util.cpp util.h
  videowidget.cpp videowidget.h
  hdrpreviewwindow.cpp hdrpreviewwindow.h
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
  widgets/flowlayout.cpp widgets/flowlayout.h
  widgets/frameratewidget.cpp widgets/frameratewidget.h
  widgets/glaxnimateproducerwidget.cpp widgets/glaxnimateproducerwidget.h
  widgets/glaxnimateproducerwidget.ui
  widgets/htmlgeneratorwidget.cpp widgets/htmlgeneratorwidget.h
  widgets/htmlgeneratorwidget.ui
  widgets/imageproducerwidget.cpp widgets/imageproducerwidget.h
  widgets/imageproducerwidget.ui
  widgets/isingwidget.cpp widgets/isingwidget.h
  widgets/isingwidget.ui
  widgets/lineeditclear.cpp widgets/lineeditclear.h
  widgets/lissajouswidget.cpp widgets/lissajouswidget.h
  widgets/lissajouswidget.ui
  widgets/lumamixtransition.cpp widgets/lumamixtransition.h
  widgets/lumamixtransition.ui
  widgets/mltclipproducerwidget.cpp widgets/mltclipproducerwidget.h
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
  widgets/scopes/audiosurroundscopewidget.cpp widgets/scopes/audiosurroundscopewidget.h
  widgets/scopes/audiovectorscopewidget.cpp widgets/scopes/audiovectorscopewidget.h
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
  ../packaging/linux/org.snapflow.Snapflow.desktop
  ../packaging/linux/org.snapflow.Snapflow.metainfo.xml.in
  ../packaging/linux/org.snapflow.Snapflow.xml
  ../packaging/linux/snapflow.1
  ../packaging/linux/snapcraft.yaml.in
  ../packaging/macos/Info.plist.in
  ../packaging/macos/snapflow.icns
  ../packaging/windows/snapflow.iss
  ../packaging/windows/snapflow.rc.in
  ../README.md
  ../scripts/build-snapflow-msys2.sh
  ../scripts/build-snapflow.sh
  ../scripts/codesign_and_notarize.sh
  ../scripts/notarize.sh
  ../scripts/staple.sh
)

# Compile HDR gain shader (used by HdrPreview.qml ShaderEffect)
find_program(QSB_EXECUTABLE qsb HINTS
  "${Qt6_DIR}/../../../bin" "${Qt6Core_DIR}/../../../bin")
if(QSB_EXECUTABLE)
  set(HDR_GAIN_FRAG ${CMAKE_CURRENT_SOURCE_DIR}/qml/views/hdr_gain.frag)
  set(HDR_GAIN_QSB ${CMAKE_CURRENT_SOURCE_DIR}/qml/views/hdr_gain.frag.qsb)
  if(NOT EXISTS ${HDR_GAIN_QSB})
    add_custom_command(
      OUTPUT ${HDR_GAIN_QSB}
      COMMAND ${QSB_EXECUTABLE} --glsl "100 es,120,150" --hlsl 50 --msl 12 -o ${HDR_GAIN_QSB} ${HDR_GAIN_FRAG}
      DEPENDS ${HDR_GAIN_FRAG}
      COMMENT "Compiling HDR gain shader"
    )
  endif()
  add_custom_target(hdr_shaders ALL DEPENDS ${HDR_GAIN_QSB})
  add_dependencies(snapflow hdr_shaders)
endif()

# panel_rust is a static Rust archive; its rusqlite symbols are encountered
# after CMake's imported SQLite target in the final link line.
target_link_options(snapflow PRIVATE "LINKER:--no-as-needed")

target_link_libraries(snapflow
  PRIVATE
  CuteLogger
  PkgConfig::mlt++
  PkgConfig::FFTW
  Qt6::Charts
  Qt6::GuiPrivate
  Qt6::Multimedia
  Qt6::Network
  Qt6::OpenGL
  Qt6::OpenGLWidgets
  Qt6::QuickControls2
  Qt6::QuickWidgets
  Qt6::Sql
  SQLite3::SQLite3
  PkgConfig::Fontconfig
  Qt6::WebSockets
  Qt6::Widgets
  Qt6::Xml
)

# sap_rust/panel_rust link separately (not in the big PRIVATE list above)
# so Windows can wrap them with bcrypt (needed for getrandom's
# BCryptGenRandom) in a $<LINK_GROUP:RESCAN,...>. Two prior attempts to
# just add `bcrypt` as a plain link library -- once directly on snapflow,
# once as an INTERFACE requirement on the crate targets (mirroring how
# Corrosion's own kernel32/ntdll/dbghelp land correctly) -- both compiled
# but still failed at actual link time on a real windows-latest run:
# CMake's link-line assembly placed `-lbcrypt` before whichever archive
# actually referenced BCryptGenRandom, and GNU ld's single left-to-right
# pass never revisited it. LINK_GROUP:RESCAN (CMake >= 3.24) wraps its
# arguments in `-Wl,--start-group ... --end-group`, which makes the
# linker rescan the group until every symbol resolves regardless of
# ordering -- immune to the reordering that broke the first two attempts.
if(WIN32)
  target_link_libraries(snapflow PRIVATE "$<LINK_GROUP:RESCAN,sap_rust,panel_rust,bcrypt>")
else()
  target_link_libraries(snapflow PRIVATE sap_rust panel_rust)
endif()
if(UNIX AND NOT APPLE)
  target_link_libraries(snapflow PRIVATE Qt6::DBus X11::X11)
  target_sources(snapflow PRIVATE linuxtools.cpp linuxtools.h)
endif()

file(GLOB_RECURSE QML_SRC "qml/*")
target_sources(snapflow PRIVATE ${QML_SRC})

target_include_directories(snapflow PRIVATE ${CMAKE_SOURCE_DIR}/CuteLogger/include)
target_compile_definitions(snapflow PRIVATE SNAPFLOW_VERSION="${SNAPFLOW_VERSION}")

# Add compile definitions when certain custom cache variables are ON
if(EXTERNAL_LAUNCHERS)
  target_compile_definitions(snapflow PRIVATE EXTERNAL_LAUNCHERS)
endif()
if(USE_VULKAN)
  target_compile_definitions(snapflow PRIVATE USE_VULKAN)
endif()
if(BUILD_MINIMAL_MEDIA_BACKEND)
  target_compile_definitions(snapflow PRIVATE BUILD_MINIMAL_MEDIA_BACKEND)
endif()

if(WIN32)
  # Windows resource
  string(REPLACE . , PRODUCT_VERSION ${PROJECT_VERSION})
  configure_file(${CMAKE_SOURCE_DIR}/packaging/windows/snapflow.rc.in ${CMAKE_SOURCE_DIR}/packaging/windows/snapflow.rc)
  target_sources(snapflow PRIVATE ${CMAKE_SOURCE_DIR}/packaging/windows/snapflow.rc)

  # Windows integration features
  target_sources(snapflow PRIVATE windowstools.cpp windowstools.h)
  target_sources(snapflow PRIVATE widgets/d3dvideowidget.h widgets/d3dvideowidget.cpp)
  target_sources(snapflow PRIVATE widgets/openglvideowidget.h widgets/openglvideowidget.cpp)
  target_link_libraries(snapflow PRIVATE d3d11 d3dcompiler dxgi dxguid ole32)

  # Runtime exception handler for debug only
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
    target_include_directories(snapflow PRIVATE ${CMAKE_SOURCE_DIR}/drmingw/include)
    target_link_directories(snapflow PRIVATE ${CMAKE_SOURCE_DIR}/drmingw/x64/lib)
    target_link_libraries(snapflow PRIVATE debug exchndl)
  endif()

  if(WINDOWS_DEPLOY)
    install(TARGETS snapflow RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
  else()
    target_compile_definitions(snapflow PRIVATE NODEPLOY)
    install(TARGETS snapflow RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

  install(DIRECTORY qml DESTINATION ${CMAKE_INSTALL_PREFIX}/share/snapflow/)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/filter-sets DESTINATION ${CMAKE_INSTALL_PREFIX}/share/snapflow/)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/voices DESTINATION ${CMAKE_INSTALL_PREFIX}/share/snapflow/)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/resources DESTINATION ${CMAKE_INSTALL_PREFIX}/share/snapflow/)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/elements DESTINATION ${CMAKE_INSTALL_PREFIX}/share/snapflow/)
else()
  target_sources(snapflow PRIVATE widgets/openglvideowidget.h widgets/openglvideowidget.cpp)
endif()

if(APPLE)
  target_sources(snapflow PRIVATE macos.mm macos.h
    widgets/metalvideowidget.h widgets/metalvideowidget.mm)
  set_target_properties(snapflow PROPERTIES
    OUTPUT_NAME "Snapflow"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/packaging/macos/Info.plist.in)
  find_library(FOUNDATION Foundation)
  find_library(COCOA Cocoa)
  target_link_libraries(snapflow PRIVATE ${FOUNDATION} ${COCOA})
  set(APP_ICON ${CMAKE_SOURCE_DIR}/packaging/macos/snapflow.icns)
  target_sources(snapflow PRIVATE ${APP_ICON})
  set_source_files_properties(${APP_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

  # Create a symlink to the qml folder after building
  # These are skipped for release because it breaks the app bundling and is only for development.
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Snapflow.app/Contents/Resources/snapflow)
    file(CREATE_LINK ${CMAKE_SOURCE_DIR}/src/qml ${CMAKE_CURRENT_BINARY_DIR}/Snapflow.app/Contents/Resources/snapflow/qml SYMBOLIC)
    file(CREATE_LINK ${CMAKE_SOURCE_DIR}/filter-sets ${CMAKE_CURRENT_BINARY_DIR}/Snapflow.app/Contents/Resources/snapflow/filter-sets SYMBOLIC)
    file(CREATE_LINK ${CMAKE_SOURCE_DIR}/voices ${CMAKE_CURRENT_BINARY_DIR}/Snapflow.app/Contents/Resources/snapflow/voices SYMBOLIC)
    file(CREATE_LINK ${CMAKE_SOURCE_DIR}/elements ${CMAKE_CURRENT_BINARY_DIR}/Snapflow.app/Contents/Resources/snapflow/elements SYMBOLIC)
  endif()

  install(TARGETS snapflow BUNDLE DESTINATION ${CMAKE_INSTALL_PREFIX})
  install(DIRECTORY qml DESTINATION ${CMAKE_INSTALL_PREFIX}/Snapflow.app/Contents/Resources/snapflow/)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/filter-sets DESTINATION ${CMAKE_INSTALL_PREFIX}/Snapflow.app/Contents/Resources/snapflow/)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/voices DESTINATION ${CMAKE_INSTALL_PREFIX}/Snapflow.app/Contents/Resources/snapflow/)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/elements DESTINATION ${CMAKE_INSTALL_PREFIX}/Snapflow.app/Contents/Resources/snapflow/)
endif()

if(UNIX AND NOT APPLE)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/share/snapflow)
  file(CREATE_LINK ${CMAKE_SOURCE_DIR}/src/qml ${CMAKE_BINARY_DIR}/share/snapflow/qml SYMBOLIC)
  file(CREATE_LINK ${CMAKE_SOURCE_DIR}/filter-sets ${CMAKE_BINARY_DIR}/share/snapflow/filter-sets SYMBOLIC)
  file(CREATE_LINK ${CMAKE_SOURCE_DIR}/voices ${CMAKE_BINARY_DIR}/share/snapflow/voices SYMBOLIC)
  file(CREATE_LINK ${CMAKE_SOURCE_DIR}/elements ${CMAKE_BINARY_DIR}/share/snapflow/elements SYMBOLIC)
  include(GNUInstallDirs)
  install(TARGETS snapflow RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(DIRECTORY qml DESTINATION ${CMAKE_INSTALL_DATADIR}/snapflow/)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/filter-sets DESTINATION ${CMAKE_INSTALL_DATADIR}/snapflow/)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/voices DESTINATION ${CMAKE_INSTALL_DATADIR}/snapflow/)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/elements DESTINATION ${CMAKE_INSTALL_DATADIR}/snapflow/)

  string(REPLACE . - SNAPFLOW_DATE  ${SNAPFLOW_VERSION})
  string(PREPEND SNAPFLOW_DATE "20")
  configure_file(${CMAKE_SOURCE_DIR}/packaging/linux/org.snapflow.Snapflow.metainfo.xml.in
    ${CMAKE_BINARY_DIR}/packaging/linux/org.snapflow.Snapflow.metainfo.xml)
  install(FILES ${CMAKE_BINARY_DIR}/packaging/linux/org.snapflow.Snapflow.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo/)
  install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/org.snapflow.Snapflow.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications/)
  install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/org.snapflow.Snapflow.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages/)
  install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/icons/64x64/org.snapflow.Snapflow.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps/)
  install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/icons/128x128/org.snapflow.Snapflow.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps/)
  install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/snapflow.1
    DESTINATION ${CMAKE_INSTALL_DATADIR}/man/man1/)
endif()
