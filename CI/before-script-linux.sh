#!/bin/bash

set -ex
ccache -s || echo "CCache is not available."
mkdir build && cd build
cmake -DBUILD_CAPTIONS=ON -DBUILD_BROWSER=ON -DCEF_ROOT_DIR="../cef_binary_${CEF_BUILD_VERSION}_linux64" ..
