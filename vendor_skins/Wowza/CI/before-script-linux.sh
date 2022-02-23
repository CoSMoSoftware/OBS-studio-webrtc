#!/bin/bash

set -ex
ccache -s || echo "CCache is not available."
libwebrtc_dir=`pwd`/libwebrtc/cmake
export CC=clang
export CXX=clang++
# Parameter $1 = vendor name
cd build_$1
if [ "$1" == "Millicast" ]
then
  vendor_option=""
else
  vendor_option="-DOBS_WEBRTC_VENDOR_NAME=$1"
fi
cmake ${vendor_option} -DBUILD_NDI=ON -DBUILD_WEBSOCKET=ON -DCMAKE_BUILD_TYPE=${CI_BUILD_TYPE} -DUNIX_STRUCTURE=1 -DENABLE_PIPEWIRE=OFF -DENABLE_VLC=ON -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_BROWSER=ON -DCEF_ROOT_DIR="../cef_binary_${LINUX_CEF_BUILD_VERSION}_linux64" -DUSE_LIBC++=ON -Dlibwebrtc_DIR=${libwebrtc_dir} -DOBS_VERSION_OVERRIDE=${OBS_VERSION} -DCPACK_DEBIAN_PACKAGE_MAINTAINER="Wowza" -DCPACK_DEBIAN_PACKAGE_NAME="wowza" -DCPACK_DEBIAN_PACKAGE_VERSION=${OBS_VERSION} -DCPACK_DEBIAN_PACKAGE_ARCHITECTURE="amd64" -DCPACK_DEBIAN_PACKAGE_DEPENDS="ffmpeg, libqt5gui5, libqt5xml5, libqt5x11extras5, libqt5core5a, libx264-dev, vlc, libavcodec57, libmbedtls10, libfdk-aac1, libavfilter6, libavdevice57, libc++1, libcurl4" .. -DENABLE_WAYLAND=OFF
cd ..
