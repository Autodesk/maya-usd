import fnmatch
import ufe
import mayaUsd.ufe
import maya.internal.common.ufe_ae.template as ufeAeTemplate

from pxr import UsdGeom, UsdShade

# Base template class that can be used by other templates.
class AEBaseTemplate(ufeAeTemplate.Template):
    def __init__(self, ufeSceneItem):
        # Get the UFE Attributes interface for this scene item.
        self.attrS = ufe.Attributes.attributes(ufeSceneItem)
        super(AEBaseTemplate, self).__init__(ufeSceneItem)

    def createSection(self, layoutName, attrList, collapse=False):
        # We create the section named "layoutName" if at least one
        # of the attributes from the input list exists.
        for attr in attrList:
            if self.attrS.hasAttribute(attr):
                with ufeAeTemplate.Layout(self, layoutName, collapse):
                    # Note: addControls will silently skip any attributes
                    #       which does not exist.
                    self.addControls(attrList)
                return

    def createTransformAttributesSection(self):
        # Get the xformOp order and add those attributes (in order)
        # followed by the xformOp order attribute.
        prim = mayaUsd.ufe.getPrimFromRawItem(self.item.getRawAddress())
        geomX = UsdGeom.Xformable(prim)
        xformOps = geomX.GetOrderedXformOps()
        xformOpOrderNames = [op.GetOpName() for op in xformOps]
        xformOpOrderNames.append(UsdGeom.Tokens.xformOpOrder)
        # Don't use createSection because we want a sub-section for Unused.
        with ufeAeTemplate.Layout(self, 'Transform Attributes'):
            self.addControls(xformOpOrderNames)

            # Get the remainder of the xformOps and add them in an Unused section.
            allAttrs = self.attrS.attributeNames
            xformOpUnusedNames = fnmatch.filter(allAttrs, 'xformOp:*')
            xformOpUnusedNames = [ele for ele in xformOpUnusedNames if ele not in xformOpOrderNames]
            self.createSection('Unused Transform Attributes', xformOpUnusedNames, collapse=True)

    def buildUI(self, ufeSceneItem):
        # Create all the various sections.
        # TODO: translate the section names.
        self.createSection('Prim',
            [UsdGeom.Tokens.proxyPrim,
            UsdGeom.Tokens.purpose,
            UsdGeom.Tokens.extent,
            UsdShade.Tokens.infoId,
            UsdShade.Tokens.infoImplementationSource]
        )

        self.createTransformAttributesSection()

        self.createSection('Mesh Component Display',
            [UsdGeom.Tokens.doubleSided,
            UsdGeom.Tokens.primvarsDisplayColor,
            UsdGeom.Tokens.primvarsDisplayOpacity,
            UsdGeom.Tokens.orientation,
            UsdGeom.Tokens.faceVaryingLinearInterpolation]
        )

        self.createSection('Display',
            [UsdGeom.Tokens.visibility]
        )

        self.createSection('Smooth Mesh',
            [UsdGeom.Tokens.cornerIndices,
            UsdGeom.Tokens.cornerSharpnesses,
            UsdGeom.Tokens.creaseIndices,
            UsdGeom.Tokens.creaseLengths,
            UsdGeom.Tokens.creaseSharpnesses,
            UsdGeom.Tokens.interpolateBoundary]
        )

        self.createSection('Open Subdiv Controls',
            [UsdGeom.Tokens.subdivisionScheme,
            UsdGeom.Tokens.triangleSubdivisionRule]
        )

        self.createSection('Components',
            [UsdGeom.Tokens.points, 
            UsdGeom.Tokens.normals,
            UsdGeom.Tokens.faceVertexCounts,
            UsdGeom.Tokens.faceVertexIndices,
            UsdGeom.Tokens.holeIndices,
            UsdGeom.Tokens.accelerations,
            UsdGeom.Tokens.velocities]
        )

        self.createSection('Common Material Attributes',
            [UsdShade.Tokens.outputsSurface,
            UsdShade.Tokens.outputsVolume,
            UsdShade.Tokens.outputsDisplacement,
            'inputs:diffuseColor',
            'inputs:emissiveColor']
        )

        self.createSection('Camera Attributes',
            [UsdGeom.Tokens.projection,
            UsdGeom.Tokens.focalLength,
            UsdGeom.Tokens.clippingPlanes,
            UsdGeom.Tokens.clippingRange]
        )

        self.createSection('Film Back',
            [UsdGeom.Tokens.horizontalAperture,
            UsdGeom.Tokens.horizontalApertureOffset,
            UsdGeom.Tokens.verticalAperture,
            UsdGeom.Tokens.verticalApertureOffset]
        )

        self.createSection('Depth of Field',
            [UsdGeom.Tokens.focusDistance,
            UsdGeom.Tokens.fStop]
        )
