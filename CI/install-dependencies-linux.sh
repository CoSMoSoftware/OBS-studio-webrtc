#!/bin/sh
set -ex

curl -L https://packagecloud.io/github/git-lfs/gpgkey | sudo apt-key add -

sudo apt-get -qq update
sudo apt-get install -y \
        build-essential \
        checkinstall \
        clang \
        cmake \
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
        libva-dev \
        libvlc-dev \
        libx11-dev \
        libx11-xcb-dev \
        libx264-dev \
        libxcb-randr0-dev \
        libxcb-shm0-dev \
        libxcb-xinerama0-dev \
        libxcb-xfixes0-dev \
        libxcomposite-dev \
        libxinerama-dev \
        libmbedtls-dev \
        pkg-config \
        python3-dev \
        qtbase5-dev \
        libqt5svg5-dev \
        swig \
        libssl-dev \
        libgtkglext1-dev \
        libxi-dev

# build cef
export CC=clang
export CXX=clang++
wget --quiet --retry-connrefused --waitretry=1 https://cef-builds.spotifycdn.com/cef_binary_75.1.14%2Bgc81164e%2Bchromium-75.0.3770.100_linux64.tar.bz2
tar -xjf ./cef_binary_75.1.14+gc81164e+chromium-75.0.3770.100_linux64.tar.bz2
cd cef_binary_75.1.14+gc81164e+chromium-75.0.3770.100_linux64
# Rename "tests" directory to avoid error when compiling tests
mv tests tests.renamed
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-stdlib=libc++"
make -j4
cd ../..

# libwebrtc
wget --quiet --retry-connrefused --waitretry=1 https://www.palakis.fr/obs/obs-studio-webrtc/libWebRTC-${LIBWEBRTC_VERSION}-x64-Release-Community.sh -O libWebRTC.sh
chmod +x libWebRTC.sh
mkdir libwebrtc
./libWebRTC.sh --prefix="./libwebrtc" --skip-license
