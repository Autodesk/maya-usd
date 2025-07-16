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

def getMayaUsdLibString(key):
    return getPluginResource('mayaUsdLib', key)

def mayaUsdLibRegisterStrings():
    # This function is called from mayaUSDRegisterStrings.
    registerPluginResources('mayaUsdLib', __mayaUsdLibStringResources)

def mayaUsdLibUnregisterStrings():
    # This function is called from mayaUSDUnregisterStrings.
    # Any python strings from MayaUsd lib go here.
    unregisterPluginResources('mayaUsdLib', __mayaUsdLibStringResources)


# Any MEL or Python strings from MayaUsd lib go here.
__mayaUsdLibStringResources = {
    # ae_template.py
    'kKindMetadataAnn': 'Kind is a type of metadata (a pre-loaded string value) used to classify prims in USD. Set the classification value from the dropdown to assign a kind category to a prim. Set a kind value to activate selection by kind.',
    'kActiveMetadataAnn': "If selected, the prim is set to active and contributes to the composition of a stage. If a prim is set to inactive, it doesn't contribute to the composition of a stage (it gets striked out in the Outliner and is deactivated from the Viewport).",
    'kInstanceableMetadataAnn': 'If selected, instanceable is set to true for the prim and the prim is considered a candidate for instancing. If deselected, instanceable is set to false.',
    'kErrorAttributeMustBeArray': '"^1s" must be an array!',
    'kMenuCopyValue': 'Copy Attribute Value',
    'kMenuPrintValue': 'Print to Script Editor',
    'kLabelUnusedTransformAttrs': 'Unused',
    'kLabelMetadata': 'Metadata',
    'kLabelAssetInfo': 'Asset Info',
    'kLabelAssetName': 'Name',
    'kLabelAssetIdentifier': 'Identifier',
    'kLabelAssetVersion': 'Version',
    'kLabelAssetPayloadDeps': 'Payload Dependencies',
    'kLabelViewAssetPayloadDeps': 'View',
    'kLabelAssetPaths': 'Asset Paths',
    'kLabelPayloadDepsCount': '%s dependencies',
    'kLabelPayloadDepCount': '%s dependency',
    'kOpenImage': 'Open',
    'kLabelMaterial': 'Material',
    'kLabelAssignedMaterial': 'Assigned Material',
    'kAnnShowMaterialInLookdevx': 'Show in LookdevX',
    'kLabelInheritedMaterial': 'Inherited Material',
    'kLabelInheritedFromPrim': 'Inherited from Prim',
    'kLabelInheriting': 'inheriting',
    'kTooltipInheritingOverDirect': 'This material is being over-ridden due to the strength setting on an ancestor',
    'kTooltipInheriting': 'This material is inherited from an ancestor',
    'kLabelMaterialStrength': 'Strength',
    'kLabelWeakerMaterial': 'Weaker than descendants',
    'kLabelStrongerMaterial': 'Stronger than descendants',
    'kTooltipInheritedStrength': 'This setting cannot be changed on this prim due to the strength setting on an ancestor',
    'kLabelMaterialNewTab': 'New Tab...',
    'kUseOutlinerColorAnn': 'Apply the Outliner color to the display of the prim name in the Outliner.',
    'kOutlinerColorAnn': 'The color of the text displayed in the Outliner.',
    'kTransforms': 'Transforms',

    # mayaUsdAddMayaReference.py
    'kErrorGroupPrimExists': 'Group prim "^1s" already exists under "^2s". Choose prim name other than "^1s" to proceed.',
    'kErrorCannotAddToProxyShape': 'Cannot add Maya Reference node to ProxyShape with Variant Set unless grouped. Enable Group checkbox to proceed.',
    'kErrorMayaRefPrimExists': 'Maya Reference prim "^1s" already exists under "^2s". Choose Maya Reference prim name other than "^1s" to proceed.',
    'kErrorCreatingGroupPrim': 'Cannot create group prim under "^1s". Ensure target layer is editable and "^2s" can be added to "^1s".',
    'kErrorCreatingMayaRefPrim': 'Cannot create MayaReference prim under "^1s". Ensure target layer is editable and "^2s" can be added to "^1s".',
    'kErrorCreateVariantSet': 'Cannot create Variant Set on prim at path "^1s". Ensure target layer is editable and "^2s" can be added to "^3s".',

    # mayaUsdCacheMayaReference.py
    'kButtonNewChildPrim': 'New Child Prim',
    'kButtonNewChildPrimToolTip': 'If selected, your Maya reference will be defined in a new child prim. This will enable\nyou to work with your Maya reference and its USD cache side-by-side.',
    'kCacheFileWillAppear': 'Cache file will\nappear on parent\nprim:',
    'kCacheMayaRefCache': 'Cache',
    'kCacheMayaRefOptions': 'Cache File Options',
    'kCacheMayaRefUsdHierarchy': 'Author Cache File to USD',
    'kCaptionCacheToUsd': 'Cache to USD',
    'kErrorCacheToUsdFailed': 'Cache to USD failed for "^1s".',
    'kMenuPrepend': 'Prepend',
    'kMenuAppend': 'Append',
    'kMenuPayload': 'Payload',
    'kMenuReference': 'Reference',
    'kOptionAsUSDReference': 'Composition Arc:',
    'kOptionAsUSDReferenceToolTip': '<p>Choose the type of USD Reference composition arc for your Maya Reference:<br><br><b>Payloads</b> are a type of reference. They are recorded, but not traversed in the scene hierarchy. Select this arc if your goal is to manually construct<br>a "working set" that is a subset of an entire scene, in which only parts of the scene are required/loaded. Note: payloads are<br>weaker than direct references in any given LayerStack.<br><br><b>References</b> are general and can be used to compose smaller units of scene description into larger aggregates, building up a namespace that<br>includes the "encapsulated" result of composing the scene description targeted by a reference. Select this arc if your goal is not to unload your<br>references.</p>',
    'kOptionAsUSDReferenceStatusMsg': 'Choose the type of USD Reference composition arc for your Maya Reference.',
    'kOptionListEditedAs': 'List Edited As',
    'kOptionLoadPayload': 'Load Payload:',
    'kLoadPayloadAnnotation': 'If selected, all existing payloads on the prim will be unchanged and new payloads will be loaded as well. When deselected, all payloads on the prim will be unloaded.',
    'kTextDefineIn': 'Define in:',
    'kTextVariant': 'Variant',
    'kTextVariantToolTip': 'If selected, your Maya reference will be defined in a variant. This will enable your prim to\nhave 2 variants you can switch between in the Outliner; the Maya reference and its USD cache.',

    'kAddRefOrPayloadPrimPathToolTip':
        'Leave this field blank to use the default prim as your prim path (only viable if your file has a default prim).\n' +
        'Specifying a prim path will make an explicit reference to a prim.\n' +
        'If there is no default prim and no prim path is specified, no prim will be referenced.',
    
    'kAddRefOrPayloadPrimPathLabel': 'Prim Path',
    'kAddRefOrPayloadPrimPathPlaceHolder': ' (Default Prim)',
    'kAddRefOrPayloadPrimPathHelpLabel': 'Help on Select a Prim for Reference',
    'kAddRefOrPayloadPrimPathTitle': 'Select a Prim to Reference',
    'kAddRefOrPayloadSelectLabel': 'Select',

    # mayaUsdClearRefsOrPayloadsOptions.py
    'kClearRefsOrPayloadsOptionsTitle': 'Clear All USD References/Payloads',
    'kClearRefsOrPayloadsOptionsMessage': 'Clear all references/payloads on %s?',
    'kClearButton': 'Clear',
    'kCancelButton': 'Cancel',
    'kAllRefsLabel': 'All References',
    'kAllRefsTooltip': 'Clear all references on the prim.',
    'kAllPayloadsLabel': 'All Payloads',
    'kAllPayloadsTooltip': 'Clear all payloads on the prim.',

    # mayaUsdMergeToUSDOptions.py
    'kMergeToUSDOptionsTitle': 'Merge Maya Edits to USD Options',
    'kMergeButton': 'Merge',
    'kApplyButton': 'Apply',
    'kCloseButton': 'Close',
    'kEditMenu': 'Edit',
    'kSaveSettingsMenuItem': 'Save Settings',
    'kResetSettingsMenuItem': 'Reset Settings',
    'kHelpMenu': 'Help',
    'kHelpMergeToUSDOptionsMenuItem': 'Help on Merge Maya Edits to USD Options',

    # mayaUsdDuplicateAsUsdDataOptions.py
    'kDuplicateAsUsdDataOptionsTitle': 'Duplicate As USD Data Options',
    'kHelpDuplicateAsUsdDataOptionsMenuItem': 'Help on Duplicate As USD Data Options',

    # mayaUsdMergeToUsd.py
    'kErrorMergeToUsdMenuItem': 'Could not create menu item for merge to USD',
    'kMenuCacheToUsd': 'Cache to USD...',
    'kMenuMergeMayaEdits': 'Merge Maya Edits to USD',

    # mayaUsdStageConversion.py
    "kStageConversionUnknownMethod": "Unknown stage conversion method: %s",
    "kStageConversionSuccessful": "Mismatching axis/unit have been converted for accurate scale.",

    # mayaUsdAddMayaReference.mel
    "kMayaRefDescription": "Description",
    "kMayaRefCacheToUSDDescription1": "<p>Export your Maya Reference to a USD cache file on disk. This workflow enables you to work with Maya data in USD scenes and then export them to a USD cache and author it back into your current USD hierarchy. Use the Cache Options section to build out the scope of your output.</p>",
    "kMayaRefCacheToUSDDescription2": "<p><b>Note:</b> When authoring the exported USD cache back into a current USD hierarchy, an edit will be made on the targeted layer. The cache file will be added as a USD reference, defined either in a variant or prim.</p>",
    "kMayaRefAddToUSDDescription1": "<p>Add a Maya reference to a USD prim to enable working with original Maya data in your USD scene. Select a Maya scene file to add as a reference. Once a Maya reference file is added, a Maya transform node will appear in the Outliner at your selected prim, containing your newly added Maya reference. Use this dialog to build out the scope of your Maya reference.</p>",
    "kMayaRefAddToUSDDescription2": "<p><b>Tip:</b> Define your Maya Reference in a USD variant. This will enable your prim to have 2 variants you can switch between in the Outliner the Maya reference and its USD cache.</p>",
    "kMayaRefUsdOptions": "Author Maya Reference File to USD",
    "kMayaRefMayaRefPrimName": "Maya Reference Prim Name:",
    "kMayaRefGroup": "Group",
    "kMayaRefPrimName": "Prim Name:",
    "kMayaRefPrimType": "Prim Type:",
    "kMayaRefPrimKind": "Prim Kind:",
    "kMayaRefDefineInVariant": "Define in Variant",
    "kMayaRefDefineInVariantAnn": "Select this checkbox to define the Maya Reference in a USD variant. This will enable your prim to have 2 variants you can switch between in the Outliner the Maya reference and its USD cache.",
    "kMayaRefVariantSetName": "Variant Set Name:",
    "kMayaRefVariantName": "Variant Name:",
    "kMayaRefVariantOnPrim": "Variant will be on prim:",
    "kMayaRefEditAsMayaData": "Edit as Maya Data",
    "kMayaRefEditAsMayaDataAnn": "Select this checkbox to enable editing the MayaReference prim as a Maya Reference.",
    "kMayaRefOptions": "Maya Reference Options",
    "kMayaRefCreateNew": "Create New",
    "kMayaRefAddToPrim": "Add Maya Reference to Prim",
    "kMayaRefReference": "Reference",

    # Used in multiple places.
    "kAllUsdFiles": "All USD Files",
    "kUsdFiles": "USD Files",
    "kUsdASCIIFiles": "USD ASCII Files",
    "kUsdBinaryFiles": "USD Binary Files",
    "kUsdCompressedFiles": "USD Compressed Files",

    # readJob.cpp
    "kAboutToChangePrefs": "The USD import is about to change the Maya preferences.\n" +
                           "Save your changes in the preferences window?",
    "kAboutToChangePrefsTitle": "Save Preferences",
    "kSavePrefsChange": "Save Preferences",
    "kDiscardPrefsChange": "Discard Preferences",
}


def registerPluginResources(pluginId, resources):
    '''
    Registers all the given resources for the given plugin ID.
    '''
    for stringId, resourceStr in resources.items():
        registerPluginResource(pluginId, stringId, resourceStr)


def unregisterPluginResources(pluginId, resources):
    '''
    Unregisters all the given resources for the given plugin ID.
    '''
    for stringId, _ in resources.items():
        unregisterPluginResource(pluginId, stringId)


def unregisterPluginResource(pluginId, stringId):
    '''
    Unregisters the given string ID for the given plugin ID.
    '''
    fullId = 'p_%s.%s' % (pluginId, stringId)
    if not cmds.displayString(fullId, exists=True):
        return
    cmds.displayString(fullId, delete=True)


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
