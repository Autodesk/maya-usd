# Copyright 2021 Autodesk
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

from .custom_image_control import customImageControlCreator
from .attribute_custom_control import getNiceAttributeName
from .attribute_custom_control import cleanAndFormatTooltip
from .attribute_custom_control import AttributeCustomControl

import collections
import fnmatch
from functools import partial
import re
import ufe
import usdUfe
import maya.mel as mel
import maya.cmds as cmds
import mayaUsd.ufe as mayaUsdUfe
import mayaUsd.lib as mayaUsdLib
import maya.internal.common.ufe_ae.template as ufeAeTemplate
from mayaUsdLibRegisterStrings import getMayaUsdLibString

try:
    # This helper class was only added recently to Maya.
    import maya.internal.ufeSupport.attributes as attributes
    hasAEPopupMenu = 'AEPopupMenu' in dir(attributes)
except:
    hasAEPopupMenu = False

from maya.common.ui import LayoutManager, ParentManager
from maya.common.ui import setClipboardData
from maya.OpenMaya import MGlobal

# We manually import all the classes which have a 'GetSchemaAttributeNames'
# method so we have access to it and the 'pythonClass' method.
from pxr import Usd, UsdGeom, UsdLux, UsdRender, UsdRi, UsdShade, UsdSkel, UsdUI, UsdVol, Kind, Tf, Sdr, Sdf

nameTxt = 'nameTxt'
attrValueFld = 'attrValueFld'
attrTypeFld = 'attrTypeFld'

# Helper class to push/pop the Attribute Editor Template. This makes
# sure that controls are aligned properly.
class AEUITemplate:
    def __enter__(self):
        cmds.setUITemplate('attributeEditorTemplate', pst=True)
        return self

    def __exit__(self, mytype, value, tb):
        cmds.setUITemplate(ppt=True)

_editorRefreshQueued = False

def _refreshEditor():
    '''Reset the queued refresh flag and refresh the AE.'''
    global _editorRefreshQueued
    _editorRefreshQueued = False
    cmds.refreshEditorTemplates()
    
def _queueEditorRefresh():
    '''If there is not already a AE refresh queued, queue a refresh.'''
    global _editorRefreshQueued
    if _editorRefreshQueued:
        return
    cmds.evalDeferred(_refreshEditor, low=True)
    _editorRefreshQueued = True

# Custom control, but does not have any UI. Instead we use
# this control to be notified from UFE when any attribute has changed
# so we can update the AE. This is to fix refresh issue
# when transform is added to a prim.
class UfeAttributesObserver(ufe.Observer):
    def __init__(self, item):
        super(UfeAttributesObserver, self).__init__()
        self._item = item

    def __del__(self):
        ufe.Attributes.removeObserver(self)

    def __call__(self, notification):
        refreshEditor = False
        if isinstance(notification, ufe.AttributeValueChanged):
            if notification.name() == UsdGeom.Tokens.xformOpOrder:
                refreshEditor = True
        if hasattr(ufe, "AttributeAdded") and isinstance(notification, ufe.AttributeAdded):
            refreshEditor = True
        if hasattr(ufe, "AttributeRemoved") and isinstance(notification, ufe.AttributeRemoved):
            refreshEditor = True
        if refreshEditor:
            _queueEditorRefresh()


    def onCreate(self, *args):
        ufe.Attributes.addObserver(self._item, self)

    def onReplace(self, *args):
        # Nothing needed here since we don't create any UI.
        pass

class UfeConnectionChangedObserver(ufe.Observer):
    def __init__(self, item):
        super(UfeConnectionChangedObserver, self).__init__()
        self._item = item

    def __del__(self):
        ufe.Attributes.removeObserver(self)

    def __call__(self, notification):
        if hasattr(ufe, "AttributeConnectionChanged") and isinstance(notification, ufe.AttributeConnectionChanged):
            _queueEditorRefresh()

    def onCreate(self, *args):
        ufe.Attributes.addObserver(self._item, self)

    def onReplace(self, *args):
        # Nothing needed here since we don't create any UI.
        pass

