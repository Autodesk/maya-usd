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

from .attributeCustomControl import AttributeCustomControl
from .attributeCustomControl import cleanAndFormatTooltip
from .aeUITemplate import AEUITemplate

import maya.cmds as cmds
import maya.mel as mel
from maya.common.ui import LayoutManager, ParentManager
from maya.common.ui import setClipboardData
from maya.OpenMaya import MGlobal

from mayaUsdLibRegisterStrings import getMayaUsdLibString

import ufe

from pxr import UsdGeom


try:
    # This helper class was only added recently to Maya.
    import maya.internal.ufeSupport.attributes as attributes
    hasAEPopupMenu = 'AEPopupMenu' in dir(attributes)
except:
    hasAEPopupMenu = False


# The name of the UI elements for the name and type of some attributes.
nameTxt = 'nameTxt'
attrTypeFld = 'attrTypeFld'


class ArrayCustomControl(AttributeCustomControl):
    '''Custom control for all array attribute.'''
    if hasAEPopupMenu:
        class ArrayAEPopup(attributes.AEPopupMenu):
            '''Override the attribute AEPopupMenu so we can add extra menu items.
            '''
            def __init__(self, uiControl, ufeAttr, hasValue, values):
                self.hasValue = hasValue
                self.values = values
                super(ArrayCustomControl.ArrayAEPopup, self).__init__(uiControl, ufeAttr)

            def _copyAttributeValue(self):
                setClipboardData(str(self.values))

            def _printToScriptEditor(self):
                MGlobal.displayInfo(str(self.values))

            COPY_ACTION  = (getMayaUsdLibString('kMenuCopyValue'), _copyAttributeValue, [])
            PRINT_ACTION = (getMayaUsdLibString('kMenuPrintValue'), _printToScriptEditor, [])

            HAS_VALUE_MENU = [COPY_ACTION, PRINT_ACTION]

            def _buildMenu(self, addItemCmd):
                super(ArrayCustomControl.ArrayAEPopup, self)._buildMenu(addItemCmd)
                if self.hasValue:
                    cmds.menuItem(divider=True, parent=self.popupMenu)
                    self._buildFromActions(self.HAS_VALUE_MENU, addItemCmd)

    @staticmethod
    def isArrayAttribute(aeTemplate, attrName):
        '''
        Verify if the given attribute name is an array attribute for the prim
        currently being shown in the given AE template.
        '''
        if aeTemplate.attrs.attributeType(attrName) == ufe.Attribute.kGeneric:
            attr = aeTemplate.prim.GetAttribute(attrName)
            typeName = attr.GetTypeName()
            return typeName.isArray
        return False

    @staticmethod
    def creator(aeTemplate, attrName, label=None):
        # Note: UsdGeom.Tokens.xformOpOrder is a exception we want it to display normally.
        if attrName == UsdGeom.Tokens.xformOpOrder:
            return None

        if ArrayCustomControl.isArrayAttribute(aeTemplate, attrName):
            ufeAttr = aeTemplate.attrs.attribute(attrName)
            return ArrayCustomControl(ufeAttr, aeTemplate.prim, attrName, aeTemplate.useNiceName, label=label)
        else:
            return None

    def __init__(self, ufeAttr, prim, attrName, useNiceName, label=None):
        super(ArrayCustomControl, self).__init__(ufeAttr, attrName, useNiceName, label=label)
        self.prim = prim

    def onCreate(self, *args):
        attr = self.prim.GetAttribute(self.attrName)
        typeName = attr.GetTypeName()
        if typeName.isArray:
            values = attr.Get()
            hasValue = True if values and len(values) > 0 else False

            # build the array type string
            # We want something like int[size] or int[] if empty
            typeNameStr = str(typeName.scalarType)
            typeNameStr += ("[" + str(len(values)) + "]") if hasValue else "[]"

            attrLabel = self.getUILabel()
            singleWidgetWidth = mel.eval('global int $gAttributeEditorTemplateSingleWidgetWidth; $gAttributeEditorTemplateSingleWidgetWidth += 0')
            with AEUITemplate():
                # See comment in ConnectionsCustomControl below for why nc=5.
                rl = cmds.rowLayout(nc=5, adj=3)
                with LayoutManager(rl):
                    cmds.text(nameTxt, al='right', label=attrLabel, annotation=cleanAndFormatTooltip(attr.GetDocumentation()))
                    cmds.textField(attrTypeFld, editable=False, text=typeNameStr, font='obliqueLabelFont', width=singleWidgetWidth*1.5)

                if hasAEPopupMenu:
                    pMenu = self.ArrayAEPopup(rl, self.ufeAttr, hasValue, values)
                    self.updateUi(self.ufeAttr, rl)
                    self.attachCallbacks(self.ufeAttr, rl, None)
                else:
                    if hasValue:
                        cmds.popupMenu()
                        cmds.menuItem( label=getMayaUsdLibString('kMenuCopyValue'), command=lambda *args: setClipboardData(str(values)) )
                        cmds.menuItem( label=getMayaUsdLibString('kMenuPrintValue'), command=lambda *args: MGlobal.displayInfo(str(values)) )

        else:
            errorMsgFormat = getMayaUsdLibString('kErrorAttributeMustBeArray')
            errorMsg = cmds.format(errorMsgFormat, stringArg=(self.attrName))
            cmds.error(errorMsg)

    def onReplace(self, *args):
        pass

    # Only used when hasAEPopupMenu is True.
    def updateUi(self, attr, uiControlName):
        if not hasAEPopupMenu:
            return

        with ParentManager(uiControlName):
            bgClr = attributes.getAttributeColorRGB(self.ufeAttr)
            if bgClr:
                isLocked = attributes.isAttributeLocked(self.ufeAttr)
                cmds.textField(attrTypeFld, edit=True, backgroundColor=bgClr)

    # Only used when hasAEPopupMenu is True.
    def attachCallbacks(self, ufeAttr, uiControl, changedCommand):
        if not hasAEPopupMenu:
            return

        # Create change callback for UFE locked/unlock synchronization.
        cb = attributes.createChangeCb(self.updateUi, ufeAttr, uiControl)
        cmds.textField(attrTypeFld, edit=True, parent=uiControl, changeCommand=cb)
