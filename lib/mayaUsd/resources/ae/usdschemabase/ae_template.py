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

from .arrayCustomControl import ArrayCustomControl
from .imageCustomControl import ImageCustomControl
from .attributeCustomControl import getNiceAttributeName
from .attributeCustomControl import cleanAndFormatTooltip
from .connectionsCustomControl import ConnectionsCustomControl
from .displayCustomControl import DisplayCustomControl
from .materialCustomControl import MaterialCustomControl
from .metadataCustomControl import MetadataCustomControl
from .assetInfoCustomControl import AssetInfoCustomControl
from .observers import UfeAttributesObserver, UfeConnectionChangedObserver, UsdNoticeListener
try:
    from .collectionCustomControl import CollectionCustomControl
    collectionsSupported = True
except:
    collectionsSupported = False

import collections
import fnmatch
import re
import ufe
import maya.mel as mel
import maya.cmds as cmds
import mayaUsd.lib as mayaUsdLib
import mayaUsd.ufe as mayaUsdUfe
import maya.internal.common.ufe_ae.template as ufeAeTemplate
from mayaUsdLibRegisterStrings import getMayaUsdLibString

# We manually import all the classes which have a 'GetSchemaAttributeNames'
# method so we have access to it and the 'pythonClass' method.
from pxr import Usd, UsdGeom, UsdShade, Tf, Sdr, Vt


