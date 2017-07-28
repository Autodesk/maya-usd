#!/bin/bash

export MAYA_PLUG_IN_PATH=$1/plugin:${MAYA_PLUG_IN_PATH}
export PXR_PLUGINPATH=$1/share/usd/plugins:$PXR_PLUGINPATH
export PYTHONPATH=$1/lib/python:$PYTHONPATH

mayapy testMayaRef.py
