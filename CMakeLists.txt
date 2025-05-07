cmake_minimum_required(VERSION 3.12...3.31)

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

if(WIN32)
  option(WINDOWS_DEPLOY "Install exes/libs directly to prefix (no subdir /bin)" ON)
endif()
option(CLANG_FORMAT "Enable Clang Format" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FeatureSummary)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 6.4 REQUIRED
  COMPONENTS
  Charts
  Multimedia
  Network
  OpenGL
  OpenGLWidgets
  QuickControls2
  QuickWidgets
  Sql
  Widgets
  Xml
)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_package(Qt6 6.4 REQUIRED COMPONENTS DBus)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(mlt++ REQUIRED IMPORTED_TARGET mlt++-7>=7.32.0)
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

if(CLANG_FORMAT)
  # Formatting may change with different versions of clang-format.
  # Test new versions before changing the allowed version here to avoid
  # accidental broad changes to formatting.
  find_package(ClangFormat 14 EXACT)
  if(CLANGFORMAT_FOUND)
    file(GLOB_RECURSE FORMAT_FILES "src/*.h" "src/*.c" "src/*.cpp")
    # exclude 3rd party & generated source from format checking
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/spatialmedia/")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/defaultlayouts.h")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/*_autogen/")
    add_custom_target(clang-format COMMAND
      ${CLANGFORMAT_EXECUTABLE} -style=file -i ${FORMAT_FILES}
    )
    add_custom_target(clang-format-check COMMAND
      ${CLANGFORMAT_EXECUTABLE} --dry-run --Werror -style=file -i ${FORMAT_FILES}
    )
  else()
    set(CLANG_FORMAT OFF)
  endif()
endif()

file(GLOB_RECURSE QML_SRC "src/qml/*.qml")
add_custom_target(qmlformat COMMAND
  ${CMAKE_PREFIX_PATH}/bin/qmlformat -i ${QML_SRC}
)
