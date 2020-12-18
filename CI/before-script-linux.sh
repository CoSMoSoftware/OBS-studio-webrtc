#!/bin/bash

set -ex
ccache -s || echo "CCache is not available."
libwebrtc_dir=`pwd`/libwebrtc/cmake
export CC=clang
export CXX=clang++
mkdir build_${VENDOR_BUILD} && cd build_${VENDOR_BUILD}
if [ "${VENDOR_BUILD}" == "Millicast" ]
then
  cmake -DCMAKE_BUILD_TYPE=Release -DUNIX_STRUCTURE=1 -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_CAPTIONS=ON -DBUILD_BROWSER=ON -DCEF_ROOT_DIR="../cef_binary_85.3.12+g3e94ebf+chromium-85.0.4183.121_linux64" -DUSE_LIBC++=ON -Dlibwebrtc_DIR=${libwebrtc_dir} -DCPACK_DEBIAN_PACKAGE_MAINTAINER="CoSMo Software" -DCPACK_DEBIAN_PACKAGE_NAME="obs" -DCPACK_DEBIAN_PACKAGE_VERSION=${OBS_VERSION} -DCPACK_DEBIAN_PACKAGE_ARCHITECTURE="amd64" -DCPACK_DEBIAN_PACKAGE_DEPENDS="ffmpeg, libqt5gui5, libqt5xml5, libqt5x11extras5, libqt5core5a, libx264-dev, vlc, libavcodec58, libmbedtls12, libfdk-aac1, libavfilter7, libavdevice58, libc++1, libcurl4" ..
else
  cmake -DOBS_WEBRTC_VENDOR_NAME=${VENDOR_BUILD} -DCMAKE_BUILD_TYPE=Release -DUNIX_STRUCTURE=1 -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_CAPTIONS=ON -DBUILD_BROWSER=ON -DCEF_ROOT_DIR="cef_binary_85.3.12+g3e94ebf+chromium-85.0.4183.121_linux64" -DUSE_LIBC++=ON -Dlibwebrtc_DIR=${libwebrtc_dir} -DCPACK_DEBIAN_PACKAGE_MAINTAINER="CoSMo Software" -DCPACK_DEBIAN_PACKAGE_NAME="obs" -DCPACK_DEBIAN_PACKAGE_VERSION=${OBS_VERSION} -DCPACK_DEBIAN_PACKAGE_ARCHITECTURE="amd64" -DCPACK_DEBIAN_PACKAGE_DEPENDS="ffmpeg, libqt5gui5, libqt5xml5, libqt5x11extras5, libqt5core5a, libx264-dev, vlc, libavcodec58, libmbedtls12, libfdk-aac1, libavfilter7, libavdevice58, libc++1, libcurl4" ..
fi