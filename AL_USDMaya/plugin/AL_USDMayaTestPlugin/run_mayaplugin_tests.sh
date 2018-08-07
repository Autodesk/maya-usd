#!/bin/bash

export PXR_WORK_THREAD_LIMIT=-3
export AL_USDMAYA_LOCATION=$1
export LD_LIBRARY_PATH=$MAYA_LOCATION/lib:$AL_USDMAYA_LOCATION/lib:$LD_LIBRARY_PATH
export PATH=$MAYA_LOCATION/bin:$PATH
export MAYA_PLUG_IN_PATH=$AL_USDMAYA_LOCATION/plugin:$MAYA_PLUG_IN_PATH

maya -batch -script "$2/run_mayaplugin_tests.mel"
