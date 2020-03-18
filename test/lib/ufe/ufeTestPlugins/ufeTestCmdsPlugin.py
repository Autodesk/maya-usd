#!/usr/bin/env python

#
# Copyright 2019 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
Plugin to register commands for UFE tests.
"""

import maya.api.OpenMaya as OpenMaya

from ufeScripts import ufeSelectCmd

ufeTestCmdsVersion = '0.1'

# Using the Maya Python API 2.0.
def maya_useNewAPI():
    pass

commands = [ufeSelectCmd.SelectAppendCmd, ufeSelectCmd.SelectRemoveCmd,
            ufeSelectCmd.SelectClearCmd, ufeSelectCmd.SelectReplaceWithCmd]

def initializePlugin(mobject):
    mplugin = OpenMaya.MFnPlugin(mobject, "Autodesk", ufeTestCmdsVersion, "Any")

    for cmd in commands:
        try:
            mplugin.registerCommand(cmd.kCmdName, cmd.creator) 
        except:
            OpenMaya.MGlobal.displayError('Register failed for %s' % cmd.kCmdName)

def uninitializePlugin(mobject):
    """ Uninitialize all the nodes """
    mplugin = OpenMaya.MFnPlugin(mobject, "Autodesk", ufeTestCmdsVersion, "Any")

    for cmd in commands:
        try:
            mplugin.deregisterCommand(cmd.kCmdName) 
        except:
            OpenMaya.MGlobal.displayError('Unregister failed for %s' % cmd.kCmdName)
