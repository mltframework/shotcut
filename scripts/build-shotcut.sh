#!/bin/bash

# This script builds shotcut and many of its dependencies.
# It can accept a configuration file, default: build-shotcut.conf

# List of programs used:
# bash, test, tr, awk, ps, make, cmake, cat, sed, curl or wget, meson, ninja, and possibly others

# Author: Dan Dennedy <dan@dennedy.org>
# License: GPL2

################################################################################
# ARGS AND GLOBALS
################################################################################

# These are all of the configuration variables with defaults
INSTALL_DIR="$HOME/shotcut"
AUTO_APPEND_DATE=0
SOURCE_DIR="$INSTALL_DIR/src"
ACTION_CLEAN_SOURCE=0
ACTION_GET=1
ACTION_CONFIGURE=1
ACTION_COMPILE_INSTALL=1
ACTION_ARCHIVE=1
ACTION_CLEANUP=1
DEBUG_BUILD=0
ASAN_BUILD=0
CREATE_STARTUP_SCRIPT=1
ENABLE_FREI0R=1
FREI0R_HEAD=1
FREI0R_REVISION=
ENABLE_MOVIT=1
SUBDIRS=
MOVIT_HEAD=0
MOVIT_REVISION="origin/shotcut"
X264_HEAD=0
X264_REVISION="origin/stable"
X265_HEAD=0
X265_REVISION="origin/stable"
LIBVPX_HEAD=0
LIBVPX_REVISION="v1.12.0"
LIBOPUS_HEAD=0
LIBOPUS_REVISION="v1.3.1"
ENABLE_SWH_PLUGINS=1
FFMPEG_HEAD=0
FFMPEG_REVISION="origin/release/5.1"
FFMPEG_SUPPORT_H264=1
FFMPEG_SUPPORT_H265=1
FFMPEG_SUPPORT_JACK=1
FFMPEG_SUPPORT_LIBVPX=1
FFMPEG_SUPPORT_THEORA=1
FFMPEG_SUPPORT_MP3=1
FFMPEG_SUPPORT_FAAC=0
FFMPEG_SUPPORT_OPUS=1
FFMPEG_SUPPORT_ZIMG=1
FFMPEG_SUPPORT_NVENC=1
FFMPEG_SUPPORT_AMF=1
FFMPEG_SUPPORT_QSV=1
FFMPEG_SUPPORT_DAV1D=1
FFMPEG_SUPPORT_AOM=1
FFMPEG_SUPPORT_WEBP=1
FFMPEG_SUPPORT_VMAF=1
FFMPEG_ADDITIONAL_OPTIONS=
ENABLE_VIDSTAB=1
VIDSTAB_HEAD=1
VIDSTAB_REVISION=
MLT_HEAD=1
MLT_REVISION=
LOG_COLORS=0
SHOTCUT_HEAD=1
SHOTCUT_REVISION=
SHOTCUT_VERSION=$(date '+%y.%m.%d')
ENABLE_RUBBERBAND=1
RUBBERBAND_HEAD=0
RUBBERBAND_REVISION="v3.1.1"
ENABLE_BIGSH0T=1
BIGSH0T_HEAD=0
BIGSH0T_REVISION="8fe56f6d4e"
ENABLE_ZIMG=1
ZIMG_HEAD=0
ZIMG_REVISION="51c3c7f750c2af61955377faad56e3ba1b03589f"
DAV1D_HEAD=0
DAV1D_REVISION="1.0.0"
AOM_HEAD=0
AOM_REVISION="v3.4.0"
VMAF_HEAD=0
VMAF_REVISION="v2.3.1"
ENABLE_GLAXNIMATE=1
GLAXNIMATE_HEAD=0
GLAXNIMATE_REVISION="v0.5.1"
ENABLE_GOPRO2GPX=1

PYTHON_VERSION_DEFAULT=3.8
PYTHON_VERSION_DARWIN=3.10

QT_VERSION_DEFAULT=6.4.2
QT_VERSION_DARWIN=6.4.2

# QT_INCLUDE_DIR="$(pkg-config --variable=prefix QtCore)/include"
QT_INCLUDE_DIR=${QTDIR:+${QTDIR}/include}
# QT_LIB_DIR="$(pkg-config --variable=prefix QtCore)/lib"
QT_LIB_DIR=${QTDIR:+${QTDIR}/lib}
MLT_DISABLE_SOX=0

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
  echo "Usage: $0 [-c config-file] [-o target-os] [-s] [-t] [-h]"
  echo "Where:"
  echo -e "\t-c config-file\tDefaults to $CONFIGFILE"
  echo -e "\t-a target-arch\tDefaults to $(uname -m)"
  echo -e "\t-s\t\tbuild SDK"
  echo -e "\t-t\t\tSpawn into sep. process"
  echo -e "\t-v shotcut-version\t\tSet the Shotcut version; defaults to $SHOTCUT_VERSION"
}

#################################################################
# parse_args
# Parses the arguments passed in $@ and sets some global vars
function parse_args {
  CONFIGFILEOPT=""
  DETACH=0
  while getopts ":tsc:a:o:v:" OPT; do
    case $OPT in
      c ) CONFIGFILEOPT=$OPTARG
          echo Setting configfile to $CONFIGFILEOPT
      ;;
      s ) SDK=1;;
      t ) DETACH=1;;
      h ) usage
          exit 0;;
      a ) TARGET_ARCH=$OPTARG;;
      o ) TARGET_OS=$OPTARG;;
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
    x264)
      echo 3
    ;;
    libvpx)
      echo 4
    ;;
    movit)
      echo 5
    ;;
    shotcut)
      echo 7
    ;;
    ladspa)
      echo 8
    ;;
    vid.stab)
      echo 10
    ;;
    opus)
      echo 12
    ;;
    x265)
      echo 13
    ;;
    nv-codec-headers)
      echo 15
    ;;
    AMF)
      echo 16
    ;;
    mfx_dispatch)
      echo 17
    ;;
    rubberband)
      echo 18
    ;;
    bigsh0t)
      echo 19
    ;;
    zimg)
      echo 20
    ;;
    dav1d)
      echo 21
    ;;
    aom)
      echo 22
    ;;
    vmaf)
      echo 23
    ;;
    glaxnimate)
      echo 24
    ;;
    gopro2gpx)
      echo 25
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

#################################################################
# version - convert version string to a number to make comparisons

