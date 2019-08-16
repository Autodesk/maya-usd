#!/bin/bash

export AL_USDMAYA_LOCATION=$1
export USD_LIBRARY_PATH=$2
export MAYA_PLUG_IN_PATH=$AL_USDMAYA_LOCATION/plugin:$MAYA_PLUG_IN_PATH
export LD_LIBRARY_PATH=$AL_USDMAYA_LOCATION/lib:$USD_LIBRARY_PATH:$LD_LIBRARY_PATH
export PYTHONPATH=$AL_USDMAYA_LOCATION/lib/python:$USD_LIBRARY_PATH/python:$3:$PYTHONPATH
export PXR_PLUGINPATH_NAME=$AL_USDMAYA_LOCATION/lib/usd:$PXR_PLUGINPATH_NAME
export PATH=$MAYA_LOCATION/bin:$PATH

maya -batch -command "python(\"execfile(\\\"$3/testProxyShape.py\\\")\nexecfile(\\\"$3/testTranslators.py\\\")\nexecfile(\\\"$3/testLayerManager.py\\\")\")"

exit $?
