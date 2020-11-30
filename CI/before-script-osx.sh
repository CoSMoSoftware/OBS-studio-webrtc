# Make sure ccache is found
export PATH=/usr/local/opt/ccache/libexec:$PATH

git fetch --tags

mkdir build
cd build
cmake \
  -DENABLE_SPARKLE_UPDATER=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 \
  -DENABLE_SCRIPTING=ON \
  -DDISABLE_PYTHON=ON \
  -DQTDIR=/usr/local/Cellar/qt/5.14.1 \
  -DDepsPath=/tmp/obsdeps \
  -DVLCPath=$PWD/../../vlc-3.0.8 \
  -DBUILD_BROWSER=ON \
  -DBROWSER_DEPLOY=ON \
  -DBUILD_CAPTIONS=ON \
  -DWITH_RTMPS=ON \
  -DCEF_ROOT_DIR=$PWD/../../cef_binary_${CEF_BUILD_VERSION}_macosx64 \
  -DOBS_WEBRTC_VENDOR_NAME=Millicast \
  -DOBS_VERSION_OVERRIDE=26 \
  -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl@1.1 \
  -Dlibwebrtc_DIR=/tmp/libWebRTC-84.0-x64-Rel-COMMUNITY-BETA/cmake \
  ..
