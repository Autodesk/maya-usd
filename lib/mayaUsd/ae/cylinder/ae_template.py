import ufe
import ufe_ae.usd.nodes.base.ae_template as UfeAeBaseTemplate

from pxr import UsdGeom

# This class must be named "AETemplate"
class AETemplate(UfeAeBaseTemplate.AEBaseTemplate):
    def __init__(self, ufeSceneItem):
        super(AETemplate, self).__init__(ufeSceneItem)

    def buildUI(self, ufeSceneItem):
        super(AETemplate, self).buildUI(ufeSceneItem)
