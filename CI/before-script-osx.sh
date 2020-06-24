# Make sure ccache is found
export PATH=/usr/local/opt/ccache/libexec:$PATH
mkdir build
cd build
cmake \
  -DENABLE_SCRIPTING=OFF \
  -DDepsPath=/tmp/obsdeps \
  -DCMAKE_BUILD_TYPE=Release \
  # -DCMAKE_INSTALL_PREFIX=/opt/ebs \
  -DVLCPath=$PWD/../../vlc-master \
  # this is brew latest qt version. OBS latest is 5.14.1
  -DQTDIR=/usr/local/Cellar/qt/5.14.2 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 \
  -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl@1.1 \
  -Dlibwebrtc_DIR=/tmp/libWebRTC-79.0-x64-Rel-COMMUNITY-BETA/cmake \
  -DBUILD_BROWSER=false \
  -DOBS_WEBRTC_VENDOR_NAME=Evercast \
  -DOBS_VERSION_OVERRIDE=23.2.0 \
  ..
