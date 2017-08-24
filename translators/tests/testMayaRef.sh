#!/bin/bash

export AL_USDMAYA_LOCATION=$1
export MAYA_PLUG_IN_PATH=$AL_USDMAYA_LOCATION/plugin:$MAYA_PLUG_IN_PATH
export PYTHONPATH=$AL_USDMAYA_LOCATION/lib/python:$2/python:$PYTHONPATH

mayapy testMayaRef.py
