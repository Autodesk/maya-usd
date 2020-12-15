# This template file is no longer required for Maya PR 121.
# It will eventually be removed.
import mayaUtils
if mayaUtils.previewReleaseVersion() < 121:
    import ufe
    import ufe_ae.usd.nodes.base.ae_template as UfeAeBaseTemplate

    from pxr import UsdGeom

    # This class must be named "AETemplate"
    class AETemplate(UfeAeBaseTemplate.AEBaseTemplate):
        def __init__(self, ufeSceneItem):
            super(AETemplate, self).__init__(ufeSceneItem)

        def buildUI(self, ufeSceneItem):
            super(AETemplate, self).buildUI(ufeSceneItem)
