cmake_minimum_required(VERSION 3.12...3.31)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20")
  cmake_policy(SET CMP0115 NEW)
endif()

if(NOT DEFINED SNAPFLOW_VERSION)
  string(TIMESTAMP SNAPFLOW_VERSION "%y.%m.%d")
  # Remove leading zeros from each digit group
  string(REGEX REPLACE "0*([0-9]+)" "\\1" SNAPFLOW_VERSION "${SNAPFLOW_VERSION}")
endif()

project(snapflow
  VERSION ${SNAPFLOW_VERSION}
  DESCRIPTION "cross-platform (Qt), open-source (GPLv3) video editor"
  HOMEPAGE_URL "https://www.snapflow.org"
  LANGUAGES CXX
)

include(GNUInstallDirs)

if(WIN32)
  option(WINDOWS_DEPLOY "Install exes/libs directly to prefix (no subdir /bin)" ON)
endif()
option(CLANG_FORMAT "Enable Clang Format" ON)
option(BUILD_DOCS "Build QDoc API documentation" OFF)
option(EXTERNAL_LAUNCHERS "Whether include features to launch external programs; for example, this should be off for Flatpak due to sandbox." ON)
set(RUI_ACPX_SERVER_BIN "" CACHE FILEPATH
  "Optional built acpx-server binary to install beside Shotcut for the Rust chat panel")
option(USE_VULKAN "Whether to use Vulkan for hardware video decoding" OFF)
if (UNIX AND NOT APPLE)
  option(BUILD_MINIMAL_MEDIA_BACKEND "Whether to build the `minimal` Qt media backend for Linux/BSD" OFF)
endif()

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
  WebSockets
  Widgets
  Xml
)
if(Qt6_VERSION VERSION_GREATER_EQUAL "6.9.0")
  find_package(Qt6 6.4 REQUIRED GuiPrivate)
endif()
if(UNIX AND NOT APPLE)
  find_package(Qt6 6.4 REQUIRED COMPONENTS DBus)
  # X11 for WindowPicker (Linux/X11)
  find_package(X11 REQUIRED)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(mlt++ REQUIRED IMPORTED_TARGET mlt++-7>=7.36.0)
pkg_check_modules(FFTW IMPORTED_TARGET fftw3)
if(NOT FFTW_FOUND)
  pkg_check_modules(FFTW REQUIRED IMPORTED_TARGET fftw)
endif()

# SAP (Snapshot App Protocol) Rust layer, per
# memory/head/gen/rust-fork/02-rust-embedding.md. Additive only: builds
# ../sap-rust (a sibling crate, not nested inside this fork) via corrosion
# and exposes it as a linkable `sap_rust` CMake target (a static library,
# per Cargo.toml's `crate-type = ["rlib", "staticlib"]`). real_ffi enables
# the real Qt/C-ABI-backed Backend (src/rustbridge/sap_ffi.{h,cpp} below)
# instead of the crate's own default MockBackend.
include(FetchContent)
FetchContent_Declare(
  Corrosion
  GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
  GIT_TAG v0.5.2
)
FetchContent_MakeAvailable(Corrosion)
# CRATE_TYPES staticlib: same fix as panel-rust below and for the same
# reason -- without it, corrosion also builds sap-rust's own
# `[[bin]] name = "sap-rust"` (src/main.rs) as part of this one
# `cargo build --features real_ffi` invocation. With real_ffi on, that bin
# statically links the same crate's ffi_backend.rs (its
# `#[no_mangle] pub extern "C" fn sap_start_server` and friends), which
# call back into sap_* symbols only defined on this file's own C++ side
# (src/rustbridge/sap_ffi.cpp) -- undefined at link time for a standalone
# .exe with no C++ code linked in, breaking the whole cargo invocation
# (confirmed on a real windows-latest run: `error: could not compile
# sap-rust (bin "sap-rust")`, ~40 undefined-reference errors). Restricting
# to the staticlib matches what this CMakeLists.txt actually links below
# and what panel-rust's own corrosion_import_crate call already does.
corrosion_import_crate(MANIFEST_PATH ${CMAKE_SOURCE_DIR}/../sap-rust/Cargo.toml
  FEATURES real_ffi
  CRATE_TYPES staticlib)

