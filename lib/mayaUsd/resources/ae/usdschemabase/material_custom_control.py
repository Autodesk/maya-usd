# Copyright 2024 Autodesk
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

from functools import partial

import ufe
import mayaUsd.ufe
from pxr import UsdShade

import maya.mel as mel
import maya.cmds as cmds
import maya.common.ui as mui

from mayaUsdLibRegisterStrings import getMayaUsdLibString

class MaterialCustomControl(object):
    strengthLabels = {
        'weakerThanDescendants'     : getMayaUsdLibString('kLabelWeakerMaterial'),
        'strongerThanDescendants'   : getMayaUsdLibString('kLabelStrongerMaterial'),
    }

    strengthTokens = {
        getMayaUsdLibString('kLabelWeakerMaterial')     : 'weakerThanDescendants',
        getMayaUsdLibString('kLabelStrongerMaterial')   : 'strongerThanDescendants',
    }

    def __init__(self, item, prim, useNiceName):
        super(MaterialCustomControl, self).__init__()
        self.item = item
        self.prim = prim
        self.useNiceName = useNiceName

    def onCreate(self, *args):
        '''
        Create the custom UI for the material.
        '''
        self.assignedMatLayout, self.assignedMatField, _ = self._createTextField(
            'material',  'kLabelAssignedMaterial')

        self.strengthMenu = self._createDropDownField(
            'strength', 'kLabelMaterialStrength',
            ['kLabelWeakerMaterial', 'kLabelStrongerMaterial'])
                
        self.inheritedMatLayout, self.inheritedMatField, _ = self._createTextField(
            'inherited', 'kLabelInheritedMaterial')
        # Note: icon image taken from Maya resources.
        self.fromPrimLayout, self.inheritedFromPrimField, self.gotoPrimButton = self._createTextField(
            'from prim', 'kLabelInheritedFromPrim', 'inArrow.png')
        
        # Fill the UI.
        self._refreshUI()

    def _createTextField(self, longName, uiNameRes, image=None):
        '''
        Create a disabled text field group and an optional image button with the correct label.
        '''
        uiLabel = getMayaUsdLibString(uiNameRes) if self.useNiceName else longName
        rowLayout = cmds.rowLayout(numberOfColumns=3, adjustableColumn3=2)
        with mui.LayoutManager(rowLayout):
            cmds.text(label=uiLabel, annotation=uiLabel)
            textField = cmds.textField(annotation=uiLabel, editable=False, enableKeyboardFocus=True)
            if image:
                button = cmds.symbolButton(enable=False, image=image)
            else:
                button = None
        return (rowLayout, textField, button)
    
    def _createDropDownField(self, longName, uiNameRes, elementsRes):
        '''
        Create a disabled drop-down menu with the given elements.
        '''
        uiLabel = getMayaUsdLibString(uiNameRes) if self.useNiceName else longName
        command = partial(MaterialCustomControl._onStrengthChanged, prim=self.prim, item=self.item)
        menu = cmds.optionMenuGrp(label=uiLabel, cc=command, annotation=uiLabel)
        for eleRes in elementsRes:
            text = getMayaUsdLibString(eleRes)
            cmds.menuItem(label=text)
        return menu

    @staticmethod
    def _onStrengthChanged(value, prim, item):
        '''
        React to change of the strength drop-down choice by updating the direct
        material binding relationship strength.
        '''
        if value not in MaterialCustomControl.strengthTokens:
            return

        matAPI = UsdShade.MaterialBindingAPI(prim)
        directBinding = matAPI.GetDirectBinding()
        directRel = directBinding.GetBindingRel()
        token = MaterialCustomControl.strengthTokens[value]
        UsdShade.MaterialBindingAPI.SetMaterialBindingStrength(directRel, token)
        # Force update of all children in the VP2 delegate.
        cmds.evalDeferred(lambda: ufe.Scene.notify(ufe.ObjectRename(item, item.path())))

    def onReplace(self, *args):
        '''
        Refresh the UI when the time changes.
        '''
        # Nothing needed here since USD data is not time varying. Normally this template
        # is force rebuilt all the time, except in response to time change from Maya. In
        # that case we don't need to update our controls since none will change.
        pass

    def _refreshUI(self):
        '''
        Refresh the UI when the data is modified.
        '''
        matAPI = UsdShade.MaterialBindingAPI(self.prim)
        mat, matRel = matAPI.ComputeBoundMaterial()
        directBinding = matAPI.GetDirectBinding()

        token = 'weakerThanDescendants'
        if directBinding:
            directRel = directBinding.GetBindingRel()
            token = UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(directRel)
        strength = self.strengthLabels[token]

        if matRel.GetPrim() == self.prim:
            self._refreshUIForDirect(mat, strength)
        else:
            self._refreshUIForInherited(mat, matRel, directBinding.GetMaterialPath(), strength)

    def _refreshUIForDirect(self, mat, strength):
        '''
        Refresh the UI when the material is directly on the prim.
        '''
        # Note: hide UI elements before filling values.
        cmds.button(self.gotoPrimButton, command='', e=True)
        cmds.symbolButton(self.gotoPrimButton, edit=True, enable=False, visible=False)
        cmds.rowLayout(self.inheritedMatLayout, edit=True, visible=False)
        cmds.rowLayout(self.fromPrimLayout, edit=True, visible=False)

        self._fillUIValues(mat.GetPath().pathString, '', '', strength)

    def _refreshUIForInherited(self, mat, matRel, directMatPath, strength):
        '''
        Refresh the UI when the material is inherited from an ancestor prim.
        '''
        direct = directMatPath.pathString if directMatPath else ''
        inherited = mat.GetPath().pathString
        fromPath = matRel.GetPrim().GetPath().pathString

        ufePath = ufe.Path([
            self.item.path().segments[0],
            ufe.PathSegment(fromPath, mayaUsd.ufe.getUsdRunTimeId(), '/')])
        ufePathStr = ufe.PathString.string(ufePath)
        melCommand = 'updateAE "%s"' % ufePathStr
        command = lambda *_: mel.eval(melCommand)

        # Note: fill values before showing UI elements.
        self._fillUIValues(direct, inherited, fromPath, strength)

        cmds.button(self.gotoPrimButton, command=command, e=True)
        cmds.symbolButton(self.gotoPrimButton, edit=True, enable=True, visible=True)
        cmds.rowLayout(self.inheritedMatLayout, edit=True, visible=True)
        cmds.rowLayout(self.fromPrimLayout, edit=True, visible=True)

    def _fillUIValues(self, direct, inherited, fromPath, strength):
        '''
        Fill the UI with the given values.
        '''
        if inherited:
            text = ''
            annotation = getMayaUsdLibString('kTooltipInheritingOverDirect' if direct else 'kTooltipInheriting')
            placeholder = direct if direct else getMayaUsdLibString('kLabelInheriting')

            strengthVisible = bool(direct)
            strengthEnabled = False
            strengthAnnotation = getMayaUsdLibString('kTooltipInheritedStrength')
        else:
            text = direct
            annotation = getMayaUsdLibString('kLabelAssignedMaterial')
            placeholder = ''

            strengthVisible = True
            strengthEnabled = True
            strengthAnnotation = getMayaUsdLibString('kLabelMaterialStrength')

        cmds.textField(self.assignedMatField, edit=True, text=text, placeholderText=placeholder,
                       annotation=annotation)
        
        cmds.textField(self.inheritedMatField, edit=True, text=inherited)
        cmds.textField(self.inheritedFromPrimField, edit=True, text=fromPath)

        cmds.optionMenuGrp(self.strengthMenu, edit=True,
                           enable=strengthEnabled, visible=strengthVisible,
                           value=strength, annotation=strengthAnnotation)

