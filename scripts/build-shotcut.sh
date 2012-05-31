#!/bin/bash

# This script builds shotcut and many of its dependencies.
# It can accept a configuration file, default: build-shotcut.conf

# List of programs used:
# bash, test, tr, awk, ps, make, cmake, cat, sed, curl or wget, and possibly others

# Author: Dan Dennedy <dan@dennedy.org>
# Version: 6
# License: GPL2

################################################################################
# ARGS AND GLOBALS
################################################################################

# These are all of the configuration variables with defaults
INSTALL_DIR="$HOME/shotcut"
AUTO_APPEND_DATE=0
SOURCE_DIR="$INSTALL_DIR/src"
ACTION_GET_COMPILE_INSTALL=1
ACTION_GET_ONLY=0
ACTION_COMPILE_INSTALL=1
SOURCES_CLEAN=1
INSTALL_AS_ROOT=0
CREATE_STARTUP_SCRIPT=1
ENABLE_FREI0R=1
FREI0R_HEAD=1
FREI0R_REVISION=
ENABLE_SWFDEC=0
SWFDEC_HEAD=
SWFDEC_REVISION=
X264_HEAD=1
X264_REVISION=
LIBVPX_HEAD=1
LIBVPX_REVISION=
ENABLE_LAME=1
ENABLE_SWH_PLUGINS=1
FFMPEG_HEAD=1
FFMPEG_REVISION=
FFMPEG_SUPPORT_H264=1
FFMPEG_SUPPORT_LIBVPX=1
FFMPEG_SUPPORT_THEORA=1
FFMPEG_SUPPORT_MP3=1
FFMPEG_SUPPORT_FAAC=0
FFMPEG_ADDITIONAL_OPTIONS=
MLT_HEAD=1
MLT_REVISION=
SHOTCUT_HEAD=1
SHOTCUT_REVISION=
# QT_INCLUDE_DIR="$(pkg-config --variable=prefix QtCore)/include"
QT_INCLUDE_DIR=
# QT_LIB_DIR="$(pkg-config --variable=prefix QtCore)/lib"
QT_LIB_DIR=
MLT_DISABLE_SOX=0

################################################################################
# Location of config file - if not overriden on command line
CONFIGFILE=build-shotcut.conf

# If defined to 1, outputs trace log lines
TRACE=0

# If defined to 1, outputs debug log lines
DEBUG=0

# We need to set LANG to C to avoid e.g. svn from getting to funky
export LANG=C

# User CFLAGS and LDFLAGS sometimes prevent more recent local headers.
# Also, you can adjust some flags here.
export CFLAGS=
export CXXFLAGS=
export LDFLAGS=

################################################################################
# FUNCTION SECTION
################################################################################

#################################################################
# usage
# Reports legal options to this script
function usage {
  echo "Usage: $0 [-c config-file] [-o target-os] [-t] [-h]"
  echo "Where:"
  echo -e "\t-c config-file\tDefaults to $CONFIGFILE"
  echo -e "\t-o target-os\tDefaults to $(uname -s); use Win32 to cross-compile"
  echo -e "\t-t\t\tSpawn into sep. process"
}

#################################################################
# parse_args
# Parses the arguments passed in $@ and sets some global vars
function parse_args {
  CONFIGFILEOPT=""
  DETACH=0
  while getopts ":tc:o:" OPT; do
    case $OPT in
      c ) CONFIGFILEOPT=$OPTARG
          echo Setting configfile to $CONFIGFILEOPT
      ;;
      t ) DETACH=1;;
      h ) usage
          exit 0;;
      o ) TARGET_OS=$OPTARG;;
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
    swfdec)
      echo 5
    ;;
    lame)
      echo 6
    ;;
    shotcut)
      echo 7
    ;;
    swh-plugins)
      echo 8
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
    echo "TRACE: $@"
  fi
}

#################################################################
# debug
# Function that prints a debug line
# $@ : arguments to be printed
function debug {
  if test "1" = "$DEBUG" ; then 
    echo "DEBUG: $@"
  fi
}

#################################################################
# log
# Function that prints a log line
# $@ : arguments to be printed
function log {
  echo "LOG: $@"
}

#################################################################
# log warning
# Function that prints a warning line
# $@ : arguments to be printed
function warn {
  echo "WARN: $@"
}

