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

from .image_custom_control import customImageControlCreator
from .metadata_custom_control import MetaDataCustomControl
from .connections_custom_control import connectionsCustomControlCreator
from .array_custom_control import arrayCustomControlCreator
from .attribute_custom_control import getNiceAttributeName

import collections
import fnmatch
from functools import partial
import re
import ufe
import maya.mel as mel
import maya.cmds as cmds
import mayaUsd.ufe as mayaUsdUfe
import mayaUsd.lib as mayaUsdLib
import maya.internal.common.ufe_ae.template as ufeAeTemplate
from mayaUsdLibRegisterStrings import getMayaUsdLibString

# We manually import all the classes which have a 'GetSchemaAttributeNames'
# method so we have access to it and the 'pythonClass' method.
from pxr import Usd, UsdGeom, UsdLux, UsdRender, UsdRi, UsdShade, UsdSkel, UsdUI, UsdVol, Kind, Tf, Sdr, Sdf

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
            mel.eval("evalDeferred -low \"refreshEditorTemplates\";")


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
            mel.eval("evalDeferred -low \"refreshEditorTemplates\";")

    def onCreate(self, *args):
        ufe.Attributes.addObserver(self._item, self)

    def onReplace(self, *args):
        # Nothing needed here since we don't create any UI.
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

def defaultControlCreator(aeTemplate, c):
    ufeAttr = aeTemplate.attrS.attribute(c)
    uiLabel = getNiceAttributeName(ufeAttr, c) if aeTemplate.useNiceName else c
    cmds.editorTemplate(addControl=[c], label=uiLabel)
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

        self._attributeLayout = AEShaderLayout.Group(self._attributeLayout.name + ": " + mayaUsdLib.Util.prettifyName(label))

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

        schemaTypeName = mayaUsdLib.Util.prettifyName(schemaTypeName)

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