class MetaDataCustomControl(object):
    # Custom control for all prim metadata we want to display.
    def __init__(self, item, prim, useNiceName):
        # In Maya 2022.1 we need to hold onto the Ufe SceneItem to make
        # sure it doesn't go stale. This is not needed in latest Maya.
        super(MetaDataCustomControl, self).__init__()
        mayaVer = '%s.%s' % (cmds.about(majorVersion=True), cmds.about(minorVersion=True))
        self.item = item if mayaVer == '2022.1' else None
        self.prim = prim
        self.useNiceName = useNiceName

        # There are four metadata that we always show: primPath, kind, active, instanceable
        # We use a dictionary to store the various other metadata that this prim contains.
        self.extraMetadata = dict()

    def onCreate(self, *args):
        # Metadata: PrimPath
        # The prim path is for display purposes only - it is not editable, but we
        # allow keyboard focus so you copy the value.
        self.primPath = cmds.textFieldGrp(label='Prim Path', editable=False, enableKeyboardFocus=True)

        # Metadata: Kind
        # We add the known Kind types, in a certain order ("model hierarchy") and then any
        # extra ones that were added by extending the kind registry.
        # Note: we remove the "model" kind because in the USD docs it states, 
        #       "No prim should have the exact kind "model".
        allKinds = Kind.Registry.GetAllKinds()
        allKinds.remove(Kind.Tokens.model)
        knownKinds = [Kind.Tokens.group, Kind.Tokens.assembly, Kind.Tokens.component, Kind.Tokens.subcomponent]
        temp1 = [ele for ele in allKinds if ele not in knownKinds]
        knownKinds.extend(temp1)

        # If this prim's kind is not registered, we need to manually
        # add it to the list.
        model = Usd.ModelAPI(self.prim)
        primKind = model.GetKind()
        if primKind not in knownKinds:
            knownKinds.insert(0, primKind)
        if '' not in knownKinds:
            knownKinds.insert(0, '')    # Set metadata value to "" (or empty).

        self.kind = cmds.optionMenuGrp(label='Kind',
                                       cc=self._onKindChanged,
                                       ann=getMayaUsdLibString('kKindMetadataAnn'))

        for ele in knownKinds:
            cmds.menuItem(label=ele)

        # Metadata: Active
        self.active = cmds.checkBoxGrp(label='Active',
                                       ncb=1,
                                       cc1=self._onActiveChanged,
                                       ann=getMayaUsdLibString('kActiveMetadataAnn'))

        # Metadata: Instanceable
        self.instan = cmds.checkBoxGrp(label='Instanceable',
                                       ncb=1,
                                       cc1=self._onInstanceableChanged,
                                       ann=getMayaUsdLibString('kInstanceableMetadataAnn'))

        # Get all the other Metadata and remove the ones above, as well as a few
        # we don't ever want to show.
        allMetadata = self.prim.GetAllMetadata()
        keysToDelete = ['kind', 'active', 'instanceable', 'typeName', 'documentation']
        for key in keysToDelete:
            allMetadata.pop(key, None)
        if allMetadata:
            cmds.separator(h=10, style='single', hr=True)

            for k in allMetadata:
                # All extra metadata is for display purposes only - it is not editable, but we
                # allow keyboard focus so you copy the value.
                mdLabel = mayaUsdUfe.prettifyName(k) if self.useNiceName else k
                self.extraMetadata[k] = cmds.textFieldGrp(label=mdLabel, editable=False, enableKeyboardFocus=True)

        # Update all metadata values.
        self.refresh()

    def onReplace(self, *args):
        # Nothing needed here since USD data is not time varying. Normally this template
        # is force rebuilt all the time, except in response to time change from Maya. In
        # that case we don't need to update our controls since none will change.
        pass

    def refresh(self):
        # PrimPath
        cmds.textFieldGrp(self.primPath, edit=True, text=str(self.prim.GetPath()))

        # Kind
        model = Usd.ModelAPI(self.prim)
        primKind = model.GetKind()
        if not primKind:
            # Special case to handle the empty string (for meta data value empty).
            cmds.optionMenuGrp(self.kind, edit=True, select=1)
        else:
            cmds.optionMenuGrp(self.kind, edit=True, value=primKind)

        # Active
        cmds.checkBoxGrp(self.active, edit=True, value1=self.prim.IsActive())

        # Instanceable
        cmds.checkBoxGrp(self.instan, edit=True, value1=self.prim.IsInstanceable())

        # All other metadata types
        for k in self.extraMetadata:
            v = self.prim.GetMetadata(k) if k != 'customData' else self.prim.GetCustomData()
            cmds.textFieldGrp(self.extraMetadata[k], edit=True, text=str(v))

    def _onKindChanged(self, value):
        with mayaUsdLib.UsdUndoBlock():
            model = Usd.ModelAPI(self.prim)
            model.SetKind(value)

    def _onActiveChanged(self, value):
        with mayaUsdLib.UsdUndoBlock():
            try:
                usdUfe.ToggleActiveCommand(self.prim).execute()
            except Exception as ex:
                # Note: the command might not work because there is a stronger
                #       opinion, so update the checkbox.
                cmds.checkBoxGrp(self.active, edit=True, value1=self.prim.IsActive())
                cmds.error(str(ex))

    def _onInstanceableChanged(self, value):
        with mayaUsdLib.UsdUndoBlock():
            try:
                usdUfe.ToggleInstanceableCommand(self.prim).execute()
            except Exception as ex:
                # Note: the command might not work because there is a stronger
                #       opinion, so update the checkbox.
                cmds.checkBoxGrp(self.instan, edit=True, value1=self.prim.IsInstanceable())
                cmds.error(str(ex))

