cmake_minimum_required(VERSION 3.13)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20")
  cmake_policy(SET CMP0115 NEW)
endif()

if(NOT DEFINED SHOTCUT_VERSION)
  string(TIMESTAMP SHOTCUT_VERSION "%y.%m.%d")
endif()

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
pkg_check_modules(FFTW IMPORTED_TARGET fftw3)
if(NOT FFTW_FOUND)
  pkg_check_modules(FFTW REQUIRED IMPORTED_TARGET fftw)
endif()

add_subdirectory(CuteLogger)
add_subdirectory(src)
add_subdirectory(translations)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

add_custom_target(codespell COMMAND
  cd "${CMAKE_SOURCE_DIR}" &&
  codespell -w -q 3
  -L shotcut,uint,seeked,really,prevent
  -S export-chapters.js,export-edl.js,CuteLogger,drmingw,node_modules,moc_*,index.theme,*.ts,three.min.js,three.js,jquery.js,rangy-*.js,Makefile,shotcut.pro,*.desktop
)

add_custom_target(astyle COMMAND
  cd "${CMAKE_SOURCE_DIR}" &&
  astyle --project=.astylerc --ignore-exclude-errors --recursive "./*.cpp,*.h"
)

