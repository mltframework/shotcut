#!/bin/bash

# This script builds shotcut and many of its dependencies.
# It can accept a configuration file, default: build-shotcut.conf

# List of programs used:
# bash, test, tr, awk, ps, make, cmake, ninja, cat, sed, curl or wget, and possibly others

# Author: Dan Dennedy <dan@dennedy.org>
# License: GPL2

################################################################################
# ARGS AND GLOBALS
################################################################################

# These are all of the configuration variables with defaults
INSTALL_DIR="$HOME/build"
AUTO_APPEND_DATE=0
SOURCE_DIR="$INSTALL_DIR/src"
ACTION_GET=1
ACTION_CONFIGURE=1
ACTION_COMPILE_INSTALL=1
ACTION_CLEAN_SOURCE=0
DEBUG_BUILD=0
ASAN_BUILD=0
DEPLOY=1
ENABLE_FREI0R=1
FREI0R_HEAD=0
FREI0R_REVISION="36e7da5d9e1f8b8cac9e97e204db1c5834fee580"
SUBDIRS=
ENABLE_MOVIT=1
MOVIT_HEAD=0
MOVIT_REVISION="origin/shotcut-opengl3"
FFMPEG_HEAD=0
FFMPEG_REVISION="origin/release/7.1"
FFMPEG_ADDITIONAL_OPTIONS=
MLT_HEAD=1
MLT_REVISION=
LOG_COLORS=0
SHOTCUT_HEAD=1
SHOTCUT_REVISION=
SHOTCUT_VERSION=$(date '+%y.%m.%d')
ENABLE_BIGSH0T=1
BIGSH0T_HEAD=0
BIGSH0T_REVISION="8fe56f6d4e"
DAV1D_HEAD=0
DAV1D_REVISION="1.4.2"
AOM_HEAD=0
AOM_REVISION="v3.8.0"
ENABLE_VMAF=1
VMAF_HEAD=0
VMAF_REVISION="v3.0.0"
ENABLE_GLAXNIMATE=1
GLAXNIMATE_HEAD=0
GLAXNIMATE_REVISION="origin/v0.5.4"
ENABLE_GOPRO2GPX=1
ENABLE_OPENCV=1
OPENCV_HEAD=0
OPENCV_REVISION="4.9.0"
ENABLE_LIBSPATIALAUDIO=1
LIBSPATIALAUDIO_HEAD=1
LIBSPATIALAUDIO_REVISION=
ENABLE_LADSPA=1
LADSPA_HEAD=0
LADSPA_REVISION="origin/shotcut"
MLT_DISABLE_SOX=0
ENABLE_WHISPERCPP=1
WHISPERCPP_HEAD=1
WHISPERCPP_REVISION=

################################################################################
# Location of config file - if not overridden on command line
CONFIGFILE=build-shotcut.conf

# If defined to 1, outputs trace log lines
TRACE=0

# If defined to 1, outputs debug log lines
DEBUG=0

# We need to set LANG to C to avoid e.g. svn from getting to funky
export LANG=C

################################################################################
# FUNCTION SECTION
################################################################################

#################################################################
# usage
# Reports legal options to this script
function usage {
  echo "Usage: $0 [-c config-file] [-h] [-s] [-v shotcut-version]"
  echo "Where:"
  echo -e "\t-c config-file\tDefaults to $CONFIGFILE"
  echo -e "\t-s\t\tbuild SDK"
  echo -e "\t-v shotcut-version\t\tSet the Shotcut version; defaults to $SHOTCUT_VERSION"
}

#################################################################
# parse_args
# Parses the arguments passed in $@ and sets some global vars
function parse_args {
  CONFIGFILEOPT=""
  while getopts ":tsc:a:o:v:" OPT; do
    case $OPT in
      a ) TARGET_ARCH=$OPTARG;;
      c ) CONFIGFILEOPT=$OPTARG
          echo Setting configfile to $CONFIGFILEOPT
      ;;
      s ) SDK=1;;
      h ) usage
          exit 0;;
      v ) SHOTCUT_VERSION=$OPTARG;;
      * ) echo "Unknown option $OPT"
          usage
          exit 1;;
    esac
  done

  # Check configfile
  if test "$CONFIGFILEOPT" != ""; then
    if test ! -r "$CONFIGFILEOPT" ; then
      echo "Unable to read config-file $CONFIGFILEOPT"
      exit 1
    fi
    CONFIGFILE="$CONFIGFILEOPT"
  fi
}
######################################################################
# DATA HANDLING FUNCTIONS
######################################################################

#################################################################
# to_key
# Returns a numeric key from a known subproject
# $1 : string: ffmpeg, mlt, etc.
function to_key {
  case $1 in
    ffmpeg)
      echo 0
    ;;
    mlt)
      echo 1
    ;;
    frei0r)
      echo 2
    ;;
    movit)
      echo 3
    ;;
    shotcut)
      echo 4
    ;;
    ladspa-swh)
      echo 5
    ;;
    nv-codec-headers)
      echo 6
    ;;
    AMF)
      echo 7
    ;;
    bigsh0t)
      echo 8
    ;;
    whisper.cpp)
      echo 9
    ;;
    dav1d)
      echo 10
    ;;
    aom)
      echo 11
    ;;
    vmaf)
      echo 12
    ;;
    glaxnimate)
      echo 13
    ;;
    gopro2gpx)
      echo 14
    ;;
    opencv)
      echo 15
    ;;
    opencv_contrib)
      echo 16
    ;;
    libspatialaudio)
      echo 17
    ;;
    *)
      echo UNKNOWN
    ;;
  esac
}

#################################################################
# lookup - lookup a value from an array and return it
# $1 array name, $2 subdir name, that is, text string
function lookup {
  eval echo "\${${1}[`to_key $2`]}"
}

######################################################################
# LOG FUNCTIONS
######################################################################

#################################################################
# init_log_file
# Write some init stuff
function init_log_file {
  log `date`
  log $0 starting
}

#################################################################
# trace
# Function that prints a trace line
# $@ : arguments to be printed
function trace {
  if test "1" = "$TRACE" ; then
    if test "1" = "$LOG_COLORS"; then
      echo -e "\e[35mTRACE:\e[0m $@"
    else
      echo "TRACE: $@"
    fi
  fi
}

#################################################################
# debug
# Function that prints a debug line
# $@ : arguments to be printed
function debug {
  if test "1" = "$DEBUG" ; then
    if test "1" = "$LOG_COLORS"; then
      echo -e "\e[34mDEBUG:\e[0m $@"
    else
      echo "DEBUG: $@"
    fi
  fi
}

#################################################################
# log
# Function that prints a log line
# $@ : arguments to be printed
function log {
  if test "1" = "$LOG_COLORS"; then
    echo -e "\e[96mLOG:\e[0m $@"
  else
    echo "LOG: $@"
  fi
}

#################################################################
# log warning
# Function that prints a warning line
# $@ : arguments to be printed
function warn {
  if test "1" = "$LOG_COLORS"; then
    echo -e "\e[33mWARN:\e[0m $@"
  else
    echo "WARN: $@"
  fi
}

#################################################################
# die
# Function that prints a line and exists
# $@ : arguments to be printed
function die {
  if test "1" = "$LOG_COLORS"; then
    echo -e "\e[31mERROR:\e[0m $@"
  else
    echo "ERROR: $@"
  fi
  feedback_result FAILURE "Some kind of error occurred: $@"
  exit -1
}