function version {
  echo "$@" | awk -F. '{ printf("%d%03d%03d%03d\n", $1,$2,$3,$4); }'
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

  # Set convenience variables.
  test "$TARGET_OS" = "" && TARGET_OS="$(uname -s)"
  test "$TARGET_ARCH" = "" && TARGET_ARCH="$(uname -m)"
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

  # The script sets CREATE_STARTUP_SCRIPT to true always, disable if not COMPILE_INSTALL
  if test 0 = "$COMPILE_INSTALL" ; then
    CREATE_STARTUP_SCRIPT=0
  fi
  debug "CREATE_STARTUP_SCRIPT=$CREATE_STARTUP_SCRIPT"

  # Subdirs list, for number of common operations
  # Note, the function to_key depends on this
  if [ -z "$SUBDIRS" ]; then
    SUBDIRS="FFmpeg mlt shotcut"
    if test "$ENABLE_FREI0R" = 1 ; then
        SUBDIRS="frei0r $SUBDIRS"
    fi
    if test "$ENABLE_MOVIT" = 1 && test "$MOVIT_HEAD" = 1 -o "$MOVIT_REVISION" != ""; then
        SUBDIRS="movit $SUBDIRS"
    fi
    if test "$FFMPEG_SUPPORT_H264" = 1 && test "$X264_HEAD" = 1 -o "$X264_REVISION" != ""; then
        SUBDIRS="x264 $SUBDIRS"
    fi
    if test "$FFMPEG_SUPPORT_H265" = 1 && test "$X265_HEAD" = 1 -o "$X265_REVISION" != ""; then
        SUBDIRS="x265 $SUBDIRS"
    fi
    if test "$FFMPEG_SUPPORT_LIBVPX" = 1 && test "$LIBVPX_HEAD" = 1 -o "$LIBVPX_REVISION" != ""; then
        SUBDIRS="libvpx $SUBDIRS"
    fi
    if test "$FFMPEG_SUPPORT_OPUS" = 1 && test "$LIBOPUS_HEAD" = 1 -o "$LIBOPUS_REVISION" != ""; then
        SUBDIRS="opus $SUBDIRS"
    fi
    if test "$FFMPEG_SUPPORT_NVENC" = 1 && test "$TARGET_OS" != "Darwin"; then
        SUBDIRS="nv-codec-headers $SUBDIRS"
    fi
    if test "$FFMPEG_SUPPORT_AMF" = 1 && test "$TARGET_OS" != "Darwin" && test "$TARGET_OS" != "Linux"; then
        SUBDIRS="AMF $SUBDIRS"
    fi
    if test "$FFMPEG_SUPPORT_QSV" = 1 && test "$TARGET_OS" != "Darwin" && test "$TARGET_OS" != "Linux"; then
        SUBDIRS="mfx_dispatch $SUBDIRS"
    fi
    if test "$ENABLE_SWH_PLUGINS" = "1" && test "$TARGET_OS" = "Darwin" -o "$TARGET_OS" = "Linux"; then
        SUBDIRS="ladspa $SUBDIRS"
    fi
    if test "$ENABLE_VIDSTAB" = 1 ; then
        SUBDIRS="vid.stab $SUBDIRS"
    fi
    if test "$ENABLE_RUBBERBAND" = 1 ; then
        SUBDIRS="rubberband $SUBDIRS"
    fi
    if test "$ENABLE_BIGSH0T" = 1 ; then
        SUBDIRS="$SUBDIRS bigsh0t"
    fi
    if test "$ENABLE_ZIMG" = 1 ; then
        SUBDIRS="zimg $SUBDIRS"
    fi
    if test "$FFMPEG_SUPPORT_DAV1D" = 1 && test "$DAV1D_HEAD" = 1 -o "$DAV1D_REVISION" != ""; then
        SUBDIRS="dav1d $SUBDIRS"
    fi
    if test "$FFMPEG_SUPPORT_AOM" = 1 && test "$AOM_HEAD" = 1 -o "$AOM_REVISION" != ""; then
        SUBDIRS="aom $SUBDIRS"
    fi
    if test "$FFMPEG_SUPPORT_VMAF" = 1 && test "$VMAF_HEAD" = 1 -o "$VMAF_REVISION" != ""; then
        SUBDIRS="vmaf $SUBDIRS"
    fi
    if test "$ENABLE_GLAXNIMATE" = 1 ; then
        SUBDIRS="$SUBDIRS glaxnimate"
    fi
    if test "$ENABLE_GOPRO2GPX" = 1 ; then
        SUBDIRS="$SUBDIRS gopro2gpx"
    fi
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
    QMAKE_ASAN_FLAGS="QMAKE_CXXFLAGS+=-fsanitize=address QMAKE_CXXFLAGS+=-fno-omit-frame-pointer QMAKE_LFLAGS+=-fsanitize=address"
  else
    ASAN_CFLAGS=
    ASAN_LDFLAGS=
    QMAKE_ASAN_FLAGS=
  fi

  debug "SUBDIRS = $SUBDIRS"

  # REPOLOCS Array holds the repo urls
  REPOLOCS[0]="https://github.com/FFmpeg/FFmpeg.git"
  REPOLOCS[1]="https://github.com/mltframework/mlt.git"
  REPOLOCS[2]="https://github.com/dyne/frei0r.git"
  REPOLOCS[3]="https://github.com/mirror/x264.git"
  REPOLOCS[4]="https://chromium.googlesource.com/webm/libvpx.git"
  REPOLOCS[5]="https://github.com/ddennedy/movit.git"
  REPOLOCS[7]="https://github.com/mltframework/shotcut.git"
  REPOLOCS[8]="https://github.com/swh/ladspa.git"
  REPOLOCS[10]="https://github.com/georgmartius/vid.stab.git"
  REPOLOCS[12]="https://github.com/xiph/opus.git"
  REPOLOCS[13]="https://github.com/videolan/x265"
  REPOLOCS[15]="https://github.com/FFmpeg/nv-codec-headers.git"
  REPOLOCS[16]="https://github.com/GPUOpen-LibrariesAndSDKs/AMF.git"
  REPOLOCS[17]="https://github.com/lu-zero/mfx_dispatch.git"
  REPOLOCS[18]="https://github.com/breakfastquay/rubberband.git"
  REPOLOCS[19]="https://bitbucket.org/leo_sutic/bigsh0t.git"
  REPOLOCS[20]="https://github.com/sekrit-twc/zimg.git"
  REPOLOCS[21]="https://github.com/videolan/dav1d"
  REPOLOCS[22]="https://aomedia.googlesource.com/aom"
  REPOLOCS[23]="https://github.com/Netflix/vmaf.git"
  REPOLOCS[24]="https://gitlab.com/ddennedy/glaxnimate.git"
  REPOLOCS[25]="https://github.com/ddennedy/gopro2gpx.git"

  # REPOTYPE Array holds the repo types. (Yes, this might be redundant, but easy for me)
  REPOTYPES[0]="git"
  REPOTYPES[1]="git"
  REPOTYPES[2]="git"
  REPOTYPES[3]="git"
  REPOTYPES[4]="git"
  REPOTYPES[5]="git"
  REPOTYPES[7]="git"
  REPOTYPES[8]="git"
  REPOTYPES[9]="git"
  REPOTYPES[10]="git"
  REPOTYPES[12]="git"
  REPOTYPES[13]="git"
  REPOTYPES[15]="git"
  REPOTYPES[16]="git"
  REPOTYPES[17]="git"
  REPOTYPES[18]="git"
  REPOTYPES[19]="git"
  REPOTYPES[20]="git"
  REPOTYPES[21]="git"
  REPOTYPES[22]="git"
  REPOTYPES[23]="git"
  REPOTYPES[24]="git"
  REPOTYPES[25]="git"

  # And, set up the revisions
  REVISIONS[0]=""
  if test 0 = "$FFMPEG_HEAD" -a "$FFMPEG_REVISION" ; then
    REVISIONS[0]="$FFMPEG_REVISION"
  fi
  # Git, just use blank or the hash.
  REVISIONS[1]=""
  if test 0 = "$MLT_HEAD" -a "$MLT_REVISION" ; then
    REVISIONS[1]="$MLT_REVISION"
  fi
  REVISIONS[2]=""
  if test 0 = "$FREI0R_HEAD" -a "$FREI0R_REVISION" ; then
    REVISIONS[2]="$FREI0R_REVISION"
  fi
  REVISIONS[3]=""
  if test 0 = "$X264_HEAD" -a "$X264_REVISION" ; then
    REVISIONS[3]="$X264_REVISION"
  fi
  REVISIONS[4]=""
  if test 0 = "$LIBVPX_HEAD" -a "$LIBVPX_REVISION" ; then
    REVISIONS[4]="$LIBVPX_REVISION"
  fi
  REVISIONS[5]=""
  if test 0 = "$MOVIT_HEAD" -a "$MOVIT_REVISION" ; then
    REVISIONS[5]="$MOVIT_REVISION"
  fi
  REVISIONS[7]=""
  if test 0 = "$SHOTCUT_HEAD" -a "$SHOTCUT_REVISION" ; then
    REVISIONS[7]="$SHOTCUT_REVISION"
  fi
  REVISIONS[8]=""
  REVISIONS[10]=""
  if test 0 = "$VIDSTAB_HEAD" -a "$VIDSTAB_REVISION" ; then
    REVISIONS[10]="$VIDSTAB_REVISION"
  fi
  REVISIONS[12]=""
  if test 0 = "$LIBOPUS_HEAD" -a "$LIBOPUS_REVISION" ; then
    REVISIONS[12]="$LIBOPUS_REVISION"
  fi
  REVISIONS[13]=""
  if test 0 = "$X265_HEAD" -a "$X265_REVISION" ; then
    REVISIONS[13]="$X265_REVISION"
  fi
  REVISIONS[15]="sdk/8.1"
  REVISIONS[16]=""
  REVISIONS[17]="1.25"
  REVISIONS[18]=""
  if test 0 = "$RUBBERBAND_HEAD" -a "$RUBBERBAND_REVISION" ; then
    REVISIONS[18]="$RUBBERBAND_REVISION"
  fi
  REVISIONS[19]=""
  if test 0 = "$BIGSH0T_HEAD" -a "$BIGSH0T_REVISION" ; then
    REVISIONS[19]="$BIGSH0T_REVISION"
  fi
  REVISIONS[20]=""
  if test 0 = "$ZIMG_HEAD" -a "$ZIMG_REVISION" ; then
    REVISIONS[20]="$ZIMG_REVISION"
  fi
  REVISIONS[21]=""
  if test 0 = "$DAV1D_HEAD" -a "$DAV1D_REVISION" ; then
    REVISIONS[21]="$DAV1D_REVISION"
  fi
  REVISIONS[22]=""
  if test 0 = "$AOM_HEAD" -a "$AOM_REVISION" ; then
    REVISIONS[22]="$AOM_REVISION"
  fi
  REVISIONS[22]=""
  if test 0 = "$AOM_HEAD" -a "$AOM_REVISION" ; then
    REVISIONS[22]="$AOM_REVISION"
  fi
  REVISIONS[23]=""
  if test 0 = "$VMAF_HEAD" -a "$VMAF_REVISION" ; then
    REVISIONS[23]="$VMAF_REVISION"
  fi
  REVISIONS[24]=""
  if test 0 = "$GLAXNIMATE_HEAD" -a "$GLAXNIMATE_REVISION" ; then
    REVISIONS[24]="$GLAXNIMATE_REVISION"
  fi
  REVISIONS[25]=""

  # Figure out the number of cores in the system. Used both by make and startup script
  if test "$TARGET_OS" = "Darwin"; then
    CPUS=$(sysctl -a hw | grep "ncpu:" | cut -d ' ' -f 2)
  else
    CPUS=$(grep "processor.*:" /proc/cpuinfo | wc -l)
  fi
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
  elif test "$TARGET_OS" = "Darwin"; then
    FINAL_INSTALL_DIR="$INSTALL_DIR/build"
  else
    FINAL_INSTALL_DIR="$INSTALL_DIR/Shotcut/Shotcut.app"
  fi
  debug "Using install dir FINAL_INSTALL_DIR=$FINAL_INSTALL_DIR"

  # set global environment for all jobs
  if test "$TARGET_OS" = "Darwin"; then
    [ "$QTDIR" = "" ] && export QTDIR="$HOME/Qt/$QT_VERSION_DARWIN/macos"
    export RANLIB=ranlib
  else
    if test -z "$QTDIR" ; then
      if [ "$(uname -m)" = "x86_64" ]; then
        export QTDIR="$HOME/Qt/$QT_VERSION_DEFAULT/gcc_64"
      else
        export QTDIR="$HOME/Qt/$QT_VERSION_DEFAULT/gcc"
      fi
    fi
    export RANLIB=ranlib
  fi
  export PATH="$FINAL_INSTALL_DIR/bin:$PATH"
  export LD_RUN_PATH="$FINAL_INSTALL_DIR/lib"
  export PKG_CONFIG_PATH="$FINAL_INSTALL_DIR/lib/pkgconfig:$PKG_CONFIG_PATH"

  # PRECONFIG Array holds a command to run before configure for each project
  # CONFIG Array holds the ./configure (or equivalent) command for each project
  # CFLAGS_ Array holds additional CFLAGS for the configure/make step of a given project
  # LDFLAGS_ Array holds additional LDFLAGS for the configure/make step of a given project
  # BUILD Array holds the compile and link command for each project, e.g. make or ninja
  # INSTALL Array holds the install command for each project, e.g. make install

  #####
  # ffmpeg
  CONFIG[0]="./configure --disable-static --disable-doc --enable-gpl --enable-version3 --enable-shared --enable-runtime-cpudetect $CONFIGURE_DEBUG_FLAG"
  if test 1 = "$FFMPEG_SUPPORT_THEORA" ; then
    CONFIG[0]="${CONFIG[0]} --enable-libtheora --enable-libvorbis"
  fi
  if test 1 = "$FFMPEG_SUPPORT_MP3" ; then
    CONFIG[0]="${CONFIG[0]} --enable-libmp3lame"
  fi
  if test 1 = "$FFMPEG_SUPPORT_FAAC" ; then
    CONFIG[0]="${CONFIG[0]} --enable-libfaac --enable-nonfree"
  fi
  if test 1 = "$FFMPEG_SUPPORT_H264" ; then
    CONFIG[0]="${CONFIG[0]} --enable-libx264"
  fi
  if test 1 = "$FFMPEG_SUPPORT_H265" ; then
    CONFIG[0]="${CONFIG[0]} --enable-libx265"
  fi
  if test 1 = "$FFMPEG_SUPPORT_LIBVPX" ; then
    CONFIG[0]="${CONFIG[0]} --enable-libvpx"
  fi
  if test 1 = "$FFMPEG_SUPPORT_OPUS" ; then
    CONFIG[0]="${CONFIG[0]} --enable-libopus"
  fi
  if test 1 = "$FFMPEG_SUPPORT_ZIMG" ; then
    CONFIG[0]="${CONFIG[0]} --enable-libzimg"
  fi
  if test 1 = "$FFMPEG_SUPPORT_QSV" && test "$TARGET_OS" != "Darwin" && test "$TARGET_OS" != "Linux" ; then
    CONFIG[0]="${CONFIG[0]} --enable-libmfx"
  fi
  if test 1 = "$FFMPEG_SUPPORT_DAV1D" ; then
    CONFIG[0]="${CONFIG[0]} --enable-libdav1d"
  fi
  if test 1 = "$FFMPEG_SUPPORT_AOM" ; then
    CONFIG[0]="${CONFIG[0]} --enable-libaom --disable-decoder=libaom_av1"
  fi
  if test 1 = "$FFMPEG_SUPPORT_WEBP" ; then
    CONFIG[0]="${CONFIG[0]} --enable-libwebp"
  fi
  if test 1 = "$FFMPEG_SUPPORT_VMAF" ; then
    CONFIG[0]="${CONFIG[0]} --enable-libvmaf"
  fi
  if test 1 = "$FFMPEG_SUPPORT_JACK" ; then
    CONFIG[0]="${CONFIG[0]} --enable-libjack"
  fi
  # Add optional parameters
  CONFIG[0]="${CONFIG[0]} $FFMPEG_ADDITIONAL_OPTIONS"
  CFLAGS_[0]="-I$FINAL_INSTALL_DIR/include $CFLAGS"
  LDFLAGS_[0]="-L$FINAL_INSTALL_DIR/lib $LDFLAGS"
  if test "$TARGET_OS" = "Darwin"; then
    CFLAGS_[0]="${CFLAGS_[0]} -I/opt/local/include"
    LDFLAGS_[0]="${LDFLAGS_[0]} -L/opt/local/lib"
    BUILD[0]="build_ffmpeg_darwin"
    INSTALL[0]="install_ffmpeg_darwin"
  elif test "$TARGET_OS" = "Linux" ; then
    CONFIG[0]="${CONFIG[0]} --enable-libxcb --enable-libpulse --prefix=$FINAL_INSTALL_DIR"
    BUILD[0]="make -j$MAKEJ"
    INSTALL[0]="make install"
  fi

  #####
  # mlt
  CONFIG[1]="cmake -GNinja -DCMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR -DCMAKE_PREFIX_PATH=$QTDIR -DMOD_QT=OFF -DMOD_QT6=ON -DMOD_GLAXNIMATE_QT6=ON -DMOD_GDK=OFF -DMOD_SDL1=OFF $CMAKE_DEBUG_FLAG"
  # Remember, if adding more of these, to update the post-configure check.
  if test "1" = "$MLT_DISABLE_SOX" ; then
    CONFIG[1]="${CONFIG[1]} -DMOD_SOX=OFF"
  fi
  CFLAGS_[1]="-I$FINAL_INSTALL_DIR/include $ASAN_CFLAGS $CFLAGS"
  if [ "$TARGET_OS" = "Darwin" ]; then
    CONFIG[1]="${CONFIG[1]} -DCMAKE_OSX_ARCHITECTURES='arm64;x86_64'"
    CFLAGS_[1]="${CFLAGS_[1]} -I/opt/local/include"
    LDFLAGS_[1]="${LDFLAGS_[1]} -L/opt/local/lib/libomp"
  fi
  CXXFLAGS_[1]="$CFLAGS[1] -std=c++11"
  LDFLAGS_[1]="${LDFLAGS_[1]} -L$FINAL_INSTALL_DIR/lib $ASAN_LDFLAGS $LDFLAGS"
  BUILD[1]="ninja -j $MAKEJ"
  INSTALL[1]="ninja install"

  #####
  # frei0r
  CONFIG[2]="cmake -GNinja -DCMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR -DWITHOUT_GAVL=1 -DWITHOUT_OPENCV=1 $CMAKE_DEBUG_FLAG"
  [ "$TARGET_OS" = "Darwin" ] && CONFIG[2]="${CONFIG[2]} -DCMAKE_OSX_ARCHITECTURES='arm64;x86_64'"
  CFLAGS_[2]=$CFLAGS
  LDFLAGS_[2]=$LDFLAGS
  BUILD[2]="ninja -j $MAKEJ"
  INSTALL[2]="ninja install"

  #####
  # x264
  CONFIG[3]="./configure --prefix=$FINAL_INSTALL_DIR --disable-lavf --disable-ffms --disable-gpac --disable-swscale --enable-shared --disable-cli $CONFIGURE_DEBUG_FLAG"
  CFLAGS_[3]=$CFLAGS
  if test "$TARGET_OS" = "Darwin" ; then
    CFLAGS_[3]="-I. -fno-common -read_only_relocs suppress ${CFLAGS_[3]}"
  fi
  LDFLAGS_[3]=$LDFLAGS
  BUILD[3]="make -j$MAKEJ"
  INSTALL[3]="make install"

  #####
  # libvpx
  CONFIG[4]="./configure --prefix=$FINAL_INSTALL_DIR --enable-vp8 --enable-postproc --enable-multithread --disable-install-docs --disable-debug-libs --disable-examples --disable-unit-tests --extra-cflags=-std=c99 $CONFIGURE_DEBUG_FLAG"
  [ "$TARGET_ARCH" != "arm64" ] && CONFIG[4]="${CONFIG[4]} --enable-runtime-cpu-detect"
  if test "$TARGET_OS" = "Linux" ; then
    CONFIG[4]="${CONFIG[4]} --enable-shared"
  elif test "$TARGET_OS" = "Darwin" ; then
    [ "$TARGET_ARCH" != "arm64" ] && CONFIG[4]="${CONFIG[4]} --disable-avx512"
  fi
  CFLAGS_[4]=$CFLAGS
  LDFLAGS_[4]=$LDFLAGS
  BUILD[4]="make -j$MAKEJ"
  INSTALL[4]="make install"

  #####
  # movit
  CONFIG[5]="./autogen.sh --prefix=$FINAL_INSTALL_DIR"
  if test "$TARGET_OS" = "Darwin"; then
    CFLAGS_[5]="$CFLAGS -I/opt/local/include"
  else
    CFLAGS_[5]="$CFLAGS"
  fi
  CXXFLAGS_[5]=$CFLAGS[5]
  LDFLAGS_[5]=$LDFLAGS
  if [ "$TARGET_OS" = "Darwin" ]; then
    BUILD[5]="build_movit_darwin"
  else
    BUILD[5]="make -j$MAKEJ RANLIB="$RANLIB" libmovit.la"
  fi
  INSTALL[5]="make install"

  #####
  # shotcut
  CONFIG[7]="cmake -G Ninja -D CMAKE_PREFIX_PATH=$QTDIR -D SHOTCUT_VERSION=$SHOTCUT_VERSION $CMAKE_DEBUG_FLAG"
  if test "$TARGET_OS" = "Darwin" ; then
    CONFIG[7]="${CONFIG[7]} -D CMAKE_INSTALL_PREFIX=."
    CONFIG[7]="${CONFIG[7]} -D CMAKE_OSX_ARCHITECTURES='arm64;x86_64'"
  else
    CONFIG[7]="${CONFIG[7]} -D CMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR"
  fi
  CFLAGS_[7]="$ASAN_CFLAGS $CFLAGS"
  LDFLAGS_[7]="$ASAN_LDFLAGS $LDFLAGS"
  BUILD[7]="ninja -j $MAKEJ"
  if [ "$TARGET_OS" = "Darwin" ]; then
    INSTALL[7]="ninja install"
  else
    INSTALL[7]="install_shotcut_linux"
  fi

  #####
  # ladspa
  PRECONFIG[8]="autoreconf -i"
  CONFIG[8]="./configure --prefix=$FINAL_INSTALL_DIR"
  if [ "$TARGET_OS" = "Darwin" -a "$TARGET_ARCH" = "arm64" ]; then
    BUILD[8]="build_ladspa_darwin"
  else
    CONFIG[8]="${CONFIG[8]} --enable-sse"
    BUILD[8]="make -j$MAKEJ"
  fi
  LDFLAGS_[8]=$LDFLAGS
  INSTALL[8]="install_ladspa"

  #####
  # vid.stab
  CONFIG[10]="cmake -GNinja -DCMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR -DCMAKE_INSTALL_LIBDIR=lib $CMAKE_DEBUG_FLAG"
  if test "$TARGET_OS" = "Darwin" ; then
    CONFIG[10]="${CONFIG[10]} -DCMAKE_OSX_ARCHITECTURES='arm64;x86_64'"
    CONFIG[10]="${CONFIG[10]} -DOpenMP_C_FLAGS=-I/opt/local/include/libomp -DOpenMP_C_LIB_NAMES=libomp -DOpenMP_libomp_LIBRARY=omp"
    LDFLAGS_[10]="$LDFLAGS -L/opt/local/lib/libomp"
  fi
  CFLAGS_[10]=$CFLAGS
  BUILD[10]="ninja -j $MAKEJ"
  INSTALL[10]="ninja install"

  #####
  # opus
  [ ! -e "$SOURCE_DIR"/opus/configure ] && PRECONFIG[12]="./autogen.sh"
  CONFIG[12]="./configure --prefix=$FINAL_INSTALL_DIR"
  if test "$TARGET_OS" = "Darwin"; then
    CFLAGS_[12]="$CFLAGS -I/opt/local/include"
  else
    CFLAGS_[12]="$CFLAGS"
  fi
  LDFLAGS_[12]=$LDFLAGS
  BUILD[12]="make -j$MAKEJ"
  INSTALL[12]="make install"

  #####
  # x265
  PRECONFIG[13]="preconfig_x265"
  CONFIG[13]="cmake -G Ninja -D CMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR -DENABLE_CLI=OFF -D ENABLE_SHARED=ON -D EXTRA_LIB='x265_main10.a' -D LINKED_10BIT=ON -D EXTRA_LINK_FLAGS='-L.' $CMAKE_DEBUG_FLAG"
  CFLAGS_[13]=$CFLAGS
  LDFLAGS_[13]=$LDFLAGS
  BUILD[13]="ninja -j $MAKEJ"
  INSTALL[13]="install_x265"

  #####
  # nv-codec-headers
  CONFIG[15]="sed -i s,/usr/local,$FINAL_INSTALL_DIR, Makefile"
  BUILD[15]="make -j$MAKEJ"
  INSTALL[15]="make install"

  #####
  # AMF - no build required
  CONFIG[16]=""

  #####
  # QSV mfx_dispatch
  CONFIG[17]="./configure --prefix=$FINAL_INSTALL_DIR"
  CFLAGS_[17]="$CFLAGS"
  LDFLAGS_[17]=$LDFLAGS
  BUILD[17]="make -j$MAKEJ"
  INSTALL[17]="make install"

  #####
  # rubberband
  CONFIG[18]="meson setup builddir --prefix=$FINAL_INSTALL_DIR --libdir=$FINAL_INSTALL_DIR/lib"
  CFLAGS_[18]=$CFLAGS
  LDFLAGS_[18]=$LDFLAGS
  BUILD[18]="ninja -C builddir -j $MAKEJ"
  INSTALL[18]="meson install -C builddir"

  #####
  # bigsh0t
  CONFIG[19]="cmake -GNinja -DCMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR $CMAKE_DEBUG_FLAG"
  [ "$TARGET_OS" = "Darwin" ] && CONFIG[19]="${CONFIG[19]} -DCMAKE_OSX_ARCHITECTURES='arm64;x86_64'"
  CFLAGS_[19]=$CFLAGS
  LDFLAGS_[19]=$LDFLAGS
  BUILD[19]="ninja -j $MAKEJ"
  INSTALL[19]="install -p -c *.so $FINAL_INSTALL_DIR/lib/frei0r-1"

  #####
  # zimg
  [ ! -e "$SOURCE_DIR"/zimg/configure ] && PRECONFIG[20]="./autogen.sh"
  CONFIG[20]="./configure --prefix=$FINAL_INSTALL_DIR"
  if test "$TARGET_OS" = "Darwin"; then
    CFLAGS_[20]="$CFLAGS -I/opt/local/include"
  else
    CFLAGS_[20]="$CFLAGS"
  fi
  LDFLAGS_[20]=$LDFLAGS
  BUILD[20]="make -j$MAKEJ"
  INSTALL[20]="make install"

  #####
  # dav1d
  CONFIG[21]="meson setup builddir --prefix=$FINAL_INSTALL_DIR --libdir=$FINAL_INSTALL_DIR/lib"
  if [ "$DEBUG_BUILD" = "1" ]; then
    CONFIG[21]="${CONFIG[21]} --buildtype=debug"
  else
    CONFIG[21]="${CONFIG[21]} --buildtype=release"
  fi
  CFLAGS_[21]=$CFLAGS
  LDFLAGS_[21]=$LDFLAGS
  BUILD[21]="ninja -C builddir -j $MAKEJ"
  INSTALL[21]="meson install -C builddir"

  #####
  # aom
  CONFIG[22]="cmake -GNinja -DCMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR $CMAKE_DEBUG_FLAG -DBUILD_SHARED_LIBS=1 -DENABLE_DOCS=0 -DENABLE_EXAMPLES=0 -DENABLE_TESTDATA=0 -DENABLE_TESTS=0 -DENABLE_TOOLS=0 ../aom"
  [ "$TARGET_OS" = "Darwin" -a "$TARGET_ARCH" = "arm64" ] && CONFIG[22]="${CONFIG[22]} -DCONFIG_RUNTIME_CPU_DETECT=0"
  CFLAGS_[22]=$CFLAGS
  LDFLAGS_[22]=$LDFLAGS
  BUILD[22]="ninja -j $MAKEJ"
  INSTALL[22]="ninja install"

  #####
  # vmaf
  if [ "$TARGET_OS" = "Darwin" ]; then
    CONFIG[23]="meson setup libvmaf/build-arm64 libvmaf --prefix=$FINAL_INSTALL_DIR --libdir=$FINAL_INSTALL_DIR/lib --cross-file=arm64-darwin -Denable_tests=false -Denable_docs=false -Dbuilt_in_models=false"
    BUILD[23]="build_vmaf_darwin"
  else
    CONFIG[23]="meson setup libvmaf/build libvmaf --prefix=$FINAL_INSTALL_DIR --libdir=$FINAL_INSTALL_DIR/lib"
    BUILD[23]="ninja -C libvmaf/build -j $MAKEJ"
  fi
  if [ "$DEBUG_BUILD" = "1" ]; then
    CONFIG[23]="${CONFIG[23]} --buildtype=debug"
  else
    CONFIG[23]="${CONFIG[23]} --buildtype=release"
  fi
  CFLAGS_[23]=$CFLAGS
  LDFLAGS_[23]=$LDFLAGS
  INSTALL[23]="install_vmaf"

  #####
  # glaxnimate
  CONFIG[24]="cmake -G Ninja -DCMAKE_PREFIX_PATH=$QTDIR -DCMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR $CMAKE_DEBUG_FLAG"
  if [ "$TARGET_OS" = "Darwin" ]; then
    CONFIG[24]="${CONFIG[24]} -DCMAKE_OSX_ARCHITECTURES='arm64;x86_64'"
    CONFIG[24]="${CONFIG[24]} -DPython3_EXECUTABLE=/opt/local/Library/Frameworks/Python.framework/Versions/${PYTHON_VERSION_DARWIN}/bin/python${PYTHON_VERSION_DARWIN} -D PYTHON_EXECUTABLE=/opt/local/Library/Frameworks/Python.framework/Versions/${PYTHON_VERSION_DARWIN}/bin/python${PYTHON_VERSION_DARWIN}"
  fi
  CFLAGS_[24]="$ASAN_CFLAGS $CFLAGS"
  LDFLAGS_[24]="$ASAN_LDFLAGS $LDFLAGS"
  BUILD[24]="ninja -j $MAKEJ"
  INSTALL[24]="ninja translations install"

  #####
  # gopro2gpx
  CONFIG[25]="cmake -G Ninja -DCMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR $CMAKE_DEBUG_FLAG"
  [ "$TARGET_OS" = "Darwin" ] && CONFIG[25]="${CONFIG[25]} -DCMAKE_OSX_ARCHITECTURES='arm64;x86_64'"
  CFLAGS_[25]="$CFLAGS"
  LDFLAGS_[25]="$LDFLAGS"
  BUILD[25]="ninja -j $MAKEJ"
  INSTALL[25]="install -p -c gopro2gpx $FINAL_INSTALL_DIR/bin"
}

