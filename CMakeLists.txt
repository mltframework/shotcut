cmake_minimum_required(VERSION 3.13)

string(TIMESTAMP SHOTCUT_VERSION "%y.%m.%d")

project(shotcut
  VERSION ${SHOTCUT_VERSION}
  DESCRIPTION "cross-platform (Qt), open-source (GPLv3) video editor"
  HOMEPAGE_URL "https://www.shotcut.org"
  LANGUAGES CXX
)

include(FeatureSummary)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 5.15 REQUIRED
  COMPONENTS
  Widgets
  Network
  QuickControls2
  QuickWidgets
  Xml
  WebSockets
  OpenGL
  Sql
  Multimedia
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(mlt++ REQUIRED IMPORTED_TARGET mlt++-7)

add_subdirectory(CuteLogger)
add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