#################################################################
# cmd
# Function that does a (non-background, non-outputting) command, after logging it
function cmd {
  trace "Entering cmd @ = $@"
  log About to run command: "$@"
  "$@"
}


######################################################################
# SETUP FUNCTIONS
######################################################################

#################################################################
# read_configuration
# Reads $CONFIGFILE, parses it, and exports global variables reflecting the
# content. Aborts, if the file does not exist or is not readable
CONFIGURATION=""
function read_configuration {
  trace "Entering read_configuration @ = $@"
  if test ! -r "$CONFIGFILE"; then
    warn "Unable to read config file $CONFIGFILE"
    return
  fi
  debug "Reading configuration from $CONFIGFILE"
  # This is for replacement in startup scripts
  for LINE in `tr "\t" "=" < $CONFIGFILE`; do
    debug Setting $LINE
    CONFIGURATION="$CONFIGURATION$LINE   "
    #export $LINE || die "Invalid export line: $LINE. Unable to set configuration options from CONFIGFILE"
  done ||\
    die "Unable to set configuration options from $CONFIGFILE"
  source "$CONFIGFILE" || die "Unable to evaluate configuration options from $CONFIGFILE"
}

#################################################################
# set_globals
# Set up globals based on configuration
# This is where the configuration options for each subproject is assembled
function set_globals {
  trace "Entering set_globals @ = $@"

  # Set debug flags
  if [ "$DEBUG_BUILD" = "1" ]; then
    export CFLAGS=
    export CXXFLAGS=
  else
    export CFLAGS=-DNDEBUG
    export CXXFLAGS=-DNDEBUG
  fi
  export LDFLAGS=

  # Set Qt installation variables.
  test "$TARGET_ARCH" = "" && TARGET_ARCH=${MSYSTEM,,}
  if [ "$TARGET_ARCH" = "clangarm64" ]; then
    export QTDIR="$(pkg-config --variable=prefix Qt6Core)"
    QT_INCLUDE_DIR="$QTDIR/qt6/include"
    QT_LIB_DIR="$QTDIR/lib"
    QT_SHARE_DIR="$QTDIR/share/qt6"
  else
    export QTDIR="$HOME/Qt/6.7.2/mingw_64"
    QT_INCLUDE_DIR="$QTDIR/include"
    QT_LIB_DIR="$QTDIR/lib"
    QT_SHARE_DIR="$QTDIR"
  fi

  # Set convenience variables.
  if test 1 = "$ACTION_GET" ; then
    GET=1
  else
    GET=0
  fi
  if test 1 = "$ACTION_COMPILE_INSTALL" ; then
    COMPILE_INSTALL=1
  else
    COMPILE_INSTALL=0
  fi
  debug "GET=$GET, COMPILE_INSTALL=$COMPILE_INSTALL"

  # The script sets DEPLOY to true always, disable if not COMPILE_INSTALL
  if test 0 = "$COMPILE_INSTALL" ; then
    DEPLOY=0
  fi
  debug "DEPLOY=$DEPLOY"

  # Subdirs list, for number of common operations
  # Note, the function to_key depends on this
  if [ -z "$SUBDIRS" ]; then
    [ "$TARGET_ARCH" = "mingw64" ] && SUBDIRS="AMF nv-codec-headers"
    SUBDIRS="$SUBDIRS aom dav1d FFmpeg"

    if test "$ENABLE_MOVIT" = 1 && test "$MOVIT_HEAD" = 1 -o "$MOVIT_REVISION" != ""; then
        SUBDIRS="$SUBDIRS movit"
    fi
    if test "$ENABLE_FREI0R" = 1 ; then
        SUBDIRS="$SUBDIRS frei0r"
    fi
    if test "$ENABLE_BIGSH0T" = 1 ; then
        SUBDIRS="$SUBDIRS bigsh0t"
    fi
    if test "$ENABLE_VMAF" = 1 ; then
        SUBDIRS="vmaf $SUBDIRS"
    fi
    if test "$ENABLE_GLAXNIMATE" = 1 ; then
        SUBDIRS="$SUBDIRS glaxnimate"
    fi
    if test "$ENABLE_GOPRO2GPX" = 1 ; then
        SUBDIRS="$SUBDIRS gopro2gpx"
    fi
    if test "$ENABLE_OPENCV" = 1 ; then
        SUBDIRS="opencv opencv_contrib $SUBDIRS"
    fi
    if test "$ENABLE_LIBSPATIALAUDIO" = 1  && test "$LIBSPATIALAUDIO_HEAD" = 1 -o "$LIBSPATIALAUDIO_REVISION" != ""; then
        SUBDIRS="libspatialaudio $SUBDIRS"
    fi
    if test "$ENABLE_LADSPA" = 1 && test "$LADSPA_HEAD" = 1 -o "$LADSPA_REVISION" != ""; then
        SUBDIRS="ladspa-swh $SUBDIRS"
    fi
    if test "$ENABLE_WHISPERCPP" = 1  && test "$WHISPERCPP_HEAD" = 1 -o "$WHISPERCPP_REVISION" != ""; then
        SUBDIRS="whisper.cpp $SUBDIRS"
    fi
    SUBDIRS="$SUBDIRS mlt shotcut"
  fi

  if [ "$DEBUG_BUILD" = "1" ]; then
    CONFIGURE_DEBUG_FLAG="--enable-debug"
    QMAKE_DEBUG_FLAG="CONFIG+=debug"
    CMAKE_DEBUG_FLAG="-DCMAKE_BUILD_TYPE=Debug"
  else
    CONFIGURE_DEBUG_FLAG=
    QMAKE_DEBUG_FLAG=
    CMAKE_DEBUG_FLAG="-DCMAKE_BUILD_TYPE=Release"
  fi

  if [ "$ASAN_BUILD" = "1" ]; then
    ASAN_CFLAGS="-fsanitize=address -fno-omit-frame-pointer"
    ASAN_LDFLAGS="-lasan -fsanitize=address"
  else
    ASAN_CFLAGS=
    ASAN_LDFLAGS=
  fi

  debug "SUBDIRS = $SUBDIRS"

  # REPOLOCS Array holds the repo urls
  REPOLOCS[0]="https://github.com/FFmpeg/FFmpeg.git"
  REPOLOCS[1]="https://github.com/mltframework/mlt.git"
  REPOLOCS[2]="https://github.com/dyne/frei0r.git"
  REPOLOCS[3]="https://github.com/ddennedy/movit.git"
  REPOLOCS[4]="https://github.com/mltframework/shotcut.git"
  REPOLOCS[5]="https://github.com/ddennedy/ladspa-swh.git"
  REPOLOCS[6]="https://github.com/FFmpeg/nv-codec-headers.git"
  REPOLOCS[7]="https://github.com/GPUOpen-LibrariesAndSDKs/AMF.git"
#  REPOLOCS[8]="https://bitbucket.org/dandennedy/bigsh0t.git"
  REPOLOCS[8]="https://bitbucket.org/leo_sutic/bigsh0t.git"
  REPOLOCS[9]="https://github.com/ggerganov/whisper.cpp.git"
  REPOLOCS[10]="https://code.videolan.org/videolan/dav1d.git"
  REPOLOCS[11]="https://aomedia.googlesource.com/aom"
  REPOLOCS[12]="https://github.com/Netflix/vmaf.git"
  REPOLOCS[13]="https://gitlab.com/ddennedy/glaxnimate.git"
  REPOLOCS[14]="https://github.com/ddennedy/gopro2gpx.git"
  REPOLOCS[15]="https://github.com/opencv/opencv.git"
  REPOLOCS[16]="https://github.com/opencv/opencv_contrib.git"
  REPOLOCS[17]="https://github.com/videolabs/libspatialaudio.git"

  # REPOTYPE Array holds the repo types. (Yes, this might be redundant, but easy for me)
  REPOTYPES[0]="git"
  REPOTYPES[1]="git"
  REPOTYPES[2]="git"
  REPOTYPES[3]="git"
  REPOTYPES[4]="git"
  REPOTYPES[5]="git"
  REPOTYPES[6]="git"
  REPOTYPES[7]="git"
  REPOTYPES[8]="git"
  REPOTYPES[9]="git"
  REPOTYPES[10]="git"
  REPOTYPES[11]="git"
  REPOTYPES[12]="git"
  REPOTYPES[13]="git"
  REPOTYPES[14]="git"
  REPOTYPES[15]="git"
  REPOTYPES[16]="git"
  REPOTYPES[17]="git"

  # And, set up the revisions
  REVISIONS[0]=""
  if test 0 = "$FFMPEG_HEAD" -a "$FFMPEG_REVISION" ; then
    REVISIONS[0]="$FFMPEG_REVISION"
  fi
  REVISIONS[1]=""
  if test 0 = "$MLT_HEAD" -a "$MLT_REVISION" ; then
    REVISIONS[1]="$MLT_REVISION"
  fi
  REVISIONS[2]=""
  if test 0 = "$FREI0R_HEAD" -a "$FREI0R_REVISION" ; then
    REVISIONS[2]="$FREI0R_REVISION"
  fi
  REVISIONS[3]=""
  if test 0 = "$MOVIT_HEAD" -a "$MOVIT_REVISION" ; then
    REVISIONS[3]="$MOVIT_REVISION"
  fi
  REVISIONS[4]=""
  if test 0 = "$SHOTCUT_HEAD" -a "$SHOTCUT_REVISION" ; then
    REVISIONS[4]="$SHOTCUT_REVISION"
  fi
  REVISIONS[5]=""
  if test 0 = "$LADSPA_HEAD" -a "$LADSPA_REVISION" ; then
    REVISIONS[5]="$LADSPA_REVISION"
  fi
  REVISIONS[6]="sdk/12.0" # nv-codec-headers
  REVISIONS[7]="" # AMF
  REVISIONS[8]=""
  if test 0 = "$BIGSH0T_HEAD" -a "$BIGSH0T_REVISION" ; then
    REVISIONS[8]="$BIGSH0T_REVISION"
  fi
  REVISIONS[9]=""
  REVISIONS[10]=""
  if test 0 = "$DAV1D_HEAD" -a "$DAV1D_REVISION" ; then
    REVISIONS[10]="$DAV1D_REVISION"
  fi
  REVISIONS[11]=""
  if test 0 = "$AOM_HEAD" -a "$AOM_REVISION" ; then
    REVISIONS[11]="$AOM_REVISION"
  fi
  REVISIONS[12]=""
  if test 0 = "$VMAF_HEAD" -a "$VMAF_REVISION" ; then
    REVISIONS[12]="$VMAF_REVISION"
  fi
  REVISIONS[13]=""
  if test 0 = "$GLAXNIMATE_HEAD" -a "$GLAXNIMATE_REVISION" ; then
    REVISIONS[13]="$GLAXNIMATE_REVISION"
  fi
  REVISIONS[14]=""
  REVISIONS[15]=""
  if test 0 = "$OPENCV_HEAD" -a "$OPENCV_REVISION" ; then
    REVISIONS[15]="$OPENCV_REVISION"
  fi
  REVISIONS[16]=""
  if test 0 = "$OPENCV_HEAD" -a "$OPENCV_REVISION" ; then
    REVISIONS[16]="$OPENCV_REVISION"
  fi
  REVISIONS[17]=""
  if test 0 = "$LIBSPATIALAUDIO_HEAD" -a "$LIBSPATIALAUDIO_REVISION" ; then
    REVISIONS[17]="$LIBSPATIALAUDIO_REVISION_REVISION"
  fi

  # Figure out the number of cores in the system. Used both by make and startup script
  CPUS=$(nproc)
  # Sanity check
  if test 0 = $CPUS ; then
    CPUS=1
  fi
  MAKEJ=$(( $CPUS + 1 ))
  debug "Using make -j$MAKEJ for compilation"

  # Figure out the install dir - we may not install, but then we know it.
  FINAL_INSTALL_DIR=$INSTALL_DIR
  if test 1 = "$AUTO_APPEND_DATE" ; then
    FINAL_INSTALL_DIR="$INSTALL_DIR/`date +'%Y%m%d'`"
  else
    FINAL_INSTALL_DIR="$INSTALL_DIR/Shotcut"
  fi
  debug "Using install dir FINAL_INSTALL_DIR=$FINAL_INSTALL_DIR"

  # set global environment for all jobs
  alias make=mingw32-make
  export PKG_CONFIG_PATH="$HOME/lib/pkgconfig:$PKG_CONFIG_PATH"
  export PATH="$FINAL_INSTALL_DIR/bin:$PATH"
  export LD_RUN_PATH="$FINAL_INSTALL_DIR/lib"
  export PKG_CONFIG_PATH="$FINAL_INSTALL_DIR/lib/pkgconfig:$PKG_CONFIG_PATH"

  # CONFIG Array holds the ./configure (or equiv) command for each project
  # CFLAGS_ Array holds additional CFLAGS for the configure/make step of a given project
  # LDFLAGS_ Array holds additional LDFLAGS for the configure/make step of a given project

  #####
  # ffmpeg
  CONFIG[0]="./configure --prefix=$FINAL_INSTALL_DIR --disable-static --disable-doc --enable-gpl --enable-version3 --enable-shared --enable-runtime-cpudetect $CONFIGURE_DEBUG_FLAG"
  CONFIG[0]="${CONFIG[0]} --enable-libtheora --enable-libvorbis --enable-libmp3lame --enable-libx264 --enable-libx265 --enable-libvpx --enable-libopus --enable-libvpl --enable-libdav1d --enable-libaom --disable-decoder=libaom_av1 --enable-libsvtav1 --enable-libwebp --enable-libzimg --disable-vulkan --disable-vaapi"
  [ "$TARGET_ARCH" = "clangarm64" ] && CONFIG[0]="${CONFIG[0]} --cc=clang --cxx=clang++ --arch=aarch64"
  # Add optional parameters
  if [ "$ENABLE_VMAF" = "1" ]; then
    CONFIG[0]="${CONFIG[0]} --enable-libvmaf --disable-w32threads"
  fi
  CONFIG[0]="${CONFIG[0]} $FFMPEG_ADDITIONAL_OPTIONS"
  CFLAGS_[0]="-I$FINAL_INSTALL_DIR/include  -Wno-incompatible-pointer-types $CFLAGS"

  #####
  # mlt
  CONFIG[1]="cmake -GNinja -DCMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR -DCMAKE_PREFIX_PATH=$QTDIR -DMOD_GDK=OFF -DMOD_GLAXNIMATE_QT6=ON -DMOD_QT=OFF -DMOD_QT6=ON -DMOD_SDL1=OFF -DUSE_VST2=OFF"
  [ "$ENABLE_OPENCV" = "1" ] && CONFIG[1]="${CONFIG[1]} -DMOD_OPENCV=ON"
  if [ "$DEBUG_BUILD" = "1" ]; then
    CONFIG[1]="${CONFIG[1]} -DCMAKE_BUILD_TYPE=RelWithDebInfo"
  else
    CONFIG[1]="${CONFIG[1]} -DCMAKE_BUILD_TYPE=Release"
  fi
  CFLAGS_[1]="-I$FINAL_INSTALL_DIR/include $ASAN_CFLAGS $CFLAGS"
  CXXFLAGS_[1]="${CFLAGS_[1]} -std=c++11 -D_XOPEN_SOURCE=700"
  LDFLAGS_[1]="-L$FINAL_INSTALL_DIR/lib $ASAN_LDFLAGS $LDFLAGS"
  BUILD[1]="ninja -j $MAKEJ"
  INSTALL[1]="ninja install"

  #####
  # frei0r
  CONFIG[2]="cmake -DCMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR -DWITHOUT_GAVL=1 -DWITHOUT_OPENCV=1 -GNinja $CMAKE_DEBUG_FLAG"
  CFLAGS_[2]="$CFLAGS"
  LDFLAGS_[2]=$LDFLAGS
  BUILD[2]="ninja -j $MAKEJ"
  INSTALL[2]="ninja install"

  #####
  # movit
  CONFIG[3]="./autogen.sh --prefix=$FINAL_INSTALL_DIR"
  # MinGW does not provide ffs(), but there is a gcc intrinsic for it.
  CFLAGS_[3]="$CFLAGS -Dffs=__builtin_ffs -fpermissive"
  CXXFLAGS_[3]="${CFLAGS_[3]}"
  LDFLAGS_[3]=$LDFLAGS
  BUILD[3]="make -j$MAKEJ libmovit.la"

  #####
  # shotcut
  CONFIG[4]="cmake -G Ninja -D CMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR -DCMAKE_PREFIX_PATH=$QTDIR -D SHOTCUT_VERSION=$SHOTCUT_VERSION $CMAKE_DEBUG_FLAG"
  CFLAGS_[4]="$ASAN_CFLAGS $CFLAGS"
  LDFLAGS_[4]="$ASAN_LDFLAGS $LDFLAGS"
  BUILD[4]="ninja -j $MAKEJ"
  INSTALL[4]="install_shotcut"

  #####
  # ladspa-swh
  CONFIG[5]="./autogen.sh --prefix=$FINAL_INSTALL_DIR --disable-nls"
  CFLAGS_[5]=$CFLAGS
  LDFLAGS_[5]=$LDFLAGS
  BUILD[5]="make -j $MAKEJ LDFLAGS=-no-undefined"
  INSTALL[5]="install_ladspa-swh"

  #######
  # nv-codec-headers
  CONFIG[6]="sed -i s,/usr/local,$FINAL_INSTALL_DIR, Makefile"

  #######
  # AMF - no build required
  CONFIG[7]=""
  [ ! -d "$FINAL_INSTALL_DIR/include/AMF" ] && INSTALL[7]="install_amf"

  #########
  # bigsh0t
  CONFIG[8]="cmake -DCMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR -GNinja $CMAKE_DEBUG_FLAG"
  CFLAGS_[8]=$CFLAGS
  LDFLAGS_[8]=$LDFLAGS
  BUILD[8]="ninja -j $MAKEJ"
  INSTALL[8]="install -p -c *.dll "$FINAL_INSTALL_DIR"/lib/frei0r-1"

  #####
  # whisper.cpp
  CONFIG[9]="cmake -B build -G Ninja -D CMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR $CMAKE_DEBUG_FLAG -D BUILD_SHARED_LIBS=ON -D GGML_NATIVE=OFF -D WHISPER_BUILD_SERVER=OFF -D WHISPER_BUILD_TESTS=OFF"
  CFLAGS_[9]=$CFLAGS
  LDFLAGS_[9]=$LDFLAGS
  BUILD[9]="ninja -C build -j $MAKEJ"
  INSTALL[9]="install_whispercpp"

  #####
  # dav1d
  CONFIG[10]="meson setup builddir --prefix=$FINAL_INSTALL_DIR --libdir=$FINAL_INSTALL_DIR/lib"
  if [ "$DEBUG_BUILD" = "1" ]; then
    CONFIG[10]="${CONFIG[10]} --buildtype=debug"
  else
    CONFIG[10]="${CONFIG[10]} --buildtype=release"
  fi
  CFLAGS_[10]=$CFLAGS
  LDFLAGS_[10]=$LDFLAGS
  BUILD[10]="build_dav1d"
  INSTALL[10]="meson install -C builddir"

  #####
  # aom
  # Use -DCMAKE_POSITION_INDEPENDENT_CODE=ON if -DBUILD_SHARED_LIBS=1 does not work.
  CONFIG[11]="cmake -GNinja -DCMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR $CMAKE_DEBUG_FLAG -DBUILD_SHARED_LIBS=1 -DCONFIG_AV1_DECODER=0 -DENABLE_EXAMPLES=0 -DENABLE_TESTS=0 ../aom"
  CFLAGS_[11]=$CFLAGS
  LDFLAGS_[11]=$LDFLAGS
  BUILD[11]="ninja -j $MAKEJ"
  INSTALL[11]="ninja install"

  #####
  # vmaf
  CONFIG[12]="meson setup libvmaf/build libvmaf --prefix=$FINAL_INSTALL_DIR --libdir=$FINAL_INSTALL_DIR/lib"
  if [ "$DEBUG_BUILD" = "1" ]; then
    CONFIG[12]="${CONFIG[12]} --buildtype=debug"
  else
    CONFIG[12]="${CONFIG[12]} --buildtype=release"
  fi
  CFLAGS_[12]="$CFLAGS"
  [ "$TARGET_ARCH" = "clangarm64" ] && LDFLAGS_[12]="$LDFLAGS -lwinpthread"
  BUILD[12]="ninja -C libvmaf/build -j $MAKEJ"
  INSTALL[12]="install_vmaf"

  #####
  # glaxnimate
  CONFIG[13]="cmake -G Ninja -D CMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR -D Python3_FIND_REGISTRY=NEVER"
  if [ "$TARGET_ARCH" = "clangarm64" ]; then
    CONFIG[13]="${CONFIG[13]} -D CMAKE_PREFIX_PATH=$FINAL_INSTALL_DIR"
  else
    CONFIG[13]="${CONFIG[13]} -D CMAKE_PREFIX_PATH=$QTDIR"
  fi
  if [ "$DEBUG_BUILD" = "1" ]; then
    CONFIG[13]="${CONFIG[13]} -D CMAKE_BUILD_TYPE=RelWithDebInfo"
  else
    CONFIG[13]="${CONFIG[13]} -D CMAKE_BUILD_TYPE=Release"
  fi
  CFLAGS_[13]="$ASAN_CFLAGS $CFLAGS"
  LDFLAGS_[13]="$ASAN_LDFLAGS $LDFLAGS"
  BUILD[13]="ninja -j $MAKEJ"
  INSTALL[13]="ninja translations install"

  #####
  # gopro2gpx
  CONFIG[14]="cmake -G Ninja -D CMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR $CMAKE_DEBUG_FLAG"
  CFLAGS_[14]="$CFLAGS"
  LDFLAGS_[14]="$LDFLAGS"
  BUILD[14]="ninja -j $MAKEJ"
  INSTALL[14]="install -p -c gopro2gpx $FINAL_INSTALL_DIR"

  #####
  # opencv
  CONFIG[15]="cmake -B build -G Ninja -D CMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR -D BUILD_LIST=tracking -D OPENCV_GENERATE_PKGCONFIG=YES -D OPENCV_EXTRA_MODULES_PATH=../opencv_contrib/modules -D WITH_OPENMP=ON $CMAKE_DEBUG_FLAG"
  CFLAGS_[15]="$CFLAGS"
  LDFLAGS_[15]="$LDFLAGS"
  BUILD[15]="build_opencv"
  INSTALL[15]="ninja -C build install"

  #####
  # libspatialaudio
  CONFIG[17]="cmake -G Ninja -B build -D CMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR $CMAKE_DEBUG_FLAG"
  CFLAGS_[17]="$CFLAGS"
  LDFLAGS_[17]="$LDFLAGS"
  BUILD[17]="ninja -C build -j $MAKEJ"
  INSTALL[17]="install_spatialaudio"
}