# panel-rust: Rust-rendered (Slint) chat panel spike, per
# memory/head/gen/plans/rust-qt-cross-render-option-b.md. Additive only,
# same sibling-crate + corrosion pattern as sap-rust above. Renders into a
# CPU pixel buffer (no GPU texture sharing yet); composited by
# src/qmltypes/rustpanelitem.{h,cpp} via QQuickPaintedItem.
# CRATE_TYPES staticlib: only the panel_rust static library is linked below
# (src/CMakeLists.txt) -- without this, corrosion also tries to build every
# [[bin]] in the manifest, including ui-dev-viewer, which is gated behind a
# required-features = ["ui-dev-viewer"] not enabled here and fails the whole
# cargo invocation (a real CI break, see build-linux.yml run 29826369548).
corrosion_import_crate(MANIFEST_PATH ${CMAKE_SOURCE_DIR}/../panel-rust/Cargo.toml
  CRATE_TYPES staticlib)
pkg_check_modules(Fontconfig REQUIRED IMPORTED_TARGET fontconfig)

add_subdirectory(CuteLogger)
add_subdirectory(src)
add_subdirectory(translations)
if(UNIX AND NOT APPLE AND BUILD_MINIMAL_MEDIA_BACKEND)
  add_subdirectory(MinimalMediaBackend)
endif()

option(SNAPFLOW_BUILD_TESTS "Build unit tests" OFF)
if(SNAPFLOW_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

if(RUI_ACPX_SERVER_BIN)
  # panel-rust resolves an installed gateway beside the Shotcut executable,
  # while RUI_ACPX_SERVER_BIN remains available as an explicit runtime
  # override for non-standard deployments.
  if(APPLE)
    set(RUI_ACPX_INSTALL_DIR
        "${CMAKE_INSTALL_PREFIX}/Shotcut.app/Contents/MacOS")
  elseif(WIN32 AND WINDOWS_DEPLOY)
    set(RUI_ACPX_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}")
  else()
    set(RUI_ACPX_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}")
  endif()
  install(PROGRAMS "${RUI_ACPX_SERVER_BIN}"
          DESTINATION "${RUI_ACPX_INSTALL_DIR}"
          RENAME acpx-server)
endif()

add_custom_target(codespell COMMAND
  cd "${CMAKE_SOURCE_DIR}" &&
  codespell -w -q 3
  -L snapflow,uint,seeked,really,prevent
  -S export-chapters.js,export-edl.js,CuteLogger,drmingw,node_modules,moc_*,index.theme,*.ts,three.min.js,three.js,jquery.js,rangy-*.js,Makefile,snapflow.pro,*.desktop
)

if(CLANG_FORMAT)
  # Formatting may change with different versions of clang-format.
  # Test new versions before changing the allowed version here to avoid
  # accidental broad changes to formatting.
  find_package(ClangFormat 14 EXACT)
  if(ClangFormat_FOUND)
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

if(BUILD_DOCS)
  find_program(QDOC_EXECUTABLE qdoc
    PATHS
      "${Qt6_DIR}/../../.."
      "${CMAKE_PREFIX_PATH}"
    PATH_SUFFIXES bin
  )
  if(NOT QDOC_EXECUTABLE)
    message(FATAL_ERROR "qdoc not found. Install Qt documentation tools or set Qt6_DIR correctly.")
  endif()
  add_custom_target(docs
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/docs/qml-api
    COMMAND ${QDOC_EXECUTABLE} ${CMAKE_SOURCE_DIR}/docs/qml-api.qdocconf
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating QDoc API documentation"
    VERBATIM
  )
endif()
