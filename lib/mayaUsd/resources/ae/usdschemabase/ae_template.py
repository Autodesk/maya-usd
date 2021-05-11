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

import fnmatch
import ufe
import maya.mel as mel
import maya.cmds as cmds
import mayaUsd.ufe as mayaUsdUfe
import mayaUsd.lib as mayaUsdLib
import maya.internal.common.ufe_ae.template as ufeAeTemplate
from maya.common.ui import LayoutManager
from maya.common.ui import setClipboardData

# We manually import all the classes which have a 'GetSchemaAttributeNames'
# method so we have access to it and the 'pythonClass' method.
from pxr import Usd, UsdGeom, UsdLux, UsdRender, UsdRi, UsdShade, UsdSkel, UsdUI, UsdVol, Kind, Tf

def getPrettyName(name):
    # Put a space in the name when preceded by a capital letter.
    # Exceptions: Number followed by capital
    #             Multiple capital letters together
    prettyName = str(name[0])
    nbChars = len(name)
    for i in range(1, nbChars):
        if name[i].isupper() and not name[i-1].isdigit():
            if (i < (nbChars-1)) and not name[i+1].isupper():
                prettyName += ' '
            prettyName += name[i]
        elif name[i] == '_':
            continue
        else:
            prettyName += name[i]

    # Make each word start with an uppercase.
    return prettyName.title()

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
        if isinstance(notification, ufe.AttributeValueChanged):
            if notification.name() == UsdGeom.Tokens.xformOpOrder:
                mel.eval("evalDeferred -low \"refreshEditorTemplates\";")

    def onCreate(self, *args):
        ufe.Attributes.addObserver(self._item, self)

    def onReplace(self, *args):
        # Nothing needed here since we don't create any UI.
        pass

class MetaDataCustomControl(object):
    # Custom control for all prim metadata we want to display.
    def __init__(self, prim, useNiceName):
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
                                       ann='Kind is a type of metadata (a pre-loaded string value) used to classify prims in USD. Set the classification value from the dropdown to assign a kind category to a prim. Set a kind value to activate selection by kind.')

        for ele in knownKinds:
            cmds.menuItem(label=ele)

        # Metadata: Active
        self.active = cmds.checkBoxGrp(label='Active',
                                       ncb=1,
                                       cc1=self._onActiveChanged,
                                       ann='If selected, the prim is set to active and contributes to the composition of a stage. If a prim is set to inactive, it doesn’t contribute to the composition of a stage (it gets striked out in the Outliner and is deactivated from the Viewport).')

        # Metadata: Instanceable
        self.instan = cmds.checkBoxGrp(label='Instanceable',
                                       ncb=1,
                                       cc1=self._onInstanceableChanged,
                                       ann='If selected, instanceable is set to true for the prim and the prim is considered a candidate for instancing. If deselected, instanceable is set to false.')

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
                mdLabel = getPrettyName(k) if self.useNiceName else k
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
            self.prim.SetActive(value)

    def _onInstanceableChanged(self, value):
        with mayaUsdLib.UsdUndoBlock():
            self.prim.SetInstanceable(value)

# Custom control for all array attribute.
class ArrayCustomControl(object):

    def __init__(self, prim, attrName, useNiceName):
        self.prim = prim
        self.attrName = attrName
        self.useNiceName = useNiceName
        super(ArrayCustomControl, self).__init__()

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

            mdLabel = getPrettyName(self.attrName) if self.useNiceName else self.attrName
            cmds.textFieldGrp(editable=False, label=mdLabel, text=typeNameStr, annotation=attr.GetDocumentation())

            if hasValue:
                cmds.popupMenu()
                cmds.menuItem( label="Copy Attribute Value",   command=lambda *args: setClipboardData(str(values)) )
                cmds.menuItem( label="Print to Script Editor", command=lambda *args: print(str(values)) )
        else:
            cmds.error(self.attrName + " must be an array!")

    def onReplace(self, *args):
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


