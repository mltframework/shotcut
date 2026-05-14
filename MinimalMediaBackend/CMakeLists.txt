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

install(TARGETS minimalmediaplugin
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/plugins/multimedia
)
