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
FREI0R_HEAD=1
FREI0R_REVISION=
ENABLE_MOVIT=1
SUBDIRS=
MOVIT_HEAD=0
MOVIT_REVISION="origin/shotcut"
ENABLE_SWH_PLUGINS=0
FFMPEG_HEAD=0
FFMPEG_REVISION="origin/release/5.0"
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
ENABLE_ZIMG=1
ZIMG_HEAD=1
ZIMG_REVISION=
DAV1D_HEAD=0
DAV1D_REVISION="0.9.2"
AOM_HEAD=0
AOM_REVISION="v3.2.0"
ENABLE_VMAF=1
VMAF_HEAD=0
VMAF_REVISION="v2.3.0"


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
  while getopts ":tsc:o:v:" OPT; do
    case $OPT in
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
    swh-plugins)
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
    zimg)
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
    SUBDIRS="aom dav1d AMF nv-codec-headers FFmpeg"
    if test "$ENABLE_SWH_PLUGINS" = "1"; then
        SUBDIRS="$SUBDIRS swh-plugins"
    fi
    if test "$ENABLE_MOVIT" = 1 && test "$MOVIT_HEAD" = 1 -o "$MOVIT_REVISION" != ""; then
        SUBDIRS="$SUBDIRS movit"
    fi
    if test "$ENABLE_FREI0R" = 1 ; then
        SUBDIRS="$SUBDIRS frei0r"
    fi
    if test "$ENABLE_BIGSH0T" = 1 ; then
        SUBDIRS="$SUBDIRS bigsh0t"
    fi
    if test "$ENABLE_ZIMG" = 1 ; then
        SUBDIRS="zimg $SUBDIRS"
    fi
    if test "$ENABLE_VMAF" = 1 ; then
        SUBDIRS="vmaf $SUBDIRS"
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
  REPOLOCS[5]="http://ftp.us.debian.org/debian/pool/main/s/swh-plugins/swh-plugins_0.4.15+1.orig.tar.gz"
  REPOLOCS[6]="https://github.com/FFmpeg/nv-codec-headers.git"
  REPOLOCS[7]="https://github.com/GPUOpen-LibrariesAndSDKs/AMF.git"
