#!/usr/bin/env bash
#This script is used to setup the environment so maya can pickup the installed AL_USDMaya plugin
#$1 root installation directory of AL_USDMaya
echo "root:$1"

echo "-Updating PATH-"
export PATH=$1/src:$PATH
echo $PATH

echo "-Updating PYTHONPATH-"
export PYTHONPATH=$1/lib/python:$PYTHONPATH
echo $PYTHONPATH

echo "-Updating LD_LIBRARY_PATH-"
export LD_LIBRARY_PATH=$1/lib:$LD_LIBRARY_PATH
echo $LD_LIBRARY_PATH

echo "-Updating MAYA_PLUG_IN_PATH-"
export MAYA_PLUG_IN_PATH=$1/plugin:$MAYA_PLUG_IN_PATH
echo $MAYA_PLUG_IN_PATH

echo "-Updating MAYA_SCRIPT_PATH-"
export MAYA_SCRIPT_PATH=$1/lib:$1/share/usd/plugins/usdMaya/resources:$MAYA_SCRIPT_PATH
echo $MAYA_SCRIPT_PATH

echo "-Updating PXR_PLUGINPATH-"
export PXR_PLUGINPATH=$1/share/usd/plugins:$PXR_PLUGINPATH
echo $PXR_PLUGINPATH
