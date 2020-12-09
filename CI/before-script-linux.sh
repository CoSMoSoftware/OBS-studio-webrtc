#!/bin/bash

set -ex
ccache -s || echo "CCache is not available."
libwebrtc_dir=`pwd`/libwebrtc/cmake
mkdir build && cd build
cmake -DUNIX_STRUCTURE=1 -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_CAPTIONS=ON -DBUILD_BROWSER=ON -DCEF_ROOT_DIR="../cef_binary_${CEF_BUILD_VERSION}_linux64" -DLibWebRTC_DIR=${libwebrtc_dir} -DWEBRTC_INCLUDE_DIR="../libwebrtc/include" -DWEBRTC_LIB="../libwebrtc/lib/libwebrtc.a" -DCPACK_DEBIAN_PACKAGE_MAINTAINER="CoSMo Software" -DCPACK_DEBIAN_PACKAGE_NAME="obs" -DCPACK_DEBIAN_PACKAGE_VERSION=${OBS_VERSION} -DCPACK_DEBIAN_PACKAGE_ARCHITECTURE="amd64" -DCPACK_DEBIAN_PACKAGE_DEPENDS="ffmpeg, libqt5gui5, libqt5xml5, libqt5x11extras5, libqt5core5a, libx264-dev, vlc, libavcodec57, libmbedtls10, libfdk-aac1, libavfilter6, libavdevice57, libc++1, libcurl4" ..
