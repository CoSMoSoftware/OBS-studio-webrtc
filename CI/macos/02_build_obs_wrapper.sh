#!/usr/bin/env bash

log () {
  echo "#########################"
  echo "$1"
  echo "#########################"
}

find_artifact () {
  log "Find build artifact..."
  BUILD_ARTIFACT_NAME=$(basename $(find build/. -name $1 | sort -rn | head -1))
  if [[ ! -z ${BUILD_ARTIFACT_NAME} ]];then
    echo "BUILD_ARTIFACT_NAME_$2=${BUILD_ARTIFACT_NAME}" >> $GITHUB_ENV
    log "Build artifact name: ${BUILD_ARTIFACT_NAME}"
  else
    log "Cannot find artifact name."
    exit 1
  fi
}

du -sh ${GITHUB_WORKSPACE}/../*

log "Build MacOS no-ndi"
${GITHUB_WORKSPACE}/CI/macos/02_build_obs.sh --codesign --architecture ${TARGET_ARCH} --vendor ${VENDOR}

log "Create artifact MacOS no-ndi"
${GITHUB_WORKSPACE}/CI/macos/03_package_obs.sh --codesign --notarize --architecture ${TARGET_ARCH} --vendor ${VENDOR}

find_artifact "obs-webrtc-*.dmg" "NO-NDI"

log "Build MacOS ndi"
${GITHUB_WORKSPACE}/CI/macos/02_build_obs.sh --codesign --architecture ${TARGET_ARCH} --vendor ${VENDOR} --ndi

log "Create artifact MacOS ndi"
${GITHUB_WORKSPACE}/CI/macos/03_package_obs.sh --codesign --notarize --architecture ${TARGET_ARCH} --vendor ${VENDOR} --ndi

find_artifact "obs-webrtc-*.dmg" "NO-NDI"

ls -la ${GITHUB_WORKSPACE}/build_${VENDOR}_${TARGET_ARCH}/

log "Done!"
