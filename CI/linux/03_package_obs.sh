#!/bin/bash

##############################################################################
# Linux libobs plugin package function
##############################################################################
#
# This script file can be included in build scripts for Linux or run directly
#
##############################################################################

# Halt on errors
set -eE

package_obs() {
    status "Create Linux debian package"
    trap "caught_error 'package app'" ERR

    ensure_dir "${CHECKOUT_DIR}"

    step "Package OBS..."
    cmake --build ${BUILD_DIR} -t package

    DEB_NAME=$(find ${BUILD_DIR} -maxdepth 1 -type f -name "obs*.deb" | sort -rn | head -1)

    if [ "${DEB_NAME}" ]; then
        mkdir package_${VENDOR_NAME}
        mv ${DEB_NAME} package_${VENDOR_NAME}
    else
        error "ERROR No suitable OBS debian package generated"
    fi
}

package-obs-standalone() {
    PRODUCT_NAME="OBS-WebRTC"

    CHECKOUT_DIR="$(git rev-parse --show-toplevel)"
    DEPS_BUILD_DIR="${CHECKOUT_DIR}/../obs-build-dependencies"
    source "${CHECKOUT_DIR}/CI/include/build_support.sh"
    source "${CHECKOUT_DIR}/CI/include/build_support_linux.sh"

    if [ -z "${CI}" ]; then
        step "Fetch OBS tags..."
        git fetch --tags origin
    fi

    GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
    GIT_HASH=$(git rev-parse --short=9 HEAD)
    GIT_TAG=$(git describe --tags --abbrev=0)
    UBUNTU_VERSION=$(lsb_release -sr)

    if [ "${BUILD_FOR_DISTRIBUTION}" = "true" ]; then
        VERSION_STRING="${GIT_TAG}"
    else
        VERSION_STRING="${GIT_TAG}-${GIT_HASH}"
    fi

    if [ "${ENABLE_NDI}" ]; then
        NDI_PLUGIN="-ndi"
    else
        NDI_PLUGIN=""
    fi

    FILE_NAME="obs-webrtc${NDI_PLUGIN}-${VERSION_STRING}-ubuntu-${UBUNTU_VERSION}.deb"
    package_obs
}

print_usage() {
    echo -e "Usage: ${0}\n" \
            "-h, --help                     : Print this help\n" \
            "-q, --quiet                    : Suppress most build process output\n" \
            "-v, --verbose                  : Enable more verbose build process output\n" \
            "--build-dir                    : Specify alternative build directory (default: build)\n" \
            "--vendor                       : Vendor name (default: Millicast)\n" \
            "--ndi                          : Enable plugin obs-ndi (default: off)\n"
}

package-obs-main() {
    if [ -z "${_RUN_OBS_BUILD_SCRIPT}" ]; then
        while true; do
            case "${1}" in
                -h | --help ) print_usage; exit 0 ;;
                -q | --quiet ) export QUIET=TRUE; shift ;;
                -v | --verbose ) export VERBOSE=TRUE; shift ;;
                --build-dir ) BUILD_DIR="${2}"; shift 2 ;;
                --vendor ) VENDOR_NAME="${2}"; shift 2 ;;
                --ndi ) ENABLE_NDI=TRUE; shift ;;
                -- ) shift; break ;;
                * ) break ;;
            esac
        done

        package-obs-standalone
    fi
}

package-obs-main $*
