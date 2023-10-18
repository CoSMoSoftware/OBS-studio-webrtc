log () {
  echo "#########################"
  echo "$1"
  echo "#########################"
}
du -sh ${GITHUB_WORKSPACE}/../*

log "Build Ubuntu ${UBUNTU_VERSION} no-ndi"
${GITHUB_WORKSPACE}/CI/linux/02_build_obs.sh --disable-pipewire --vendor ${VENDOR}

log "Create artifact Ubuntu ${UBUNTU_VERSION} no-ndi"
${GITHUB_WORKSPACE}/CI/linux/03_package_obs.sh --vendor ${VENDOR}

log "Prepare obi-ndi plugin"
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
mv *.deb ../../../package_${{ vendor }}
popd
cd build
rm CMakeCache.txt
cd ..

log "Build Ubuntu ${UBUNTU_VERSION} ndi"
${GITHUB_WORKSPACE}/CI/linux/02_build_obs.sh --disable-pipewire --vendor ${VENDOR} --ndi

log "Create artifact Ubuntu ${UBUNTU_VERSION} no-ndi"
${GITHUB_WORKSPACE}/CI/linux/03_package_obs.sh --vendor ${VENDOR}

log "Done"