#  REPOLOCS[8]="https://bitbucket.org/dandennedy/bigsh0t.git"
  REPOLOCS[8]="https://bitbucket.org/leo_sutic/bigsh0t.git"
  REPOLOCS[9]="https://github.com/sekrit-twc/zimg.git"
  REPOLOCS[10]="https://code.videolan.org/videolan/dav1d.git"
  REPOLOCS[11]="https://aomedia.googlesource.com/aom"
  REPOLOCS[12]="https://github.com/Netflix/vmaf.git"

  # REPOTYPE Array holds the repo types. (Yes, this might be redundant, but easy for me)
  REPOTYPES[0]="git"
  REPOTYPES[1]="git"
  REPOTYPES[2]="git"
  REPOTYPES[3]="git"
  REPOTYPES[4]="git"
  REPOTYPES[5]="http-tgz"
  REPOTYPES[6]="git"
  REPOTYPES[7]="git"
  REPOTYPES[8]="git"
  REPOTYPES[9]="git"
  REPOTYPES[10]="git"
  REPOTYPES[11]="git"
  REPOTYPES[12]="git"

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
  REVISIONS[5]="swh-plugins-0.4.15+1"
  REVISIONS[6]="sdk/8.1" # nv-codec-headers
  REVISIONS[7]="" # AMF
  REVISIONS[8]=""
  if test 0 = "$BIGSH0T_HEAD" -a "$BIGSH0T_REVISION" ; then
    REVISIONS[8]="$BIGSH0T_REVISION"
  fi
  REVISIONS[9]=""
  if test 0 = "$ZIMG_HEAD" -a "$ZIMG_REVISION" ; then
    REVISIONS[9]="$ZIMG_REVISION"
  fi
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
  export QTDIR="$HOME/Qt/5.15.2/mingw81_64"
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
  CONFIG[0]="${CONFIG[0]} --enable-libtheora --enable-libvorbis --enable-libmp3lame --enable-libx264 --enable-libx265 --enable-libvpx --enable-libopus --enable-libmfx --enable-libdav1d --enable-libaom --disable-decoder=libaom_av1 --enable-libwebp"
  # Add optional parameters
  if [ "$ENABLE_ZIMG" = "1" ]; then
    CONFIG[0]="${CONFIG[0]} --enable-libzimg"
  fi
  if [ "$ENABLE_VMAF" = "1" ]; then
    CONFIG[0]="${CONFIG[0]} --enable-libvmaf --disable-w32threads"
  fi
  CONFIG[0]="${CONFIG[0]} $FFMPEG_ADDITIONAL_OPTIONS"
  CFLAGS_[0]="-I$FINAL_INSTALL_DIR/include $CFLAGS"

  #####
  # mlt
  CONFIG[1]="cmake -GNinja -DCMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR -DCMAKE_PREFIX_PATH=$QTDIR -DGPL=ON -DGPL3=ON -DMOD_GLAXNIMATE=ON -DMOD_GDK=OFF -DMOD_SDL1=OFF $CMAKE_DEBUG_FLAG"
  CFLAGS_[1]="-I$FINAL_INSTALL_DIR/include $ASAN_CFLAGS $CFLAGS"
  LDFLAGS_[1]="-L$FINAL_INSTALL_DIR/lib $ASAN_LDFLAGS $LDFLAGS"

  ####
  # frei0r
  CONFIG[2]="cmake -DCMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR -DWITHOUT_GAVL=1 -DWITHOUT_OPENCV=1 -GNinja $CMAKE_DEBUG_FLAG"
  CFLAGS_[2]="$CFLAGS"
  LDFLAGS_[2]=$LDFLAGS

  #####
  # movit
  CONFIG[3]="./autogen.sh --prefix=$FINAL_INSTALL_DIR"
  # MinGW does not provide ffs(), but there is a gcc intrinsic for it.
  CFLAGS_[3]="$CFLAGS -Dffs=__builtin_ffs"
  CFLAGS_[3]="${CFLAGS_[3]} -fpermissive"
  LDFLAGS_[3]=$LDFLAGS

  #####
  # shotcut
  CONFIG[4]="cmake -G Ninja -D CMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR -DCMAKE_PREFIX_PATH=$QTDIR -D SHOTCUT_VERSION=$SHOTCUT_VERSION $CMAKE_DEBUG_FLAG"
  CFLAGS_[4]="$ASAN_CFLAGS $CFLAGS"
  LDFLAGS_[4]="$ASAN_LDFLAGS $LDFLAGS"

  #####
  # swh-plugins
  CONFIG[5]="./configure --prefix=$FINAL_INSTALL_DIR --enable-sse"
  CFLAGS_[5]=$CFLAGS
  LDFLAGS_[5]=$LDFLAGS

  #######
  # nv-codec-headers
  CONFIG[6]="sed -i s,/usr/local,$FINAL_INSTALL_DIR, Makefile"

  #######
  # AMF - no build required
  CONFIG[7]=""

  #########
  # bigsh0t
  CONFIG[8]="cmake -DCMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR -GNinja $CMAKE_DEBUG_FLAG"
  CFLAGS_[8]=$CFLAGS
  LDFLAGS_[8]=$LDFLAGS

  #####
  # zimg
  CONFIG[9]="./configure --prefix=$FINAL_INSTALL_DIR"
  CFLAGS_[9]=$CFLAGS
  LDFLAGS_[9]=$LDFLAGS

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

  #####
  # aom
  # Use -DCMAKE_POSITION_INDEPENDENT_CODE=ON if -DBUILD_SHARED_LIBS=1 does not work.
  CONFIG[11]="cmake -GNinja -DCMAKE_INSTALL_PREFIX=$FINAL_INSTALL_DIR $CMAKE_DEBUG_FLAG -DBUILD_SHARED_LIBS=1 -DCONFIG_AV1_DECODER=0 -DENABLE_EXAMPLES=0 -DENABLE_TESTS=0 ../aom"
  CFLAGS_[11]=$CFLAGS
  LDFLAGS_[11]=$LDFLAGS

  #####
  # vmaf
  CONFIG[12]="meson setup libvmaf/build libvmaf --prefix=$FINAL_INSTALL_DIR --libdir=$FINAL_INSTALL_DIR/lib"
  if [ "$DEBUG_BUILD" = "1" ]; then
    CONFIG[12]="${CONFIG[12]} --buildtype=debug"
  else
    CONFIG[12]="${CONFIG[12]} --buildtype=release"
  fi
  CFLAGS_[12]=$CFLAGS
  LDFLAGS_[12]=$LDFLAGS
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
  export LDFLAGS=`lookup LDFLAGS_ $1`
  log LDFLAGS=$LDFLAGS

  # Configure
  if [ "$ACTION_CONFIGURE" = "1" ]; then

  # Configure
  feedback_status Configuring $1

  # Special hack for mlt
  if test "mlt" = "$1"; then
    export CXXFLAGS="$CFLAGS -std=c++11"
  fi

  # Special hack for movit
  if test "movit" = "$1"; then
