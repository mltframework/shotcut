#!/bin/bash

# Install dependencies for building shotcut on Linux.

# Exit on first failure.
set -e

# Exit on unset variable.
set -u

# Echo commands before executing them, by default to stderr.
set -x

apt-get install -yqq \
    apt-transport-https \
    autoconf \
    automake \
    autopoint \
    bison \
    bzip2 \
    ca-certificates \
    curl \
    flex \
    gettext \
    git \
    gnupg \
    gperf \
    intltool \
    intltool \
    ladspa-sdk \
    libarchive-dev \
    libcurl4-openssl-dev \
    libdouble-conversion-dev \
    libdv-dev \
    libegl1-mesa-dev \
    libeigen3-dev \
    libepoxy-dev \
    libexif-dev \
    libffi-dev \
    libfftw3-dev \
    libgtk2.0-dev \
    libjack-dev \
    liblist-moreutils-perl \
    libltdl-dev \
    libmp3lame-dev \
    libmp3lame-dev \
    libpotrace-dev \
    libsamplerate-dev \
    libsdl2-dev \
    libsox-dev \
    libssl-dev \
    libtheora-dev \
    libtool \
    libva-dev \
    libvorbis-dev \
    libwebp-dev \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-render-util0 \
    libxcb-xinerama0 \
    libxkbcommon-x11-0 \
    libxml-parser-perl \
    libxml2-dev \
    libxslt1-dev \
    make \
    nasm \
    ninja-build \
    openssl \
    p7zip \
    patch \
    perl \
    pkg-config \
    python-is-python3 \
    python3-pip \
    python3.9-dev \
    ruby \
    scons \
    sed \
    software-properties-common \
    swig \
    unzip \
    va-driver-all \
    wget \
    xutils-dev \
    xz-utils \
    yasm \
    zip

curl https://apt.kitware.com/keys/kitware-archive-latest.asc \
  | gpg --dearmor - >/etc/apt/trusted.gpg.d/kitware.gpg

apt-add-repository -y 'ppa:ubuntu-toolchain-r/test'
apt-add-repository -y "deb https://apt.kitware.com/ubuntu/ $(lsb_release -c -s) main"
apt-get update -qq
apt-get install -yqq \
    cmake \
    g++-10 \
    libclang-cpp10

pip3 install meson
