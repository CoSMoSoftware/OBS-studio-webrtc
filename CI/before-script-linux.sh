#!/bin/bash
set -ex

ccache -s || echo "CCache is not available."

BASE_PATH="$(pwd)"
mkdir build && cd build
cmake -DBUILD_CAPTIONS=ON -DBUILD_BROWSER=ON -DCEF_ROOT_DIR="../cef_binary_${CEF_BUILD_VERSION}_linux64" -DWEBRTC_INCLUDE_DIR="${BASE_PATH}/libwebrtc/include" -DWEBRTC_LIB="${BASE_PATH}/libwebrtc/lib/libwebrtc.a" -Dlibwebrtc_DIR="${BASE_PATH}/libwebrtc/cmake" ..