function build_ffmpeg_darwin {
  make distclean || true
  MYCONFIG=`lookup CONFIG ffmpeg`
  cmd $MYCONFIG --prefix=build-arm64
  cmd make -j $MAKEJ
  cmd make install
  cmd make distclean
  ASFLAGS="-arch x86_64"
  cmd $MYCONFIG --prefix=build-x86_64 --enable-cross-compile --arch=x86_64 --enable-x86asm --cc='clang -arch x86_64'
  cmd make -j $MAKEJ
  cmd make install
}

function install_ffmpeg_darwin {
  for file in ffmpeg ffplay ffprobe; do
    cmd lipo -create build-arm64/bin/$file build-x86_64/bin/$file -output "$FINAL_INSTALL_DIR"/bin/$file
    libs=$(otool -L "$FINAL_INSTALL_DIR"/bin/$file | awk '/^\tbuild-arm64\// || /^\tbuild-x86_64\// {print $1}')
    for lib in $libs; do
      basename_lib=$(basename "$lib")
      cmd install_name_tool -change "$lib" "$FINAL_INSTALL_DIR"/lib/$basename_lib "$FINAL_INSTALL_DIR"/bin/$file
    done
  done
  for lib in libavcodec libavdevice libavfilter libavformat libavutil libpostproc libswresample libswscale; do
    file=$(find build-arm64 -type l -name $lib'.*.dylib' -exec basename {} \;)
    cmd ln -sf $file "$FINAL_INSTALL_DIR"/lib/$lib.dylib
    cmd lipo -create build-arm64/lib/$file build-x86_64/lib/$file -output "$FINAL_INSTALL_DIR"/lib/$file
    cmd install_name_tool -id "$FINAL_INSTALL_DIR"/lib/$file "$FINAL_INSTALL_DIR"/lib/$file
    libs=$(otool -L "$FINAL_INSTALL_DIR"/lib/$file | awk '/^\tbuild-arm64\// || /^\tbuild-x86_64\// {print $1}')
    for lib in $libs; do
      basename_lib=$(basename "$lib")
      cmd install_name_tool -change "$lib" "$FINAL_INSTALL_DIR"/lib/$basename_lib "$FINAL_INSTALL_DIR"/lib/$file
    done
  done
  cmd sed -e "s,=build-arm64,=${FINAL_INSTALL_DIR},g" -i .bak build-arm64/lib/pkgconfig/*.pc
  cmd cp -a build-arm64/lib/pkgconfig/*.pc "$FINAL_INSTALL_DIR"/lib/pkgconfig/
  cmd cp -a build-arm64/include/ "$FINAL_INSTALL_DIR"/include/
}

function build_ladspa_darwin {
  cmd make -j $MAKEJ CC="clang -arch arm64 -arch x86_64"
}

function install_ladspa {
  cmd make install
  cmd install -d "$FINAL_INSTALL_DIR"/include
  cmd install -p -c ladspa.h "$FINAL_INSTALL_DIR"/include
}

function build_movit_darwin {
  cmd make -j $MAKEJ RANLIB="$RANLIB" CC="clang -arch arm64 -arch x86_64" CXX="clang++ -arch arm64 -arch x86_64" libmovit.la
}

function install_shotcut_linux {
  cmd ninja install
  cmd install -p -c COPYING "$FINAL_INSTALL_DIR"
  cmd install -p -c "$QTDIR"/translations/qt_*.qm "$FINAL_INSTALL_DIR"/share/shotcut/translations
  cmd install -p -c "$QTDIR"/translations/qtbase_*.qm "$FINAL_INSTALL_DIR"/share/shotcut/translations
  cmd install -p -c "$QTDIR"/lib/libQt6{Core,Core5Compat,DBus,Gui,Multimedia,Network,OpenGL,OpenGLWidgets,Qml,QmlModels,QmlWorkerScript,Quick,QuickControls2,QuickControls2Impl,QuickDialogs2,QuickDialogs2QuickImpl,QuickDialogs2Utils,QuickLayouts,QuickTemplates2,QuickWidgets,Sql,Svg,SvgWidgets,UiTools,Widgets,Xml,X11Extras,XcbQpa}.so.6 "$FINAL_INSTALL_DIR"/lib
  cmd install -p -c "$QTDIR"/lib/lib{icudata,icui18n,icuuc}.so* "$FINAL_INSTALL_DIR"/lib
  cmd install -d "$FINAL_INSTALL_DIR"/lib/qt6/sqldrivers
  cmd cp -a "$QTDIR"/plugins/{egldeviceintegrations,generic,iconengines,imageformats,multimedia,platforminputcontexts,platforms,platformthemes,tls,wayland-decoration-client,wayland-graphics-integration-client,wayland-shell-integration,xcbglintegrations} "$FINAL_INSTALL_DIR"/lib/qt6
  cmd cp -p "$QTDIR"/plugins/sqldrivers/libqsqlite.so "$FINAL_INSTALL_DIR"/lib/qt6/sqldrivers
  cmd cp -a "$QTDIR"/qml "$FINAL_INSTALL_DIR"/lib
  cmd install -d "$FINAL_INSTALL_DIR"/lib/va
  cmd install -p -c /usr/lib/x86_64-linux-gnu/dri/*_drv_video.so "$FINAL_INSTALL_DIR"/lib/va
}

function build_vmaf_darwin {
  cmd ninja -C libvmaf/build-arm64 -j $MAKEJ
  export CFLAGS="$CFLAGS -arch x86_64" CXXFLAGS="$CXXFLAGS -arch x86_64"
  cmd meson setup libvmaf/build-x86_64 libvmaf --cross-file=x86_64-darwin -Denable_tests=false -Denable_docs=false -Dbuilt_in_models=false
  cmd ninja -C libvmaf/build-x86_64 -j $MAKEJ
}

function install_vmaf {
  if [ "$TARGET_OS" = "Darwin" ]; then
    cmd ninja install -C libvmaf/build-arm64
    cmd rm "$FINAL_INSTALL_DIR"/lib/libvmaf.1.dylib
    cmd lipo -create libvmaf/build-arm64/src/libvmaf.1.dylib libvmaf/build-x86_64/src/libvmaf.1.dylib -output "$FINAL_INSTALL_DIR"/lib/libvmaf.1.dylib
    cmd install_name_tool -id "$FINAL_INSTALL_DIR"/lib/libvmaf.1.dylib "$FINAL_INSTALL_DIR"/lib/libvmaf.1.dylib
  else
    cmd ninja -C libvmaf/build install
  fi
  cmd install -d "$FINAL_INSTALL_DIR"/share/vmaf
  cmd install -p -c model/*.json "$FINAL_INSTALL_DIR"/share/vmaf
}

function preconfig_x265 {
  [ ! -d "10bit" ] && mkdir 10bit
  cd 10bit
  cmd cmake -G Ninja -D ENABLE_CLI=OFF -D ENABLE_SHARED=OFF -D EXPORT_C_API=OFF -D HIGH_BIT_DEPTH=ON $CMAKE_DEBUG_FLAG ../source
  cmd ninja -j$MAKEJ
  cd ../source
  cmd ln -s ../10bit/libx265.a libx265_main10.a
}

function install_x265 {
  cmd mv libx265.a libx265_main.a
  ar -M <<EOF
CREATE libx265.a
ADDLIB libx265_main.a
ADDLIB libx265_main10.a
SAVE
END
EOF
  cmd ninja install
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
              if [ "$1" = "rubberband" ]; then
                MAIN_GIT_BRANCH=default
              elif [ "$1" = "bigsh0t" ]; then
                MAIN_GIT_BRANCH=main
              else
                MAIN_GIT_BRANCH=master
              fi
              cmd git checkout $MAIN_GIT_BRANCH || die "Unable to git checkout $MAIN_GIT_BRANCH"
              cmd git --no-pager pull $REPOLOC $MAIN_GIT_BRANCH || die "Unable to git pull sources for $1"
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
  if test "$TARGET_OS" = "Linux" -a "$ACTION_ARCHIVE" = "1" ; then
    feedback_status Making source archive
    cmd cd "$SOURCE_DIR"/..
    cat >src/README <<END_OF_SRC_README
Basic Build Instructions for Shotcut

We will not be able to cover everything here, and you are largely on your
own, but here are some hints. There is a big build bash script that is used
to make Shotcuts daily builds. It is the authoritative install reference:
  src/shotcut/scripts/build-shotcut.sh

We cannot cover how to build all of Shotcut's dependencies from scratch here.
On Linux, we rely upon Ubuntu's packages to provide most of the
more mundane dependencies. The rest like x264, x265, libvpx, libopus,
FFmpeg, and frei0r are provided by the script.

For macOS, we rely upon macports to provide the dependencies:
  port install ffmpeg libsamplerate libsdl2 sox glib2 jack

For Windows, see this page on the MLT wiki about getting pre-built
dependencies from various sources on the Internet:
  https://www.mltframework.org/docs/windowsbuild/
Except, now we build FFmpeg instead of using a pre-built copy.

As for Shotcut itself:
  cd shotcut; mkdir build ; cd build ; cmake .. ; cmake --build .

Then, there is the app bundling so that dependencies can be located and Qt
plugins included. For that you really need to see the build script; it
is fairly complicated especially on macOS. On Linux, we just use a
common install prefix and the build script generates shell scripts to
establish a redirected environment. On Windows, everything is relative
to the directory containing the .exe. DLLs are in the same directory as
the .exe, and the lib and share folders are sub-directories. On macOS, all
dependencies need to be put into the correct locations in Shotcut.app,
and the build script modifies all dylibs to pull them in and make their
inter-dependencies relative to the executable. If you are just building for
yourself, you do not need to do that. You can just let Shotcut use
the macports dependencies in /opt/local.
END_OF_SRC_README
    cmd mkdir -p "$INSTALL_DIR" 2> /dev/null
    cmd tar --exclude-vcs -cJf "$INSTALL_DIR"/src.txz src
  fi
}

######################################################################
# ACTION COMPILE-INSTALL FUNCTIONS
######################################################################

#################################################################
# replace_rpath
# changes the embedded name of a dylib to its full install path so
# it can be found later during bundling
function replace_rpath {
  library=$1
  install_name=$(otool -D "$FINAL_INSTALL_DIR"/lib/lib${library}.dylib | tail -n 1)
  cmd install_name_tool -id "$FINAL_INSTALL_DIR"/lib/$(basename "$install_name") "$FINAL_INSTALL_DIR"/lib/lib${library}.dylib
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

  #####
  # Configure
  if [ "$ACTION_CONFIGURE" = "1" ]; then

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

  if [ "$MYCONFIG" != "" ]; then
    cmd $MYCONFIG || die "Unable to configure $1"
    feedback_status Done configuring $1
  fi

  fi # if [ "$ACTION_CONFIGURE" = "1" ]

  #####
  # Compile
  feedback_status Building $1 - this could take some time
  if [ "$MYBUILD" != "" ]; then
    cmd $MYBUILD || die "Unable to build $1"
  fi

  feedback_status Done building $1

  #####
  # Install
  feedback_status Installing $1
  if [ "$MYINSTALL" != "" ]; then
    cmd $MYINSTALL || die "Unable to install $1"
  fi

  # Special hack for macOS
  if [ "Darwin" = "$TARGET_OS" ]; then
    # CMake identifies the dylibs with an @rpath that breaks our recursive bundling process.
    # These names need to changed immediately after each lib is installed so that dependants
    # link using the full name, and the bundling process can locate the dependency.
    if [ "aom" = "$1" -o "x265" = "$1" ]; then
      replace_rpath $1
    elif [ "mlt" = "$1" ]; then
      replace_rpath mlt-7
      replace_rpath mlt++-7
    elif [ "vid.stab" = "$1" ]; then
      cmd sed -e 's/-fopenmp//' -i .bak "$FINAL_INSTALL_DIR/lib/pkgconfig/vidstab.pc"
    fi
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

  if [ "$ACTION_ARCHIVE" = "1" ] && [ "$TARGET_OS" = "Linux" ]; then
    log Copying some libs from system
    for lib in "$FINAL_INSTALL_DIR"/lib/qt6/{egldeviceintegrations,generic,iconengines,imageformats,multimedia,platforminputcontexts,platforms,platformthemes,tls,wayland-decoration-client,wayland-graphics-integration-client,wayland-shell-integration,xcbglintegrations}/*.so; do
      bundle_libs "$lib"
    done
    for lib in "$FINAL_INSTALL_DIR"/{lib,lib/mlt,lib/frei0r-1,lib/ladspa,lib/va}/*.so*; do
      bundle_libs "$lib"
    done
    bundle_libs "$FINAL_INSTALL_DIR"/bin/glaxnimate
    cmd rm *.bundled
  fi

  feedback_status Done configuring, compiling and installing all sources
}

######################################################################
# ACTION CREATE_STARTUP_SCRIPT
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
  if which dpkg ; then
    echo Found dpkg - running dpkg -l to grep libc6
    dpkg -l | grep libc6
  else
    if which rpm ; then
      echo Found rpm - running rpm -qa to grep libc6
      rpm -qa | grep libc
    else
      echo Found neither dpkg or rpm...
    fi
  fi
}

function bundle_libs
{
  log bundling library dependencies of $(basename "$1")
  target=$(dirname "$1")/$(basename "$1")
  basename_target=$(basename "$target")
  # See https://github.com/AppImage/pkg2appimage/blob/master/excludelist
  libs=$(ldd "$target" |
    awk '($3  ~ /^\/(lib|usr)\//) &&
         ($3 !~ /\/libld-linux\./) &&
         ($3 !~ /\/libld-linux-x86-64\./) &&
         ($3 !~ /\/libanl\./) &&
         ($3 !~ /\/libBrokenLocale\./) &&
         ($3 !~ /\/libcidn\./) &&
         ($3 !~ /\/libc\./) &&
         ($3 !~ /\/libdl\./) &&
         ($3 !~ /\/libm\./) &&
         ($3 !~ /\/libmvec\./) &&
         ($3 !~ /\/libnss_compat\./) &&
         ($3 !~ /\/libnss_dns\./) &&
         ($3 !~ /\/libnss_files\./) &&
         ($3 !~ /\/libnss_hesiod\./) &&
         ($3 !~ /\/libnss_nisplus\./) &&
         ($3 !~ /\/libnss_nis\./) &&
         ($3 !~ /\/libpthread\./) &&
         ($3 !~ /\/libresolv\./) &&
         ($3 !~ /\/librt\./) &&
         ($3 !~ /\/libthread_db\./) &&
         ($3 !~ /\/libutil\./) &&
         ($3 !~ /\/libstdc\+\+\./) &&
         ($3 !~ /\/libGL\./) &&
         ($3 !~ /\/libGLdispatch\./) &&
         ($3 !~ /\/libGLX\./) &&
         ($3 !~ /\/libEGL\./) &&
         ($3 !~ /\/libgmp\./) &&
         ($3 !~ /\/libgbm\./) &&
         ($3 !~ /\/libdrm/) &&
         ($3 !~ /\/libglapi\./) &&
         ($3 !~ /\/libxcb\./) &&
         ($3 !~ /\/libxcb-dri2\./) &&
         ($3 !~ /\/libxcb-dri3\./) &&
         ($3 !~ /\/libfribidi\./) &&
         ($3 !~ /\/libX11\./) &&
         ($3 !~ /\/libgio\./) &&
         ($3 !~ /\/libasound\./) &&
         ($3 !~ /\/libgdk_pixbuf-2.0\./) &&
         ($3 !~ /\/libfontconfig\./) &&
         ($3 !~ /\/libthai\./) &&
         ($3 !~ /\/libfreetype\./) &&
         ($3 !~ /\/libharfbuzz\./) &&
         ($3 !~ /\/libcom_err\./) &&
         ($3 !~ /\/libexpat\./) &&
         ($3 !~ /\/libgcc_s\./) &&
         ($3 !~ /\/libglib-2.0\./) &&
         ($3 !~ /\/libgpg-error\./) &&
         ($3 !~ /\/libICE\./) &&
         ($3 !~ /\/libp11-kit\./) &&
         ($3 !~ /\/libSM\./) &&
         ($3 !~ /\/libusb-1.0\./) &&
         ($3 !~ /\/libuuid\./) &&
         ($3 !~ /\/libz\./) &&
         ($3 !~ /\/libgobject-2.0\./) &&
         ($3 !~ /\/libpangoft2-1.0\./) &&
         ($3 !~ /\/libpangocairo-1.0\./) &&
         ($3 !~ /\/libpango-1.0\./) &&
         ($3 !~ /\/libjack\./) &&
         ($3 !~ /nvidia/) &&
         ($3 !~ /\/libcairo\./) \
         {print $3}')
  for lib in $libs; do
    basename_lib=$(basename "$lib")
    if [ "$basename_lib" != "$basename_target" ] && [ ! -e "$FINAL_INSTALL_DIR/lib/$basename_lib" ]; then
      cmd cp --preserve=timestamps "$lib" "$FINAL_INSTALL_DIR/lib" || die "failed to copy $lib"
    fi
  done
  for lib in $libs; do
    basename_lib=$(basename "$lib")
    if [ "$basename_lib" != "$basename_target" ] && [ ! -e "$basename_lib".bundled ]; then
      touch "$basename_lib".bundled
      bundle_libs "$lib"
    fi
  done
}

function fixlibs()
{
  log bundling and fixing library paths of $(basename "$1")
  target=$(dirname "$1")/$(basename "$1")
  trace fixlibs $target
  basename_target=$(basename "$target")
  libs=$(otool -L "$target" |
    awk '/^\t@rpath\/Qt/ || /^\t\/opt\/local/ || /^\t\/Applications\// || /^\t\/Users\// || /^\tlibvidstab/ {print $1}')

  # if the target is a lib, change its id
  if [ $(echo "$1" | grep '\.dylib$') ] || [ $(echo "$1" | grep '\.so$') ]; then
    cmd install_name_tool -id "@rpath/$(basename "$1")" "$target"
  fi

  for lib in $libs; do
    basename_lib=$(basename "$lib")
    if [ "$basename_lib" != "$basename_target" ]; then
      libpath=$(echo $lib | sed "s|@rpath\/Qt|${QTDIR}\/lib\/Qt|")
      if [ $(echo "$lib" | grep -v '\.dylib$') ] && [ $(echo "$lib" | grep -v '\.so$') ]; then
        basename_lib="$basename_lib".dylib
      fi
      cmd cp -n "$libpath" "Frameworks/$basename_lib"
      cmd install_name_tool -change "$lib" "@rpath/$basename_lib" "$target"
    fi
  done

  libs=$(otool -L "$target" | awk '/^\t@rpath\// {print $1}')
  for lib in $libs; do
    basename_lib=$(basename "$lib")
    if [ "$basename_lib" != "$basename_target" ] && [ ! -e "$basename_lib".bundled ]; then
      touch "$basename_lib".bundled
      fixlibs "Frameworks/$basename_lib"
    fi
  done
}

function deploy_mac
{
  trace "Entering deploy_mac @ = $@"
  pushd .

  # Change to right directory
  log Changing directory to shotcut
  cmd cd shotcut || die "Unable to change to directory shotcut"

  BUILD_DIR="./Shotcut.app/Contents"

  # copy Qt translations
  cmd mkdir -p "$BUILD_DIR/Resources/shotcut/translations"
  # try QTDIR first
  if [ -d "$QTDIR/translations" ]; then
    cmd cp -p "$QTDIR"/translations/qt_*.qm "$BUILD_DIR/Resources/shotcut/translations/"
    cmd cp -p "$QTDIR"/translations/qtbase_*.qm "$BUILD_DIR/Resources/shotcut/translations/"
  # try Qt Creator after that
  elif [ -d "/Applications/Qt Creator.app/Contents/Resources/translations" ]; then
    cmd cp -p "/Applications/Qt Creator.app/Contents/Resources/translations/"qt_*.qm "$BUILD_DIR/Resources/shotcut/translations/"
    cmd cp -p "/Applications/Qt Creator.app/Contents/Resources/translations/"qtbase_*.qm "$BUILD_DIR/Resources/shotcut/translations/"
  fi
  # copy Shotcut translations
  cmd cp translations/*.qm "$BUILD_DIR/Resources/shotcut/translations/"

  # This little guy helps Qt apps find the Qt plugins!
  printf "[Paths]\nPlugins=PlugIns/qt\nQml2Imports=Resources/qml\n" > "$BUILD_DIR/Resources/qt.conf"

  cmd cd "$BUILD_DIR" || die "Unable to change directory to $BUILD_DIR"

  log Copying supplementary executables
  cmd mkdir -p MacOS 2>/dev/null
  cmd cp -a "$FINAL_INSTALL_DIR"/bin/{melt,ffmpeg,ffplay,ffprobe,glaxnimate,gopro2gpx} MacOS
  cmd mkdir -p Frameworks 2>/dev/null
  cmd cp -p ../../lib/libCuteLogger.dylib Frameworks
  for exe in MacOS/Shotcut MacOS/melt MacOS/ffmpeg MacOS/ffplay MacOS/ffprobe MacOS/glaxnimate; do
    fixlibs "$exe"
    log fixing rpath of executable "$exe"
    cmd install_name_tool -delete_rpath "$FINAL_INSTALL_DIR/lib" "$exe" 2> /dev/null
    cmd install_name_tool -delete_rpath "$QTDIR/lib" "$exe" 2> /dev/null
    cmd install_name_tool -add_rpath "@executable_path/../Frameworks" "$exe"
  done
  cmd cp -p "$FINAL_INSTALL_DIR"/lib/libaom.2.dylib Frameworks

  # MLT plugins
  log Copying MLT plugins
  cmd mkdir -p PlugIns/mlt 2>/dev/null
  cmd cp "$FINAL_INSTALL_DIR"/lib/mlt/libmlt*.so PlugIns/mlt
  cmd cp -a "$FINAL_INSTALL_DIR"/share/mlt Resources
  # Copy libvidstab here temporarily so it can be found by fixlibs.
  cmd cp -p "$FINAL_INSTALL_DIR"/lib/libvidstab*.dylib .
  for lib in PlugIns/mlt/*; do
    fixlibs "$lib"
  done
  cmd rm libvidstab*.dylib

  # Qt plugins
  log Copying Qt plugins
  cmd mkdir -p PlugIns/qt/sqldrivers 2>/dev/null
  # try QTDIR first
  if [ -d "$QTDIR/plugins" ]; then
    cmd cp -a "$QTDIR/plugins"/{generic,iconengines,imageformats,mediaservice,multimedia,platforms,styles,tls} PlugIns/qt
    cmd cp -p "$QTDIR/plugins/sqldrivers/libqsqlite.dylib" PlugIns/qt/sqldrivers
  # try Qt Creator next
  elif [ -d "/Applications/Qt Creator.app/Contents/PlugIns" ]; then
    cmd cp -a "/Applications/Qt Creator.app/Contents/PlugIns"/{generic,iconengines,imageformats,mediaservice,multimedia,platforms,styles,tls} PlugIns/qt
  fi
  for dir in PlugIns/qt/*; do
    for lib in $dir/*; do
      fixlibs "$lib"
    done
  done

  # Qt QML modules
  log Copying Qt QML modules
  # try QTDIR first
  if [ -d "$QTDIR/qml" ]; then
    cmd cp -a "$QTDIR/qml" Resources
  # try Qt Creator next
  elif [ -d "/Applications/Qt Creator.app/Contents/Imports/qtquick2" ]; then
    cmd cp -a "/Applications/Qt Creator.app/Contents/Imports/qtquick2" Resources/qml
  fi
  for lib in $(find Resources -name '*.dylib'); do
    fixlibs "$lib"
  done

  # frei0r plugins
  log Copying frei0r plugins
  cmd mkdir PlugIns/frei0r-1 2>/dev/null
  cmd cp -a "$FINAL_INSTALL_DIR"/lib/frei0r-1 PlugIns
  for lib in PlugIns/frei0r-1/*; do
    fixlibs "$lib"
  done

  # LADSPA plugins
  log Copying LADSPA plugins
  cmd mkdir PlugIns/ladspa 2>/dev/null
  cmd cp -a "$FINAL_INSTALL_DIR"/lib/ladspa/* PlugIns/ladspa
  for lib in PlugIns/ladspa/*; do
    fixlibs "$lib"
  done
  cmd rm *.bundled

  # Movit shaders
  log Copying Movit shaders
  cmd cp -a "$FINAL_INSTALL_DIR"/share/movit Resources

  # VMAF models
  log Copying VMAF models
  cmd cp -a "$FINAL_INSTALL_DIR"/share/vmaf Resources

  # Glaxnimate data
  log Copying Glaxnimate data
  cmd cp -a "$FINAL_INSTALL_DIR"/share/glaxnimate Resources
  cmd install -d lib
  cmd cp -pLR /opt/local/Library/Frameworks/Python.framework/Versions/${PYTHON_VERSION_DARWIN}/lib/python${PYTHON_VERSION_DARWIN} lib

  log Fixing rpath in libraries
  cmd find . -name '*.dylib' -exec sh -c "install_name_tool -delete_rpath \"/opt/local/lib/libomp\" {} 2> /dev/null" \;
  cmd find . -name '*.dylib' -exec sh -c "install_name_tool -delete_rpath \"$FINAL_INSTALL_DIR/lib\" {} 2> /dev/null" \;
  cmd find . -name '*.so' -exec sh -c "install_name_tool -delete_rpath \"$FINAL_INSTALL_DIR/lib/mlt\" {} 2>/dev/null" \;
  cmd find . -name '*.dylib' -exec sh -c "install_name_tool -delete_rpath \"$QTDIR/lib\" {} 2>/dev/null" \;
  cmd find . -name '*.dylib' -exec sh -c "install_name_tool -delete_rpath \"@loader_path/../../../\" {} 2>/dev/null" \;
  cmd find . -name '*.dylib' -exec sh -c "install_name_tool -delete_rpath \"@loader_path/../../lib\" {} 2>/dev/null" \;
  cmd find . -name '*.dylib' -exec sh -c "install_name_tool -delete_rpath \"@loader_path/../../../lib\" {} 2>/dev/null" \;
  cmd find . -name '*.dylib' -exec sh -c "install_name_tool -delete_rpath \"@loader_path/../../../../lib\" {} 2>/dev/null" \;
  cmd find . -name '*.dylib' -exec sh -c "install_name_tool -delete_rpath \"@loader_path/../../../../../lib\" {} 2>/dev/null" \;
  cmd find . -name '*.dylib' -exec sh -c "install_name_tool -add_rpath    \"@executable_path/../Frameworks\" {} 2>/dev/null" \;

  popd

  if [ "$SDK" = "1" ]; then
    # Prepare src for archiving
    cmd rm -rf "$INSTALL_DIR"/Shotcut
    cmd mv shotcut/Shotcut.app "$INSTALL_DIR"/Shotcut
    clean_dirs
    pushd "$INSTALL_DIR"
    log Copying src
    cmd cp -a "$SOURCE_DIR" Shotcut
    log Copying includes
    cmd cp -a "$FINAL_INSTALL_DIR"/include Shotcut/Contents/Frameworks
    log Copying pkg-config files
    cmd mkdir -p Shotcut/Contents/Frameworks/lib 2> /dev/null
    cmd cp -a "$FINAL_INSTALL_DIR"/lib/pkgconfig Shotcut/Contents/Frameworks/lib
    log Symlinking libs
    pushd Shotcut/Contents/Frameworks
    for lib in avcodec avdevice avfilter avformat avutil epoxy mlt++-7 mlt-7 movit mp3lame opus postproc swresample swscale vidstab x264 x265; do
      dylib=$(ls lib$lib.*.dylib | head -n 1)
      cmd ln -sf $dylib lib$lib.dylib
    done
    popd
  fi

  if [ "$ACTION_ARCHIVE" = "1" ]; then
    if [ "$SDK" = "1" ]; then
      log Making archive
      cmd tar -cJvf shotcut.txz Shotcut
      [ "$ACTION_CLEANUP" = "1" ] && cmd rm -rf Shotcut
      popd
    else
      # build DMG
      log Staging disk image
      cmd rm -rf staging 2>/dev/null
      cmd mkdir staging
      cmd mv shotcut/Shotcut.app staging
      cmd ln -s /Applications staging
      cmd cp shotcut/COPYING staging

      log Making disk image
      dmg_name="$INSTALL_DIR/unsigned.dmg"
      cmd rm "$dmg_name" 2>/dev/null
      sync
      cmd hdiutil create -fs HFS+ -srcfolder staging -volname Shotcut -format UDBZ -size 800m "$dmg_name"

      if [ "$ACTION_CLEANUP" = "1" ]; then
        cmd rm -rf staging
      fi
    fi
  fi
}

#################################################################
# create_startup_script
# Creates a startup script. Note, that the actual script gets
# embedded by the Makefile
function create_startup_script {
  if test "$TARGET_OS" = "Darwin" ; then
    deploy_mac
    return
  fi

  cmd cp -pLR /usr/lib/python${PYTHON_VERSION_DEFAULT} "$FINAL_INSTALL_DIR"/lib

  trace "Entering create_startup_script @ = $@"
  pushd .

  log Changing to $FINAL_INSTALL_DIR
  cd $FINAL_INSTALL_DIR || die "Unable to change to directory $FINAL_INSTALL_DIR"

  TMPFILE=`mktemp -t build-shotcut.env.XXXXXXXXX`
  log Creating environment script in $TMPFILE
  cat > $TMPFILE <<End-of-environment-setup-template
#!/bin/sh
# Set up environment
# Source this file using a bash/sh compatible shell, to get an environment,
# where you use the binaries and libraries for this melt build.
INSTALL_DIR=\$(pwd)
export PATH="\$INSTALL_DIR/bin":\$PATH
export LD_LIBRARY_PATH="\$INSTALL_DIR/lib":\$LD_LIBRARY_PATH
export MLT_REPOSITORY="\$INSTALL_DIR/lib/mlt-7"
export MLT_DATA="\$INSTALL_DIR/share/mlt-7"
export MLT_PROFILES_PATH="\$INSTALL_DIR/share/mlt-7/profiles"
export MLT_MOVIT_PATH="\$INSTALL_DIR/share/movit"
export FREI0R_PATH="\$INSTALL_DIR/lib/frei0r-1"
# Temporarily ignore user and default path because csladspa bug is crashing with
# LADSPA_PATH set, and Shotcut only needs the supplied SWH plugins.
# export LADSPA_PATH="\$LADSPA_PATH:/usr/local/lib/ladspa:/usr/lib/ladspa:/usr/lib64/ladspa:\$INSTALL_DIR/lib/ladspa"
export LADSPA_PATH="\$INSTALL_DIR/lib/ladspa"
export LIBVA_DRIVERS_PATH="\$INSTALL_DIR/lib/va"
export MANPATH=\$MANPATH:"\$INSTALL_DIR/share/man"
export PKG_CONFIG_PATH="\$INSTALL_DIR/lib/pkgconfig":\$PKG_CONFIG_PATH
export QT_PLUGIN_PATH="\$INSTALL_DIR/lib/qt6"
export QML2_IMPORT_PATH="\$INSTALL_DIR/lib/qml"
End-of-environment-setup-template
  if test 0 != $? ; then
    die "Unable to create environment script"
  fi
  chmod 755 $TMPFILE || die "Unable to make environment script executable"
  cp $TMPFILE "$FINAL_INSTALL_DIR/source-me" || die "Unable to create environment script - cp failed"

  log Creating wrapper scripts in $TMPFILE
  for exe in melt ffmpeg ffplay ffprobe glaxnimate; do
    cat > $TMPFILE <<End-of-exe-wrapper
#!/bin/sh
# Set up environment
# Run this instead of trying to run bin/$exe. It runs $exe with the correct environment.
CURRENT_DIR=\$(readlink -f "\$0")
INSTALL_DIR=\$(dirname "\$CURRENT_DIR")
export LD_LIBRARY_PATH="\$INSTALL_DIR/lib":\$LD_LIBRARY_PATH
export MLT_REPOSITORY="\$INSTALL_DIR/lib/mlt-7"
export MLT_DATA="\$INSTALL_DIR/share/mlt-7"
export MLT_PROFILES_PATH="\$INSTALL_DIR/share/mlt-7/profiles"
export MLT_MOVIT_PATH="\$INSTALL_DIR/share/movit"
export FREI0R_PATH="\$INSTALL_DIR/lib/frei0r-1"
export LADSPA_PATH="\$LADSPA_PATH:/usr/local/lib/ladspa:/usr/lib/ladspa:/usr/lib64/ladspa:\$INSTALL_DIR/lib/ladspa"
export LIBVA_DRIVERS_PATH="\$INSTALL_DIR/lib/va"
export PYTHONHOME="\$INSTALL_DIR"
export QT_PLUGIN_PATH="\$INSTALL_DIR/lib/qt6"
export QML2_IMPORT_PATH="\$INSTALL_DIR/lib/qml"
"\$INSTALL_DIR/bin/$exe" "\$@"
End-of-exe-wrapper
    if test 0 != $? ; then
      die "Unable to create wrapper script"
    fi
    chmod 755 $TMPFILE || die "Unable to make wrapper script executable"
    cp $TMPFILE "$FINAL_INSTALL_DIR/$exe" || die "Unable to create wrapper script - cp failed"
  done

  log Creating wrapper script in $TMPFILE
  cat > $TMPFILE <<End-of-shotcut-wrapper
#!/bin/sh
# Set up environment
# Run this instead of trying to run bin/shotcut. It runs shotcut with the correct environment.
CURRENT_DIR=\$(readlink -f "\$0")
INSTALL_DIR=\$(dirname "\$CURRENT_DIR")
export LD_LIBRARY_PATH="\$INSTALL_DIR/lib":\$LD_LIBRARY_PATH
export MLT_REPOSITORY="\$INSTALL_DIR/lib/mlt-7"
export MLT_DATA="\$INSTALL_DIR/share/mlt-7"
export MLT_PROFILES_PATH="\$INSTALL_DIR/share/mlt-7/profiles"
export MLT_MOVIT_PATH="\$INSTALL_DIR/share/movit"
export FREI0R_PATH="\$INSTALL_DIR/lib/frei0r-1"
# Temporarily ignore user and default path because csladspa bug is crashing with
# LADSPA_PATH set, and Shotcut only needs the supplied SWH plugins.
# export LADSPA_PATH="\$LADSPA_PATH:/usr/local/lib/ladspa:/usr/lib/ladspa:/usr/lib64/ladspa:\$INSTALL_DIR/lib/ladspa"
export LADSPA_PATH="\$INSTALL_DIR/lib/ladspa"
export LIBVA_DRIVERS_PATH="\$INSTALL_DIR/lib/va"
export PYTHONHOME="\$INSTALL_DIR"
cd "\$INSTALL_DIR"
export QT_PLUGIN_PATH="lib/qt6"
export QML2_IMPORT_PATH="lib/qml"
bin/shotcut "\$@"
End-of-shotcut-wrapper
  if test 0 != $? ; then
    die "Unable to create wrapper script"
  fi
  chmod 755 $TMPFILE || die "Unable to make wrapper script executable"
  cp $TMPFILE "$FINAL_INSTALL_DIR/shotcut" || die "Unable to create wrapper script - cp failed"

  popd

  log Creating desktop file in $TMPFILE
  cp shotcut/packaging/linux/org.shotcut.Shotcut.desktop $TMPFILE
  sed -i '1i #!/usr/bin/env xdg-open' $TMPFILE
  sed -i 's|Exec=.*|Exec=sh -c "\$(dirname "%k")/Shotcut.app/shotcut "%F""|' $TMPFILE
  sed -i 's|Icon=.*|Icon=applications-multimedia|' $TMPFILE
  if test 0 != $? ; then
    die "Unable to create desktop file"
  fi
  cp $TMPFILE "$FINAL_INSTALL_DIR/../Shotcut.desktop" || die "Unable to create desktop file - cp failed"

  feedback_status Done creating startup and environment script

  cmd pushd "$INSTALL_DIR"

  VERSION_INFO=$(readlink -f Shotcut/Shotcut.app/versions)
  rm -f $VERSION_INFO
  for DIR in $SUBDIRS; do
    if [ -d $SOURCE_DIR/$DIR/.git ]; then
      pushd $SOURCE_DIR/$DIR > /dev/null
      echo $DIR $(git rev-parse HEAD) $(git describe --tags) >> $VERSION_INFO
      popd > /dev/null
    fi
  done

  if [ "$ACTION_ARCHIVE" = "1" ]; then
    log Creating archive
    tarball="$INSTALL_DIR/shotcut.txz"
    cmd rm "$tarball" 2>/dev/null

    if [ "$SDK" = "1" ]; then
      # Prepare src for archiving
      pushd .
      clean_dirs
      popd
      log Copying src
      cmd -rf Shotcut/Shotcut.app/src 2> /dev/null
      cmd cp -a "$SOURCE_DIR" Shotcut/Shotcut.app
    else
      cmd rm -rf Shotcut/Shotcut.app/include
      cmd rm Shotcut/Shotcut.app/lib/*.a
      cmd rm -rf Shotcut/Shotcut.app/lib/pkgconfig
      cmd rm -rf Shotcut/Shotcut.app/share/doc
      cmd rm -rf Shotcut/Shotcut.app/share/man
    fi
    cmd tar -cJvf "$tarball" Shotcut
  fi

  if [ "$ACTION_CLEANUP" = "1" ]; then
    log Cleaning Up
    cmd rm -rf Shotcut
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
  if test 1 = "$CREATE_STARTUP_SCRIPT" ; then
    create_startup_script
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

  log "Done checking for sudo requirement" 2>&1

  {
  perform_action
  } 2>&1

  # All is well, that ends well
  exit 0
}

parse_args "$@"
# Call main, but if detach is given, put it in the background
if test 1 = "$DETACH"; then
  main &
  # Note, that we assume caller has setup stdin & stdout redirection
  disown -a
else
  main
fi