# SchemaBase template class for categorization of the attributes.
# We no longer use the base class ufeAeTemplate.Template as we want to control
# the placement of the metadata at the bottom (after extra attributes).
class AETemplate(object):
    '''
    This system of schema inherits groups attributes per schema helping the user
    learn how attributes are stored.
    '''
    def __init__(self, ufeSceneItem):
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

        if int(cmds.about(majorVersion=True)) > 2022:
            # Because of how we dynamically build the Transform attribute section,
            # we need this template to rebuild each time it is needed. This will
            # also restore the collapse/expand state of the sections.
            # Note: in Maya 2022 all UFE templates were forcefully rebuilt, but
            #       no restore of section states.
            try:
                cmds.editorTemplate(forceRebuild=True)
            except:
                pass

    def addControls(self, controls):
        for c in controls:
            if c not in self.suppressedAttrs:
                if self.isArrayAttribute(c):
                    arrayCustomControl = ArrayCustomControl(self.prim, c, self.useNiceName)
                    self.defineCustom(arrayCustomControl, c)
                else:
                    cmds.editorTemplate(addControl=[c])
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

        schemaTypeName = getPrettyName(schemaTypeName)

        # if the schema name ends with "api", replace it with
        # "API" and make sure to only replace the last occurence.
        # Example: "Shaping api" which contains "api" twice most become "Shaping API"
        if schemaTypeName.endswith("api"): 
            schemaTypeName = " API".join(schemaTypeName.rsplit("api", 1))

        return schemaTypeName

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
            with ufeAeTemplate.Layout(self, 'Transform Attributes'):
                attrsToAdd.remove(UsdGeom.Tokens.xformOpOrder)
                self.addControls(xformOpOrderNames)

                # Get the remainder of the xformOps and add them in an Unused section.
                xformOpUnusedNames = fnmatch.filter(allAttrs, 'xformOp:*')
                xformOpUnusedNames = [ele for ele in xformOpUnusedNames if ele not in xformOpOrderNames]
                self.createSection('Unused Transform Attributes', xformOpUnusedNames, collapse=True)

            # Then add any reamining Xformable attributes
            self.addControls(attrsToAdd)

            # Add a custom control for UFE attribute changed.
            t3dObs = UfeAttributesObserver(self.item)
            self.defineCustom(t3dObs)

    def createMetadataSection(self):
        # We don't use createSection() because these are metadata (not attributes).
        with ufeAeTemplate.Layout(self, 'Metadata', collapse=True):
            metaDataControl = MetaDataCustomControl(self.prim, self.useNiceName)
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
        # USD version 0.21.2 is required because of
        # Usd.SchemaRegistry().GetPropertyNamespacePrefix()
        if Usd.GetVersion() < (0, 21, 2):
            return

        showAppliedSchemasSection = False

        # loop on all applied schemas and store all those
        # schema into a dictionary with the attributes.
        # Storing the schema into a dictionary allow us to
        # group all instances of a MultipleApply schema together
        # so we can later display them into the same UI section.
        #
        # By example, if UsdCollectionAPI is applied twice, UsdPrim.GetAppliedSchemas()
        # will return ["CollectionAPI:instance1","CollectionAPI:instance2"] but we want to group
        # both instance inside a "CollectionAPI" section.
        #
        schemaAttrsDict = {}
        appliedSchemas = self.prim.GetAppliedSchemas()
        for schema in appliedSchemas:
            typeAndInstance = Usd.SchemaRegistry().GetTypeAndInstance(schema)
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
                    namespace = Usd.SchemaRegistry().GetPropertyNamespacePrefix(typeName)
                    prefix = namespace + ":" + instanceName + ":"
                    attrList = [prefix + i for i in attrList]

                    if typeName in schemaAttrsDict:
                        schemaAttrsDict[typeName] += attrList
                    else:
                        schemaAttrsDict[typeName] = attrList
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
            with ufeAeTemplate.Layout(self, 'Applied Schemas', collapse=True):
                for typeName, attrs in schemaAttrsDict.items():
                    typeName = self.sectionNameFromSchema(typeName)
                    self.createSection(typeName, attrs, False)


    def buildUI(self):
        usdSch = Usd.SchemaRegistry()

        self.suppressArrayAttribute()

        # We use UFE for the ancestor node types since it caches the
        # results by node type.
        for schemaType in self.item.ancestorNodeTypes():
            schemaType = usdSch.GetTypeFromName(schemaType)
            schemaTypeName = schemaType.typeName
            sectionName = self.sectionNameFromSchema(schemaTypeName)
            if schemaType.pythonClass:
                attrsToAdd = schemaType.pythonClass.GetSchemaAttributeNames(False)

                # We have a special case when building the Xformable section.
                if schemaTypeName == 'UsdGeomXformable':
                    self.createTransformAttributesSection(sectionName, attrsToAdd)
                else:
                    sectionsToCollapse = ['Curves', 'Point Based', 'Geometric Prim', 'Boundable',
                                          'Imageable', 'Field Asset', 'Light']
                    collapse = sectionName in sectionsToCollapse
                    self.createSection(sectionName, attrsToAdd, collapse)

    def suppressArrayAttribute(self):
        # Suppress all array attributes except UsdGeom.Tokens.xformOpOrder
        if not self.showArrayAttributes:
            for attrName in self.attrS.attributeNames:
                if attrName != UsdGeom.Tokens.xformOpOrder and self.isArrayAttribute(attrName):
                    self.suppress(attrName)

    def isArrayAttribute(self, attrName):
        if self.attrS.attributeType(attrName) == ufe.Attribute.kGeneric:
            attr = self.prim.GetAttribute(attrName)
            typeName = attr.GetTypeName()
            return typeName.isArray
        return False
