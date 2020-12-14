# This template file is no longer required for Maya PR 121.
# It will eventually be removed.
import mayaUtils
if mayaUtils.previewReleaseVersion() < 121:
    import ufe_ae.usd.nodes.usdschemabase.ae_template as UsdSchemaBaseAETemplate

    # Base template class that can be used by other templates.
    # Nov 2020: New schema base template. Eventually we will
    #           remove the specific node and this base template.
    class AEBaseTemplate(UsdSchemaBaseAETemplate.AETemplate):
        def __init__(self, ufeSceneItem):
            super(AEBaseTemplate, self).__init__(ufeSceneItem)

        def buildUI(self, ufeSceneItem):
            super(AEBaseTemplate, self).buildUI(ufeSceneItem)
