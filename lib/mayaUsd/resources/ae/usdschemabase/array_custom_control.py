# Copyright 2023 Autodesk
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

from .attribute_custom_control import AttributeCustomControl
from .ae_auto_template import AEUITemplate

import mayaUsd.lib as mayaUsdLib
from mayaUsdLibRegisterStrings import getMayaUsdLibString

import maya.cmds as cmds
import maya.mel as mel
import maya.internal.ufeSupport.attributes as attributes
from maya.common.ui import setClipboardData, LayoutManager, ParentManager
from maya.OpenMaya import MGlobal

from pxr import UsdGeom

try:
    # This helper class was only added recently to Maya.
    hasAEPopupMenu = 'AEPopupMenu' in dir(attributes)
except:
    hasAEPopupMenu = False

nameTxt = 'nameTxt'
attrTypeFld = 'attrTypeFld'
attrValueFld = 'attrValueFld'

class ArrayCustomControl(AttributeCustomControl):

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

    def __init__(self, ufeAttr, prim, attrName, useNiceName):
        super(ArrayCustomControl, self).__init__(ufeAttr, attrName, useNiceName)
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
                    cmds.text(nameTxt, al='right', label=attrLabel, annotation=attr.GetDocumentation())
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


def arrayCustomControlCreator(aeTemplate, c):
    # Note: UsdGeom.Tokens.xformOpOrder is a exception we want it to display normally.
    if c == UsdGeom.Tokens.xformOpOrder:
        return None

    if aeTemplate.isArrayAttribute(c):
        ufeAttr = aeTemplate.attrS.attribute(c)
        return ArrayCustomControl(ufeAttr, aeTemplate.prim, c, aeTemplate.useNiceName)
    else:
        return None

