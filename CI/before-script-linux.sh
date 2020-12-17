#!/bin/bash

set -ex
ccache -s || echo "CCache is not available."
libwebrtc_dir=`pwd`/libwebrtc/cmake
export CC=clang
export CXX=clang++

# Configure OBS for Millicast
echo "Configure OBS for Millicast"
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DUNIX_STRUCTURE=1 -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_CAPTIONS=ON -DBUILD_BROWSER=ON -DCEF_ROOT_DIR="../cef_binary_87.1.12+g03f9336+chromium-87.0.4280.88_linux64/Release" -DUSE_LIBC++=ON -Dlibwebrtc_DIR=${libwebrtc_dir} -DCPACK_DEBIAN_PACKAGE_MAINTAINER="CoSMo Software" -DCPACK_DEBIAN_PACKAGE_NAME="obs" -DCPACK_DEBIAN_PACKAGE_VERSION=${OBS_VERSION} -DCPACK_DEBIAN_PACKAGE_ARCHITECTURE="amd64" -DCPACK_DEBIAN_PACKAGE_DEPENDS="ffmpeg, libqt5gui5, libqt5xml5, libqt5x11extras5, libqt5core5a, libx264-dev, vlc, libavcodec58, libmbedtls12, libfdk-aac1, libavfilter7, libavdevice58, libc++1, libcurl4" ..
cd ..

if $PACPOST_BUILD
then
  # Configure OBS for PacPost
  echo "Configure OBS for PacPost"
  mkdir build_pacpost && cd build_pacpost
  cmake -DOBS_WEBRTC_VENDOR_NAME=PacPost -DCMAKE_BUILD_TYPE=Release -DUNIX_STRUCTURE=1 -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_CAPTIONS=ON -DBUILD_BROWSER=ON -DCEF_ROOT_DIR="../cef_binary_87.1.12+g03f9336+chromium-87.0.4280.88_linux64/Release" -DUSE_LIBC++=ON -Dlibwebrtc_DIR=${libwebrtc_dir} -DCPACK_DEBIAN_PACKAGE_MAINTAINER="CoSMo Software" -DCPACK_DEBIAN_PACKAGE_NAME="obs" -DCPACK_DEBIAN_PACKAGE_VERSION=${OBS_VERSION} -DCPACK_DEBIAN_PACKAGE_ARCHITECTURE="amd64" -DCPACK_DEBIAN_PACKAGE_DEPENDS="ffmpeg, libqt5gui5, libqt5xml5, libqt5x11extras5, libqt5core5a, libx264-dev, vlc, libavcodec58, libmbedtls12, libfdk-aac1, libavfilter7, libavdevice58, libc++1, libcurl4" ..
  cd ..
fi

if $REMOTEFILMING_BUILD
then
  # Configure OBS for RemoteFilming
  echo "Configure OBS for RemoteFilming"
  mkdir build_remotefilming && cd build_remotefilming
  cmake -DOBS_WEBRTC_VENDOR_NAME=RemoteFilming -DCMAKE_BUILD_TYPE=Release -DUNIX_STRUCTURE=1 -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_CAPTIONS=ON -DBUILD_BROWSER=ON -DCEF_ROOT_DIR="../cef_binary_87.1.12+g03f9336+chromium-87.0.4280.88_linux64/Release" -DUSE_LIBC++=ON -Dlibwebrtc_DIR=${libwebrtc_dir} -DCPACK_DEBIAN_PACKAGE_MAINTAINER="CoSMo Software" -DCPACK_DEBIAN_PACKAGE_NAME="obs" -DCPACK_DEBIAN_PACKAGE_VERSION=${OBS_VERSION} -DCPACK_DEBIAN_PACKAGE_ARCHITECTURE="amd64" -DCPACK_DEBIAN_PACKAGE_DEPENDS="ffmpeg, libqt5gui5, libqt5xml5, libqt5x11extras5, libqt5core5a, libx264-dev, vlc, libavcodec58, libmbedtls12, libfdk-aac1, libavfilter7, libavdevice58, libc++1, libcurl4" ..
  cd ..
fi