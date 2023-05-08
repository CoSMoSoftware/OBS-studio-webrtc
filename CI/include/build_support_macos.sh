#!/bin/bash

##############################################################################
# macOS support functions
##############################################################################
#
# This script file can be included in build scripts for macOS.
#
##############################################################################

# Setup build environment
WORKFLOW_CONTENT=$(/bin/cat "${CI_WORKFLOW}")

MACOS_VERSION="$(/usr/bin/sw_vers -productVersion)"
MACOS_MAJOR="$(echo ${MACOS_VERSION} | /usr/bin/cut -d '.' -f 1)"
MACOS_MINOR="$(echo ${MACOS_VERSION} | /usr/bin/cut -d '.' -f 2)"

if [ "${TERM-}" -a -z "${CI}" ]; then
    COLOR_RED=$(/usr/bin/tput setaf 1)
    COLOR_GREEN=$(/usr/bin/tput setaf 2)
    COLOR_BLUE=$(/usr/bin/tput setaf 4)
    COLOR_ORANGE=$(/usr/bin/tput setaf 3)
    COLOR_RESET=$(/usr/bin/tput sgr0)
else
    COLOR_RED=""
    COLOR_GREEN=""
    COLOR_BLUE=""
    COLOR_ORANGE=""
    COLOR_RESET=""
fi

## DEFINE UTILITIES ##
check_macos_version() {
    ARCH="${ARCH:-${CURRENT_ARCH}}"

    case "${ARCH}" in
        x86_64) ;;
        arm64) ;;
        *) caught_error "Unsupported architecture '${ARCH}' provided" ;;
    esac

    step "Check macOS version..."
    MIN_VERSION="11.0"
    MIN_MAJOR=$(echo ${MIN_VERSION} | /usr/bin/cut -d '.' -f 1)
    MIN_MINOR=$(echo ${MIN_VERSION} | /usr/bin/cut -d '.' -f 2)

    if [ "${MACOS_MAJOR}" -lt "11" -a "${MACOS_MINOR}" -lt "${MIN_MINOR}" ]; then
        error "ERROR: Minimum required macOS version is ${MIN_VERSION}, but running on ${MACOS_VERSION}"
    fi

    export CODESIGN_LINKER="ON"
}

install_homebrew_deps() {
    if ! exists brew; then
        caught_error "Homebrew not found - please install Homebrew (https://brew.sh)"
    fi

    brew bundle --file "${CHECKOUT_DIR}/CI/include/Brewfile" ${QUIET:+--quiet}

    check_curl
}

check_curl() {
    if [ "${MACOS_MAJOR}" -lt "11" -a "${MACOS_MINOR}" -lt "15" ]; then
        if [ ! -d /usr/local/opt/curl ]; then
            step "Install Homebrew curl..."
            brew install curl
        fi

        CURLCMD="/usr/local/opt/curl/bin/curl"
    else
        CURLCMD="curl"
    fi

    if [ "${CI}" -o "${QUIET}" ]; then
        export CURLCMD="${CURLCMD} --silent --show-error --location -O"
    else
        export CURLCMD="${CURLCMD} --progress-bar --location --continue-at - -O"
    fi
}

check_archs() {
    step "Check Architecture..."
    ARCH="${ARCH:-${CURRENT_ARCH}}"
    if [ "${ARCH}" = "universal" ]; then
        CMAKE_ARCHS="x86_64;arm64"
    elif [ "${ARCH}" != "x86_64" -a "${ARCH}" != "arm64" ]; then
        caught_error "Unsupported architecture '${ARCH}' provided"
    else
        CMAKE_ARCHS="${ARCH}"
    fi
}

_add_ccache_to_path() {
    if [ "${CMAKE_CCACHE_OPTIONS}" ]; then
        if [ "${CURRENT_ARCH}" == "arm64" ]; then
            PATH="/opt/homebrew/opt/ccache/libexec:${PATH}"
        else
            PATH="/usr/local/opt/ccache/libexec:${PATH}"
        fi
        status "Compiler Info:"
        local IFS=$'\n'
        for COMPILER_INFO in $(type cc c++ gcc g++ clang clang++ || true); do
            info "${COMPILER_INFO}"
        done
    fi
}

## SET UP CODE SIGNING AND NOTARIZATION CREDENTIALS ##
##############################################################################
# Apple Developer Identity needed:
#
#    + Signing the code requires a developer identity in the system's keychain
#    + codesign will look up and find the identity automatically
#
##############################################################################
read_codesign_ident() {
    if [ -z "${CODESIGN_IDENT}" ]; then
        step "Set up code signing..."
        read -p "${COLOR_ORANGE}  + Apple developer identity: ${COLOR_RESET}" CODESIGN_IDENT
    fi
    CODESIGN_IDENT_SHORT=$(echo "${CODESIGN_IDENT}" | /usr/bin/sed -En "s/.+\((.+)\)/\1/p")
}

##############################################################################
# Apple Developer credentials necessary:
#
#   + Signing for distribution and notarization require an active Apple
#     Developer membership
#   + An Apple Development identity is needed for code signing
#     (i.e. 'Apple Development: YOUR APPLE ID (PROVIDER)')
#   + Your Apple developer ID is needed for notarization
#   + An app-specific password is necessary for notarization from CLI
#   + This password will be stored in your macOS keychain under the identifier
#     'OBS-Codesign-Password' with access Apple's 'notarytool' only.
##############################################################################

read_codesign_pass() {
    step "Set up notarization..."

    if [ -z "${CODESIGN_IDENT_USER}" ]; then
        read -p "${COLOR_ORANGE}  + Apple account id: ${COLOR_RESET}" CODESIGN_IDENT_USER
    fi

    if [ -z "${CODESIGN_IDENT_PASS}" ]; then
        CODESIGN_IDENT_PASS=$(stty -echo; read -p "${COLOR_ORANGE}  + Apple developer password: ${COLOR_RESET}" secret; stty echo; echo $secret)
        echo ""
    fi

    step "Update notarization keychain..."

    echo -n "${COLOR_ORANGE}"
    /usr/bin/xcrun notarytool store-credentials "OBS-Codesign-Password" --apple-id "${CODESIGN_IDENT_USER}" --team-id "${CODESIGN_IDENT_SHORT}" --password "${CODESIGN_IDENT_PASS}"
    echo -n "${COLOR_RESET}"
}