#    export PATH="$HOME/mingw810_64/bin:$PATH"
    export CXXFLAGS="$CFLAGS"
  fi

  # Special hack for AMF
  if test "AMF" = "$1" -a ! -d "$FINAL_INSTALL_DIR/include/AMF"; then
    cmd rm -rf Thirdparty
    cmd mkdir -p "$FINAL_INSTALL_DIR/include/AMF"
    cmd cp -av "amf/public/include/." "$FINAL_INSTALL_DIR/include/AMF"
  fi

  # Special hack for zimg
  if test "zimg" = "$1" -a ! -e configure ; then
    debug "Need to create configure for $1"
    cmd ./autogen.sh || die "Unable to create configure file for $1"
    if test ! -e configure ; then
      die "Unable to confirm presence of configure file for $1"
    fi
  fi

  # Special hack for aom
  if test "aom" = "$1"; then
    cmd mkdir -p ../build-aom
    cmd cd ../build-aom || die "Unable to change to directory aom/builddir"
  fi

  MYCONFIG=`lookup CONFIG $1`
  if test "$MYCONFIG" != ""; then
    cmd $MYCONFIG || die "Unable to configure $1"
    feedback_status Done configuring $1
  fi

  fi # if [ "$ACTION_CONFIGURE" = "1" ]

  # Compile
  feedback_status Building $1 - this could take some time
  if test "movit" = "$1" ; then
    cmd make -j$MAKEJ libmovit.la || die "Unable to build $1"
  elif test "frei0r" = "$1" -o "bigsh0t" = "$1" -o "aom" = "$1" -o "mlt" = "$1" -o "shotcut" = "$1"; then
    cmd ninja -j $MAKEJ || die "Unable to build $1"
  elif test "dav1d" = "$1"; then
    cmd ninja -C builddir -j $MAKEJ || die "Unable to build $1"
  elif test "vmaf" = "$1"; then
    cmd ninja -C libvmaf/build -j $MAKEJ || die "Unable to build $1"
  elif test "$MYCONFIG" != ""; then
    cmd make -j$MAKEJ || die "Unable to build $1"
  fi
  feedback_status Done building $1

  # Install
  feedback_status Installing $1
  export LD_LIBRARY_PATH=`lookup LD_LIBRARY_PATH_ $1`
  log "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
  if test "shotcut" = "$1" ; then
    cmd ninja install
    cmd install -c COPYING "$FINAL_INSTALL_DIR"
    cmd install -c packaging/windows/shotcut.nsi "$FINAL_INSTALL_DIR"/..
    cmd sed -i "s/YY.MM.DD/$SHOTCUT_VERSION/" "$FINAL_INSTALL_DIR"/../shotcut.nsi
    cmd install -d "$FINAL_INSTALL_DIR"/share/translations
    cmd install -p -c translations/*.qm "$FINAL_INSTALL_DIR"/share/translations
  elif test "frei0r" = "$1" -o "aom" = "$1" -o "mlt" = "$1"; then
    cmd ninja install || die "Unable to install $1"
  elif test "bigsh0t" = "$1" ; then
    cmd install -p -c *.dll "$FINAL_INSTALL_DIR"/lib/frei0r-1  || die "Unable to install $1"
  elif test "dav1d" = "$1"; then
    cmd meson install -C builddir || die "Unable to install $1"
  elif test "vmaf" = "$1"; then
    cmd ninja install -C libvmaf/build || die "Unable to install $1"
    cmd install -d "$FINAL_INSTALL_DIR"/share/vmaf
    cmd install -p -c model/*.json "$FINAL_INSTALL_DIR"/share/vmaf || die "Unable to install $1"
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


#################################################################
# bundle_dlls
function bundle_dlls
{
  log bundling library dependencies of $(basename "$1")
  target=$(dirname "$1")/$(basename "$1")
  basename_target=$(basename "$target")
  libs=$(ldd "$target" | awk '($3 ~ /\/mingw64\/bin\//) {print $3}')
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
  else
    cmd mv bin/ffmpeg.exe .
    cmd mv bin/ffplay.exe .
    cmd mv bin/ffprobe.exe .
    cmd rm -rf bin include etc man manifest src *.txt
    cmd rm lib/*
    cmd rm -rf lib/cmake lib/pkgconfig lib/gdk-pixbuf-2.0 lib/glib-2.0 lib/gtk-2.0
    cmd rm -rf share/doc share/man share/ffmpeg/examples share/aclocal share/glib-2.0 share/gtk-2.0 share/gtk-doc share/themes share/locale
  fi
  cmd mv COPYING COPYING.txt

  log Copying some libs from Qt
  if [ "$DEBUG_BUILD" != "1" -o "$SDK" = "1" ]; then
    cmd cp -p "$QTDIR"/bin/Qt5{Core,Gui,Multimedia,Network,OpenGL,Qml,QmlModels,QmlWorkerScript,Quick,QuickControls2,QuickTemplates2,QuickWidgets,Sql,Svg,WebSockets,Widgets,WinExtras,Xml}.dll .
  fi
  if [ "$DEBUG_BUILD" = "1" -o "$SDK" = "1" ]; then
    cmd cp -p "$QTDIR"/bin/Qt5{Core,Gui,Multimedia,Network,OpenGL,Qml,QmlModels,QmlWorkerScript,Quick,QuickControls2,QuickTemplates2,QuickWidgets,Sql,Svg,WebSockets,Widgets,WinExtras,Xml}d.dll .
  fi

  if [ "ENABLE_SWH_PLUGINS" != "1" ]; then
    log Copying LADSPA plugins
    cmd tar -xJf "$HOME/swh-plugins-win64-0.4.15.tar.xz"
  fi

  log Copying some libs from mlt-prebuilt
  cmd cp -p /mingw64/bin/.dll

  for lib in *.dll; do
    bundle_dlls "$lib"
  done
  for lib in lib/{frei0r-1,ladspa,mlt}/*.dll; do
    bundle_dlls "$lib"
  done
  cmd rm *.bundled

  cmd cp -p "$HOME"/bin/*.dll .
  cmd cp -p "$QTDIR"/bin/{libEGL,libGLESv2,d3dcompiler_47}.dll .

  log Copying some libs from msys2
  cmd cp -p /mingw64/bin/{libcrypto-1_1-x64,libssl-1_1-x64}.dll .
  if [ "$DEBUG_BUILD" = "1" -o "$SDK" = "1" ]; then
    cmd cp -p "$SOURCE_DIR"/shotcut/drmingw/x64/bin/*.{dll,yes} .
  fi

  log Copying some plugins, qml, and translations from Qt
  cmd mkdir -p lib/qt5/sqldrivers
  cmd cp -pr "$QTDIR"/plugins/{audio,generic,iconengines,imageformats,mediaservice,platforms,sqldrivers,styles} lib/qt5
  cmd cp -pr "$QTDIR"/qml lib
  sed -i "s/onClicked()/onClicked(mouse)/" lib/qml/QtQuick/Controls/Private/EditMenu_base.qml
  cmd rm lib/qml/QtQuick/Controls/Private/EditMenu_base.qmlc
  cmd cp -pr "$QTDIR"/translations/qt_*.qm share/translations
  cmd cp -pr "$QTDIR"/translations/qtbase_*.qm share/translations

  log Removing things not needed
  cmd rm lib/qt5/sqldrivers/{qsqlodbc,qsqlodbcd,qsqlpsql,qsqlpsqld}.dll
  # colortap is not used by Shotcut and no plans to include it, but
  # Kaspersky antivirus is flagging it and scaring people.
  cmd rm lib/frei0r-1/colortap.dll
  if [ "$DEBUG_BUILD" != "1" -a "$SDK" != "1" ]; then
    cmd rm lib/qt5/audio/qtaudio_windowsd.dll
    cmd rm lib/qt5/generic/qtuiotouchplugind.dll
    cmd rm lib/qt5/iconengines/qsvgicond.dll
    cmd rm lib/qt5/imageformats/qddsd.dll
    cmd rm lib/qt5/imageformats/qgifd.dll
    cmd rm lib/qt5/imageformats/qicod.dll
    cmd rm lib/qt5/imageformats/qicnsd.dll
    cmd rm lib/qt5/imageformats/qjpegd.dll
    cmd rm lib/qt5/imageformats/qsvgd.dll
    cmd rm lib/qt5/imageformats/qtgad.dll
    cmd rm lib/qt5/imageformats/qtiffd.dll
    cmd rm lib/qt5/imageformats/qwbmpd.dll
    cmd rm lib/qt5/imageformats/qwebpd.dll
    cmd rm lib/qt5/mediaservice/dsengined.dll
    cmd rm lib/qt5/mediaservice/qtmedia_audioengined.dll
    cmd rm lib/qt5/platforms/qminimald.dll
    cmd rm lib/qt5/platforms/qoffscreend.dll
    cmd rm lib/qt5/platforms/qwindowsd.dll
    cmd rm lib/qt5/sqldrivers/qsqlited.dll

    cmd rm lib/qml/QtLocation/declarative_locationd.dll
    cmd rm lib/qml/QtQml/StateMachine/qtqmlstatemachined.dll
    cmd rm lib/qml/QtQml/Models.2/modelsplugind.dll
    cmd rm lib/qml/QtCanvas3D/qtcanvas3dd.dll
    cmd rm lib/qml/QtPositioning/declarative_positioningd.dll
    cmd rm lib/qml/QtWinExtras/qml_winextrasd.dll
    cmd rm lib/qml/QtQuick.2/qtquick2plugind.dll
    cmd rm lib/qml/Qt/labs/settings/qmlsettingsplugind.dll
    cmd rm lib/qml/Qt/labs/folderlistmodel/qmlfolderlistmodelplugind.dll
    cmd rm lib/qml/QtWebSockets/declarative_qmlwebsocketsd.dll
    cmd rm lib/qml/QtBluetooth/declarative_bluetoothd.dll
    cmd rm lib/qml/QtTest/qmltestplugind.dll
    cmd rm lib/qml/QtQuick/LocalStorage/qmllocalstorageplugind.dll
    cmd rm lib/qml/QtQuick/Window.2/windowplugind.dll
    cmd rm lib/qml/QtQuick/Dialogs/dialogplugind.dll
    cmd rm lib/qml/QtQuick/Dialogs/Private/dialogsprivateplugind.dll
    cmd rm lib/qml/QtQuick/PrivateWidgets/widgetsplugind.dll
    cmd rm lib/qml/QtQuick/Layouts/qquicklayoutsplugind.dll
    cmd rm lib/qml/QtQuick/Controls/qtquickcontrolsplugind.dll
    cmd rm lib/qml/QtQuick/Controls/Styles/Flat/qtquickextrasflatplugind.dll
    cmd rm lib/qml/QtQuick/Particles.2/particlesplugind.dll
    cmd rm lib/qml/QtQuick/Extras/qtquickextrasplugind.dll
    cmd rm lib/qml/QtQuick/XmlListModel/qmlxmllistmodelplugind.dll
    cmd rm lib/qml/QtSensors/declarative_sensorsd.dll
    cmd rm lib/qml/QtMultimedia/declarative_multimediad.dll
    cmd rm lib/qml/QtNfc/declarative_nfcd.dll
    cmd rm lib/qml/QtWebChannel/declarative_webchanneld.dll
    cmd rm lib/qml/Qt/labs/calendar/qtlabscalendarplugind.dll
    cmd rm lib/qml/Qt/labs/platform/qtlabsplatformplugind.dll
    cmd rm lib/qml/Qt/labs/sharedimage/sharedimageplugind.dll
    cmd rm lib/qml/Qt3D/Animation/quick3danimationplugind.dll
    cmd rm lib/qml/Qt3D/Core/quick3dcoreplugind.dll
    cmd rm lib/qml/Qt3D/Extras/quick3dextrasplugind.dll
    cmd rm lib/qml/Qt3D/Input/quick3dinputplugind.dll
    cmd rm lib/qml/Qt3D/Logic/quick3dlogicplugind.dll
    cmd rm lib/qml/Qt3D/Render/quick3drenderplugind.dll
    cmd rm lib/qml/QtCharts/qtchartsqml2d.dll
    cmd rm lib/qml/QtDataVisualization/datavisualizationqml2d.dll
    cmd rm lib/qml/QtGamepad/declarative_gamepadd.dll
    cmd rm lib/qml/QtGraphicalEffects/private/qtgraphicaleffectsprivated.dll
    cmd rm lib/qml/QtGraphicalEffects/qtgraphicaleffectsplugind.dll
    cmd rm lib/qml/QtPurchasing/declarative_purchasingd.dll
    cmd rm lib/qml/QtQml/RemoteObjects/qtqmlremoteobjectsd.dll
    cmd rm lib/qml/QtQuick/Controls.2/Material/qtquickcontrols2materialstyleplugind.dll
    cmd rm lib/qml/QtQuick/Controls.2/qtquickcontrols2plugind.dll
    cmd rm lib/qml/QtQuick/Controls.2/Universal/qtquickcontrols2universalstyleplugind.dll
    cmd rm lib/qml/QtQuick/Scene2D/qtquickscene2dplugind.dll
    cmd rm lib/qml/QtQuick/Scene3D/qtquickscene3dplugind.dll
    cmd rm lib/qml/QtQuick/Templates.2/qtquicktemplates2plugind.dll
    cmd rm lib/qml/QtQuick/VirtualKeyboard/Styles/qtvirtualkeyboardstylesplugind.dll
    cmd rm lib/qml/QtScxml/declarative_scxmld.dll
  fi
  printf "[Paths]\nPlugins=lib/qt5\nQml2Imports=lib/qml\n" > qt.conf

  if [ "$SDK" = "1" ]; then
    # Prepare src for archiving
    pushd .
    clean_dirs
    popd
    log Copying src
    cmd rm -rf src 2> /dev/null
    cmd cp -a $SOURCE_DIR .

    log Creating archive
    cmd cd ..
    cmd zip -gr shotcut-sdk.zip Shotcut
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
