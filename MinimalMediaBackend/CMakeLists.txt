cmake_minimum_required(VERSION 3.12...3.31)

project(MinimalMediaBackend)

find_package(Qt6 REQUIRED COMPONENTS Core Multimedia)
find_package(Qt6MultimediaPrivate REQUIRED)

qt_add_plugin(minimalmediaplugin
  CLASS_NAME MinimalMediaPlugin
  PLUGIN_TYPE multimedia
  OUTPUT_TARGETS minimalmediaplugin_targets
  minimalmediaplugin.cpp
)

target_link_libraries(minimalmediaplugin PRIVATE
  Qt6::Core
  Qt6::Multimedia
  Qt6::MultimediaPrivate
)

# Running from the build tree (build/cc-debug/src/shotcut) adds
# <build>/lib/qt6 to QT_PLUGIN_PATH. Official installs copy this plugin
# there; without it QT_MEDIA_BACKEND=minimal cannot create QVideoSink
# and the player VideoOutput stays blank.
add_custom_command(TARGET minimalmediaplugin POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
    "${CMAKE_BINARY_DIR}/lib/qt6/multimedia"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:minimalmediaplugin>"
    "${CMAKE_BINARY_DIR}/lib/qt6/multimedia/$<TARGET_FILE_NAME:minimalmediaplugin>"
)
