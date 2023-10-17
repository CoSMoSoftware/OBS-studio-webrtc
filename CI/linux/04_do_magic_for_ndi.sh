#!/usr/bin/env bash

# This script was created only for needs of test CICD. Process needs to be evaluated and improved by developers.
if [[ $(lsb_release -a) == *"20.04"* ]];then
  UBUNTU_VERSION=2004
else
  UBUNTU_VERSION=2204
fi

export OBS_VERSION=${OBS_VERSION}_${UBUNTU_VERSION}

pushd plugins/obs-ndi
mkdir BUILD
cd BUILD
cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DLINUX_PORTABLE=OFF -DUBUNTU_VERSION=${UBUNTU_VERSION}
make -j4
cpack
cd ../release
ls -la
ls -la ../../../
mv *.deb ../../../package_${VENDOR}
popd
