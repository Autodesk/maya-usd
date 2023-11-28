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
import mayaUsd.ufe as mayaUsdUfe

import maya.cmds as cmds
import maya.mel as mel
from maya.common.ui import LayoutManager

import ufe

nameTxt = 'nameTxt'
attrTypeFld = 'attrTypeFld'
attrValueFld = 'attrValueFld'

class ConnectionsCustomControl(AttributeCustomControl):
    '''
    Custom control for all attributes that have connections.
    '''

    def __init__(self, ufeItem, ufeAttr, prim, attrName, useNiceName):
        super(ConnectionsCustomControl, self).__init__(ufeAttr, attrName, useNiceName)
        self.path = ufeItem.path()
        self.prim = prim

    def onCreate(self, *args):
        frontPath = self.path.popSegment()
        attr = self.prim.GetAttribute(self.attrName)
        attrLabel = self.getUILabel()
        attrType = attr.GetMetadata('typeName')

        singleWidgetWidth = mel.eval('global int $gAttributeEditorTemplateSingleWidgetWidth; $gAttributeEditorTemplateSingleWidgetWidth += 0')
        with AEUITemplate():
            # Because of the way the Maya AE template is defined we use a 5 column setup, even
            # though we only have two fields. We resize the main field and purposely set the
            # adjustable column to 3 (one we don't have a field in). We want the textField to
            # remain at a given width.
            rl = cmds.rowLayout(nc=5, adj=3)
            with LayoutManager(rl):
                cmds.text(nameTxt, al='right', label=attrLabel, annotation=attr.GetDocumentation())
                cmds.textField(attrTypeFld, editable=False, text=attrType, backgroundColor=[0.945, 0.945, 0.647], font='obliqueLabelFont', width=singleWidgetWidth*1.5)

                # Add a menu item for each connection.
                cmds.popupMenu()
                for c in attr.GetConnections():
                    parentPath = c.GetParentPath()
                    primName = parentPath.MakeRelativePath(parentPath.GetParentPath())
                    mLabel = '%s%s...' % (primName, c.elementString)

                    usdSeg = ufe.PathSegment(str(c.GetPrimPath()), mayaUsdUfe.getUsdRunTimeId(), '/')
                    newPath = (frontPath + usdSeg)
                    newPathStr = ufe.PathString.string(newPath)
                    cmds.menuItem(label=mLabel, command=lambda *args: _showEditorForUSDPrim(newPathStr))

    def onReplace(self, *args):
        # We only display the attribute name and type. Neither of these are time
        # varying, so we don't need to implement the replace.
        pass

def _showEditorForUSDPrim(usdPrimPathStr):
    # Simple helper to open the AE on input prim.
    mel.eval('evalDeferred "showEditor(\\\"%s\\\")"' % usdPrimPathStr)


def connectionsCustomControlCreator(aeTemplate, c):
    if aeTemplate.attributeHasConnections(c):
        ufeAttr = aeTemplate.attrS.attribute(c)
        return ConnectionsCustomControl(aeTemplate.item, ufeAttr, aeTemplate.prim, c, aeTemplate.useNiceName)
    else:
        return None
