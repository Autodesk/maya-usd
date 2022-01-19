# Copyright 2022 Autodesk
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
    registerPluginResource('mayaUsdLib', key, value)

def getMayaUsdLibString(key):
    return getPluginResource('mayaUsdLib', key)

def mayaUsdLibRegisterStrings():
    # Any python strings from MayaUsd lib go here.
    pass

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
