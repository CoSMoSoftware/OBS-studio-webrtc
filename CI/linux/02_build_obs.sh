#!/bin/bash

##############################################################################
# Linux build function
##############################################################################
#
# This script file can be included in build scripts for Linux or run directly
#
##############################################################################

# Halt on errors
set -eE

build_obs() {
    status "Build OBS"
    trap "caught_error 'build app'" ERR
    if [ -z "${CI}" ]; then
        _backup_artifacts
    fi

    step "Configure OBS..."
    _configure_obs

    ensure_dir "${CHECKOUT_DIR}/"
    step "Build OBS targets..."
    cmake --build ${BUILD_DIR}
}

# Function to configure OBS build
_configure_obs() {
    ensure_dir "${CHECKOUT_DIR}"
    status "Configuration of OBS build system..."
    trap "caught_error 'configure build'" ERR
    check_ccache

    if [ "${TWITCH_CLIENTID}" -a "${TWITCH_HASH}" ]; then
        TWITCH_OPTIONS="-DTWITCH_CLIENTID='${TWITCH_CLIENTID}' -DTWITCH_HASH='${TWITCH_HASH}'"
    fi

    if [ "${RESTREAM_CLIENTID}" -a "${RESTREAM_HASH}" ]; then
        RESTREAM_OPTIONS="-DRESTREAM_CLIENTID='${RESTREAM_CLIENTID}' -DRESTREAM_HASH='${RESTREAM_HASH}'"
    fi

    if [ "${YOUTUBE_CLIENTID}" -a "${YOUTUBE_CLIENTID_HASH}" -a "${YOUTUBE_SECRET}" -a "{YOUTUBE_SECRET_HASH}" ]; then
        YOUTUBE_OPTIONS="-DYOUTUBE_CLIENTID='${YOUTUBE_CLIENTID}' -DYOUTUBE_CLIENTID_HASH='${YOUTUBE_CLIENTID_HASH}' -DYOUTUBE_SECRET='${YOUTUBE_SECRET}' -DYOUTUBE_SECRET_HASH='${YOUTUBE_SECRET_HASH}'"
    fi

    if [ "${PORTABLE}" ]; then
        PORTABLE_BUILD="ON"
    fi

    if [ "${DISABLE_PIPEWIRE}" ]; then
        PIPEWIRE_OPTION="-DENABLE_PIPEWIRE=OFF"
    fi

    if [ "$VENDOR_NAME" == "Millicast" ]
    then
        VENDOR_OPTION=""
    else
        VENDOR_OPTION="-DOBS_WEBRTC_VENDOR_NAME=$VENDOR_NAME"
    fi

    libwebrtc_dir=`pwd`/libwebrtc/cmake
    export CC=clang
    export CXX=clang++

    cmake -S . -B ${BUILD_DIR} -G Ninja \
        ${VENDOR_OPTION} \
        -DCEF_ROOT_DIR="${DEPS_BUILD_DIR}/cef_binary_${LINUX_CEF_BUILD_VERSION:-${CI_LINUX_CEF_VERSION}}_linux64" \
        -DCMAKE_BUILD_TYPE=${BUILD_CONFIG} \
        -DLINUX_PORTABLE=${PORTABLE_BUILD:-OFF} \
        -DENABLE_AJA=OFF \
        -DENABLE_NEW_MPEGTS_OUTPUT=OFF \
        ${PIPEWIRE_OPTION} \
        ${YOUTUBE_OPTIONS} \
        ${TWITCH_OPTIONS} \
        ${RESTREAM_OPTIONS} \
        ${CI:+-DENABLE_UNIT_TESTS=ON -DBUILD_FOR_DISTRIBUTION=${BUILD_FOR_DISTRIBUTION} -DOBS_BUILD_NUMBER=${GITHUB_RUN_ID}} \
        ${QUIET:+-Wno-deprecated -Wno-dev --log-level=ERROR} \
        -DBUILD_NDI=ON \
        -DBUILD_WEBSOCKET=ON \
        -DUNIX_STRUCTURE=1 \
        -DENABLE_VLC=ON \
        -DUSE_LIBC++=ON \
        -Dlibwebrtc_DIR=${libwebrtc_dir} \
        -DOBS_VERSION_OVERRIDE=${OBS_VERSION} \
        -DCPACK_DEBIAN_PACKAGE_MAINTAINER="CoSMo Software" \
        -DCPACK_DEBIAN_PACKAGE_NAME="obs" \
        -DCPACK_DEBIAN_PACKAGE_VERSION=${OBS_VERSION} \
        -DCPACK_DEBIAN_PACKAGE_ARCHITECTURE="amd64" \
        -DCPACK_DEBIAN_PACKAGE_DEPENDS="ffmpeg, libqt5gui5, libqt5xml5, libqt5x11extras5, libqt5core5a, libx264-dev, vlc, libavcodec58, libmbedtls12, libfdk-aac1, libavfilter7, libavdevice58, libc++1, libcurl4"
}

# Function to backup previous build artifacts
_backup_artifacts() {
    ensure_dir "${CHECKOUT_DIR}"
    if [ -d ${BUILD_DIR} ]; then
        status "Backup of old OBS build artifacts"

        CUR_DATE=$(date +"%Y-%m-%d@%H%M%S")
        NIGHTLY_DIR="${CHECKOUT_DIR}/nightly-${CUR_DATE}"
        PACKAGE_NAME=$(find ${BUILD_DIR} -maxdepth 1 -name "*.deb" | sort -rn | head -1)

        if [ "${PACKAGE_NAME}" ]; then
            step "Back up $(basename "${PACKAGE_NAME}")..."
            ensure_dir "${NIGHTLY_DIR}"
            mv "../${BUILD_DIR}/$(basename "${PACKAGE_NAME}")" ${NIGHTLY_DIR}/
            info "You can find ${PACKAGE_NAME} in ${NIGHTLY_DIR}"
        fi
    fi
}

build-obs-standalone() {
    CHECKOUT_DIR="$(git rev-parse --show-toplevel)"
    PRODUCT_NAME="OBS-Studio"
    DEPS_BUILD_DIR="${CHECKOUT_DIR}/../obs-build-dependencies"
    source "${CHECKOUT_DIR}/CI/include/build_support.sh"
    source "${CHECKOUT_DIR}/CI/include/build_support_linux.sh"

    build_obs
}

print_usage() {
    echo -e "Usage: ${0}\n" \
            "-h, --help                     : Print this help\n" \
            "-q, --quiet                    : Suppress most build process output\n" \
            "-v, --verbose                  : Enable more verbose build process output\n" \
            "-p, --portable                 : Create portable build (default: off)\n" \
            "--disable-pipewire             : Disable building with PipeWire support (default: off)\n" \
            "--build-dir                    : Specify alternative build directory (default: build)\n"
}

build-obs-main() {
    if [ -z "${_RUN_OBS_BUILD_SCRIPT}" ]; then
        while true; do
            case "${1}" in
                -h | --help ) print_usage; exit 0 ;;
                -q | --quiet ) export QUIET=TRUE; shift ;;
                -v | --verbose ) export VERBOSE=TRUE; shift ;;
                -p | --portable ) export PORTABLE=TRUE; shift ;;
                --disable-pipewire ) DISABLE_PIPEWIRE=TRUE; shift ;;
                --build-dir ) BUILD_DIR="${2}"; shift 2 ;;
                --vendor ) VENDOR_NAME="${2}"; shift 2 ;;
                -- ) shift; break ;;
                * ) break ;;
            esac
        done

        build-obs-standalone
    fi
}

build-obs-main $*