function build_dav1d {
  # dav1d frequently fails on Windows on generate symbol file
  cmd ninja -C builddir -j $MAKEJ || cmd ninja -C builddir -j $MAKEJ
}

function build_opencv {
  [ grep M_PI 3rdparty/carotene/include/carotene/definitions.hpp ] || printf >>3rdparty/carotene/include/carotene/definitions.hpp '#ifndef M_PI\n#define M_PI (3.14159265358979323846)\n#endif'
  cmd ninja -C build -j $MAKEJ
}

function install_amf {
  cmd rm -rf Thirdparty
  cmd mkdir -p "$FINAL_INSTALL_DIR/include/AMF"
  cmd cp -av "amf/public/include/." "$FINAL_INSTALL_DIR/include/AMF"
}

function install_shotcut {
  cmd ninja install
  cmd install -c COPYING "$FINAL_INSTALL_DIR"
  cmd install -c packaging/windows/shotcut.iss "$FINAL_INSTALL_DIR"/..
  cmd sed -i "s/YY.MM.DD/$SHOTCUT_VERSION/" "$FINAL_INSTALL_DIR"/../shotcut.iss
  cmd install -d "$FINAL_INSTALL_DIR"/share/translations
  cmd install -p -c translations/*.qm "$FINAL_INSTALL_DIR"/share/translations
}

function install_ladspa-swh {
  cmd install -d "$FINAL_INSTALL_DIR"/lib/ladspa
  cmd install -p -c .libs/*.dll "$FINAL_INSTALL_DIR"/lib/ladspa
}

function install_vmaf {
  cmd ninja install -C libvmaf/build || die "Unable to install $1"
  cmd install -d "$FINAL_INSTALL_DIR"/share/vmaf
  cmd install -p -c model/*.json "$FINAL_INSTALL_DIR"/share/vmaf || die "Unable to install $1"
}

function install_spatialaudio {
  cmd ninja -C build install || die "Unable to install $1"
  cmd sed -i "s,-I/${TARGET_ARCH}/include,," "$FINAL_INSTALL_DIR"/lib/pkgconfig/spatialaudio.pc
}

function install_whispercpp {
  cmd ninja -C build install
  cmd install -p -c build/bin/main.exe $FINAL_INSTALL_DIR/bin/whisper.cpp-main.exe
  cmd mkdir -p $FINAL_INSTALL_DIR/share/shotcut/whisper_models
  cmd install -p -c models/ggml-base-q5_1.bin $FINAL_INSTALL_DIR/share/shotcut/whisper_models
}

######################################################################
# FEEDBACK FUNCTIONS
######################################################################

#################################################################
# feedback_status
# $@ status information
function feedback_status {
  trace "Entering feedback_status @ = $@"
  # Need to collect $@ in a single variable for cmd to work
  ARG=$@
  log "$ARG"
}

#################################################################
# feedback_result
# $1 : SUCCESS, FAILURE, ABORTED
# $2 : Additional information
# Does the relevant feedback, and terminates.
function feedback_result {
  trace "Entering feedback_result @ = $@"

  # If needed, kill the checker process
  if test "" != "$CHECKERPID" ; then
    # Kill the checker process
    kill -9 $CHECKERPID &> /dev/null
  fi

  log "Process has finished. Reason: $@"
}


#################################################################
# check_abort
# Function that checks if the user wanted to cancel what we are doing.
# returns "stop" or "cont" as appropriate
function check_abort {
  # log "$ARG"
  echo
}

######################################################################
# GLOBAL TEST FUNCTIONS
######################################################################

#################################################################
# is_newer_equal
# Compares versions strings, and returns 1 if $1 is newer than $2
# This is highly ineffective, I am sorry to say...
function is_newer_equal {
  trace "Entering is_newer_equal @ = $@"
  A1=`echo $1 | cut -d. -f1`
  A2=`echo $1 | cut -d. -f2`
  A3=`echo $1 | cut -d. -f3 | sed 's/^\([0-9]\{1,3\}\).*/\1/'`
  B1=`echo $2 | cut -d. -f1`
  B2=`echo $2 | cut -d. -f2`
  B3=`echo $2 | cut -d. -f3 | sed 's/^\([0-9]\{1,3\}\).*/\1/'`
  debug "A = $A1 $A2 $A3, B = $B1 $B2 $B3"
  test "$A1" -gt "$B1" -o \( "$A1" = "$B1" -a "$A2" -gt "$B2" \) -o \( "$A1" = "$B1" -a "$A2" = "$B2" -a "$A3" -ge "$B3" \)
}

