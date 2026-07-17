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
from dataclasses import dataclass

import ufe
import mayaUsd.ufe
import maya.internal.ufeSupport.ufeCmdWrapper as ufeCmdWrapper

from pxr import UsdShade

import maya.mel as mel
import maya.cmds as cmds
import maya.common.ui as mui

from mayaUsdLibRegisterStrings import getMayaUsdLibString

@dataclass(slots=True)
class MaterialPurposeUI:
    '''
    Data structure to hold the UI elements for a given material purpose.
    '''
    material: object
    inherited: object
    fromPrim: object


class MaterialCustomControl(object):
    strengthLabels = {
        'weakerThanDescendants'     : getMayaUsdLibString('kLabelWeakerMaterial'),
        'strongerThanDescendants'   : getMayaUsdLibString('kLabelStrongerMaterial'),
    }

    strengthTokens = {
        getMayaUsdLibString('kLabelWeakerMaterial')     : 'weakerThanDescendants',
        getMayaUsdLibString('kLabelStrongerMaterial')   : 'strongerThanDescendants',
    }

    TextField = collections.namedtuple('TextField', ['layout', 'field', 'button', 'graphMenu'])

    @staticmethod
    def hasMaterial(prim):
        if not UsdShade.MaterialBindingAPI.CanApply(prim):
            return False
        matAPI = UsdShade.MaterialBindingAPI(prim)
        for purpose in [UsdShade.Tokens.allPurpose, UsdShade.Tokens.preview, UsdShade.Tokens.full]:
            mat, _ = matAPI.ComputeBoundMaterial(purpose)
            if mat:
                return True
        return False
    
    @staticmethod
    def setUndoLabel(label):
        '''
        This function decorator sets the function metadata so that it has
        a nice label in the Maya undo system and UI.
        '''
        def wrap(func):
            nonBreakSpace = '\xa0'
            func.__module__ = label.replace(' ', nonBreakSpace)
            func.__name__ = ''
            return func
        return wrap

    def __init__(self, item, prim, useNiceName):
        super(MaterialCustomControl, self).__init__()
        self.item = item
        self.prim = prim
        self.useNiceName = useNiceName

    def onCreate(self, *args):
        '''
        Create the custom UI for the material.
        '''

        # Note: we create empty UI instances and fill them afterward because
        #       the UI creation functions need to be called in a specific order
        #       to make the fields appear in the correct order to the user as
        #       specified by the design.
        self.materialPurposeUIs = {
            UsdShade.Tokens.allPurpose : MaterialPurposeUI(None, None, None),
            UsdShade.Tokens.preview    : MaterialPurposeUI(None, None, None),
            UsdShade.Tokens.full       : MaterialPurposeUI(None, None, None),
        }

        self._createMaterialUI(UsdShade.Tokens.allPurpose)
        self._createMaterialUI(UsdShade.Tokens.preview)
        self._createMaterialUI(UsdShade.Tokens.full)

        self._createInheritedUI(UsdShade.Tokens.allPurpose)
        self._createInheritedUI(UsdShade.Tokens.preview)
        self._createInheritedUI(UsdShade.Tokens.full)

        self.strengthMenu = self._createDropDownField(
            'strength', 'kLabelMaterialStrength',
            ['kLabelWeakerMaterial', 'kLabelStrongerMaterial'])
        
        for purpose in [UsdShade.Tokens.allPurpose, UsdShade.Tokens.preview, UsdShade.Tokens.full]:
            textField = self.materialPurposeUIs[purpose].material.field
            self._connectTextFieldChangeCallback(purpose, textField)
                
        # Fill the UI.
        self.refresh()

    @staticmethod
    def _hasLookdevX():
        '''
        Verify if the LookdevX plugin is loaded.
        '''
        return bool(cmds.pluginInfo('LookdevXMaya', query=True, loaded=True))
    
    def _createMaterialUI(self, purpose):
        '''
        Create the UI for a given material purpose.
        '''
        purposeName = purpose.capitalize() if purpose else 'Default'

        # Note: icon image taken from LookdevX plugin.
        hasLookdevX = self._hasLookdevX()
        graphIcon = 'LookdevX.png' if hasLookdevX else None

        purposeUI = self.materialPurposeUIs[purpose]
        purposeUI.material = self._createTextField(
            'material',
            f'kLabel{purposeName}Material',
            f'kAnn{purposeName}Material',
            graphIcon,
            'kAnnShowMaterialInLookdevx', True)

    def _createInheritedUI(self, purpose):
        '''
        Create the UI for a given material purpose.
        '''
        purposeName = purpose.capitalize() if purpose else 'Default'

        # Note: icon image taken from LookdevX plugin.
        hasLookdevX = self._hasLookdevX()
        graphIcon = 'LookdevX.png' if hasLookdevX else None

        purposeUI = self.materialPurposeUIs[purpose]
        purposeUI.inherited = self._createTextField('inherited', f'kLabel{purposeName}InheritedMaterial', image=graphIcon, imageTooltipRes='kAnnShowMaterialInLookdevx', canGraph=True)
        # Note: inArrow.png icon image taken from Maya resources.
        purposeUI.fromPrim = self._createTextField('from prim', f'kLabel{purposeName}InheritedFromPrim', image='inArrow.png')

    def _createTextField(self, longName, uiNameRes, uiTooltipRes=None, image=None, imageTooltipRes=None, canGraph=False):
        '''
        Create a disabled text field group and an optional image button with the correct label.
        '''
        uiLabel = getMayaUsdLibString(uiNameRes) if self.useNiceName else longName
        uiTooltip = getMayaUsdLibString(uiTooltipRes) if uiTooltipRes else uiLabel
        rowLayout = cmds.rowLayout(numberOfColumns=3, adjustableColumn3=2)
        with mui.LayoutManager(rowLayout):
            cmds.text(label=uiLabel, annotation=uiTooltip)
            textField = cmds.textField(annotation=uiTooltip, editable=False, enableKeyboardFocus=True)
            if image:
                imageTooltip = getMayaUsdLibString(imageTooltipRes) if imageTooltipRes else ''
                button = cmds.symbolButton(enable=False, image=image, annotation=imageTooltip)
            else:
                button = None

        if canGraph:
            graphMenu = self._createGraphMenu(button)
        else:
            graphMenu = None

        return MaterialCustomControl.TextField(rowLayout, textField, button, graphMenu)

    def _connectTextFieldChangeCallback(self, purpose, textField):

        @MaterialCustomControl.setUndoLabel(getMayaUsdLibString('kLabelSetMaterialBindingUndo'))
        def callback(value, *args, **kwargs):
            try:
                ufePath = ufe.PathString.string(self.item.path())
                if value:
                    cmd = mayaUsd.ufe.BindMaterialCommand(ufePath, value, purpose)
                else:
                    cmd = mayaUsd.ufe.UnbindMaterialCommand(ufePath, purpose)
                ufeCmdWrapper.execute(cmd)
            except Exception as e:
                print(f'Error executing material command: {e}')
                self.refresh()

        try:
            cmds.textField(textField, edit=True, changeCommand=callback)
        except Exception as e:
            print(f'Error connecting text field change callback: {e}')

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
        @MaterialCustomControl.setUndoLabel(getMayaUsdLibString('kLabelSetMaterialBindingStrengthUndo'))
        def callback(value, *args, **kwargs):
            try:
                if value not in MaterialCustomControl.strengthTokens:
                    return
                
                ufePath = ufe.PathString.string(self.item.path())
                strength = MaterialCustomControl.strengthTokens[value]
                affectAllPurposes = True
                cmd = mayaUsd.ufe.SetMaterialBindingStrengthCommand(ufePath, strength, affectAllPurposes)
                ufeCmdWrapper.execute(cmd)

                # Force update of all children in the VP2 delegate.
                cmds.evalDeferred(lambda: ufe.Scene.notify(ufe.ObjectRename(self.item, self.item.path())))
            except Exception as e:
                print(f'Error executing material command: {e}')
                self.refresh()

        uiLabel = getMayaUsdLibString(uiNameRes) if self.useNiceName else longName
        menu = cmds.optionMenuGrp(label=uiLabel, cc=callback, annotation=uiLabel)
        for eleRes in elementsRes:
            text = getMayaUsdLibString(eleRes)
            cmds.menuItem(label=text)
        return menu

    def onReplace(self, *args):
        '''
        Refresh the UI when the time changes.
        '''
        # Nothing needed here since USD data is not time varying. Normally this template
        # is force rebuilt all the time, except in response to time change from Maya. In
        # that case we don't need to update our controls since none will change.
        pass

    def refresh(self):
        '''
        Fill the UI with the material data.
        '''
        matAPI = UsdShade.MaterialBindingAPI(self.prim)

        for purpose in [UsdShade.Tokens.allPurpose, UsdShade.Tokens.preview, UsdShade.Tokens.full]:
            mat, matRel = matAPI.ComputeBoundMaterial(purpose)
            directBinding = matAPI.GetDirectBinding(purpose)
            self._fillUIForPurpose(purpose, mat, matRel, directBinding)

        _, defaultMatRel = matAPI.ComputeBoundMaterial(UsdShade.Tokens.allPurpose)
        isInherithing = bool(defaultMatRel.GetPrim() != self.prim)

        defaultDirectBinding = matAPI.GetDirectBinding(UsdShade.Tokens.allPurpose)
        self._fillStrengthValue(defaultDirectBinding, isInherithing)

    def _fillStrengthValue(self, directBinding, isInherithing):

        if isInherithing:
            strengthEnabled = False
            strengthVisible = bool(directBinding)
            strengthAnnotation = getMayaUsdLibString('kTooltipInheritedStrength')
        else:
            strengthEnabled = True
            strengthVisible = True
            strengthAnnotation = getMayaUsdLibString('kLabelMaterialStrength')


        if directBinding:
            directRel = directBinding.GetBindingRel()
            bindingStengthToken = UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(directRel)
        else:
            bindingStengthToken = 'weakerThanDescendants'

        strength = self.strengthLabels[bindingStengthToken]

        cmds.optionMenuGrp(self.strengthMenu, edit=True,
                           enable=strengthEnabled, visible=strengthVisible,
                           value=strength, annotation=strengthAnnotation)
        
    def _fillUIForPurpose(self, purpose, mat, matRel, directMat):
        '''
        Fill the UI for a given material purpose.
        '''
        purposeName = purpose.capitalize() if purpose else 'Default'
        purposeUI = self.materialPurposeUIs[purpose]

        matPathStr = mat.GetPath().pathString
        fromPathStr = matRel.GetPrim().GetPath().pathString if matRel else ''
        directMatPathStr = directMat.GetMaterialPath().pathString if directMat else ''

        text = ''
        annotation = ''
        placeholder = ''
        inherited = ''

        if matRel.GetPrim() == self.prim:
            cmds.rowLayout(purposeUI.inherited.layout, edit=True, visible=False)
            cmds.rowLayout(purposeUI.fromPrim.layout, edit=True, visible=False)

            text = directMatPathStr
            annotation = getMayaUsdLibString(f'kLabel{purposeName}Material')
        elif matRel.GetPrim():
            cmds.rowLayout(purposeUI.inherited.layout, edit=True, visible=True)
            cmds.rowLayout(purposeUI.fromPrim.layout, edit=True, visible=True)

            annotation = getMayaUsdLibString('kTooltipInheritingOverDirect' if directMatPathStr else 'kTooltipInheriting')
            placeholder = directMatPathStr if directMatPathStr else getMayaUsdLibString('kLabelInheriting')
            inherited = matPathStr
        else:
            cmds.rowLayout(purposeUI.inherited.layout, edit=True, visible=False)
            cmds.rowLayout(purposeUI.fromPrim.layout, edit=True, visible=False)

        self._fillGraphButton(text, purposeUI.material.button, purposeUI.material.graphMenu)
        self._fillGraphButton(inherited, purposeUI.inherited.button, purposeUI.inherited.graphMenu)
        self._fillGotoPrimButton(purposeUI, fromPathStr)

        cmds.textField(purposeUI.material.field, edit=True, editable=True, text=text, placeholderText=placeholder, annotation=annotation)
        cmds.textField(purposeUI.inherited.field, edit=True, text=inherited)
        cmds.textField(purposeUI.fromPrim.field, edit=True, text=fromPathStr)

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

    def _fillGotoPrimButton(self, purposeUI, fromPath):
        '''
        Fill the goto-prim button with the correct command.
        '''
        showButton = bool(fromPath)
        cmds.symbolButton(purposeUI.fromPrim.button, edit=True, enable=showButton, visible=showButton)

        if fromPath:
            ufePathStr = self._createUFEPathFromUSDPath(fromPath)
            melCommand = 'updateAE "%s"' % ufePathStr
            command = lambda *_: mel.eval(melCommand)
        else:
            command = ''
        cmds.symbolButton(purposeUI.fromPrim.button, edit=True, command=command)

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

