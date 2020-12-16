#!/bin/bash

set -ex
ccache -s || echo "CCache is not available."
libwebrtc_dir=`pwd`/libwebrtc/cmake
mkdir build && cd build
export CC=clang
export CXX=clang++
cmake -DCMAKE_BUILD_TYPE=Release -DUNIX_STRUCTURE=1 -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_CAPTIONS=ON -DUSE_LIBC++=ON -Dlibwebrtc_DIR=${libwebrtc_dir} -DCPACK_DEBIAN_PACKAGE_MAINTAINER="CoSMo Software" -DCPACK_DEBIAN_PACKAGE_NAME="obs" -DCPACK_DEBIAN_PACKAGE_VERSION=${OBS_VERSION} -DCPACK_DEBIAN_PACKAGE_ARCHITECTURE="amd64" -DCPACK_DEBIAN_PACKAGE_DEPENDS="ffmpeg, libqt5gui5, libqt5xml5, libqt5x11extras5, libqt5core5a, libx264-dev, vlc, libavcodec58, libmbedtls12, libfdk-aac1, libavfilter7, libavdevice58, libc++1, libcurl4" ..