#################################################################
# die
# Function that prints a line and exists
# $@ : arguments to be printed
function die {
  echo "ERROR: $@"
  feedback_result FAILURE "Some kind of error occured: $@"
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
  # Set convenience variables.
  test "$TARGET_OS" = "" && TARGET_OS="$(uname -s)"
  if test 1 = "$ACTION_GET_ONLY" -o 1 = "$ACTION_GET_COMPILE_INSTALL" ; then
    GET=1
  else
    GET=0
  fi
  NEED_SUDO=0
  if test 1 = "$ACTION_GET_COMPILE_INSTALL" -o 1 = "$ACTION_COMPILE_INSTALL" ; then
    COMPILE_INSTALL=1
    if test 1 = $INSTALL_AS_ROOT ; then
      NEED_SUDO=1
    fi
  else
    COMPILE_INSTALL=0
  fi
  debug "GET=$GET, COMPILE_INSTALL=$COMPILE_INSTALL, NEED_SUDO=$NEED_SUDO"

  # The script sets CREATE_STARTUP_SCRIPT to true always, disable if not COMPILE_INSTALL
  if test 0 = "$COMPILE_INSTALL" ; then
    CREATE_STARTUP_SCRIPT=0
  fi
  debug "CREATE_STARTUP_SCRIPT=$CREATE_STARTUP_SCRIPT"

  # Subdirs list, for number of common operations
  # Note, the function to_key depends on this
  SUBDIRS="ffmpeg mlt shotcut"
  if test "$ENABLE_FREI0R" = 1 ; then
      SUBDIRS="frei0r $SUBDIRS"
  fi
  if test "$ENABLE_SWFDEC" = 1 && test "$SWFDEC_HEAD" = 1 -o "$SWFDEC_REVISION" != ""; then
      SUBDIRS="swfdec $SUBDIRS"
  fi
  if test "$FFMPEG_SUPPORT_H264" = 1 && test "$X264_HEAD" = 1 -o "$X264_REVISION" != ""; then
      SUBDIRS="x264 $SUBDIRS"
  fi
  if test "$FFMPEG_SUPPORT_LIBVPX" = 1 && test "$LIBVPX_HEAD" = 1 -o "$LIBVPX_REVISION" != ""; then
      SUBDIRS="libvpx $SUBDIRS"
  fi
  if test "$FFMPEG_SUPPORT_MP3" = 1 && test "$ENABLE_LAME" = 1; then
      SUBDIRS="lame $SUBDIRS"
  fi
  if test "$ENABLE_SWH_PLUGINS" = "1" && test "$TARGET_OS" = "Darwin"; then
      SUBDIRS="swh-plugins $SUBDIRS"
  fi
  debug "SUBDIRS = $SUBDIRS"

  # REPOLOCS Array holds the repo urls
  REPOLOCS[0]="git://git.videolan.org/ffmpeg.git"
  REPOLOCS[1]="git://github.com/mltframework/mlt.git"
  REPOLOCS[2]="git://code.dyne.org/frei0r.git"
  REPOLOCS[3]="git://git.videolan.org/x264.git"
  REPOLOCS[4]="http://git.chromium.org/webm/libvpx.git"
  REPOLOCS[5]="git://github.com/mltframework/swfdec.git"
  REPOLOCS[6]="http://downloads.sourceforge.net/project/lame/lame/3.98.4/lame-3.98.4.tar.gz"
  REPOLOCS[7]="git://github.com/mltframework/shotcut.git"
  REPOLOCS[8]="http://plugin.org.uk/releases/0.4.15/swh-plugins-0.4.15.tar.gz"


  # REPOTYPE Array holds the repo types. (Yes, this might be redundant, but easy for me)
  REPOTYPES[0]="git"
  REPOTYPES[1]="git"
  REPOTYPES[2]="git"
  REPOTYPES[3]="git"
  REPOTYPES[4]="git"
  REPOTYPES[5]="git"
  REPOTYPES[6]="http-tgz"
  REPOTYPES[7]="git"
  REPOTYPES[8]="http-tgz"

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
  if test 0 = "$SWFDEC_HEAD" -a "$SWFDEC_REVISION" ; then
    REVISIONS[5]="$SWFDEC_REVISION"
  fi
  REVISIONS[6]="lame-3.98.4"
  REVISIONS[7]=""
  if test 0 = "$SHOTCUT_HEAD" -a "$SHOTCUT_REVISION" ; then
    REVISIONS[7]="$SHOTCUT_REVISION"
  fi
  REVISIONS[8]="swh-plugins-0.4.15"

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
  elif test "$TARGET_OS" = "Win32" ; then
    FINAL_INSTALL_DIR="$INSTALL_DIR/Shotcut"
  else
    FINAL_INSTALL_DIR="$INSTALL_DIR/Shotcut/.app"
  fi
  debug "Using install dir FINAL_INSTALL_DIR=$FINAL_INSTALL_DIR"

  # CONFIG Array holds the ./configure (or equiv) command for each project
  # CFLAGS_ Array holds additional CFLAGS for the configure/make step of a given project 
  # LDFLAGS_ Array holds additional LDFLAGS for the configure/make step of a given project
  if test "$TARGET_OS" = "Win32" ; then
    FFMPEG_SUPPORT_THEORA=0
    export CROSS=i686-w64-mingw32-
    export CC=${CROSS}gcc
    export CXX=${CROSS}g++
    export AR=${CROSS}ar
    export RANLIB=${CROSS}ranlib
    export CFLAGS="-DHAVE_STRUCT_TIMESPEC -I$FINAL_INSTALL_DIR/include"
    export CXXFLAGS="$CFLAGS"
    export LDFLAGS="-L$FINAL_INSTALL_DIR/bin -L$FINAL_INSTALL_DIR/lib"
    export QTDIR="$HOME/qt/4.8.1"
  fi

  #####
  # ffmpeg
  CONFIG[0]="./configure --prefix=$FINAL_INSTALL_DIR --disable-static --disable-doc --disable-ffserver --enable-gpl --enable-version3 --enable-shared --enable-debug --enable-pthreads --enable-runtime-cpudetect"
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
  if test 1 = "$FFMPEG_SUPPORT_LIBVPX" ; then
    CONFIG[0]="${CONFIG[0]} --enable-libvpx"
  fi
  # Add optional parameters
  CONFIG[0]="${CONFIG[0]} $FFMPEG_ADDITIONAL_OPTIONS"
  CFLAGS_[0]="-I$FINAL_INSTALL_DIR/include $CFLAGS"
  if test "$TARGET_OS" = "Win32" ; then
    CONFIG[0]="${CONFIG[0]} --enable-memalign-hack --cross-prefix=$CROSS --arch=x86 --target-os=mingw32"
    LDFLAGS_[0]="$LDFLAGS"
  else
    LDFLAGS_[0]="-L$FINAL_INSTALL_DIR/lib $LDFLAGS"
  fi
  if test "$TARGET_OS" = "Darwin"; then
    CFLAGS_[0]="${CFLAGS_[0]} -I/opt/local/include"
    LDFLAGS_[0]="${LDFLAGS_[0]} -L/opt/local/lib"
  fi

  #####
  # mlt
  CONFIG[1]="./configure --prefix=$FINAL_INSTALL_DIR --enable-gpl --enable-gpl3"
  # Remember, if adding more of these, to update the post-configure check.
  [ "$QT_INCLUDE_DIR" ] && CONFIG[1]="${CONFIG[1]} --qimage-includedir=$QT_INCLUDE_DIR"
  [ "$QT_LIB_DIR" ] && CONFIG[1]="${CONFIG[1]} --qimage-libdir=$QT_LIB_DIR"
  if test "1" = "$MLT_DISABLE_SOX" ; then
    CONFIG[1]="${CONFIG[1]} --disable-sox"
  fi
  if test "$TARGET_OS" = "Win32" ; then
    CONFIG[1]="${CONFIG[1]} --disable-kino --disable-vorbis --gtk2-prefix=\"$FINAL_INSTALL_DIR\" --target-os=MinGW --target-arch=i686 --rename-melt=melt.exe"
  fi
  CFLAGS_[1]="-I$FINAL_INSTALL_DIR/include $CFLAGS"
  [ "$TARGET_OS" = "Darwin" ] && CFLAGS_[1]="${CFLAGS_[1]} -DRELOCATABLE"
  LDFLAGS_[1]="-L$FINAL_INSTALL_DIR/lib $LDFLAGS"

  ####
  # frei0r
  if test "$TARGET_OS" = "Win32" ; then
    CONFIG[2]="cmake -DCMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR -DCMAKE_TOOLCHAIN_FILE=my.cmake"
  else
    CONFIG[2]="./configure --prefix=$FINAL_INSTALL_DIR"
  fi
  CFLAGS_[2]=$CFLAGS
  LDFLAGS_[2]=$LDFLAGS

  ####
  # x264
  CONFIG[3]="./configure --prefix=$FINAL_INSTALL_DIR --disable-lavf --disable-ffms --disable-gpac --disable-swscale --enable-shared --disable-cli"
  CFLAGS_[3]=$CFLAGS
  if test "$TARGET_OS" = "Win32" ; then
    CONFIG[3]="${CONFIG[3]} --enable-win32thread --host=i686-w64-mingw32 --cross-prefix=$CROSS"
  elif test "$TARGET_OS" = "Darwin" ; then
    CFLAGS_[3]="-I. -fno-common -read_only_relocs suppress ${CFLAGS_[3]}"
  fi
  LDFLAGS_[3]=$LDFLAGS
  
  ####
  # libvpx
  CONFIG[4]="./configure --prefix=$FINAL_INSTALL_DIR --enable-vp8 --enable-postproc --enable-multithread --enable-runtime-cpu-detect --disable-install-docs --disable-debug-libs --disable-examples"
  if test "$TARGET_OS" = "Linux" ; then
    CONFIG[4]="${CONFIG[4]} --enable-shared"
  elif test "$TARGET_OS" = "Win32" ; then
    CONFIG[4]="${CONFIG[4]} --target=x86-win32-gcc"
  fi
  CFLAGS_[4]=$CFLAGS
  LDFLAGS_[4]=$LDFLAGS

  #####
  # swfdec
  CONFIG[5]="./configure --prefix=$FINAL_INSTALL_DIR --disable-gtk --disable-gstreamer"
  CFLAGS_[5]=$CFLAGS
  LDFLAGS_[5]=$LDFLAGS

  #####
  # lame
  CONFIG[6]="./configure --prefix=$FINAL_INSTALL_DIR --disable-decoder --disable-frontend"
  if test "$TARGET_OS" = "Win32" ; then
    CONFIG[6]="${CONFIG[6]} --libdir=$FINAL_INSTALL_DIR/lib --host=x86-w64-mingw32"
  fi
  CFLAGS_[6]=$CFLAGS
  LDFLAGS_[6]=$LDFLAGS

  #####
  # shotcut 
  if [ "$TARGET_OS" = "Darwin" ]; then
    CONFIG[7]="qmake -r -spec macx-g++"
  elif [ "$(which qmake-qt4)" != "" ]; then
    CONFIG[7]="qmake-qt4 -r"
  else
    CONFIG[7]="qmake -r"
  fi
  if test "$TARGET_OS" = "Win32" ; then
    CONFIG[7]="${CONFIG[7]} -spec mingw-mkspec CONFIG+=link_pkgconfig PKGCONFIG+=mlt++ CONFIG-=debug"
  fi
  CFLAGS_[7]=$CFLAGS
  LDFLAGS_[7]=$LDFLAGS

  #####
  # swh-plugins
  CONFIG[8]="./configure --prefix=$FINAL_INSTALL_DIR --enable-darwin --enable-sse"
  CFLAGS_[8]="-march=nocona $CFLAGS"
  LDFLAGS_[8]=$LDFLAGS

}