# Custom control for all array attribute.
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

def showEditorForUSDPrim(usdPrimPathStr):
    # Simple helper to open the AE on input prim.
    mel.eval('evalDeferred "showEditor(\\\"%s\\\")"' % usdPrimPathStr)

# Custom control for all attributes that have connections.
class ConnectionsCustomControl(AttributeCustomControl):
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
                cmds.text(nameTxt, al='right', label=attrLabel, annotation=cleanAndFormatTooltip(attr.GetDocumentation()))
                cmds.textField(attrTypeFld, editable=False, text=attrType, backgroundColor=[0.945, 0.945, 0.647], font='obliqueLabelFont', width=singleWidgetWidth*1.5)

                # Add a menu item for each connection.
                cmds.popupMenu()
                for mLabel, newPathStr in self.gatherConnections(attr, frontPath):
                    cmds.menuItem(label=mLabel, command=lambda *args: showEditorForUSDPrim(newPathStr))

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


class NoticeListener(object):
    # Inserted as a custom control, but does not have any UI. Instead we use
    # this control to be notified from USD when any metadata has changed
    # so we can update the AE fields.
    def __init__(self, prim, metadataControls):
        self.prim = prim
        self.metadataControls = metadataControls

    def onCreate(self, *args):
        self.listener = Tf.Notice.Register(Usd.Notice.ObjectsChanged,
                                           self.__OnPrimsChanged, self.prim.GetStage())
        pname = cmds.setParent(q=True)
        cmds.scriptJob(uiDeleted=[pname, self.onClose], runOnce=True)

    def onReplace(self, *args):
        # Nothing needed here since we don't create any UI.
        pass

    def onClose(self):
        if self.listener:
            self.listener.Revoke()
            self.listener = None

    def __OnPrimsChanged(self, notice, sender):
        if notice.HasChangedFields(self.prim):
            # Iterate thru all the metadata controls (we were given when created) and
            # call the refresh method (if it exists).
            for ctrl in self.metadataControls:
                if hasattr(ctrl, 'refresh'):
                    ctrl.refresh()

def connectionsCustomControlCreator(aeTemplate, c):
    if aeTemplate.attributeHasConnections(c):
        ufeAttr = aeTemplate.attrS.attribute(c)
        return ConnectionsCustomControl(aeTemplate.item, ufeAttr, aeTemplate.prim, c, aeTemplate.useNiceName)
    else:
        return None

def arrayCustomControlCreator(aeTemplate, c):
    # Note: UsdGeom.Tokens.xformOpOrder is a exception we want it to display normally.
    if c == UsdGeom.Tokens.xformOpOrder:
        return None

    if aeTemplate.isArrayAttribute(c):
        ufeAttr = aeTemplate.attrS.attribute(c)
        return ArrayCustomControl(ufeAttr, aeTemplate.prim, c, aeTemplate.useNiceName)
    else:
        return None

