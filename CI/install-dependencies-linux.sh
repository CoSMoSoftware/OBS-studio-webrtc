#!/bin/sh
set -ex

curl -L https://packagecloud.io/github/git-lfs/gpgkey | sudo apt-key add -

sudo apt-get -qq update
sudo apt-get install -y \
        build-essential \
        checkinstall \
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
        libssl-dev

# build cef
wget --quiet --retry-connrefused --waitretry=1 https://cdn-fastly.obsproject.com/downloads/cef_binary_${CEF_BUILD_VERSION}_linux64.tar.bz2
tar -xjf ./cef_binary_${CEF_BUILD_VERSION}_linux64.tar.bz2

# libwebrtc
wget --quiet --retry-connrefused --waitretry=1 https://www.palakis.fr/obs/obs-studio-webrtc/libWebRTC-${LIBWEBRTC_VERSION}-x64-Release-Community.sh -O libWebRTC.sh
chmod +x libWebRTC.sh
mkdir libwebrtc
./libWebRTC.sh --prefix="./libwebrtc" --skip-license