######################################################################
# ACTION GET FUNCTIONS
######################################################################

#################################################################
# make_clean_dir
# Make clean in a specific directory
# $1: The directory to make clean in.
# Any errors are ignored. Make clean is only called if cd success.
# Assumes cwd is common parent dir
function make_clean_dir {
  trace "Entering make_clean_dir @ = $@"
  log Make clean for $1 called
  feedback_status "Cleaning out sources for $1"
  cmd pushd .
  # Special hack for ffmpeg, it sometimes requires distclean to work.
  if test "FFmpeg" = "$1" ; then
      cmd cd $1 && cmd make distclean
  else
      cmd cd $1 && cmd make clean
  fi
  feedback_status Cleaned up in $1
  cmd popd
}

#################################################################
# clean_dirs
# Make clean in all directories
function clean_dirs {
  trace "Entering clean_dirs @ = $@"
  feedback_status Cleaning out all subdirs
  cmd cd $SOURCE_DIR || mkdir -p $SOURCE_DIR
  cmd cd $SOURCE_DIR || die "Unable to change to directory $SOURCE_DIR"
  for DIR in $SUBDIRS ; do
    make_clean_dir $DIR
  done
  feedback_status Done cleaning out in source dirs
}

#################################################################
# get_subproject
# $1 The sourcedir to get sources for
# Get the sources for a single project
# Assumes cwd is common parent dir
# Errors abort
function get_subproject {
  trace "Entering get_subproject @ = $@"
  feedback_status Getting or updating source for $1 - this could take some time
  cmd pushd .

  # Check for repository setup
  REPOTYPE=`lookup REPOTYPES $1`
  REPOLOC=`lookup REPOLOCS $1`
  REVISION=`lookup REVISIONS $1`
  debug "REPOTYPE=$REPOTYPE, REPOLOC=$REPOLOC, REVISION=$REVISION"

  # Note that svn can check out to current directory, whereas git will not. Sigh.
  if test "git" = "$REPOTYPE" ; then
      # If the dir is there, check if it is a git repo
      if test -d "$1" ; then
          # Change to it
          cmd cd $1 || die "Unable to change to directory $1"
          debug "About to look for git repo"
          git --no-pager status 2>&1 | grep "fatal" &> /dev/null
          if test 0 != $? ; then
              # Found git repo
              debug "Found git repo, will update"

              if ! git diff-index --quiet ${REVISION:-master}; then
                  die "git repository has local changes, aborting checkout. Consider disabling ACTION_GET in your build config if you want to compile with these changes"
              fi

              feedback_status "Pulling git sources for $1"
              cmd git reset --hard || die "Unable to reset git tree for $1"
              cmd git checkout master || die "Unable to git checkout master"
              cmd git --no-pager pull $REPOLOC master || die "Unable to git pull sources for $1"
              cmd git checkout $REVISION || die "Unable to git checkout $REVISION"
          else
              # A dir with the expected name, but not a git repo, bailing out
              PWD=`pwd`
              die "Found a dir with the expected name $1 ($PWD), but it was not a git repo. Unable to proceed. If you have old mlt/mlt++ sources, please delete these directories, before rerunning the script."
          fi
      else
          # No git repo
          debug "No git repo, need to check out"
          feedback_status "Cloning git sources for $1"
          cmd git --no-pager clone --quiet --recurse-submodules $REPOLOC || die "Unable to git clone source for $1 from $REPOLOC"
          cmd cd $1 || die "Unable to change to directory $1"
          cmd git checkout --recurse-submodules $REVISION || die "Unable to git checkout $REVISION"
          [ "$SDK" = "1" -a "shotcut" != "$1" -a "mlt" != "$1" ] && cmd rm -rf .git
      fi
  elif test "svn" = "$REPOTYPE" ; then
      # Create subdir if not exist
      if test ! -d "$1" ; then
          cmd mkdir -p $1 || die "Unable to create directory $1"
      fi
      # Change to it
      cmd cd $1 || die "Unable to change to directory $1"
      FIND_STR="\(Revision\|Last\ Changed\ Date\)"
      debug "About to look for SVN revision info for $REPOLOC $REVISION"
      svn --non-interactive info | grep "$FIND_STR"
      if test 0 = $? ; then
          debug "Found existing SVN checkout"
          # Found svn info
          # For KDENLIVE: If the svn info URL matches the one we have in the REPOLOCS array, do an update, otherwise, do a switch.
          REPOLOCURL=`svn --non-interactive info | grep URL | awk '{print $2}'`
          # Now, we have to be a bit clever here, because if the user originally checked it out using
          # https, we can not change to http. So, we check for https in the current URL
          # Note, that being clever almost always fails at some point. But, at least we give it a try...
          if test "${REPOLOCURL:0:5}" = "https" ; then
              REPOLOC=${REPOLOC/http/https}
          fi
          if test "kdenlive" = "$1" -a $REPOLOCURL != $REPOLOC ; then
              warn "Existing url $REPOLOCURL for $1 does not match the url for selected version: $REPOLOC. Trying svn switch to update"
              feedback_status "Trying to switch repo url for $1"
              cmd svn --non-interactive switch $REPOLOC $REVISION || die "Unable to switch svn repo from $REPOLOCURL to $REPOLOC $REVISION"
          else
              feedback_status "Updating SVN sources for $1"
              cmd svn --non-interactive update $REVISION || die "Unable to update SVN repo in $1 to $REVISION"
          fi
      else
          # No svn info
          feedback_status "Getting SVN sources for $1"
          cmd svn --non-interactive co $REPOLOC . $REVISION || die "Unable to get SVN source for $1 from $REPOLOC $REVISION"
          cmd cd $1 || die "Unable to change to directory $1"
      fi
  elif test "http-tgz" = "$REPOTYPE" ; then
      if test ! -d "$1" ; then
          feedback_status "Downloading archive for $1"
          which curl > /dev/null
          if test 0 = $?; then
              cmd $(curl -L $REPOLOC | tar -xz) || die "Unable to download source for $1 from $REPOLOC"
          else
              which wget > /dev/null
              if test 0 = $?; then
                  cmd $(wget -O - $REPOLOC | tar -xz) || die "Unable to download source for $1 from $REPOLOC"
              fi
          fi
          cmd mv "$REVISION" "$1" || die "Unable to rename $REVISION to $1"
      fi
      cmd cd $1 || die "Unable to change to directory $1"
  fi # git/svn

  if [ "$1" = "whisper.cpp" ]; then
    cmd sh ./models/download-ggml-model.sh base-q5_1
  fi

  feedback_status Done getting or updating source for $1
  cmd popd
}

