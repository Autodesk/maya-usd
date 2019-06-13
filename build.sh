#!/usr/bin/env bash

# variables to be set
export CORE_USD_BUILD_DIRECTORY='path to core usd directory'
export MAYA_RUNTIME='path to maya runtime'
export INSTALL_LOCATION='path to where you want to install the plugins and libraries'
export BUILD_TYPE='Debug, Release, RelWithDebInfo'
export CORE_NUM='core#'

export MAYA_USD_BUILD_DIRECTORY=$PWD
if [[ ! -e build ]]; then
    mkdir build
fi
cd $MAYA_USD_BUILD_DIRECTORY/build

cmake .. \
-DCMAKE_BUILD_TYPE=$BUILD_TYPE \
-DCMAKE_INSTALL_PREFIX=$INSTALL_LOCATION \
-DMAYA_LOCATION=$MAYA_RUNTIME \
-DUSD_LOCATION_OVERRIDE=$CORE_USD_BUILD_DIRECTORY \
-DUSD_CONFIG_FILE=$CORE_USD_BUILD_DIRECTORY/pxrConfig.cmake \
-DBOOST_ROOT=$CORE_USD_BUILD_DIRECTORY \
-DPXR_BUILD_MAYA_PLUGIN=FALSE \
-DBUILD_USDMAYA_PXR_TRANSLATORS=FALSE

cmake --build . --config $BUILD_TYPE --target install -j $CORE_NUM

