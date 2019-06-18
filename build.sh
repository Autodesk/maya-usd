#!/usr/bin/env bash

# path to pixar's usd build location 
export USD_LOCATION_PATH=''
# path to maya location
export MAYA_LOCATION_PATH=''
# path to maya devkit
export MAYA_DEVKIT_LOCATION=''
# path to where you want to install the project
export INSTALL_LOCATION=''
# Debug, Release, RelWithDebInfo
export BUILD_TYPE=RelWithDebInfo
# core num
export CORE_NUM=8
# Genrators (Ninja|XCode|Make)
export GENERATOR_NAME=Ninja
# Want flags
export WANT_MAYAUSD_CORE=ON
export WANT_ADSK_PLUGIN=ON
export WANT_PXRUSD_PLUGIN=ON
export WANT_ALUSD_PLUGIN=ON

# ----------------------------------------------------------

export CUR_DIR=$PWD
if [[ ! -e build ]]; then
    mkdir build
fi
cd $CUR_DIR/build

if [[ "$GENERATOR_NAME" == "Ninja" ]]; then
    if [[ "$OSTYPE" == "darwin"* ]]; then
        export platform=mac
    fi
    if [[ "$OSTYPE" == "linux-gnu" ]]; then
        export platform=linux
    fi
    export G="-G Ninja -DCMAKE_MAKE_PROGRAM=$CUR_DIR/bin/$platform/ninja"
    echo $G
elif [[ $GENERATOR_NAME == "XCode" ]]; then
    export G='-G Xcode'
elif [[ $GENERATOR_NAME == "Make" ]]; then
  export G=''
fi

cmake .. $G \
-DCMAKE_BUILD_TYPE=$BUILD_TYPE \
-DCMAKE_INSTALL_PREFIX=$INSTALL_LOCATION \
-DMAYA_LOCATION=$MAYA_LOCATION_PATH \
-DUSD_LOCATION_OVERRIDE=$USD_LOCATION_PATH \
-DUSD_CONFIG_FILE=$USD_LOCATION_PATH/pxrConfig.cmake \
-DBOOST_ROOT=$USD_LOCATION_PATH \
-DMAYA_DEVKIT_LOCATION=$MAYA_DEVKIT_LOCATION \
-DBUILD_MAYAUSD_CORE_LIBRARY=$WANT_MAYAUSD_CORE \
-DBUILD_ADSK_USD_PLUGIN=$WANT_ADSK_PLUGIN \
-DBUILD_PXR_USD_PLUGIN=$WANT_PXRUSD_PLUGIN \
-DBUILD_AL_USD_PLUGIN=$WANT_ALUSD_PLUGIN

cmake --build . --config $BUILD_TYPE --target install -j $CORE_NUM