#################################################################
# get_all_sources
# Gets all the sources for all subprojects
function get_all_sources {
  trace "Entering get_all_sources @ = $@"
  feedback_status Getting all sources
  log Changing to $SOURCE_DIR
  cd $SOURCE_DIR || mkdir -p "$SOURCE_DIR"
  cd $SOURCE_DIR || die "Unable to change to directory $SOURCE_DIR"
  for DIR in $SUBDIRS ; do
    get_subproject $DIR
  done
  feedback_status Done getting all sources
}

######################################################################
# ACTION COMPILE-INSTALL FUNCTIONS
######################################################################

#################################################################
# mlt_format_required
# log a string that expresses a requirement
function mlt_format_required {
  log 'MLTDISABLED <b>'$1'</b> : this is a <b>required</b> module. '$2'Will abort compilation.'
}

#################################################################
# mlt_format_optional
# log a string that expresses missing an optional
function mlt_format_optional {
  log 'MLTDISABLED <b>'$1'</b> : this is an <b>optional</b> module that provides '$2'. To enable it, try installing a package called something like '$3'.'
}

#################################################################
# configure_compile_install_subproject
# $1 The sourcedir to configure, compile, and install
# Configures, compiles, and installs a single subproject.
# Assumes cwd is common parent dir
# Errors abort
function configure_compile_install_subproject {
  trace "Entering configure_compile_install_subproject @ = $@"
  feedback_status Configuring, compiling, and installing $1

  OLDCFLAGS=$CFLAGS
  OLDCXXFLAGS=$CXXFLAGS
  OLDLD_LIBRARY_PATH=$LD_LIBRARY_PATH
  cmd pushd .

  # Change to right directory
  cmd cd $1 || die "Unable to change to directory $1"

  # Set cflags, log settings
  log PATH=$PATH
  log LD_RUN_PATH=$LD_RUN_PATH
  log PKG_CONFIG_PATH=$PKG_CONFIG_PATH
  export CFLAGS=`lookup CFLAGS_ $1`
  log CFLAGS=$CFLAGS
  export CXXFLAGS=`lookup CXXFLAGS_ $1`
  log CXXFLAGS=$CFLAGS
  export LDFLAGS=`lookup LDFLAGS_ $1`
  log LDFLAGS=$LDFLAGS

  MYPRECONFIG=`lookup PRECONFIG $1`
  MYCONFIG=`lookup CONFIG $1`
  MYBUILD=`lookup BUILD $1`
  MYINSTALL=`lookup INSTALL $1`

  # Configure
  if [ "$ACTION_CONFIGURE" = "1" ]; then

  # Configure
  feedback_status Configuring $1

  if test "$MYPRECONFIG" != ""; then
    cmd $MYPRECONFIG || die "Unable to pre-configure $1"
    feedback_status Done pre-configuring $1
  fi

  # Special hack for aom
  if test "aom" = "$1"; then
    cmd mkdir -p ../build-aom
    cmd cd ../build-aom || die "Unable to change to directory aom/builddir"
  fi

  if test "$MYCONFIG" != ""; then
    cmd $MYCONFIG || die "Unable to configure $1"
    feedback_status Done configuring $1
  fi

  fi # if [ "$ACTION_CONFIGURE" = "1" ]

  # Compile
  feedback_status Building $1 - this could take some time
  if test "$MYBUILD" != ""; then
    cmd $MYBUILD || die "Unable to build $1"
  elif test "$MYCONFIG" != ""; then
    cmd make -j$MAKEJ || die "Unable to build $1"
  fi
  feedback_status Done building $1

  # Install
  feedback_status Installing $1
  export LD_LIBRARY_PATH=`lookup LD_LIBRARY_PATH_ $1`
  log "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
  if test "$MYINSTALL" != "" ; then
    cmd $MYINSTALL || die "Unable to install $1"
  elif test "$MYCONFIG" != "" ; then
    cmd make install || die "Unable to install $1"
  fi
  feedback_status Done installing $1

  # Reestablish
  cmd popd
  export CFLAGS=$OLDCFLAGS
  export CXXFLAGS=$OLDCXXFLAGS
  export LD_LIBRARY_PATH=$OLDLD_LIBRARY_PATH
}


