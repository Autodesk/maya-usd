#!/bin/bash

export PXR_WORK_THREAD_LIMIT=-3
export LD_LIBRARY_PATH=$1/lib:$2:$MAYA_LOCATION/lib:$LD_LIBRARY_PATH
export PATH=$MAYA_LOCATION/bin:$PATH
export MAYA_PLUG_IN_PATH=$1/plugin:$MAYA_PLUG_IN_PATH
export PXR_PLUGINPATH=$1/share/usd/plugins:$PXR_PLUGINPATH

maya -batch -script "$3/run_mayaplugin_tests.mel"
