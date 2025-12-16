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
from maya.common.ui import LayoutManager

import mayaUsd.ufe as mayaUsdUfe

import ufe

from pxr import UsdShade


# The name of the UI elements for the name and type of some attributes.
nameTxt = 'nameTxt'
attrTypeFld = 'attrTypeFld'


class ConnectionsCustomControl(AttributeCustomControl):
    '''Custom control for all attributes that have connections.'''

    @staticmethod
    def isAttributeConnected(aeTemplate, attrName):
        '''
        Verify if the given attribute name is connected for the prim
        currently being shown in the given AE template.
        '''
        attr = aeTemplate.prim.GetAttribute(attrName)
        return attr.HasAuthoredConnections() if attr else False

    @staticmethod
    def creator(aeTemplate, attrName, label=None):
        if ConnectionsCustomControl.isAttributeConnected(aeTemplate, attrName):
            ufeAttr = aeTemplate.attrs.attribute(attrName)
            return ConnectionsCustomControl(aeTemplate.item, ufeAttr, aeTemplate.prim, attrName, aeTemplate.useNiceName, label=label)
        else:
            return None

    def __init__(self, ufeItem, ufeAttr, prim, attrName, useNiceName, label=None):
        super(ConnectionsCustomControl, self).__init__(ufeAttr, attrName, useNiceName, label=label)
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
                cmds.text(nameTxt, al='right', label=attrLabel, annotation=cleanAndFormatTooltip(attr.GetDocumentation()))
                cmds.textField(attrTypeFld, editable=False, text=attrType, backgroundColor=[0.945, 0.945, 0.647], font='obliqueLabelFont', width=singleWidgetWidth*1.5)

                # Add a menu item for each connection.
                cmds.popupMenu()
                for mLabel, newPathStr in self.gatherConnections(attr, frontPath):
                    cmds.menuItem(label=mLabel, command=lambda *args: ConnectionsCustomControl.showEditorForUSDPrim(newPathStr))

    @staticmethod
    def showEditorForUSDPrim(usdPrimPathStr):
        # Simple helper to open the AE on input prim.
        mel.eval('evalDeferred "showEditor(\\\"%s\\\")"' % usdPrimPathStr)

    @staticmethod
    def isHiddenComponentHandlingNode(prim):
        """
        Connections at the component level in LookdevX is handled by hidden separate and
        combine nodes. So what appears as a connection:

        add1.out.r -> surface.color.r

        is in fact a connection:

        add1.out -> separate1.in/separate1.outr -> combine1.in1/combine1.out -> surface.color

        The separate and combine nodes are hidden and we expect the attribute editor to
        also consider them hidden and to traverse from surface.color directly to add1.out
        """
        hiddenByMetadata = False
        metadata = prim.GetCustomDataByKey("Autodesk")
        if metadata and metadata.get("hidden", "").lower() in ["1", "true"]:
            hiddenByMetadata = True

        if not prim.IsHidden() and not hiddenByMetadata:
            return False

        shader = UsdShade.Shader(prim)
        if not shader:
            return False

        nodeId = shader.GetIdAttr().Get()
        return nodeId.startswith("ND_separate") or nodeId.startswith("ND_combine")

    def gatherConnections(self, attr, frontPath):
        """Gather all connections to the attribute. Can be more than one due to component connections."""
        retVal = set()
        for c in attr.GetConnections():
            connectedPrim = attr.GetPrim().GetStage().GetPrimAtPath(c.GetPrimPath())
            if ConnectionsCustomControl.isHiddenComponentHandlingNode(connectedPrim):
                # Traverse through input attributes:
                shader = UsdShade.Shader(connectedPrim)
                for inputAttr in shader.GetInputs():
                    for menuEntry in self.gatherConnections(inputAttr.GetAttr(), frontPath):
                        retVal.add(menuEntry)
            else:
                parentPath = c.GetParentPath()
                primName = parentPath.MakeRelativePath(parentPath.GetParentPath())
                mLabel = '%s%s...' % (primName, c.elementString)
                usdSeg = ufe.PathSegment(str(c.GetPrimPath()), mayaUsdUfe.getUsdRunTimeId(), '/')
                newPath = (frontPath + usdSeg)
                retVal.add((mLabel, ufe.PathString.string(newPath)))

        return sorted(retVal)

    def onReplace(self, *args):
        # We only display the attribute name and type. Neither of these are time
        # varying, so we don't need to implement the replace.
        pass