def defaultControlCreator(aeTemplate, c):
    ufeAttr = aeTemplate.attrS.attribute(c)
    uiLabel = getNiceAttributeName(ufeAttr, c) if aeTemplate.useNiceName else c
    cmds.editorTemplate(addControl=[c], label=uiLabel, annotation=cleanAndFormatTooltip(ufeAttr.getDocumentation()))
    return None

class AEShaderLayout(object):
    '''
    This class takes care of sorting attributes that use uifolder and uiorder metadata and to prepare
    a data structure that has the exact sequence of layouts and attributes necessary to properly
    lay out all the attributes in the right folders.
    '''
    # A named tuple for attribute information:
    _AttributeInfo = collections.namedtuple("_AttributeInfo", ["uiorder", "name", "uifolder"])
    # A string splitter for the most commonly used separators:
    _groupSplitter = re.compile(r"[/|\;:]")
    # Valid uiorder:
    _isDecimal = re.compile("^[0-9]+$")

    class Group(object):
        """Base class to return layout information. The list of items can contain subgroups."""
        def __init__(self, name):
            self._name = name
            self._items = []
        @property
        def name(self):
            return self._name
        @property
        def items(self):
            return self._items

    def __init__(self, ufeSceneItem):
        self.item = ufeSceneItem
        self.prim = mayaUsdUfe.ufePathToPrim(ufe.PathString.string(self.item.path()))
        self._attributeInfoList = []
        self._canCompute = True
        self._attributeLayout = AEShaderLayout.Group(self.item.nodeType())
        if ufeSceneItem.nodeType() == "Shader":
            self.parseShaderAttributes()
        else:
            self.parseNodeGraphAttributes()

    def parseShaderAttributes(self):
        """Parse shader attributes using the Sdr node and property APIs."""
        if not hasattr(ufe, "NodeDef"):
            # Ufe does not yet have unauthored attribute support. We can compute, but it won't help.
            self._canCompute = False
            return
        shader = UsdShade.Shader(self.prim)
        nodeId = shader.GetIdAttr().Get()
        if not nodeId:
            self._canCompute = False
            return
        nodeDef = Sdr.Registry().GetShaderNodeByIdentifier(nodeId)
        if not nodeDef:
            self._canCompute = False
            return
        label = nodeDef.GetLabel()
        if not label:
            label = nodeDef.GetFamily()

        self._attributeLayout = AEShaderLayout.Group(self._attributeLayout.name + ": " + mayaUsdUfe.prettifyName(label))

        # Best option: Use ordering metadata found the Sdr properties:
        hasMetadataOrdering = False
        for inputName in nodeDef.GetInputNames():
            input = nodeDef.GetShaderInput(inputName)
            metadata = input.GetHints()
            metadata.update(input.GetMetadata())
            if "uiorder" in metadata or "uifolder" in metadata:
                hasMetadataOrdering = True
                break

        if hasMetadataOrdering:
            # Prefer metadata over GetPages. The metadata can contain subgroups.
            unorderedIndex = 10000
            for inputName in nodeDef.GetInputNames():
                input = nodeDef.GetShaderInput(inputName)
                metadata = input.GetHints()
                metadata.update(input.GetMetadata())
                uiorder = metadata.get("uiorder", "").strip()
                if AEShaderLayout._isDecimal.match(uiorder):
                    uiorder = int(uiorder)
                else:
                    uiorder = unorderedIndex
                    unorderedIndex += 1
                uifolder = metadata.get("uifolder", "").strip()
                self._attributeInfoList.append(
                    AEShaderLayout._AttributeInfo(uiorder,
                                                  UsdShade.Utils.GetFullName(inputName, UsdShade.AttributeType.Input),
                                                  uifolder))
            return

        # Second best option: Use page information populated in the Sdr node at shader discovery time.
        pages = nodeDef.GetPages()
        if len(pages) == 1 and not pages[0]:
            pages = []

        if not pages:
            # Worst case: Flat layout
            for name in nodeDef.GetInputNames():
                self._attributeLayout.items.append(UsdShade.Utils.GetFullName(name, UsdShade.AttributeType.Input))
            return

        for page in pages:
            pageLabel = page
            if not page:
                pageLabel = 'Extra Shader Attributes'
            group = AEShaderLayout.Group(pageLabel)
            for name in nodeDef.GetPropertyNamesForPage(page):
                if nodeDef.GetInput(name):
                    name = UsdShade.Utils.GetFullName(name, UsdShade.AttributeType.Input)
                    group.items.append(name)
            if group.items:
                self._attributeLayout.items.append(group)

    def parseNodeGraphAttributes(self):
        """NodeGraph and Material do not have associated Sdr information, but some layouting metadata
           might have been added by shader graph editors."""
        nodegraph = UsdShade.NodeGraph(self.prim)
        if not nodegraph:
            self._canCompute = False
            return
        # This should make sure items without uiorder appear at the end. Still no guarantee since
        # the user can use any numbering sequence he wants.
        unorderedIndex = 10000
        for input in nodegraph.GetInputs():
            uiorder = input.GetSdrMetadataByKey("uiorder").strip()
            if AEShaderLayout._isDecimal.match(uiorder):
                uiorder = int(uiorder)
            else:
                uiorder = unorderedIndex
                unorderedIndex += 1
            uifolder = input.GetSdrMetadataByKey("uifolder").strip()
            self._attributeInfoList.append(AEShaderLayout._AttributeInfo(uiorder, input.GetFullName(), uifolder))

    def get(self):
        '''Computes the layout based on metadata ordering if an info list is present. If the list
           was computed in a different way, the attribute info list will be empty, and we return the
           computed attributeLayout unchanged.'''
        if not self._canCompute:
            return None

        self._attributeInfoList.sort()
        folderIndex = {(): self._attributeLayout}

        for attributeInfo in self._attributeInfoList:
            groups = tuple()
            if attributeInfo.uifolder:
                groups = tuple(AEShaderLayout._groupSplitter.split(attributeInfo.uifolder))
                # Ensure the parent groups are there 
                for i in range(len(groups)):
                    subgroup = groups[0:i+1]
                    if subgroup not in folderIndex:
                        newGroup = AEShaderLayout.Group(subgroup[-1])
                        folderIndex[subgroup[0:i]].items.append(newGroup)
                        folderIndex[subgroup] = newGroup
            # Add the attribute to the group
            folderIndex[groups].items.append(attributeInfo.name)
        return self._attributeLayout

