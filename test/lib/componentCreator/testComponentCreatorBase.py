import mayaUtils
import tempfile
from maya import cmds

from testComponentCreatorUtils import _CC_AVAILABLE, _HAVE_CC_MAYA_PLUGIN

class _ComponentCreatorTestBase:
    """Shared helpers for component creator test cases."""

    def _setUpCC(self):
        """Load required plugins for CC tests.  Call from setUp()."""
        if not _CC_AVAILABLE:
            self.fail('Could not find the USD component creator plugin')
        mayaUtils.loadPlugin('mayaUsdPlugin')
        if _HAVE_CC_MAYA_PLUGIN:
            mayaUtils.loadPlugin('usd_component_creator')
        else:
            # See comment in LoadComponentCreatorPluginTestCase.testPluginLoadable
            cmds.flushIdleQueue(resume=True)
            cmds.flushIdleQueue()
        cmds.file(new=True, force=True)

    def _snapshotProxyShapes(self):
        """Return the set of all mayaUsdProxyShape node paths currently in the scene."""
        return set(cmds.ls(type='mayaUsdProxyShape', long=True) or [])

    def _findNewProxyShape(self, before):
        """Return the first proxy shape that was added since *before* was captured."""
        new = self._snapshotProxyShapes() - before
        return list(new)[0] if new else None

    def _getActiveDesc(self):
        """Return the ComponentDescription currently shown in the variant editor, or None."""
        from usd_component_creator_plugin import get_variant_editor_component_description
        return get_variant_editor_component_description()

    def _getDescFromStage(self, stage):
        """Create a ComponentDescription from a USD stage's metadata."""
        import AdskUsdComponentCreator
        return AdskUsdComponentCreator.ComponentDescription.CreateFromStageMetadata(stage)

    def _createComponent(self):
        """Create a new Adsk USD Component with a `model/default` variant and a
        proxy shape pointing to its root layer.

        Returns (proxyShapePath, ComponentDescription).
        """
        import AdskUsdComponentCreator

        tempDir = tempfile.mkdtemp()
        opts = AdskUsdComponentCreator.Options()
        opts.component_folder = tempDir
        opts.component_name = 'TestComp'
        opts.component_filename = 'TestComp'
        opts.file_extension = 'usda'

        info = AdskUsdComponentCreator.ComponentAPI.CreateFromNothing(opts)
        desc = AdskUsdComponentCreator.ComponentDescription.CreateFromInfo(info)
        vsDesc = AdskUsdComponentCreator.ComponentAPI.AddVariantSet(desc, [], 'model')
        AdskUsdComponentCreator.ComponentAPI.AddVariant(desc, [], vsDesc, 'default')

        rootLayerPath = info.stage.GetRootLayer().realPath

        transform = cmds.createNode('transform', name='TestComp')
        shape = cmds.createNode('mayaUsdProxyShape', name='TestCompShape', parent=transform)
        cmds.setAttr(shape + '.filePath', rootLayerPath, type='string')

        proxyShapePath = cmds.ls(shape, long=True)[0]
        return proxyShapePath, desc