#################################################################
# configure_compile_install_all
# Configures, compiles, and installs all subprojects
function configure_compile_install_all {
  trace "Entering configure_compile_install_all @ = $@"
  feedback_status Configuring, compiling and installing all sources

  # Set some more vars for this operation
  log "Using install dir $FINAL_INSTALL_DIR"
  log "Found $CPUS cpus. Will use make -j $MAKEJ for compilation"

  log Changing to $SOURCE_DIR
  cd $SOURCE_DIR || die "Unable to change to directory $SOURCE_DIR"
  for DIR in $SUBDIRS ; do
    configure_compile_install_subproject $DIR
  done

  feedback_status Done configuring, compiling and installing all sources
}

######################################################################
# ACTION DEPLOY
######################################################################


#################################################################
# get_dir_info
# Helper function for startup script creating - returns svn rev information
# for a given directory
function get_dir_info {
  # trace "Entering get_dir_info @ = $@"
  pushd . &> /dev/null
  cd $1 || die "Unable to change directory to $1"
  REPOTYPE=`lookup REPOTYPES $1`
  if test "xgit" = "x$REPOTYPE" ; then
    FIND_STR="\(commit\|Date\)"
    INFO_TEXT=`git --no-pager log -n1 | grep "$FIND_STR"`
  else
    FIND_STR="\(Revision\|Last\ Changed\ Date\)"
    INFO_TEXT=`svn info | grep "$FIND_STR"`
  fi
  echo
  echo -e $1: ${INFO_TEXT:-Warning: No $REPOTYPE information found in $SOURCE_DIR/$1.}
  echo
  popd  &> /dev/null
}

