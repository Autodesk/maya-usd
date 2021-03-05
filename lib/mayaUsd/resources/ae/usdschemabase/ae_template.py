import fnmatch
import ufe
import maya.mel as mel
import maya.cmds as cmds
import mayaUsd.ufe
import maya.internal.common.ufe_ae.template as ufeAeTemplate

# We manually import all the classes which have a 'GetSchemaAttributeNames'
# method so we have access to it and the 'pythonClass' method.
from pxr import Usd, UsdGeom, UsdLux, UsdRender, UsdRi, UsdShade, UsdSkel, UsdUI, UsdVol


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
            if notification.name() == "xformOpOrder":
                mel.eval("evalDeferred(\"AEbuildControls\");")

    def onCreate(self, *args):
        ufe.Attributes.addObserver(self._item, self)

    def onReplace(self, *args):
        pass


# SchemaBase template class for categorization of the attributes.
class AETemplate(ufeAeTemplate.Template):
    '''
    This system of schema inherits groups attributes per schema helping the user
    learn how attributes are stored.
    '''
    def __init__(self, ufeSceneItem):
        # Get the UFE Attributes interface for this scene item.
        self.attrS = ufe.Attributes.attributes(ufeSceneItem)

        # And finally call our base class init which will in turn
        # build the UI.
        super(AETemplate, self).__init__(ufeSceneItem)

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

        # Put a space in the name when preceded by a capital letter.
        # Exceptions: Number followed by capital
        #             Multiple capital letters together
        catName = str(schemaTypeName[0])
        nbChars = len(schemaTypeName)
        for i in range(1, nbChars):
            if schemaTypeName[i].isupper() and not schemaTypeName[i-1].isdigit():
                if (i < (nbChars-1)) and not schemaTypeName[i+1].isupper():
                    catName = catName + str(' ') + str(schemaTypeName[i])
                else:
                    catName = catName + str(schemaTypeName[i])
            elif schemaTypeName[i] == '_':
                continue
            else:
                catName = catName + str(schemaTypeName[i])

        # Make each word start with an uppercase.
        catName = catName.title()

        return catName

    def createTransformAttributesSection(self, ufeSceneItem, sectionName, attrsToAdd):
        # Get the xformOp order and add those attributes (in order)
        # followed by the xformOp order attribute.
        allAttrs = self.attrS.attributeNames
        prim = mayaUsd.ufe.getPrimFromRawItem(ufeSceneItem.getRawAddress())
        geomX = UsdGeom.Xformable(prim)
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
            self.addUfeTransform3dObserver()

    def buildUI(self, ufeSceneItem):
        usdSch = Usd.SchemaRegistry()

        # We use UFE for the ancestor node types since it caches the
        # results by node type.
        for schemaType in ufeSceneItem.ancestorNodeTypes():
            schemaType = usdSch.GetTypeFromName(schemaType)
            schemaTypeName = schemaType.typeName
            sectionName = self.sectionNameFromSchema(schemaTypeName)
            if schemaType.pythonClass:
                attrsToAdd = schemaType.pythonClass.GetSchemaAttributeNames(False)

                # We have a special case when building the Xformable section.
                if schemaTypeName == 'UsdGeomXformable':
                    self.createTransformAttributesSection(ufeSceneItem, sectionName, attrsToAdd)
                else:
                    self.createSection(sectionName, attrsToAdd)

    def addUfeTransform3dObserver(self):
        t3dObs = UfeAttributesObserver(self.item)
        create = lambda *args : t3dObs.onCreate(args)
        replace = lambda *args : t3dObs.onReplace(args)
        cmds.editorTemplate([], callCustom=[create, replace])
