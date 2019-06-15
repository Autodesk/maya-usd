#!/usr/bin/env bash

# path to core usd directory
export CORE_USD_BUILD_DIRECTORY=''
# path to maya runtime
export MAYA_RUNTIME=''
# path to maya devkit
export MAYA_DEVKIT_LOCATION=''
# path to where you want to install the plugins and libraries
export INSTALL_LOCATION=''
# Debug, Release, RelWithDebInfo
export BUILD_TYPE=''
# core numbers
export CORE_NUM=4

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
-DMAYA_DEVKIT_LOCATION=$MAYA_DEVKIT_LOCATION 


cmake --build . --config $BUILD_TYPE --target install -j $CORE_NUM

