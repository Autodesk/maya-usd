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
    # This function is called from the equivalent MEL proc
    # with the same name. The strings registered here and all the
    # ones registered from the MEL proc can be used in either
    # MEL or python.

    # Any python strings from MayaUsd lib go here.

    # mayaUsdAddMayaReference.py
    register('kErrorGroupPrimExists', 'Group prim "^1s" already exists under "^2s". Choose prim name other than "^1s" to proceed.')
    register('kErrorCannotAddToProxyShape', 'Cannot add Maya Reference node to ProxyShape with Variant Set unless grouped. Enable Group checkbox to proceed.')
    register('kErrorMayaRefPrimExists', 'Maya Reference prim "^1s" already exists under "^2s". Choose Maya Reference prim name other than "^1s" to proceed.')
    register('kErrorCreatingGroupPrim', 'Cannot create group prim under "^1s". Ensure target layer is editable and "^2s" can be added to "^1s".')
    register('kErrorCreatingMayaRefPrim', 'Cannot create MayaReference prim under "^1s". Ensure target layer is editable and "^2s" can be added to "^1s".')
    register('kErrorCreateVariantSet', 'Cannot create Variant Set on prim at path "^1s". Ensure target layer is editable and "^2s" can be added to "^3s".')

    # mayaUsdCacheMayaReference.py
    register('kButtonNewChildPrim', 'New Child Prim')
    register('kButtonNewChildPrimToolTip', 'If selected, your Maya reference will be defined in a new child prim. This will enable\nyou to work with your Maya reference and its USD cache side-by-side.')
    register('kCacheFileWillAppear', 'Cache file will appear\non parent prim:')
    register('kCacheMayaRefCache', 'Cache')
    register('kCacheMayaRefOptions', 'Cache File Options')
    register('kCacheMayaRefUsdHierarchy', 'Author Cache File to USD Hierarchy')
    register('kCaptionCacheToUsd', 'Cache to USD')
    register('kErrorCacheToUsdFailed', 'Cache to USD failed for "^1s".')
    register('kMenuAppend', 'Append')
    register('kMenuPayload', 'Payload')
    register('kMenuPrepend', 'Prepend')
    register('kMenuReference', 'Reference')
    register('kOptionAsUSDReference', 'As USD Reference:')
    register('kOptionAsUSDReferenceToolTip', '<p>Choose the type of USD Reference composition arc for your Maya Reference:<br><br><b>Payloads</b> are a type of reference. They are recorded, but not traversed in the scene hierarchy. Select this arc if your goal is to manually construct<br>a "working set" that is a subset of an entire scene, in which only parts of the scene are required/loaded. Note: payloads are<br>weaker than direct references in any given LayerStack.<br><br><b>References</b> are general and can be used to compose smaller units of scene description into larger aggregates, building up a namespace that<br>includes the "encapsulated" result of composing the scene description targeted by a reference. Select this arc if your goal is not to unload your<br>references.</p>')
    register('kOptionListEditedAs', 'List Edited As')
    register('kTextDefineIn', 'Define in:')
    register('kTextVariant', 'Variant')
    register('kTextVariantToolTip','If selected, your Maya reference will be defined in a variant. This will enable your prim to\nhave 2 variants you can switch between in the Outliner; the Maya reference and its USD cache.')

    # mayaUsdMergeToUSDOptions.py
    register('kMergeToUSDOptionsTitle', 'Merge Maya Edits to USD Options')
    register('kMergeButton', 'Merge')
    register('kApplyButton', 'Apply')
    register('kCloseButton', 'Close')
    register('kEditMenu', 'Edit')
    register('kSaveSettingsMenuItem', 'Save Settings')
    register('kResetSettingsMenuItem', 'Reset Settings')
    register('kHelpMenu', 'Help')
    register('kHelpMergeToUSDOptionsMenuItem', 'Help on Merge Maya Edits to USD Options')

    # mayaUsdDuplicateAsMayaDataOptions.py
    register('kDuplicateAsMayaDataOptionsTitle', 'Duplicate As Maya Data Options')
    register('kHelpDuplicateAsMayaDataOptionsMenuItem', 'Help on Duplicate As Maya Data Options')

    # mayaUsdMergeToUsd.py
    register('kErrorMergeToUsdMenuItem', 'Could not create menu item for merge to USD')
    register('kMenuCacheToUsd', 'Cache to USD...')
    register('kMenuMergeMayaEdits', 'Merge Maya Edits to USD');

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