######################################################################
# FEEDBACK FUNCTIONS
######################################################################

#################################################################
# feedback_init
# $1 : ProgressBar maximum
function feedback_init {
  trace "Entering feedback_init @ = $@"
  log Total number of steps needed to complete $1
  log Press Ctrl+C to abort
  PROGRESS=0
  feedback_set_progress $PROGRESS
}

#################################################################
# feedback_progress
# $1 : ProgressBar position
function feedback_set_progress {
  trace "Entering feedback_set_progress @ = $@"
  log Number of steps completed : $1
}

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
# feedback_progress
# $@ : Description of task completed
# Increases the progressbar with 1 and sets the status to $@
function feedback_progress {
  trace "Entering feedback_progress @ = $@"
  PROGRESS=$(( $PROGRESS + 1 ))
  feedback_status $@
  feedback_set_progress $PROGRESS
}

#################################################################
# prepare_feedback
# Function to prepare the feedback. E.g. set up max progress steps
# Based on configuration read
function prepare_feedback {
  trace "Entering prepare_feedback @ = $@"
  # Figure out the number of steps
  # Get adds 8 if cleaning, 4 otherwise (2/1 pr. proj)
  # Compile/Install adds 12 (3/proj)
  # Script install adds 1
  NUMSTEPS=0
  if test 1 = "$GET" ; then
    debug Adding 3 steps for get
    NUMSTEPS=$(( $NUMSTEPS + 3 ))
    if test 1 = "$ENABLE_FREI0R" ; then
      debug Adding 1 step for get frei0r
      NUMSTEPS=$(( $NUMSTEPS + 1 ))
    fi
    if test 1 = "$ENABLE_SWFDEC" ; then
      debug Adding 1 step for get swfdec
      NUMSTEPS=$(( $NUMSTEPS + 1 ))
    fi
  fi
  if test 1 = "$GET" -a 1 = "$SOURCES_CLEAN" ; then
    debug Adding 3 steps for clean on get
    NUMSTEPS=$(( $NUMSTEPS + 3 ))
    if test 1 = "$ENABLE_FREI0R" ; then
      debug Adding 1 step for clean frei0r
      NUMSTEPS=$(( $NUMSTEPS + 1 ))
    fi
    if test 1 = "$ENABLE_SWFDEC" ; then
      debug Adding 1 step for clean swfdec
      NUMSTEPS=$(( $NUMSTEPS + 1 ))
    fi
  fi   
  if test 1 = "$COMPILE_INSTALL" ; then
    debug Adding 9 steps for compile-install
    NUMSTEPS=$(( $NUMSTEPS + 9 ))
    if test 1 = "$ENABLE_FREI0R" ; then
      debug Adding 3 steps for compile-install frei0r
      NUMSTEPS=$(( $NUMSTEPS + 3 ))
    fi
    if test 1 = "$ENABLE_SWFDEC" ; then
      debug Adding 3 steps for compile-install swfdec
      NUMSTEPS=$(( $NUMSTEPS + 3 ))
    fi
  fi
  if test 1 = "$CREATE_STARTUP_SCRIPT" ; then 
    debug Adding 1 step for script creating
    NUMSTEPS=$(( $NUMSTEPS + 1 ))
  fi
  log Number of steps determined to $NUMSTEPS
  feedback_init $NUMSTEPS
}

