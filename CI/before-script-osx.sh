# Make sure ccache is found
export PATH=/usr/local/opt/ccache/libexec:$PATH
mkdir build
cd build
cmake \
-DCMAKE_OSX_DEPLOYMENT_TARGET=10.10 \
-DCMAKE_BUILD_TYPE=RELEASE \
-DDepsPath=/tmp/obsdeps \
-DVLCPath=$PWD/../../vlc-master \
-Dlibwebrtc_DIR=/tmp/libWebRTC-73.0-x64-Rel-COMMUNITY-BETA/cmake \
-DQTDIR=/usr/local/Cellar/qt/5.10.1 \
..
cmake --build .
