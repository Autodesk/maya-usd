import ufe
import maya.internal.common.ufe_ae.template as ufeAeTemplate

from pxr import UsdGeom

# This class must be named "AETemplate"
class AETemplate(ufeAeTemplate.Template):
    def __init__(self, ufeSceneItem):
        super(AETemplate, self).__init__(ufeSceneItem)

    def buildUI(self, ufeSceneItem):
        attrS = ufe.Attributes.attributes(ufeSceneItem)
        if attrS.hasAttribute(UsdGeom.Tokens.visibility):
            with ufeAeTemplate.Layout(self, 'Display'):
                self.addControl(UsdGeom.Tokens.visibility)

        if attrS.hasAttribute(UsdGeom.Tokens.xformOpOrder):
            with ufeAeTemplate.Layout(self, 'XformOp'):
                self.addControl(UsdGeom.Tokens.xformOpOrder)
