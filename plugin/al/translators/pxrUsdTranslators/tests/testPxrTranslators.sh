#!/bin/bash

export AL_USDMAYA_LOCATION=$1
export USD_LIBRARY_PATH=$2
export MAYA_PLUG_IN_PATH=$AL_USDMAYA_LOCATION/plugin:$4/maya/plugin:$MAYA_PLUG_IN_PATH
export LD_LIBRARY_PATH=$AL_USDMAYA_LOCATION/lib:$USD_LIBRARY_PATH:$LD_LIBRARY_PATH
export PYTHONPATH=$AL_USDMAYA_LOCATION/lib/python:$USD_LIBRARY_PATH/python:$3:$PYTHONPATH
export PXR_PLUGINPATH_NAME=$AL_USDMAYA_LOCATION/plugin:$PXR_PLUGINPATH_NAME
export PATH=$MAYA_LOCATION/bin:$PATH
export TEST_DIR=$3

# if usd_maya installed in separate location than usd, need to make sure it
# comes BEFORE usd's python on the path, so it's "pxr" pacakge is picked up
# first
if [[ -n "${USD_MAYA_ROOT}" ]]; then
	export PYTHONPATH="${USD_MAYA_ROOT}/lib/python:${PYTHONPATH}"
fi

maya -batch -command "python(\"execfile(\\\"$3/testPxrTranslators.py\\\")\")"

exit $?
