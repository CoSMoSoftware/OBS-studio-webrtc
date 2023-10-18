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

log "Build Ubuntu ${UBUNTU_VERSION} no-ndi"
${GITHUB_WORKSPACE}/CI/linux/02_build_obs.sh --disable-pipewire --vendor ${VENDOR}

log "Create artifact Ubuntu${UBUNTU_VERSION} no-ndi"
${GITHUB_WORKSPACE}/CI/linux/03_package_obs.sh --vendor ${VENDOR}

find_artifact "obs-webrtc-*.deb" "NO-NDI"

log "Prepare obi-ndi plugin"
# It needs to be checked what exactly is this for to separate builds for flavours no-ndi and ndi in CI matrix.
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=../install
cmake --install .
cd ..
pushd plugins/obs-ndi
mkdir BUILD
cd BUILD
cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DLINUX_PORTABLE=OFF -DUBUNTU_VERSION=${UBUNTU_VERSION}
make -j4
cpack
cd ../release
mv *.deb ../../../package_${VENDOR}
popd
cd build
rm CMakeCache.txt
cd ..

log "Build Ubuntu ${UBUNTU_VERSION} ndi"
${GITHUB_WORKSPACE}/CI/linux/02_build_obs.sh --disable-pipewire --vendor ${VENDOR} --ndi

log "Create artifact Ubuntu${UBUNTU_VERSION} no-ndi"
${GITHUB_WORKSPACE}/CI/linux/03_package_obs.sh --vendor ${VENDOR}

find_artifact "obs-webrtc-ndi*.deb" "NDI"

ls -la ${GITHUB_WORKSPACE}/build/

log "Done!"