#################################################################
# check_abort
# Function that checks if the user wanted to cancel what we are doing.
# returns "stop" or "cont" as appropiate
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
  if test "ffmpeg" = "$1" ; then
      cmd cd $1 && cmd make distclean
  else
      cmd cd $1 && cmd make clean
  fi
  feedback_progress Cleaned up in $1
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

function get_win32_build {
 
  if test "frei0r" = "$1" ; then
      debug "Create cmake rules for frei0r"
      cat >my.cmake <<END_OF_CMAKE_RULES
# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER ${CROSS}gcc)
SET(CMAKE_CXX_COMPILER ${CROSS}g++)
SET(CMAKE_LINKER ${CROSS}ld)
SET(CMAKE_STRIP ${CROSS}strip)
SET(CMAKE_RC_COMPILER ${CROSS}windres)

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH  /usr/i686-w64-mingw32 $FINAL_INSTALL_DIR)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
END_OF_CMAKE_RULES

  elif test "shotcut" = "$1" ; then
      mkdir mingw-mkspec 2> /dev/null
      debug "Create qmake mkspec for shotcut"
      cat >mingw-mkspec/qmake.conf <<END_OF_QMAKE_SPEC
#
# qmake configuration for win32-g++
#
# Written for MinGW
#

MAKEFILE_GENERATOR	= MINGW
TEMPLATE		= app
CONFIG			+= qt warn_on release link_prl copy_dir_files debug_and_release debug_and_release_target precompile_header
CONFIG			+= exceptions windows win32 rtti
QT			+= core gui
DEFINES			+= UNICODE
DEFINES			+= QT_LARGEFILE_SUPPORT
DEFINES 		+= HAVE_STRUCT_TIMESPEC
QMAKE_COMPILER_DEFINES  += __GNUC__ WIN32

QMAKE_EXT_OBJ           = .o
QMAKE_EXT_RES           = _res.o

QMAKE_CC		= i686-w64-mingw32-gcc
QMAKE_LEX		= flex
QMAKE_LEXFLAGS		=
QMAKE_YACC		= byacc
QMAKE_YACCFLAGS		= -d
QMAKE_CFLAGS		=
QMAKE_CFLAGS_DEPS	= -M
QMAKE_CFLAGS_WARN_ON	= -Wall
QMAKE_CFLAGS_WARN_OFF	= -w
QMAKE_CFLAGS_RELEASE	= -O2
QMAKE_CFLAGS_DEBUG	= -g
QMAKE_CFLAGS_YACC	= -Wno-unused -Wno-parentheses

QMAKE_CXX		= i686-w64-mingw32-g++
QMAKE_CXXFLAGS		= \$\$QMAKE_CFLAGS
QMAKE_CXXFLAGS_DEPS	= \$\$QMAKE_CFLAGS_DEPS
QMAKE_CXXFLAGS_WARN_ON	= \$\$QMAKE_CFLAGS_WARN_ON
QMAKE_CXXFLAGS_WARN_OFF	= \$\$QMAKE_CFLAGS_WARN_OFF
QMAKE_CXXFLAGS_RELEASE	= \$\$QMAKE_CFLAGS_RELEASE
QMAKE_CXXFLAGS_DEBUG	= \$\$QMAKE_CFLAGS_DEBUG
QMAKE_CXXFLAGS_YACC	= \$\$QMAKE_CFLAGS_YACC
QMAKE_CXXFLAGS_THREAD	= \$\$QMAKE_CFLAGS_THREAD
QMAKE_CXXFLAGS_RTTI_ON	= -frtti
QMAKE_CXXFLAGS_RTTI_OFF	= -fno-rtti
QMAKE_CXXFLAGS_EXCEPTIONS_ON = -fexceptions -mthreads
QMAKE_CXXFLAGS_EXCEPTIONS_OFF = -fno-exceptions

QMAKE_INCDIR		= /usr/i686-w64-mingw32/include
QMAKE_INCDIR_QT		= \$(QTDIR)/include
QMAKE_LIBDIR_QT		= \$(QTDIR)/lib


QMAKE_RUN_CC		= \$(CC) -c \$(CFLAGS) \$(INCPATH) -o \$obj \$src
QMAKE_RUN_CC_IMP	= \$(CC) -c \$(CFLAGS) \$(INCPATH) -o \$@ \$<
QMAKE_RUN_CXX		= \$(CXX) -c \$(CXXFLAGS) \$(INCPATH) -o \$obj \$src
QMAKE_RUN_CXX_IMP	= \$(CXX) -c \$(CXXFLAGS) \$(INCPATH) -o \$@ \$<