#################################################################
# sys_info
# Returns some information about the system
function sys_info {
  echo
  echo uname -a at time of compilation:
  uname -a
  echo Information about cc at the time of compilation:
  LANG=C cc -v 2>&1
}


#################################################################
# bundle_dlls
function bundle_dlls
{
  log bundling library dependencies of $(basename "$1")
  target=$(dirname "$1")/$(basename "$1")
  basename_target=$(basename "$target")
  libs=$(ldd "$target" | awk "(\$3 ~ /\/$TARGET_ARCH\/bin\//) {print \$3}")
  for lib in $libs; do
    basename_lib=$(basename "$lib")
    if [ "$basename_lib" != "$basename_target" ]; then
      [ ! -e "$FINAL_INSTALL_DIR/$basename_lib" ] && cmd cp -p "$lib" "$FINAL_INSTALL_DIR"
    fi
  done
  for lib in $libs; do
    basename_lib=$(basename "$lib")
    if [ "$basename_lib" != "$basename_target" ] && [ ! -e "$basename_lib".bundled ]; then
      touch "$basename_lib".bundled
      bundle_dlls "$lib"
    fi
  done
}


#################################################################
# deploy
function deploy
{
  trace "Entering deploy @ = $@"
  pushd .

  # Change to right directory
  log Changing directory to $FINAL_INSTALL_DIR
  cmd cd $FINAL_INSTALL_DIR || die "Unable to change to directory $FINAL_INSTALL_DIR"

  log Reorganizing installed files
  cmd mv bin/*.dll .
  cmd mv lib/libaom.dll .
  cmd mv lib/libCuteLogger.dll .
  if [ "$SDK" = "1" ]; then
    cmd mv bin/*.exe .
    # OpenCV installs binaries to a weird place
    cmd mv {x64,x86}/mingw/bin/*.dll .
  else
    cmd mv bin/ffmpeg.exe .
    cmd mv bin/ffplay.exe .
    cmd mv bin/ffprobe.exe .
    cmd mv bin/glaxnimate.exe .
    cmd mv bin/whisper.cpp-main.exe .
    cmd rm -rf bin include etc man manifest src *.txt
    cmd rm lib/*
    cmd rm -rf lib/cmake lib/pkgconfig lib/gdk-pixbuf-2.0 lib/glib-2.0 lib/gtk-2.0
    cmd rm -rf share/doc share/man share/ffmpeg/examples share/aclocal share/glib-2.0 share/gtk-2.0 share/gtk-doc share/themes share/locale
    # OpenCV installs binaries to a weird place
    cmd mv {x64,x86}/mingw/bin/*.dll .
    cmd rm -rf {x64,x86}
    cmd rm OpenCVConfig*
    cmd rm setup_vars_opencv4.cmd
  fi
  cmd mv COPYING COPYING.txt

  log Copying some libs from Qt
  if [ "$DEBUG_BUILD" != "1" -o "$SDK" = "1" ]; then
    cmd cp -p "$QTDIR"/bin/Qt6{Charts,Core,Core5Compat,Gui,Multimedia,Network,OpenGL,OpenGLWidgets,Qml,QmlModels,QmlWorkerScript,Quick,QuickControls2*,QuickDialogs2,QuickDialogs2QuickImpl,QuickDialogs2Utils,QuickLayouts,QuickTemplates2,QuickWidgets,Sql,Svg,SvgWidgets,UiTools,Widgets,Xml}.dll .
  fi

  for lib in *.dll; do
    bundle_dlls "$lib"
  done
  for lib in lib/{frei0r-1,ladspa,mlt}/*.dll; do
    bundle_dlls "$lib"
  done
  bundle_dlls glaxnimate.exe
  # for good measure
  for lib in $(find -name '*.dll' -or -name '*.exe'); do
    bundle_dlls "$lib"
  done

  log Copying some DLLs and python libraries for Glaxnimate
  cmd cp -p /${TARGET_ARCH}/bin/libpython3.11.dll python311.dll
  cmd cp -p "$SOURCE_DIR"/glaxnimate/external/Qt-Color-Widgets/libQtColorWidgets.dll .
  cmd mkdir -p share/glaxnimate/glaxnimate/pythonhome/lib/python
  cmd cp -r /${TARGET_ARCH}/lib/python3.11/*.py \
            /${TARGET_ARCH}/lib/python3.11/lib-dynload/* \
            /${TARGET_ARCH}/lib/python3.11/{json,collections,encodings,logging,urllib} \
      share/glaxnimate/glaxnimate/pythonhome/lib/python

  log Copying some libs from mlt-prebuilt
  cmd cp -p "$HOME"/bin/*.dll .
  cmd cp -p "$QTDIR"/bin/d3dcompiler_47.dll .

  log Copying some libs from msys2
  cmd cp -p /${TARGET_ARCH}/bin/libcrypto-3.dll .
  if [ "$TARGET_ARCH" = "clangarm64" ]; then
    cmd cp -p /${TARGET_ARCH}/bin/{libjasper.dll,libjpeg-8.dll,libmng-2.dll,liblcms2-2.dll,libtiff-6.dll,libjbig-0.dll,libdeflate.dll,libLerc.dll,libunwind.dll,libwebpdemux-2.dll,libcairo-2.dll,libfontconfig-1.dll,libpixman-1-0.dll,libxml2-2.dll,libomp.dll,libebur128.dll,libsamplerate-0.dll,librubberband-2.dll,libsox-3.dll,libopencore-amrnb-0.dll,libvo-amrwbenc-0.dll,libFLAC.dll,libltdl-7.dll,libgsm.dll,libmad-0.dll,libao-4.dll,libid3tag-0.dll,libtwolame-0.dll,libvorbisfile-3.dll,libwavpack-1.dll,libsndfile-1.dll,libopencore-amrwb-0.dll,libmpg123-0.dll,libopusfile-0.dll,libmysofa.dll,libvidstab.dll,libexpat-1.dll,liblz4.dll,libfftw3f-3.dll} .
  fi
  if [ "$DEBUG_BUILD" = "1" -o "$SDK" = "1" ]; then
    [ "$TARGET_ARCH" != "clangarm64" ] && cmd cp -p "$SOURCE_DIR"/shotcut/drmingw/x64/bin/*.{dll,yes} .
    cmd cp -p /${TARGET_ARCH}/bin/libfftw3*.dll bin/
    cmd cp -p /${TARGET_ARCH}/lib/libfftw3*.a lib/
    cmd cp -p /${TARGET_ARCH}/include/fftw3* include/
    cmd cp -p /${TARGET_ARCH}/lib/pkgconfig/fftw3*.pc lib/pkgconfig/
#    cmd mkdir -p lib/cmake/fftw3 lib/cmake/fftw3f lib/cmake/fftw3l lib/cmake/fftw3q
#    cmd cp -p /${TARGET_ARCH}/lib/cmake/fftw3/*.cmake lib/cmake/fftw3/
#    cmd cp -p /${TARGET_ARCH}/lib/cmake/fftw3f/*.cmake lib/cmake/fftw3f/
#    cmd cp -p /${TARGET_ARCH}/lib/cmake/fftw3l/*.cmake lib/cmake/fftw3l/
#    cmd cp -p /${TARGET_ARCH}/lib/cmake/fftw3q/*.cmake lib/cmake/fftw3q/
  fi

  log Copying some plugins, qml, and translations from Qt
  cmd mkdir -p lib/qt6/generic
  for plugin in generic iconengines imageformats multimedia platforms sqldrivers styles tls; do
    cmd cp -pr "$QT_SHARE_DIR"/plugins/${plugin} lib/qt6
    cmd rm lib/qt6/${plugin}/*.debug 2>/dev/null
    cmd rm lib/qt6/multimedia/ffmpegmediaplugin.dll 2>/dev/null
    cmd rm lib/qt6/tls/qcertonlybackend.dll 2>/dev/null
    cmd rm lib/qt6/tls/qopensslbackend.dll 2>/dev/null
    for lib in lib/qt6/${plugin}/*.dll; do
      bundle_dlls "$lib"
    done
  done
  cmd mkdir -p lib/qml
  cmd cp -pr "$QT_SHARE_DIR"/qml/{Qt,QtCore,QtQml,QtQuick} lib/qml
  cmd cp -pr "$QT_SHARE_DIR"/translations/qt_*.qm share/translations
  cmd cp -pr "$QT_SHARE_DIR"/translations/qtbase_*.qm share/translations

  log Removing things not needed
  cmd rm *.bundled
  cmd rm lib/qt6/sqldrivers/{qsqlodbc,qsqlpsql}.dll
  # colortap is not used by Shotcut and no plans to include it, but
  # Kaspersky antivirus is flagging it and scaring people.
  cmd rm lib/frei0r-1/colortap.dll
  printf "[Paths]\nPlugins=lib/qt6\nQml2Imports=lib/qml\n" > qt.conf

  if [ "$SDK" = "1" ]; then
    # Prepare src for archiving
    pushd .
    clean_dirs
    popd
    log Moving src
    cmd mv $SOURCE_DIR .

    log Creating archive
    cmd cd ..
    cmd tar -cJf shotcut-sdk.txz Shotcut
    fi
  popd
}

#################################################################
# perform_action
# Actually do what the user wanted
function perform_action {
  trace "Entering perform_action @ = $@"
  # Test that may fail goes here, before we do anything
  if test 1 = "$ACTION_CLEAN_SOURCE"; then
    clean_dirs
  fi
  if test 1 = "$GET"; then
    get_all_sources
  fi
  if test 1 = "$COMPILE_INSTALL" ; then
    sys_info
    configure_compile_install_all
  fi
  if test 1 = "$DEPLOY" ; then
    deploy
  fi
  feedback_result SUCCESS "Everything succeeded"
}

################################################################################
# MAIN AND FRIENDS
################################################################################

#################################################################
# kill_recursive
# The intention of this is to be able to kill all children, whenever the
# user aborts.
# This does not really work very very well, but its the best I can offer.
# It may leave some defunct around(?)
# $1 pid
function kill_recursive {
  trace "Entering kill_recursive @ = $@"
  if test "$1" != "$$"; then
    # Stop it from spawning more kids
    kill -9 $1 &> /dev/null
    wait $1
    for CP in `ps --ppid $1 -o pid=` ; do
      kill_recursive $CP
    done
  fi
}

#################################################################
# keep_checking_abort
# Checks if the user indicated an abort through
function keep_checking_abort {
  while test x`check_abort` = "xcont" ; do
    sleep 1
  done
  feedback_result ABORTED "User requested abort"
  # If we reach here, user aborted, kill everything in sight...
  kill_recursive $MAINPID
  exit
}

#################################################################
# main
# Collects all the steps
function main {
  {
  sleep 1
  init_log_file
  read_configuration
  set_globals
  } 2>&1

  # Setup abort handling
  # If anyone know of a better way to get ones pid from within a subshell, let me know...
  MAINPID=`/bin/bash -c "echo \\$PPID"`
  # debug "Main is running with pid $MAINPID"
  keep_checking_abort &
  CHECKERPID=$!
  # debug "Checker process is running with pid=$CHECKERPID"

  {
  perform_action
  } 2>&1

  # All is well, that ends well
  exit 0
}

parse_args "$@"
main
