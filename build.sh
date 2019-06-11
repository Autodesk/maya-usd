#!/usr/bin/env bash
export CORE_USD_BUILD_DIRECTORY='path to core usd directory'

export MAYA_USD_BUILD_DIRECTORY=$PWD
echo $MAYA_USD_BUILD_DIRECTORY
if [[ ! -e build ]]; then
    mkdir build
fi
cd $MAYA_USD_BUILD_DIRECTORY/build

cmake .. \
-DCMAKE_BUILD_TYPE=RelWithDebInfo \
-DCMAKE_INSTALL_PREFIX='path to where you want to install the plugins and libraries' \
-DUSD_LOCATION_OVERRIDE=$CORE_USD_BUILD_DIRECTORY \
-DMAYA_LOCATION='path to maya runtime' \
-DBUILD_PXR_USD=TRUE \
-DPXR_BUILD_MAYA_PLUGIN=TRUE \

cmake --build . --target install -j 'core#'