def defaultControlCreator(aeTemplate, attrName):
    '''
    Custom control creator for attribute not handled by any other custom control.
    '''
    ufeAttr = aeTemplate.attrS.attribute(attrName)
    uiLabel = getNiceAttributeName(ufeAttr, attrName) if aeTemplate.useNiceName else attrName
    cmds.editorTemplate(addControl=[attrName], label=uiLabel, annotation=cleanAndFormatTooltip(ufeAttr.getDocumentation()))
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
        if Usd.GetVersion() < (0, 25, 5):
            inputNames = nodeDef.GetInputNames()
        else:
            inputNames = nodeDef.GetShaderInputNames()

        for inputName in inputNames:
            input = nodeDef.GetShaderInput(inputName)
            metadata = input.GetHints()
            metadata.update(input.GetMetadata())
            if "uiorder" in metadata or "uifolder" in metadata:
                hasMetadataOrdering = True
                break

        if hasMetadataOrdering:
            # Prefer metadata over GetPages. The metadata can contain subgroups.
            unorderedIndex = 10000
            for inputName in inputNames:
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
            for name in inputNames:
                self._attributeLayout.items.append(UsdShade.Utils.GetFullName(name, UsdShade.AttributeType.Input))
            return

        for page in pages:
            pageLabel = page
            if not page:
                pageLabel = 'Extra Shader Attributes'
            group = AEShaderLayout.Group(pageLabel)
            for name in nodeDef.GetPropertyNamesForPage(page):
                if Usd.GetVersion() < (0, 25, 5):
                    nodeInput = nodeDef.GetInput(name)
                else:
                    nodeInput = nodeDef.GetShaderInput(name)

                if nodeInput:
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
        self.addedAttrs = set()
        self.suppressedAttrs = []
        self.hasConnectionObserver = False

        self.showArrayAttributes = False
        if cmds.optionVar(exists="mayaUSD_AEShowArrayAttributes"):
            self.showArrayAttributes = cmds.optionVar(query="mayaUSD_AEShowArrayAttributes")

        # Should we display nice names in AE?
        self.useNiceName = True
        if cmds.optionVar(exists='attrEditorIsLongName'):
            self.useNiceName = (cmds.optionVar(q='attrEditorIsLongName') ==1)

        self.addedMaterialSection = False

        self.suppressArrayAttribute()

        # Build the list of schemas with their associated attributes.
        schemasAttributes = {
            'customCallbacks' : [],
            'extraAttributes' : [],
            'assetInfo' : [],
            'metadata' : [],
        }
        
        schemasAttributes.update(self.findAppliedSchemas())
        schemasAttributes.update(self.findClassSchemas())
        schemasAttributes.update(self.findSpecialSections())

        # Order schema sections according to designer's choices.
        orderedSchemas = self.orderSections(schemasAttributes)

        # Build the section UI.
        cmds.editorTemplate(beginScrollLayout=True)
        self.createSchemasSections(orderedSchemas, schemasAttributes)
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

    _controlCreators = [ConnectionsCustomControl.creator, ArrayCustomControl.creator, ImageCustomControl.creator, defaultControlCreator]
    if collectionsSupported:
        _controlCreators.insert(0, CollectionCustomControl.creator)

    @staticmethod
    def prependControlCreator(controlCreator):
        AETemplate._controlCreators.insert(0, controlCreator)

    def orderSections(self, schemasAttributes):
        '''
        Choose the order in which the sections will be added to the AE template.
        '''
        availableSchemas = list(schemasAttributes.keys())

        desiredFirstSchemas = [
            'LightAPI',
            '.* Light',
            'lightLinkCollectionAPI',
            'shadowLinkCollectionAPI',
            'customCallbacks'
        ]

        desiredLastSchemas = [
            'shader',
            'transforms',
            'display',
            'extraAttributes',
            'assetInfo',
            'metadata',
        ]

        def addSchemas(desiredOrder, availableSchemas):
            orderedSchemas = []
            for order in desiredOrder:
                if '*' in order:
                    for avail in availableSchemas[:]:
                        if re.match(order, avail):
                            availableSchemas.remove(avail)
                            orderedSchemas.append(avail)
                elif order in availableSchemas:
                    availableSchemas.remove(order)
                    orderedSchemas.append(order)
            return orderedSchemas

        firstSchemas = addSchemas(desiredFirstSchemas, availableSchemas)
        lastSchemas = addSchemas(desiredLastSchemas, availableSchemas)

        return firstSchemas + availableSchemas + lastSchemas

    def addControls(self, attrNames):
        for attrName in attrNames:
            for controlCreator in AETemplate._controlCreators:
                # Control can suppress attributes in the creator function
                # so we check for supression at each loop
                if attrName in self.suppressedAttrs:
                    break
                if attrName in self.addedAttrs:
                    break

                try:
                    createdControl = controlCreator(self, attrName)
                    if createdControl:
                        self.defineCustom(createdControl, attrName)
                        break
                except Exception as ex:
                    # Do not let one custom control failure affect others.
                    print('Failed to create control %s: %s' % (attrName, ex))
            self.addedAttrs.add(attrName)

    def suppress(self, attrName):
        cmds.editorTemplate(suppress=attrName)
        self.suppressedAttrs.append(attrName)

    def defineCustom(self, customObj, attrs=[]):
        create = lambda *args : customObj.onCreate(args)
        replace = lambda *args : customObj.onReplace(args)
        cmds.editorTemplate(attrs, callCustom=[create, replace])

    def createSection(self, layoutName, attrList, collapse=False):
        # We create the section named "layoutName" if at least one
        # of the attributes from the input list exists.
        for attr in attrList:
            if attr in self.suppressedAttrs:
                continue
            if attr in self.addedAttrs:
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
        prefixToAdjust = [
            ('UsdAbc', ''),
            ('UsdGeomGprim', 'GeometricPrim'),
            ('UsdGeomImageable', mel.eval('uiRes(\"m_AEdagNodeTemplate.kDisplay\");')),
            ('UsdGeomXformable', getMayaUsdLibString('kTransforms')),
            ('UsdGeom', ''),
            ('UsdHydra', ''),
            ('UsdImagingGL', ''),
            ('UsdLux', ''),
            ('UsdMedia', ''),
            ('UsdRender', ''),
            ('UsdRi', ''),
            ('UsdShade', ''),
            ('UsdSkelAnimation', 'SkelAnimation'),
            ('UsdSkelBlendShape', 'BlendShape'),
            ('UsdSkelSkeleton', 'Skeleton'),
            ('UsdSkelRoot', 'SkelRoot'), 
            ('UsdUI', ''),
            ('UsdUtils', ''),
            ('UsdVol', '')
        ]
        for p, r in prefixToAdjust:
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
        with ufeAeTemplate.Layout(self, group.name, collapse=False):
            for item in group.items:
                if isinstance(item, AEShaderLayout.Group):
                    self.addShaderLayout(item)
                else:
                    if self.attrS.attribute(item):
                        self.addControls([item])
    def isRamp(self):
        if not hasattr(ufe, "NodeDefHandler"):
            return False

        runTimeMgr = ufe.RunTimeMgr.instance()
        nodeDefHandler = runTimeMgr.nodeDefHandler(self.item.runTimeId())
        nodeDef = nodeDefHandler.definition(self.item)
        return nodeDef and nodeDef.type() == "ND_adsk_ramp"

    def createShaderAttributesSection(self, sectionName, attrs, collapse):
        """Use an AEShaderLayout tool to populate the shader section"""
        # Add a custom control to monitor for connection changed
        # in order for the UI to update itself when the shader is modified.
        self.addConnectionObserver()
        # Hide all outputs:
        for name in self.attrS.attributeNames:
            if UsdShade.Utils.GetBaseNameAndType(name)[1] == UsdShade.AttributeType.Output:
                self.suppress(name)
        # Hide ramp attrs:
        if self.isRamp():
            for name in self.attrS.attributeNames:
                self.suppress(name)
        # Build a layout from USD metadata:
        layout = AEShaderLayout(self.item).get()
        if layout:
            self.addShaderLayout(layout)

    def addConnectionObserver(self):
        '''Add a connection observer if one has not yet been added.'''
        if self.hasConnectionObserver:
            return
        self.hasConnectionObserver = True
        obs = UfeConnectionChangedObserver(self.item)
        self.defineCustom(obs)

    def createTransformAttributesSection(self, sectionName, attrs, collapse):
        # Get the xformOp order and add those attributes (in order)
        # followed by the xformOp order attribute.
        allAttrs = self.attrS.attributeNames
        geomX = UsdGeom.Xformable(self.prim)
        xformOps = geomX.GetOrderedXformOps()
        xformOpOrderNames = [op.GetOpName() for op in xformOps]
        xformOpOrderNames.append(UsdGeom.Tokens.xformOpOrder)

        # Don't use createSection because we want a sub-sections.
        with ufeAeTemplate.Layout(self, sectionName, collapse=collapse):
            attrs.remove(UsdGeom.Tokens.xformOpOrder)
            self.addControls(xformOpOrderNames)

            # Get the remainder of the xformOps and add them in an Unused section.
            xformOpUnusedNames = fnmatch.filter(allAttrs, 'xformOp:*')
            xformOpUnusedNames = [ele for ele in xformOpUnusedNames if ele not in xformOpOrderNames]
            self.createSection(getMayaUsdLibString('kLabelUnusedTransformAttrs'), xformOpUnusedNames, collapse=True)

            # Then add any reamining Xformable attributes
            self.addControls(attrs)

            # Add a custom control for UFE attribute changed.
            t3dObs = UfeAttributesObserver(self.item)
            self.defineCustom(t3dObs)

    def createDisplaySection(self, sectionName, attrs, collapse):
        with ufeAeTemplate.Layout(self, sectionName, collapse=collapse):
            self.addControls(attrs)
            customDataControl = DisplayCustomControl(self.item, self.prim)
            usdNoticeControl = UsdNoticeListener(self.prim, [customDataControl])
            self.defineCustom(customDataControl)
            self.defineCustom(usdNoticeControl)

    def createAssetInfoSection(self, sectionName, attrs, collapse):
        if not AssetInfoCustomControl.hasAssetInfo(self.prim):
            return

        # We don't use createSection() because these are metadata (not attributes).
        with ufeAeTemplate.Layout(self, getMayaUsdLibString('kLabelAssetInfo'), collapse=collapse):
            assetInfoControl = AssetInfoCustomControl(self.item, self.prim, self.useNiceName)
            usdNoticeControl = UsdNoticeListener(self.prim, [assetInfoControl])
            self.defineCustom(assetInfoControl)
            self.defineCustom(usdNoticeControl)
    
    def createMetadataSection(self, sectionName, attrs, collapse):
        # We don't use createSection() because these are metadata (not attributes).
        with ufeAeTemplate.Layout(self, getMayaUsdLibString('kLabelMetadata'), collapse=collapse):
            metaDataControl = MetadataCustomControl(self.item, self.prim, self.useNiceName)
            usdNoticeControl = UsdNoticeListener(self.prim, [metaDataControl])
            self.defineCustom(metaDataControl)
            self.defineCustom(usdNoticeControl)
    
    def createCustomExtraAttrs(self, sectionName, attrs, collapse):
        # We are not using the maya default "Extra Attributes" section
        # because we are using custom widget for array type and it's not
        # possible to inject our widget inside the maya "Extra Attributes" section.

        # The extraAttrs will contains suppressed attribute but this is not a big deal as
        # long as the suppressed attributes are suppressed by suppress(self, control).
        # This function will keep all suppressed attributes into a list which will be use
        # by addControls(). So any suppressed attributes in extraAttrs will be ignored later.
        extraAttrs = [attr for attr in self.attrS.attributeNames if attr not in self.addedAttrs and attr not in self.suppressedAttrs]
        sectionName = mel.eval("uiRes(\"s_TPStemplateStrings.rExtraAttributes\");")
        self.createSection(sectionName, extraAttrs, collapse)

    def findAppliedSchemas(self):
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
        schemasAttributes = {}
        usdVer = Usd.GetVersion()
        appliedSchemas = self.prim.GetAppliedSchemas()
        for schema in appliedSchemas:
            typeAndInstance = Usd.SchemaRegistry().GetTypeNameAndInstance(schema)
            typeName        = typeAndInstance[0]
            schemaType      = Usd.SchemaRegistry().GetTypeFromName(typeName)
            isMultipleApplyAPISchema = Usd.SchemaRegistry().IsMultipleApplyAPISchema(typeName)

            if schemaType.pythonClass:
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
                        attrList = [namespace + ":" + instanceName] + [prefix + i for i in attrList]
                    elif usdVer <= (0, 22, 11):
                        attrList = [schema] + attrList

                    typeName = instanceName + typeName
                else:
                    attrList = schemaType.pythonClass.GetSchemaAttributeNames(False)

                schemasAttributes[typeName] = attrList
            else:
                schemaPrimDef   = Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(typeName)
                attrList = schemaPrimDef.GetPropertyNames()
                if hasattr(schemaPrimDef, 'GetAttributeDefinition'):
                    attrList = [name for name in attrList if schemaPrimDef.GetAttributeDefinition(name)]
                if isMultipleApplyAPISchema:
                    instanceName = typeAndInstance[1]
                    attrList = [name.replace('__INSTANCE_NAME__', instanceName) for name in attrList]
                schemasAttributes[typeName] = attrList

        return schemasAttributes

    @staticmethod
    def getAETemplateForCustomCallback():
        global _aeTemplate
        return _aeTemplate

    def createCustomCallbackSection(self, sectionName, attrs, collapse):
        '''Special custom callback section that gives users the opportunity to add
        layout section(s) to the AE template.
        See https://github.com/Autodesk/maya-usd/blob/dev/lib/mayaUsd/resources/ae/usdschemabase/Attribute-Editor-Template-Doc.md
        for more info.
        '''

        # Create the callback context/data (empty).
        cbContext = {
            'ufe_path_string' : ufe.PathString.string(self.item.path())
        }
        cbContextDict = Vt._ReturnDictionary(cbContext)
        cbDataDict = Vt._ReturnDictionary({})

        # Trigger the callback which will give other plugins the opportunity
        # to add controls to our AE template.
        global _aeTemplate
        try:
            _aeTemplate = self
            cbDataDict = mayaUsdLib.triggerUICallback('onBuildAETemplate', cbContextDict, cbDataDict)
        except Exception as ex:
            # Do not let any of the callback failures affect our template.
            print('Failed triggerUICallback: %s' % ex)
        _aeTemplate = None

    def findClassSchemas(self):
        schemasAttributes = {}

        usdSch = Usd.SchemaRegistry()

        specialSchemas = {
            'UsdShadeShader', 'UsdShadeNodeGraph', 'UsdShadeMaterial', 'UsdGeomXformable', 'UsdGeomImageable' }

        # We use UFE for the ancestor node types since it caches the
        # results by node type.
        for schemaType in self.item.ancestorNodeTypes():
            schemaType = usdSch.GetTypeFromName(schemaType)
            schemaTypeName = schemaType.typeName
            sectionName = self.sectionNameFromSchema(schemaTypeName)
            if schemaType.pythonClass:
                attrsToAdd = schemaType.pythonClass.GetSchemaAttributeNames(False)
                if schemaTypeName in specialSchemas:
                    continue
                schemasAttributes[sectionName] = attrsToAdd
    
        return schemasAttributes
    
    def findSpecialSections(self):
        schemasAttributes = {}

        usdSch = Usd.SchemaRegistry()

        # Material has NodeGraph as base. We want to process once for both schema types:
        hasProcessedMaterial = False

        # We use UFE for the ancestor node types since it caches the
        # results by node type.
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
                    schemasAttributes['shader'] = []
                    hasProcessedMaterial = True
                    # Note: don't show the material section for materials.
                    self.addedMaterialSection = True
                # We have a special case when building the Xformable section.
                elif schemaTypeName == 'UsdGeomXformable':
                    schemasAttributes['transforms'] = attrsToAdd
                elif schemaTypeName == 'UsdGeomImageable':
                    schemasAttributes['display'] = attrsToAdd

        return schemasAttributes

    def createSchemasSections(self, schemasOrder, schemasAttributes):
        # We want material to be either after the mesh section of the Xformable section,
        # whichever comes first, so that it is not too far down in the AE.
        primTypeName = self.sectionNameFromSchema(self.prim.GetTypeName())
        def addMatSection():
            if not self.addedMaterialSection:
                self.addedMaterialSection = True
                self.createMaterialAttributeSection()

        # Function that determines if a section should be expanded.
        def isSectionOpen(sectionName):
            if sectionName == primTypeName:
                return True
            if sectionName == 'material':
                return True
            lowerName = sectionName.lower()
            return 'light' in lowerName and 'link' not in lowerName
        
        # Dictionary of which function to call to create a given section.
        # By default, calls the generic createSection, which will search
        # in the list of known custom control creators for the one to be
        # used.
        customAttributes = {
            'shader': self.createShaderAttributesSection,
            'transforms': self.createTransformAttributesSection,
            'display': self.createDisplaySection,
            'extraAttributes': self.createCustomExtraAttrs,
            'assetInfo': self.createAssetInfoSection,
            'metadata': self.createMetadataSection,
            'customCallbacks': self.createCustomCallbackSection,
        }
        sectionCreators = collections.defaultdict(
            lambda : self.createSection, customAttributes)
        
        # Create the section in the specified order.
        for typeName in schemasOrder:
            attrs = schemasAttributes[typeName]
            sectionName = self.sectionNameFromSchema(typeName)
            collapse = not isSectionOpen(sectionName)
            creator = sectionCreators[typeName]
            creator(sectionName, attrs, collapse)

            if sectionName == primTypeName:
                addMatSection()
        
        # In case there was neither a Mesh nor Xformable section, add material section now.
        addMatSection()        

    def createMaterialAttributeSection(self):
        if not UsdShade.MaterialBindingAPI.CanApply(self.prim):
            return
        matAPI = UsdShade.MaterialBindingAPI(self.prim)
        mat, _ = matAPI.ComputeBoundMaterial()
        if not mat:
            return
        layoutName = getMayaUsdLibString('kLabelMaterial')
        with ufeAeTemplate.Layout(self, layoutName, collapse=False):
            createdControl = MaterialCustomControl(self.item, self.prim, self.useNiceName)
            self.defineCustom(createdControl)

    def suppressArrayAttribute(self):
        # Suppress all array attributes.
        if not self.showArrayAttributes:
            for attrName in self.attrS.attributeNames:
                if ArrayCustomControl.isArrayAttribute(self, attrName):
                    self.suppress(attrName)


