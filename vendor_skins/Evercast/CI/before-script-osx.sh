# Copyright Dr. Alex. Gouaillard (2015, 2020)

# Make sure ccache is found
export PATH=/usr/local/opt/ccache/libexec:$PATH
mkdir build
cd build
cmake \
-DDepsPath=/tmp/obsdeps \
-DCMAKE_BUILD_TYPE=RELEASE \
-DCMAKE_INSTALL_PREFIX=/opt/ebs \
-DVLCPath=$PWD/../../vlc-master \
-DQTDIR=/usr/local/Cellar/qt/5.14.2 \
-DCMAKE_OSX_DEPLOYMENT_TARGET=10.10 \
-DOPENSSL_ROOT_DIR=/usr/local/opt/openssl@1.1 \
-Dlibwebrtc_DIR=/tmp/libWebRTC-79.0-x64-Rel-COMMUNITY-BETA/cmake \
-DOBS_VERSION_OVERRIDE=23.2.0 \
..

