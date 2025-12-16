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

from .attributeCustomControl import AttributeCustomControl
from .attributeCustomControl import cleanAndFormatTooltip

import ufe
import mayaUsd.ufe as mayaUsdUfe
import mayaUsd.lib as mayaUsdLib
import maya.cmds as cmds
import maya.internal.ufeSupport.attributes as attributes

class EnumCustomControl(AttributeCustomControl):

    @staticmethod
    def creator(aeTemplate, attrName, label=None):
        ufeAttrType = aeTemplate.attrs.attributeType(attrName)
        ufeAttr = aeTemplate.attrs.attribute(attrName)
        ufeAttrs = ufe.Attributes.attributes(ufeAttr.sceneItem())
        enums = ufeAttrs.getEnums(ufeAttr.name)
        # For now only integer enums are supported.
        if ufeAttrType == ufe.Attribute.kInt and len(enums) > 0:
            return EnumCustomControl(ufeAttr, ufeAttrType, aeTemplate.prim, attrName, aeTemplate.useNiceName, label=label)
        else:
            return None


    def __init__(self, ufeAttr, ufeAttrType, prim, attrName, useNiceName, label=None):
        super(EnumCustomControl, self).__init__(ufeAttr, attrName, useNiceName, label=label)
        self.prim = prim
        self.ufeAttrType = ufeAttrType
        ufeAttrs = ufe.Attributes.attributes(ufeAttr.sceneItem())
        self.enums = ufeAttrs.getEnums(ufeAttr.name)

    def onCreate(self, *args):
        # Create the control.
        attrLabel = self.getUILabel()
        self.uiControl = cmds.optionMenuGrp(label=attrLabel, annotation=cleanAndFormatTooltip(self.ufeAttr.getDocumentation()))
        attributes.AEPopupMenu(self.uiControl, self.ufeAttr)

        # Add the menu items.
        self.attachMenuItems()

        # Set the value.
        self.updateUi()

        # Attach the callback function
        self.attachCallbacks(self.updateEnumDataReader)

    def onReplace(self, *args):
        if len(self.enums) > 0:
            self.attachMenuItems()
            attrLabel = self.getUILabel()
            cmds.optionMenuGrp(self.uiControl, e=True, label=attrLabel)
        self.updateUi()
        self.attachCallbacks(self.updateEnumDataReader)

    def attachMenuItems(self):
        if len(self.enums) > 0:
            attrValue = self.ufeAttr.get()
            cmds.optionMenuGrp(self.uiControl, edit=True, deleteAllItems=True)
            for enum in self.enums:
                cmds.menuItem(parent=(self.uiControl +'|OptionMenu'), label=enum[0])

    def updateUi(self, *values):
        '''Callback function to update the UI when the data model changes.'''
        if len(self.enums) > 0:
            for enum in self.enums:
                attrValue = self.ufeAttr.get()
                if self.ufeAttrType == ufe.Attribute.kInt:
                    if int(enum[1]) == attrValue:
                        cmds.optionMenuGrp(self.uiControl, edit=True, value=enum[0])
            # Update the appearance and read-only status of control(s).
            isLocked = attributes.isAttributeLocked(self.ufeAttr)
            cmds.optionMenu(self.uiControl+"|OptionMenu", e=True, enable=not isLocked)

    def attachCallbacks(self, changedCommand):
        # Create change callback for UFE and UI value synchronization.
        cb = attributes.createChangeCb(self.updateUi, self.ufeAttr, self.uiControl, updateDataReader=changedCommand)
        cmds.optionMenuGrp(self.uiControl, edit=True, changeCommand=cb)

    def updateEnumDataReader(self, *values, **kwargs):
        ufeAttr = kwargs['ufeAttr']
        ufeAttrs = ufe.Attributes.attributes(ufeAttr.sceneItem())
        enums = ufeAttrs.getEnums(ufeAttr.name)
        if len(enums) > 0:
            for enum in enums:
                if enum[0] == values[0]:
                    if self.ufeAttrType == ufe.Attribute.kInt:
                        return int(enum[1])
        return None