QMAKE_LINK		= i686-w64-mingw32-g++
QMAKE_LINK_C		= i686-w64-mingw32-gcc
QMAKE_LFLAGS		= -mthreads -Wl,-enable-stdcall-fixup -Wl,-enable-auto-import -Wl,-enable-runtime-pseudo-reloc -mwindows
QMAKE_LFLAGS_EXCEPTIONS_ON = -mthreads
QMAKE_LFLAGS_EXCEPTIONS_OFF =
QMAKE_LFLAGS_RELEASE	= -Wl,-s
QMAKE_LFLAGS_DEBUG	=
QMAKE_LFLAGS_CONSOLE	= -Wl,-subsystem,console
QMAKE_LFLAGS_WINDOWS	= -Wl,-subsystem,windows
QMAKE_LFLAGS_DLL        = -shared
QMAKE_LINK_OBJECT_MAX	= 10
QMAKE_LINK_OBJECT_SCRIPT= object_script
QMAKE_PREFIX_STATICLIB  = lib
QMAKE_EXTENSION_STATICLIB = a


QMAKE_LIBS		=
QMAKE_LIBS_CORE         = -lole32 -luuid -lws2_32 -ladvapi32 -lshell32 -luser32 -lkernel32
QMAKE_LIBS_GUI          = -lgdi32 -lcomdlg32 -loleaut32 -limm32 -lwinmm -lwinspool -lws2_32 -lole32 -luuid -luser32 -ladvapi32
QMAKE_LIBS_NETWORK      = -lws2_32
QMAKE_LIBS_OPENGL       = -lglu32 -lopengl32 -lgdi32 -luser32
QMAKE_LIBS_COMPAT       = -ladvapi32 -lshell32 -lcomdlg32 -luser32 -lgdi32 -lws2_32
QMAKE_LIBS_QT_ENTRY     = -lmingw32 -lqtmain

MINGW_IN_SHELL      = 1
QMAKE_DIR_SEP		= /
QMAKE_QMAKE		~= s,\\\\,/,
QMAKE_COPY		= cp
QMAKE_COPY_DIR		= cp -r
QMAKE_MOVE		= mv
QMAKE_DEL_FILE		= rm
QMAKE_MKDIR		= mkdir -p
QMAKE_DEL_DIR		= rmdir
QMAKE_CHK_DIR_EXISTS = test -d

QMAKE_MOC		= i686-pc-mingw32-moc
QMAKE_UIC		= i686-pc-mingw32-uic
QMAKE_IDC		= i686-pc-mingw32-idc
QMAKE_RCC		= i686-pc-mingw32-rcc

QMAKE_IDL		= midl
QMAKE_LIB		= i686-w64-mingw32-ar -ru
QMAKE_RC		= i686-w64-mingw32-windres
QMAKE_ZIP		= zip -r -9

QMAKE_STRIP		= i686-w64-mingw32-strip
QMAKE_STRIPFLAGS_LIB 	+= --strip-unneeded
load(qt_config)
      
END_OF_QMAKE_SPEC
  fi
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

  # Check for repository setyp
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
          DEPTH="--depth 1"
          cmd git --no-pager clone $DEPTH $REPOLOC || die "Unable to git clone source for $1 from $REPOLOC"
          cmd cd $1 || die "Unable to change to directory $1"
          cmd git checkout $REVISION || die "Unable to git checkout $REVISION"
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
          # Note, that beeing clever almost always fails at some point. But, at least we give it a try...
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
          cmd mv "$REVISION" "$1" || due "Unable to rename $REVISION to $1"
      fi
  fi # git/svn

  if test "$TARGET_OS" = "Win32" ; then
    get_win32_build "$1"
  fi

  feedback_progress Done getting or updating source for $1
  cmd popd
}

