#!/bin/sh
set -ex

curl -L https://packagecloud.io/github/git-lfs/gpgkey | sudo apt-key add -

sudo apt-get -qq update
sudo apt-get install -y \
        build-essential \
        checkinstall \
        clang \
        cmake \
        libc++-dev \
        libc++abi-dev \
        libmbedtls-dev \
        libasound2-dev \
        libavcodec-dev \
        libavdevice-dev \
        libavfilter-dev \
        libavformat-dev \
        libavutil-dev \
        libcurl4-openssl-dev \
        libfdk-aac-dev \
        libfontconfig-dev \
        libfreetype6-dev \
        libgl1-mesa-dev \
        libglvnd-dev \
        libjack-jackd2-dev \
        libjansson-dev \
        libluajit-5.1-dev \
        libpulse-dev \
        libqt5x11extras5-dev \
        libspeexdsp-dev \
        libswresample-dev \
        libswscale-dev \
        libudev-dev \
        libv4l-dev \
        libvlc-dev \
        libwayland-dev \
        libx11-dev \
        libx264-dev \
        libxcb-randr0-dev \
        libxcb-shm0-dev \
        libxcb-xinerama0-dev \
        libxcomposite-dev \
        libxinerama-dev \
        libmbedtls-dev \
        pkg-config \
        python3-dev \
        qtbase5-dev \
        qtbase5-private-dev \
        libqt5svg5-dev \
        swig \
        libxcb-xfixes0-dev \
        libx11-xcb-dev \
        libxcb1-dev \
        libxss-dev \
        qtwayland5 \
        libgles2-mesa \
        libgles2-mesa-dev \
        libnss3-dev

#        libcmocka-dev \
#        libssl-dev \
#        libgtkglext1-dev \
#        libxi-dev
#        libva-dev \
#        libgl1-mesa-dev \

# build cef
export CC=clang
export CXX=clang++
#wget --quiet --retry-connrefused --waitretry=1 https://cdn-fastly.obsproject.com/downloads/cef_binary_${LINUX_CEF_BUILD_VERSION}_linux64.tar.bz2
#tar -xjf ./cef_binary_${LINUX_CEF_BUILD_VERSION}_linux64.tar.bz2
wget --quiet --retry-connrefused --waitretry=1 https://cef-builds.spotifycdn.com/cef_binary_87.1.14%2Bga29e9a3%2Bchromium-87.0.4280.141_linux64.tar.bz2
tar -xjf ./cef_binary_87.1.14+ga29e9a3+chromium-87.0.4280.141_linux64.tar.bz2
cd cef_binary_87.1.14+ga29e9a3+chromium-87.0.4280.141_linux64
# Rename "tests" directory to avoid error when compiling tests
mv tests tests.renamed
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-stdlib=libc++"
make -j4
cd ../..

# libwebrtc
wget --quiet --retry-connrefused --waitretry=1 --user ${FTP_LOGIN} --password ${FTP_PASSWORD} ${FTP_PATH_PREFIX}/linux/libWebRTC-${LIBWEBRTC_VERSION}-x64-Release-H264-OpenSSL_1_1_1a.sh -O libWebRTC.sh
chmod +x libWebRTC.sh
mkdir libwebrtc
./libWebRTC.sh --prefix="./libwebrtc" --skip-license
