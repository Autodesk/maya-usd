# Copyright 2021 Autodesk
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

import maya.cmds as cmds
import maya.mel as mel

def register(key, value):
    registerPluginResource('mayaUsdPlugin', key, value)

def getMayaUsdString(key):
    return getPluginResource('mayaUsdPlugin', key)

def mayaUSDRegisterStrings():
    # This function is called from the equivalent MEL proc
    # with the same name. The strings registered here and all the
    # ones registered from the MEL proc can be used in either
    # MEL or python.
    register("kButtonYes", "Yes")
    register("kButtonNo", "No")
    register("kDiscardStageEditsTitle", "Discard Edits on ^1s's Layers")
    register("kDiscardStageEditsLoadMsg", "Are you sure you want to load in a new file as the stage source?\n\nAll edits on your layers in ^1s will be discarded.")
    register("kDiscardStageEditsReloadMsg", "Are you sure you want to reload ^1s as the stage source?\n\nAll edits on your layers (except the session layer) in ^2s will be discarded.")
    register("kLoadUSDFile", "Load USD File")

def registerPluginResource(pluginId, stringId, resourceStr):
    '''See registerPluginResource.mel in Maya.

    Unfortunately there is no equivalent python version of this MEL proc
    so we created our own version of it here.'''

    fullId = 'p_%s.%s' % (pluginId, stringId)
    if cmds.displayString(fullId, exists=True):
        # Issue warning if the string is already registered.
        msgFormat = mel.eval('uiRes("m_registerPluginResource.kNotRegistered")')
        msg = cmds.format(msgFormat, stringArg=(pluginId, stringId))
        cmds.warning(msg)
        # Replace the string's value
        cmds.displayString(fullId, replace=True, value=resourceStr)
    else:
        # Set the string's default value.
        cmds.displayString(fullId, value=resourceStr)

def getPluginResource(pluginId, stringId):
    '''See getPluginResource.mel in Maya.

    Unfortunately there is no equivalent python version of this MEL proc
    so we created our own version of it here.'''

    # Form full id string.
    # Plugin string id's are prefixed with "p_".
    fullId = 'p_%s.%s' % (pluginId, stringId)
    if cmds.displayString(fullId, exists=True):
        dispStr = cmds.displayString(fullId, query=True, value=True)
        return dispStr
    else:
        msgFormat = mel.eval('uiRes("m_getPluginResource.kLookupFailed")')
        msg = cmds.format(msgFormat, stringArg=(pluginId, stringId))
        cmds.error(msg)