function get_win32_prebuilt {
  log Extracting prebuilts tarball
  cmd pushd .
  cmd rm -rf "$FINAL_INSTALL_DIR" 2> /dev/null
  cmd mkdir -p "$FINAL_INSTALL_DIR"
  cd "$FINAL_INSTALL_DIR" || die "Unable to change to directory $FINAL_INSTALL_DIR"
  cmd tar -xjf "$HOME/mlt-prebuilt-mingw32.tar.bz2"
  cmd unzip "$HOME/gtk+-bundle_2.24.10-20120208_win32.zip"
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
  if test "$TARGET_OS" = "Win32" ; then
    get_win32_prebuilt
  fi
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
# mlt_check_configure
# This is a special hack for mlt. Mlt does not allow --enable, or abort
# if something is missing, so we check all the disable files. Some are
# optional, some are required. We stop compilation if a required file is
# missing. For optionals, we report them to the log
# Oh, and it is assumed we are in the toplevel mlt source directory, when
# this is called.
function mlt_check_configure {
  trace "Entering check_mlt_configure @ = $@"
  cmd pushd .
  DODIE=0
  cmd cd src/modules || die "Unable to check mlt modules list"
  for FILE in `ls disable-* 2>/dev/null` ; do
    debug "Checking $FILE"
    case $FILE in
      # REQUIRED
      disable-core)
        mlt_format_required core "I have no idea why this was disabled. "
        DODIE=1
      ;;
      disable-avformat)
        mlt_format_required avformat "Did ffmpeg installation fail? "
        DODIE=1
      ;;
      disable-xml)
        mlt_format_required xml "Please install libxml2-dev. "
        DODIE=1
      ;;
      disable-sdl)
        mlt_format_required sdl "Please install libsdl1.2-dev. "
        DODIE=1
      ;;
      disable-qimage)
        mlt_format_required qimage "Please provide paths for QImage on the 'Compile options' page. "
        DODIE=1
      ;;

      # AUDIO
      disable-sox)
        if test "0" = "$MLT_DISABLE_SOX" ; then
          mlt_format_optional sox "sound effects/operations" "sox-dev"
          DODIE=1
        fi 
      ;;
      disable-jackrack)
        mlt_format_optional jackrack "sound effects/operations" "libjack-dev"
      ;;
      disable-resample)
        mlt_format_optional resample "audio resampling" "libsamplerate0-dev"
      ;;

      # IMAGE
      disable-gtk2)
        mlt_format_optional gtk2 "some additional image loading support" "libgtk2-dev?"
      ;;
      disable-kdenlive)
        mlt_format_optional kdenlive "slow motion and freeze effects" "??"
      ;;
      disable-frei0r)
        mlt_format_optional frei0r "plugin architecture. Several additional effects and transitions" "see http://www.piksel.org/frei0r"
      ;;
        
      # OTHERS
      disable-dv)
        mlt_format_optional dv "loading and saving of DV files" "libdv/libdv-dev"
      ;;
      disable-vorbis)
        mlt_format_optional vorbis "loading and saving ogg/theora/vorbis files" "libvorbis-dev"
      ;;

      # FALLBACK
      disable-*)
        mlt_format_optional ${FILE/disable-} "... dunno ... " "... dunno ..."
      ;;
    esac
  done
  if test 1 = "$DODIE" ; then
    die "One or more required MLT modules could not be enabled"
  fi
  cmd popd
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
  export LDFLAGS=`lookup LDFLAGS_ $1`
  log LDFLAGS=$LDFLAGS

  # Configure
  feedback_status Configuring $1
  # Special hack for libvpx
  if test "libvpx" = "$1" ; then
    cmd make clean
  fi

  # Special hack for frei0r
  if test "frei0r" = "$1" -a ! -e configure ; then
    debug "Need to create configure for $1"
    cmd ./autogen.sh || die "Unable to create configure file for $1"
    if test ! -e configure ; then
      die "Unable to confirm presence of configure file for $1"
    fi
  fi

  # Special hack for swfdec
  if test "swfdec" = "$1" ; then
    debug "Need to create configure for $1"
    cmd autoreconf -i || die "Unable to create configure file for $1"
    if test ! -e configure ; then
      die "Unable to confirm presence of configure file for $1"
    fi
  fi

  cmd `lookup CONFIG $1` || die "Unable to configure $1"
  feedback_progress Done configuring $1

  # Special post-configure hack for ffmpeg/Win32
  if test "ffmpeg" = "$1" -a "$TARGET_OS" = "Win32" ; then
    log "Need to remove lib.exe from config.mak for $1"
    grep -v SLIB_EXTRA_CMD config.mak | grep -v SLIB_INSTALL_EXTRA_SHLIB > config.new &&
      mv config.new config.mak
  fi

  # Special hack for mlt, post-configure
  if test "mlt" = "$1" ; then
    mlt_check_configure
  fi
  
  # Compile
  feedback_status Building $1 - this could take some time
  cmd make -j$MAKEJ || die "Unable to build $1"
  feedback_progress Done building $1

  # Install
  feedback_status Installing $1
  # This export is only for kdenlive, really, and only for the install step
  export LD_LIBRARY_PATH=`lookup LD_LIBRARY_PATH_ $1`
  log "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
  if test "1" = "$NEED_SUDO" ; then
    debug "Needs to be root to install - trying"
    log About to run $SUDO make install
    TMPNAME=`mktemp -t build-shotcut.installoutput.XXXXXXXXX`
    # At least kdesudo does not return an error code if the program fails
    # Filter output for error, and dup it to the log
    # Special hack for libvpx
    if test "shotcut" = "$1" ; then
      $SUDO install -c -m 755 shotcut "$FINAL_INSTALL_DIR"
    else
      $SUDO make install > $TMPNAME 2>&1 
    fi
    cat $TMPNAME 2>&1
    # If it contains error it returns 0. 1 matches, 255 errors
    # Filter X errors out too
    grep -v "X Error" $TMPNAME | grep -i error 2>&1
    if test 0 = $? ; then
      die "Unable to install $1"
    fi
  else
    if test "shotcut" = "$1" ; then
      if test "$TARGET_OS" = "Win32" ; then
        cmd install -c -m 755 release/shotcut.exe "$FINAL_INSTALL_DIR"
      elif test "$TARGET_OS" != "Darwin"; then
        cmd install -c -m 755 shotcut "$FINAL_INSTALL_DIR"/bin
        cmd install -d "$FINAL_INSTALL_DIR"/lib/qt4
        cmd install -p -c /usr/lib/libQt{Core,Gui,Xml,Svg}.so* "$FINAL_INSTALL_DIR"/lib
        cmd install -p -c /usr/lib/libaudio.so* "$FINAL_INSTALL_DIR"/lib
        cmd cp -r /usr/lib/qt4/plugins/* "$FINAL_INSTALL_DIR"/lib/qt4
      fi
    else
      cmd make install || die "Unable to install $1"
    fi
  fi
  feedback_progress Done installing $1

  # Reestablish
  cmd popd
  export CFLAGS=$OLDCFLAGS
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

  # set global settings for all jobs
  export PATH="$FINAL_INSTALL_DIR/bin:$PATH"
  export LD_RUN_PATH="$FINAL_INSTALL_DIR/lib"
  export PKG_CONFIG_PATH="$FINAL_INSTALL_DIR/lib/pkgconfig:$PKG_CONFIG_PATH"

  log Changing to $SOURCE_DIR 
  cd $SOURCE_DIR || die "Unable to change to directory $SOURCE_DIR"
  for DIR in $SUBDIRS ; do 
    configure_compile_install_subproject $DIR
  done
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

function fixlibs()
{
  target=$(dirname "$1")/$(basename "$1")
  trace fixlibs $target
  libs=$(otool -L "$target" | awk '/^\t\/opt\/local/ || /^\t\/Applications\// || /^\t\/Users\// {print $1}')

  # if the target is a lib, change its id
  #if [ $(echo "$1" | grep '\.dylib$') ] || [ $(echo "$1" | grep '\.so$') ]; then
  #  cmd install_name_tool -id "@executable_path/lib/$(basename "$1")" "$target"
  #fi

  for lib in $libs; do
    if [ $(basename "$lib") != $(basename "$target") ]; then
      newlib=$(basename "$lib")
      cmd cp -n "$lib" lib/
      cmd install_name_tool -change "$lib" "@executable_path/lib/$newlib" "$target"
    fi
  done

  for lib in $libs; do
    if [ $(basename "$lib") != $(basename "$target") ]; then
      newlib=$(basename "$lib")
      fixlibs "lib/$newlib"
    fi
  done
}

function deploy_osx 
{
  trace "Entering deploy_osx @ = $@"
  pushd .

  # Change to right directory
  log Changing directory to shotcut
  cmd cd shotcut || die "Unable to change to directory shotcut"

  BUILD_DIR="Shotcut.app/Contents"

  # copy qt_menu.nib
  # try MacPorts first
  if [ -d "/opt/local/lib/Resources/qt_menu.nib" ]; then
    cmd cp -Rn /opt/local/lib/Resources/qt_menu.nib "$BUILD_DIR/Resources/"
  # try Qt Creator after that
  elif [ -d "/Applications/Qt Creator.app/Contents/Frameworks/QtGui.framework/Resources/qt_menu.nib" ]; then
    cmd cp -Rn "/Applications/Qt Creator.app/Contents/Frameworks/QtGui.framework/Resources/qt_menu.nib" "$BUILD_DIR/Resources/"
  fi

  cmd cd "$BUILD_DIR/MacOS" || die "Unable to change directory to MacOS"

  log Copying supplementary executables
  cmd cp -a $FINAL_INSTALL_DIR/bin/{melt,ffmpeg} .
  mkdir lib 2>/dev/null
  for exe in $(find . -perm +u+x -maxdepth 1); do
    log fixing library paths of executable "$exe"
    fixlibs "$exe"
  done

  # MLT plugins
  log Copying MLT plugins
  cmd mkdir -p lib/mlt 2>/dev/null
  cmd cp $FINAL_INSTALL_DIR/lib/mlt/libmlt*.dylib lib/mlt
  cmd mkdir share 2>/dev/null
  cmd cp -R $FINAL_INSTALL_DIR/share/mlt share
  for lib in lib/mlt/*; do
    log fixing library paths of "$lib"
    fixlibs "$lib"
  done

  # Qt4 plugins
  log Copying Qt plugins
  cmd mkdir -p lib/qt4 2>/dev/null
  # try MacPorts first
  if [ -d "/opt/local/share/qt4/plugins" ]; then
    cmd cp -Rn "/opt/local/share/qt4/plugins"/{accessible,bearer,codecs,designer,graphicssystems,iconengines,imageformats,qmltooling,sceneformats,script,sqldrivers} lib/qt4
  # try Qt Creator next
  elif [ -d "/Applications/Qt Creator.app/Contents/PlugIns" ]; then
    cmd cp -Rn "/Applications/Qt Creator.app/Contents/PlugIns"/{accessible,bearer,codecs,designer,graphicssystems,iconengines,imageformats,qmltooling,sceneformats,script,sqldrivers} lib/qt4
  fi
  for dir in lib/qt4/*; do
    for lib in $dir/*; do
      log fixing library paths of Qt plugin "$lib"
      fixlibs "$lib"
    done
  done

  # frei0r plugins
  log Copying frei0r plugins
  cmd mkdir lib/frei0r-1 2>/dev/null
  cmd cp -Rn $FINAL_INSTALL_DIR/lib/frei0r-1 lib
  for lib in lib/frei0r-1/*; do
    log fixing library paths of frei0r plugin "$lib"
    fixlibs "$lib"
  done

  # LADSPA plugins
  log Copying LADSPA plugins
  cmd mkdir lib/ladspa 2>/dev/null
  cmd cp -Rn $FINAL_INSTALL_DIR/lib/ladspa/* lib/ladspa
  for lib in lib/ladspa/*; do
    log fixing library paths of LADSPA plugin "$lib"
    fixlibs "$lib"
  done

  popd

  # build DMG
  log Making disk image
  dmg_name="$INSTALL_DIR/shotcut.dmg"
  cmd rm "$dmg_name" 2>/dev/null
  cmd rm -rf staging 2>/dev/null
  cmd mkdir staging
  cmd mv shotcut/Shotcut.app staging
  cmd ln -s /Applications staging
  sync
  cmd hdiutil create -fs HFS+ -srcfolder staging -volname Shotcut -format UDBZ "$dmg_name"
  while [ "$?" -ne 0 ]; do
    cmd hdiutil create -fs HFS+ -srcfolder staging -volname Shotcut -format UDBZ "$dmg_name"
  done
  cmd rm -rf staging
}

function deploy_win32 
{
  trace "Entering deploy_win32 @ = $@"
  pushd .

  # Change to right directory
  log Changing directory to $FINAL_INSTALL_DIR
  cmd cd $FINAL_INSTALL_DIR || die "Unable to change to directory $FINAL_INSTALL_DIR"

  cmd mv bin/*.dll .
  cmd mv bin/ffmpeg.exe .
  cmd rm -rf bin include etc man manifest src *.txt
  cmd rm lib/*
  cmd rm -rf lib/pkgconfig
  cmd rm -rf share/doc share/man share/ffmpeg/examples share/aclocal share/glib-2.0 share/gtk-2.0 share/gtk-doc share/themes
  cmd cp -p "$QTDIR"/bin/Qt{Core,Gui,OpenGL,Xml,Svg}4.dll .
  cmd mkdir lib/qt4
  cmd cp -pr "$QTDIR"/plugins/* lib/qt4
  cmd tar -xjf "$HOME/ladspa_plugins-win-0.4.15.tar.bz2"

  log Making archive
  cmd cd ..
  cmd rm shotcut.zip 2>/dev/null
  cmd zip -r -9 shotcut.zip $(basename "$FINAL_INSTALL_DIR")

  popd
}

#################################################################
# create_startup_script
# Creates a startup script. Note, that the actual script gets
# embedded by the Makefile
function create_startup_script {
  if test "$TARGET_OS" = "Darwin" ; then
    deploy_osx
    return
  elif test "$TARGET_OS" = "Win32" ; then
    deploy_win32
    return
  fi

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
export LD_LIBRARY_PATH="\$INSTALL_DIR/lib":"\$INSTALL_DIR/lib/frei0r-1":\$LD_LIBRARY_PATH
export MLT_REPOSITORY="\$INSTALL_DIR/lib/mlt"
export MLT_DATA="\$INSTALL_DIR/share/mlt"
export MLT_PROFILES_PATH="\$INSTALL_DIR/share/mlt/profiles"
export FREI0R_PATH="\$INSTALL_DIR/lib/frei0r-1":/usr/lib/frei0r-1:/usr/local/lib/frei0r-1:/opt/local/lib/frei0r-1
export MANPATH=\$MANPATH:"\$INSTALL_DIR/share/man"
export PKG_CONFIG_PATH="\$INSTALL_DIR/lib/pkgconfig":\$PKG_CONFIG_PATH
End-of-environment-setup-template
  if test 0 != $? ; then
    die "Unable to create environment script"
  fi
  chmod 755 $TMPFILE || die "Unable to make environment script executable"
  $SUDO cp $TMPFILE "$FINAL_INSTALL_DIR/source-me" || die "Unable to create environment script - cp failed"

  log Creating wrapper script in $TMPFILE
  cat > $TMPFILE <<End-of-melt-wrapper
#!/bin/sh
# Set up environment
# Run this instead of trying to run bin/melt. It runs melt with the correct environment. 
CURRENT_DIR=\$(readlink -f "\$0")
INSTALL_DIR=\$(dirname "\$CURRENT_DIR")
export LD_LIBRARY_PATH="\$INSTALL_DIR/lib":"\$INSTALL_DIR/lib/frei0r-1":\$LD_LIBRARY_PATH
export MLT_REPOSITORY="\$INSTALL_DIR/lib/mlt"
export MLT_DATA="\$INSTALL_DIR/share/mlt"
export MLT_PROFILES_PATH="\$INSTALL_DIR/share/mlt/profiles"
export FREI0R_PATH="\$INSTALL_DIR/lib/frei0r-1":/usr/lib/frei0r-1:/usr/local/lib/frei0r-1:/opt/local/lib/frei0r-1
"\$INSTALL_DIR/bin/melt" \$@
End-of-melt-wrapper
  if test 0 != $? ; then
    die "Unable to create wrapper script"
  fi
  chmod 755 $TMPFILE || die "Unable to make wrapper script executable"
  $SUDO cp $TMPFILE "$FINAL_INSTALL_DIR/melt" || die "Unable to create wrapper script - cp failed"

  log Creating wrapper script in $TMPFILE
  cat > $TMPFILE <<End-of-shotcut-wrapper
#!/bin/sh
# Set up environment
# Run this instead of trying to run bin/shotcut. It runs shotcut with the correct environment. 
CURRENT_DIR=\$(readlink -f "\$0")
INSTALL_DIR=\$(dirname "\$CURRENT_DIR")
export LD_LIBRARY_PATH="\$INSTALL_DIR/lib":"\$INSTALL_DIR/lib/frei0r-1":\$LD_LIBRARY_PATH
export MLT_REPOSITORY="\$INSTALL_DIR/lib/mlt"
export MLT_DATA="\$INSTALL_DIR/share/mlt"
export MLT_PROFILES_PATH="\$INSTALL_DIR/share/mlt/profiles"
export FREI0R_PATH="\$INSTALL_DIR/lib/frei0r-1":/usr/lib/frei0r-1:/usr/local/lib/frei0r-1:/opt/local/lib/frei0r-1
"\$INSTALL_DIR/bin/shotcut" \$@
End-of-shotcut-wrapper
  if test 0 != $? ; then
    die "Unable to create wrapper script"
  fi
  chmod 755 $TMPFILE || die "Unable to make wrapper script executable"
  $SUDO cp $TMPFILE "$FINAL_INSTALL_DIR/shotcut" || die "Unable to create wrapper script - cp failed"

  log Creating desktop file in $TMPFILE
  cat > $TMPFILE <<End-of-desktop-file
#!/usr/bin/env xdg-open
[Desktop Entry]
Type=Application
Name=Shotcut
GenericName=Video Editor
Comment=Video Editor
Terminal=false
Exec=sh -c "\$(dirname "%k")/.app/shotcut"
Icon=applications-multimedia
End-of-desktop-file
  if test 0 != $? ; then
    die "Unable to create desktop file"
  fi
  $SUDO cp $TMPFILE "$FINAL_INSTALL_DIR/../Shotcut.desktop" || die "Unable to create desktop file - cp failed"

  feedback_progress Done creating startup and environment script
  popd

  log Creating archive
  tarball="$INSTALL_DIR/shotcut.tar.bz2"
  cmd rm "$tarball" 2>/dev/null
  cmd pushd "$INSTALL_DIR"
  cmd rm -rf Shotcut/.app/include
  cmd rm Shotcut/.app/lib/*.a
  cmd rm -rf Shotcut/.app/lib/pkgconfig
  cmd rm -rf Shotcut/.app/share/doc
  cmd rm -rf Shotcut/.app/share/man
  cmd tar -cjvf "$tarball" Shotcut
  cmd rm -rf Shotcut
  popd
}

#################################################################
# perform_action
# Actually do what the user wanted
function perform_action {
  trace "Entering perform_action @ = $@"
  # Test that may fail goes here, before we do anything
  if test 1 = "$GET" -a 1 = "$SOURCES_CLEAN"; then
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

  # Special case for sudo getting
  SUDO=""
  log "Checking for sudo requirement" 2>&1
  if test "1" = "$NEED_SUDO" ; then
    log "sudo is needed"
        echo You have chosen to install as root. 
        echo
        echo 'Please provide your sudo password below.  (If you have recently provided your sudo password to this script, you may not have to do that, because the password is cached).'
        echo
        echo The password will be handled securely by the sudo program. 
        echo 
        echo If you fail to provide the password, you will have to provide it later when installing the different projects.
        sudo -v
        if test 0 != $? ; then
          die "Unable to proceed"
        fi
        SUDO=sudo
  fi
  log "Done checking for sudo requirement" 2>&1
  
  { 
  prepare_feedback
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