# SchemaBase template class for categorization of the attributes.
# We no longer use the base class ufeAeTemplate.Template as we want to control
# the placement of the metadata at the bottom (after extra attributes).
class AETemplate(object):
    '''
    This system of schema inherits groups attributes per schema helping the user
    learn how attributes are stored.
    '''
    def __init__(self, ufeSceneItem):
        self.assetPathType = Tf.Type.FindByName('SdfAssetPath')
        self.item = ufeSceneItem
        self.prim = mayaUsdUfe.ufePathToPrim(ufe.PathString.string(self.item.path()))

        # Get the UFE Attributes interface for this scene item.
        self.attrS = ufe.Attributes.attributes(self.item)
        self.addedAttrs = []
        self.suppressedAttrs = []

        self.showArrayAttributes = False
        if cmds.optionVar(exists="mayaUSD_AEShowArrayAttributes"):
            self.showArrayAttributes = cmds.optionVar(query="mayaUSD_AEShowArrayAttributes")

        # Should we display nice names in AE?
        self.useNiceName = True
        if cmds.optionVar(exists='attrEditorIsLongName'):
            self.useNiceName = (cmds.optionVar(q='attrEditorIsLongName') ==1)

        cmds.editorTemplate(beginScrollLayout=True)
        self.buildUI()
        self.createAppliedSchemasSection()
        self.createCustomExtraAttrs()
        self.createMetadataSection()
        cmds.editorTemplate(endScrollLayout=True)

        if ('%s.%s' % (cmds.about(majorVersion=True), cmds.about(minorVersion=True))) > '2022.1':
            # Because of how we dynamically build the Transform attribute section,
            # we need this template to rebuild each time it is needed. This will
            # also restore the collapse/expand state of the sections.
            # Note: in Maya 2022 all UFE templates were forcefully rebuilt, but
            #       no restore of section states.
            try:
                cmds.editorTemplate(forceRebuild=True)
            except:
                pass

    _controlCreators = [connectionsCustomControlCreator, arrayCustomControlCreator, customImageControlCreator, defaultControlCreator]

    @staticmethod
    def prependControlCreator(controlCreator):
        AETemplate._controlCreators.insert(0, controlCreator)

    def addControls(self, controls):
        for c in controls:
            if c not in self.suppressedAttrs:
                for controlCreator in AETemplate._controlCreators:
                    try:
                        createdControl = controlCreator(self, c)
                        if createdControl:
                            self.defineCustom(createdControl, c)
                            break
                    except Exception as ex:
                        # Do not let one custom control failure affect others.
                        print('Failed to create control %s: %s' % (c, ex))
                self.addedAttrs.append(c)

    def suppress(self, control):
        cmds.editorTemplate(suppress=control)
        self.suppressedAttrs.append(control)

    @staticmethod
    def defineCustom(customObj, attrs=[]):
        create = lambda *args : customObj.onCreate(args)
        replace = lambda *args : customObj.onReplace(args)
        cmds.editorTemplate(attrs, callCustom=[create, replace])

    def createSection(self, layoutName, attrList, collapse=False):
        # We create the section named "layoutName" if at least one
        # of the attributes from the input list exists.
        for attr in attrList:
            if attr in self.suppressedAttrs:
                continue
            if self.attrS.hasAttribute(attr):
                with ufeAeTemplate.Layout(self, layoutName, collapse):
                    # Note: addControls will silently skip any attributes
                    #       which do not exist.
                    self.addControls(attrList)
                return

    def sectionNameFromSchema(self, schemaTypeName):
        '''Return a section name from the input schema type name. This section
        name is a pretty name used in the UI.'''

        # List of special rules for adjusting the base schema names.
        prefixToAdjust = {
            'UsdAbc' : '',
            'UsdGeomGprim' : 'GeometricPrim',
            'UsdGeom' : '',
            'UsdHydra' : '',
            'UsdImagingGL' : '',
            'UsdLux' : '',
            'UsdMedia' : '',
            'UsdRender' : '',
            'UsdRi' : '',
            'UsdShade' : '',
            'UsdSkelAnimation' : 'SkelAnimation',
            'UsdSkelBlendShape': 'BlendShape',
            'UsdSkelSkeleton': 'Skeleton',
            'UsdSkelRoot' : 'SkelRoot', 
            'UsdUI' : '',
            'UsdUtils' : '',
            'UsdVol' : ''
        }
        for p, r in prefixToAdjust.items():
            if schemaTypeName.startswith(p):
                schemaTypeName = schemaTypeName.replace(p, r, 1)
                break

        schemaTypeName = mayaUsdUfe.prettifyName(schemaTypeName)

        # if the schema name ends with "api" or "API", trim it.
        if schemaTypeName.endswith("api") or schemaTypeName.endswith("API"):
            schemaTypeName = schemaTypeName[:-3]

        return schemaTypeName

    def addShaderLayout(self, group):
        """recursively create the full attribute layout section"""
        with ufeAeTemplate.Layout(self, group.name):
            for item in group.items:
                if isinstance(item, AEShaderLayout.Group):
                    self.addShaderLayout(item)
                else:
                    if self.attrS.attribute(item):
                        self.addControls([item])

    def createShaderAttributesSection(self):
        """Use an AEShaderLayout tool to populate the shader section"""
        # Add a custom control to monitor for connection changed.
        cnxObs = UfeConnectionChangedObserver(self.item)
        self.defineCustom(cnxObs)
        # Hide all outputs:
        for name in self.attrS.attributeNames:
            if UsdShade.Utils.GetBaseNameAndType(name)[1] == UsdShade.AttributeType.Output:
                self.suppress(name)
        # Build a layout from USD metadata:
        layout = AEShaderLayout(self.item).get()
        if layout:
            self.addShaderLayout(layout)

    def createTransformAttributesSection(self, sectionName, attrsToAdd):
        # Get the xformOp order and add those attributes (in order)
        # followed by the xformOp order attribute.
        allAttrs = self.attrS.attributeNames
        geomX = UsdGeom.Xformable(self.prim)
        xformOps = geomX.GetOrderedXformOps()
        xformOpOrderNames = [op.GetOpName() for op in xformOps]
        xformOpOrderNames.append(UsdGeom.Tokens.xformOpOrder)

        # Don't use createSection because we want a sub-sections.
        with ufeAeTemplate.Layout(self, sectionName):
            attrsToAdd.remove(UsdGeom.Tokens.xformOpOrder)
            self.addControls(xformOpOrderNames)

            # Get the remainder of the xformOps and add them in an Unused section.
            xformOpUnusedNames = fnmatch.filter(allAttrs, 'xformOp:*')
            xformOpUnusedNames = [ele for ele in xformOpUnusedNames if ele not in xformOpOrderNames]
            self.createSection(getMayaUsdLibString('kLabelUnusedTransformAttrs'), xformOpUnusedNames, collapse=True)

            # Then add any reamining Xformable attributes
            self.addControls(attrsToAdd)

            # Add a custom control for UFE attribute changed.
            t3dObs = UfeAttributesObserver(self.item)
            self.defineCustom(t3dObs)

    def createMetadataSection(self):
        # We don't use createSection() because these are metadata (not attributes).
        with ufeAeTemplate.Layout(self, getMayaUsdLibString('kLabelMetadata'), collapse=True):
            metaDataControl = MetaDataCustomControl(self.item, self.prim, self.useNiceName)
            usdNoticeControl = NoticeListener(self.prim, [metaDataControl])
            self.defineCustom(metaDataControl)
            self.defineCustom(usdNoticeControl)


    def createCustomExtraAttrs(self):
        # We are not using the maya default "Extra Attributes" section
        # because we are using custom widget for array type and it's not
        # possible to inject our widget inside the maya "Extra Attributes" section.

        # The extraAttrs will contains suppressed attribute but this is not a big deal as
        # long as the suppressed attributes are suppressed by suppress(self, control).
        # This function will keep all suppressed attributes into a list which will be use
        # by addControls(). So any suppressed attributes in extraAttrs will be ignored later.
        extraAttrs = [attr for attr in self.attrS.attributeNames if attr not in self.addedAttrs]
        sectionName = mel.eval("uiRes(\"s_TPStemplateStrings.rExtraAttributes\");")
        self.createSection(sectionName, extraAttrs, True)

    def createAppliedSchemasSection(self):
        usdVer = Usd.GetVersion()
        showAppliedSchemasSection = False

        # loop on all applied schemas and store all those
        # schema into a dictionary with the attributes.
        # Storing the schema into a dictionary allow us to
        # group all instances of a MultipleApply schema together
        # so we can later display them into the same UI section.
        #
        # We do not group the collections API. We rather create a section that
        # includes the instance in the title. This allows us to optimally trim
        # the type name and the namespace when generating the attribute nice names.
        #
        # By example, on a UsdLux light, we have two UsdCollectionAPI applied.
        # UsdPrim.GetAppliedSchemas() will return ["CollectionAPI:lightLink",
        # "CollectionAPI:shadowLink"] and we will create two sections named
        # "Light Link Collection" and "Shadow Link Collection". An attribute
        # named "collection:lightLink:includeRoot" will get the base nice name
        # "Collection Light Link Include Root" and a comparison with the schema nice name
        # "Collection Light Link" will allow of to trim the nice name to "Include Root"
        #
        schemaAttrsDict = {}
        appliedSchemas = self.prim.GetAppliedSchemas()
        for schema in appliedSchemas:
            typeAndInstance = Usd.SchemaRegistry().GetTypeNameAndInstance(schema)
            typeName        = typeAndInstance[0]
            schemaType      = Usd.SchemaRegistry().GetTypeFromName(typeName)

            if schemaType.pythonClass:
                isMultipleApplyAPISchema = Usd.SchemaRegistry().IsMultipleApplyAPISchema(typeName)
                if isMultipleApplyAPISchema:
                    # get the attributes names. They will not include the namespace and instance name.
                    instanceName = typeAndInstance[1]
                    attrList = schemaType.pythonClass.GetSchemaAttributeNames(False, instanceName)
                    # build the real attr name
                    # By example, collection:lightLink:includeRoot
                    # In 0.22.3 USD will now use the fully namespaced name in the schema definition,
                    # so it does not need to be assembled.
                    if usdVer < (0, 22, 3):
                        namespace = Usd.SchemaRegistry().GetPropertyNamespacePrefix(typeName)
                        prefix = namespace + ":" + instanceName + ":"
                        attrList = [prefix + i for i in attrList]

                    schemaAttrsDict[instanceName + typeName] = attrList
                else:
                    attrList = schemaType.pythonClass.GetSchemaAttributeNames(False)
                    schemaAttrsDict[typeName] = attrList

                # The "Applied Schemas" will be only visible if at least
                # one applied Schemas has attribute.
                if not showAppliedSchemasSection:
                    for attr in attrList:
                        if self.attrS.hasAttribute(attr):
                            showAppliedSchemasSection = True
                            break

        # Create the "Applied Schemas" section
        # with all the applied schemas
        if showAppliedSchemasSection:
            with ufeAeTemplate.Layout(self, getMayaUsdLibString('kLabelAppliedSchemas'), collapse=True):
                for typeName, attrs in schemaAttrsDict.items():
                    typeName = self.sectionNameFromSchema(typeName)
                    self.createSection(typeName, attrs, False)


    def buildUI(self):
        usdSch = Usd.SchemaRegistry()

        self.suppressArrayAttribute()

        # We use UFE for the ancestor node types since it caches the
        # results by node type.
        hasProcessedMaterial = False
        for schemaType in self.item.ancestorNodeTypes():
            schemaType = usdSch.GetTypeFromName(schemaType)
            schemaTypeName = schemaType.typeName
            sectionName = self.sectionNameFromSchema(schemaTypeName)
            if schemaType.pythonClass:
                attrsToAdd = schemaType.pythonClass.GetSchemaAttributeNames(False)
                if schemaTypeName in ['UsdShadeShader', 'UsdShadeNodeGraph', 'UsdShadeMaterial']:
                    # Material has NodeGraph as base. We want to process once for both schema types:
                    if hasProcessedMaterial:
                        continue
                    # Shader attributes are special
                    self.createShaderAttributesSection()
                    hasProcessedMaterial = True
                # We have a special case when building the Xformable section.
                elif schemaTypeName == 'UsdGeomXformable':
                    self.createTransformAttributesSection(sectionName, attrsToAdd)
                else:
                    sectionsToCollapse = ['Curves', 'Point Based', 'Geometric Prim', 'Boundable',
                                          'Imageable', 'Field Asset', 'Light']
                    collapse = sectionName in sectionsToCollapse
                    self.createSection(sectionName, attrsToAdd, collapse)

    def suppressArrayAttribute(self):
        # Suppress all array attributes.
        if not self.showArrayAttributes:
            for attrName in self.attrS.attributeNames:
                if self.isArrayAttribute(attrName):
                    self.suppress(attrName)

    def isArrayAttribute(self, attrName):
        if self.attrS.attributeType(attrName) == ufe.Attribute.kGeneric:
            attr = self.prim.GetAttribute(attrName)
            typeName = attr.GetTypeName()
            return typeName.isArray
        return False

    def isImageAttribute(self, attrName):
        kFilenameAttr = ufe.Attribute.kFilename if hasattr(ufe.Attribute, "kFilename") else 'Filename'
        if self.attrS.attributeType(attrName) != kFilenameAttr:
            return False
        shader = UsdShade.Shader(self.prim)
        if shader and attrName.startswith("inputs:"):
            # Shader attribute. The actual USD Attribute might not exist yet.
            return True
        attr = self.prim.GetAttribute(attrName)
        if not attr:
            return False
        typeName = attr.GetTypeName()
        if not typeName:
            return False
        return self.assetPathType == typeName.type

    def attributeHasConnections(self, attrName):
        # Simple helper to return whether the input attribute (by name) has
        # any connections.
        attr = self.prim.GetAttribute(attrName)
        return attr.HasAuthoredConnections() if attr else False
