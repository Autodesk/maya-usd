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
import collections

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

    TextField = collections.namedtuple('TextField', ['layout', 'field', 'button'])

    def __init__(self, item, prim, useNiceName):
        super(MaterialCustomControl, self).__init__()
        self.item = item
        self.prim = prim
        self.useNiceName = useNiceName

    def onCreate(self, *args):
        '''
        Create the custom UI for the material.
        '''
        # Note: icon image taken from LookdevX plugin.
        hasLookdevX = self._hasLookdevX()
        graphIcon = 'LookdevX.png' if hasLookdevX else None

        self.assignedMat = self._createTextField('material',  'kLabelAssignedMaterial', graphIcon)
        self.assignedMatMenu = self._createGraphMenu(self.assignedMat.button)

        self.strengthMenu = self._createDropDownField(
            'strength', 'kLabelMaterialStrength',
            ['kLabelWeakerMaterial', 'kLabelStrongerMaterial'])
                
        self.inheritedMat = self._createTextField('inherited', 'kLabelInheritedMaterial', graphIcon)
        self.inheritedMatMenu = self._createGraphMenu(self.inheritedMat.button)
        
        # Note: icon image taken from Maya resources.
        self.fromPrim = self._createTextField('from prim', 'kLabelInheritedFromPrim', 'inArrow.png')

        # Fill the UI.
        self._fillUI()

    @staticmethod
    def _hasLookdevX():
        '''
        Verify if the LookdevX plugin is loaded.
        '''
        return bool(cmds.pluginInfo('LookdevXMaya', query=True, loaded=True))

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
        return MaterialCustomControl.TextField(rowLayout, textField, button)

    def _createGraphMenu(self, button):
        '''
        Create a popup menu attached to the given button to graph a material.
        '''
        if not button:
            return None
        
        return cmds.popupMenu(parent=button, button=True)
    
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

    def _fillUI(self):
        '''
        Fill the UI with the material data.
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
            self._fillUIForDirect(mat, strength)
        else:
            self._fillUIForInherited(mat, matRel, directBinding.GetMaterialPath(), strength)

    def _fillUIForDirect(self, mat, strength):
        '''
        Fill the UI when the material is directly on the prim.
        '''
        # Note: hide UI elements before filling values.
        cmds.rowLayout(self.inheritedMat.layout, edit=True, visible=False)
        cmds.rowLayout(self.fromPrim.layout, edit=True, visible=False)

        matPathStr = mat.GetPath().pathString

        self._fillGraphDirectButton(matPathStr)
        self._fillGraphInheritedButton(None)
        self._fillGotoPrimButton(None)
        self._fillUIValues(matPathStr, '', '', strength)

    def _fillUIForInherited(self, mat, matRel, directMatPath, strength):
        '''
        Fill the UI when the material is inherited from an ancestor prim.
        '''
        directPathStr = directMatPath.pathString if directMatPath else ''
        inheritedPathStr = mat.GetPath().pathString
        fromPathStr = matRel.GetPrim().GetPath().pathString

        # Note: fill values before showing UI elements.
        self._fillGraphDirectButton(directPathStr)
        self._fillGraphInheritedButton(inheritedPathStr)
        self._fillGotoPrimButton(fromPathStr)
        self._fillUIValues(directPathStr, inheritedPathStr, fromPathStr, strength)

        cmds.rowLayout(self.inheritedMat.layout, edit=True, visible=True)
        cmds.rowLayout(self.fromPrim.layout, edit=True, visible=True)

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

        cmds.textField(self.assignedMat.field, edit=True, text=text, placeholderText=placeholder,
                       annotation=annotation)
        
        cmds.textField(self.inheritedMat.field, edit=True, text=inherited)
        cmds.textField(self.fromPrim.field, edit=True, text=fromPath)

        cmds.optionMenuGrp(self.strengthMenu, edit=True,
                           enable=strengthEnabled, visible=strengthVisible,
                           value=strength, annotation=strengthAnnotation)
        
    def _fillGraphDirectButton(self, matPathStr):
        '''
        Fill the direct material graph button with the correct command.
        '''
        self._fillGraphButton(matPathStr, self.assignedMat.button, self.assignedMatMenu)

    def _fillGraphInheritedButton(self, matPathStr):
        '''
        Fill the inherited material graph button with the correct command.
        '''
        self._fillGraphButton(matPathStr, self.inheritedMat.button, self.inheritedMatMenu)

    def _fillGraphButton(self, matPathStr, button, menu):
        '''
        Fill the graph button with the correct command.
        '''
        # Note: only show the graph button if LookdevX was loaded when the UI
        #       was created.
        if not button:
            return

        # Note: only show the graph button if LookdevX is currently loaded.
        hasLookdevX = self._hasLookdevX()
        canGraph = bool(matPathStr and hasLookdevX)
        cmds.symbolButton(button, edit=True, enable=canGraph, visible=hasLookdevX)

        if canGraph:
            ufePathStr = self._createUFEPathFromUSDPath(matPathStr)
            command = partial(MaterialCustomControl._fillGraphMenu, menu=menu, ufePathStr=ufePathStr)
        else:
            command = ''
        cmds.popupMenu(menu, edit=True, postMenuCommand=command)

    # Note: the ignore1 and ignore2 parameters are the values provided by the MEL callback
    #       specifying the menu and item being selected that we don't care about.
    @staticmethod
    def _fillGraphMenu(ignore1, ignore2, menu, ufePathStr):
        '''
        Fill the popup menu with menu items for graphing materials when it gets shown to the user.
        '''
        if not menu:
            return
        if not ufePathStr:
            return
        
        cmds.popupMenu(menu, edit=True, deleteAllItems=True)

        # Allow opening a new tab.
        command = partial(MaterialCustomControl._showInNewTab, ufePathStr=ufePathStr)
        cmds.menuItem(parent=menu, label=getMayaUsdLibString('kLabelMaterialNewTab'), command=command)

        # Allow graphing in an existing tab.
        # Requires intimate knowledge of the LookdevX window name...
        lookdevXWindow = 'LookdevXGraphEditorControl'
        if cmds.window(lookdevXWindow, exists=True):
            tabNames = cmds.lxCompoundEditor(name=lookdevXWindow, tabNames=True, dataModel='USD')
        else:
            tabNames = cmds.lxCompoundEditor(tabNames=True, dataModel='USD')

        if tabNames:
            cmds.menuItem(parent=menu, divider=True)
            for tabName in tabNames:
                command = partial(MaterialCustomControl._showInExistingTab, tabName=tabName, ufePathStr=ufePathStr)
                cmds.menuItem(parent=menu, label=tabName, command=command)

    # Note: the ignore parameter is the value provided by the MEL callback
    #       specifying the item being selected that we don't care about.
    @staticmethod
    def _showInNewTab(ignore, ufePathStr):
        '''
        Show a material in a new LookdevX tab.
        '''
        if not ufePathStr:
            return
        tabName = cmds.lookdevXGraph(openNewTab='USD')
        MaterialCustomControl._showInExistingTab(ignore, tabName=tabName, ufePathStr=ufePathStr)

    # Note: the ignore parameter is the value provided by the MEL callback
    #       specifying the item being selected that we don't care about.
    @staticmethod
    def _showInExistingTab(ignore, tabName, ufePathStr):
        '''
        Show a material in a given existing LookdevX tab.
        '''
        if not tabName:
            return
        if not ufePathStr:
            return
        cmds.lookdevXGraph(tabName=tabName, graphObject=ufePathStr)

    def _fillGotoPrimButton(self, fromPath):
        '''
        Fill the goto-prim button with the correct command.
        '''
        showButton = bool(fromPath)
        cmds.symbolButton(self.fromPrim.button, edit=True, enable=showButton, visible=showButton)

        if fromPath:
            ufePathStr = self._createUFEPathFromUSDPath(fromPath)
            melCommand = 'updateAE "%s"' % ufePathStr
            command = lambda *_: mel.eval(melCommand)
        else:
            command = ''
        cmds.button(self.fromPrim.button, edit=True, command=command)

    def _createUFEPathFromUSDPath(self, usdPath):
        '''
        Build a UFE path to a USD path on the same stage as the edited UFE item.
        '''
        # Note: the UFE item is in the same stage, so we can use
        #       this UFE item to build the UFE path to the USD path.
        ufePath = ufe.Path([
            self.item.path().segments[0],
            ufe.PathSegment(usdPath, mayaUsd.ufe.getUsdRunTimeId(), '/')])
        return ufe.PathString.string(ufePath)

