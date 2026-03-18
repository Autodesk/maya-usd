# Copyright 2025 Autodesk
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
from maya.common.ui import LayoutManager

import mayaUsd.ufe as mayaUsdUfe

from mayaUsdLibRegisterStrings import getMayaUsdLibString

from pxr import Usd

class AssetInfoCustomControl(object):
    '''Custom control for the prim asset info metadata.'''

    def __init__(self, item, prim, useNiceName):
        # In Maya 2022.1 we need to hold onto the Ufe SceneItem to make
        # sure it doesn't go stale. This is not needed in latest Maya.
        super(AssetInfoCustomControl, self).__init__()
        mayaVer = '%s.%s' % (cmds.about(majorVersion=True), cmds.about(minorVersion=True))
        self._item = item if mayaVer == '2022.1' else None
        self._prim = prim
        self._useNiceName = useNiceName
        self._assetInfoFieldUIs = dict()
        self._payloadDepsButton = None

    @staticmethod
    def hasAssetInfo(prim):
        '''
        Check if the given prim has asset info metadata.
        '''
        model = Usd.ModelAPI(prim)
        assetInfoDict  = model.GetAssetInfo()
        for key, value in assetInfoDict.items():
            if value:
                return True
        return False
    
    _labelForFields = {
        'identifier': 'kLabelAssetIdentifier',
        'assetName': 'kLabelAssetName',
        'assetVersion': 'kLabelAssetVersion',
    }

    _ignoredFields = { 'payloadAssetDependencies' }

    _nested = '_nested'

    def _createFieldsForDict(self, dataDict, parentLabel=''):
        '''
        Recursively create UI fields for dictionary data.
        Returns a dictionary structure mirroring the input with UI element names as values.
        '''
        uiElements = dict()
        
        for key, value in dataDict.items():
            if not value:
                continue

            if key in AssetInfoCustomControl._labelForFields:
                label = getMayaUsdLibString(AssetInfoCustomControl._labelForFields[key])
            else:
                label = mayaUsdUfe.prettifyName(key) if self._useNiceName else key
            
            label = f'{parentLabel}: {label}' if parentLabel else label
            
            if isinstance(value, dict):
                # Create a frame layout for dictionary values
                # Recursively create fields for nested dictionary
                nestedFields = self._createFieldsForDict(value, label)
                uiElements[key] = {
                    AssetInfoCustomControl._nested: nestedFields
                }
            else:
                # Create a text field for non-dictionary values
                uiElements[key] = cmds.textFieldGrp(
                    label=label,
                    editable=False,
                    enableKeyboardFocus=True
                )
        
        return uiElements

    def onCreate(self, *args):
        model = Usd.ModelAPI(self._prim)
        assetInfoDict  = model.GetAssetInfo()
        for key, value in assetInfoDict.items():
            if not value:
                continue
            if key in self._ignoredFields:
                continue
            if key in AssetInfoCustomControl._labelForFields:
                label = getMayaUsdLibString(AssetInfoCustomControl._labelForFields[key])
            else:
                label = mayaUsdUfe.prettifyName(key) if self._useNiceName else key

            if isinstance(value, dict):
                nestedFields = self._createFieldsForDict(value, label)
                self._assetInfoFieldUIs[key] = {
                    AssetInfoCustomControl._nested: nestedFields
                }
            else:
                self._assetInfoFieldUIs[key] = cmds.textFieldGrp(label=label, editable=False, enableKeyboardFocus=True)

        deps = model.GetPayloadAssetDependencies()
        if len(deps):
            rl = cmds.rowLayout(nc=5, adj=4)
            with LayoutManager(rl):
                cmds.text(al='right', label=getMayaUsdLibString('kLabelAssetPayloadDeps'))
                command = lambda *_: self._showPayloads()
                self._payloadDepsButton  = cmds.button('assetInfoPayloadDeps', label=getMayaUsdLibString('kLabelViewAssetPayloadDeps'), command=command, enable=False)
        else:
            self._payloadDepsButton = None

        # Update all metadata values. 
        self.refresh()

    def onReplace(self, *args):
        # Nothing needed here since USD data is not time varying. Normally this template
        # is force rebuilt all the time, except in response to time change from Maya. In
        # that case we don't need to update our controls since none will change.
        pass

    def _getAssetInfoValue(self, value):
        if hasattr(value, '__iter__') and not isinstance(value, (str, bytes)):
            value = ', '.join([self._getAssetInfoValue(v) for v in value])
        text = str(value)
        text = text.strip('@')
        return text

    def _updateFieldsForDict(self, uiElements, dataDict):
        '''
        Recursively update UI fields with dictionary data.
        '''
        for key, value in dataDict.items():
            if key not in uiElements:
                continue
            
            uiElement = uiElements[key]
            
            if isinstance(uiElement, dict) and AssetInfoCustomControl._nested in uiElement:
                # This is a nested dictionary, recursively update
                if isinstance(value, dict):
                    self._updateFieldsForDict(uiElement[AssetInfoCustomControl._nested], value)
            elif isinstance(value, dict):
                # Value is a dict but UI element is not structured as nested
                # This shouldn't happen if onCreate worked correctly, but handle it
                pass
            else:
                # Simple field, update the text
                cmds.textFieldGrp(uiElement, edit=True, text=self._getAssetInfoValue(value))

    def refresh(self):
        if not self._prim:
            return

        model = Usd.ModelAPI(self._prim)
        assetInfoDict  = model.GetAssetInfo()
        for key, value in assetInfoDict.items():
            if key in self._assetInfoFieldUIs:
                uiElement = self._assetInfoFieldUIs[key]
                
                if isinstance(uiElement, dict) and AssetInfoCustomControl._nested in uiElement:
                    # This is a nested dictionary structure
                    if isinstance(value, dict):
                        self._updateFieldsForDict(uiElement[AssetInfoCustomControl._nested], value)
                elif isinstance(value, dict):
                    # Value is a dict but UI element is not structured as nested
                    # Skip updating or log a warning
                    pass
                else:
                    # Simple text field
                    cmds.textFieldGrp(uiElement, edit=True, text=self._getAssetInfoValue(value))

        if self._payloadDepsButton:
            deps = model.GetPayloadAssetDependencies()
            cmds.button(self._payloadDepsButton, edit=True, enable=bool(deps))

    _previewWindowId = 'assetInfoCustomControlPreviewWindow'
    
    def _showPayloads(self):
        '''
        Show the payload dependencies in a new window.
        '''
        if not self._prim:
            return
        
        cleanedPrimPath =  str(self._prim.GetPath().StripAllVariantSelections()).replace('/', '_')
        previewWindowId = AssetInfoCustomControl._previewWindowId + cleanedPrimPath

        if cmds.window(previewWindowId, exists=True):
            cmds.showWindow(previewWindowId)
            return
        
        model = Usd.ModelAPI(self._prim)
        deps = model.GetPayloadAssetDependencies()
        count = len(deps)

        if hasattr(cmds, 'mayaDpiSetting'):
            uiScaleFactor = float(cmds.mayaDpiSetting(query=True, realScaleValue=True))
        else:
            uiScaleFactor = 1.0

        cmds.window(
            previewWindowId,
            title=getMayaUsdLibString('kLabelAssetPayloadDeps'),
            widthHeight=(400 * uiScaleFactor, 300 * uiScaleFactor),
            sizeable=True,
            resizeToFitChildren=True)
        
        windowLayout = cmds.setParent(query=True)
        windowFormLayout = cmds.formLayout(parent=windowLayout)

        rowLayout = cmds.rowLayout(nc=2, adj=2)
        with LayoutManager(rowLayout):
            cmds.text(label=self._prim.GetName(), align='left', font='boldLabelFont')
            cmds.text(
                label=getMayaUsdLibString('kLabelPayloadDepsCount' if count > 1 else 'kLabelPayloadDepCount') % count,
                align='right')

        payloadsLabel = cmds.frameLayout(label=getMayaUsdLibString('kLabelAssetPaths'))
        cmds.setParent('..')

        payloadsList = cmds.textScrollList(
            'assetPayloadDepsList',
            allowMultiSelection=True,
            enableKeyboardFocus=True,
            numberOfItems=count,
            numberOfRows=min(10, max(4, count)),
            enable=True)
        for item in deps:
            cmds.textScrollList('assetPayloadDepsList', edit=True, append=item.path)

        closeCommand = lambda *_: cmds.deleteUI(previewWindowId, window=True)
        closeButton = cmds.button(
            label=getMayaUsdLibString('kCloseButton'),
            command=closeCommand,
            height=24*uiScaleFactor,
            width=120*uiScaleFactor,
            enable=True)

        spacing = 8 * uiScaleFactor

        cmds.formLayout(windowFormLayout, edit=True,
            attachForm=[
                (rowLayout,         'top',      spacing),
                (rowLayout,         'left',     spacing),
                (rowLayout,         'right',    spacing),
                
                (payloadsLabel,     'left',     spacing),
                (payloadsLabel,     'right',    spacing),

                (payloadsList,      'left',     spacing),
                (payloadsList,      'right',    spacing),

                (closeButton,       'right',    spacing),
                (closeButton,      'bottom',    spacing),
            ],
            attachControl=[
                (payloadsLabel,     'top',      spacing,   rowLayout),
                (payloadsList,      'top',      0,         payloadsLabel),
                (payloadsList,      'bottom',   spacing*2, closeButton),
            ])

        cmds.showWindow(previewWindowId)