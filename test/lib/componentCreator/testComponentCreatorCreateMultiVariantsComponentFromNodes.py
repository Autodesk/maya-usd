import unittest

import fixturesUtils
import mayaUsd.lib
import mayaUsd.ufe
import mayaUsd_createStageWithNewLayer
from maya import cmds
from pxr import Sdf, Usd
import ufe
from maya.internal.ufeSupport import ufeCmdWrapper as ufeCmd

from testComponentCreatorBase import _ComponentCreatorTestBase

class CreateMultiVariantsComponentFromNodesTestCase(_ComponentCreatorTestBase, unittest.TestCase):
    """
    Tests for create_multi_variants_component_from_nodes.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def setUp(self):
        self._setUpCC()

    def testEmptyNodesIsNoOp(self):
        """create_multi_variants_component_from_nodes([]) must not create any proxy shape."""
        from usd_component_creator_plugin import create_multi_variants_component_from_nodes
        before = self._snapshotProxyShapes()
        create_multi_variants_component_from_nodes([])
        self.assertEqual(self._snapshotProxyShapes(), before,
                         "No proxy shape should be created for an empty node list")

    def testVariantCount(self):
        """Two input nodes should produce exactly two variants in the first variant set."""
        from usd_component_creator_plugin import create_multi_variants_component_from_nodes
        cmds.polyCube(name='pCubeA')
        cmds.polyCube(name='pCubeB')
        paths = [cmds.ls(n, long=True)[0] for n in ('pCubeA', 'pCubeB')]
        before = self._snapshotProxyShapes()
        create_multi_variants_component_from_nodes(paths)
        proxy = self._findNewProxyShape(before)
        self.assertIsNotNone(proxy)
        stage = mayaUsd.ufe.getStage(proxy)
        desc = self._getDescFromStage(stage)
        self.assertIsNotNone(desc, 'Could not construct ComponentDescription from stage')
        first_vs = next(iter(desc.GetVariantSets().values()))
        self.assertEqual(len(first_vs.GetVariants()), 2,
                         "Two nodes must produce exactly two variants")

    def testTargetedVariantIsDefault(self):
        """After creation the targeted variant must be the default variant."""
        import AdskUsdComponentCreator
        from usd_component_creator_plugin import create_multi_variants_component_from_nodes
        cmds.polyCube(name='ZebraNode')
        cmds.polyCube(name='AppleNode')
        paths = [cmds.ls(n, long=True)[0] for n in ('ZebraNode', 'AppleNode')]
        before = self._snapshotProxyShapes()
        create_multi_variants_component_from_nodes(paths)
        proxy = self._findNewProxyShape(before)
        self.assertIsNotNone(proxy)

        desc = self._getActiveDesc()
        if desc is None:
            stage = mayaUsd.ufe.getStage(proxy)
            desc = self._getDescFromStage(stage)
        self.assertIsNotNone(desc, 'Could not obtain ComponentDescription')

        first_vs = next(iter(desc.GetVariantSets().values()))
        default_name = first_vs.default_variant
        self.assertTrue(default_name)
        default_var_desc = first_vs.GetVariantDescription(default_name)
        self.assertIsNotNone(default_var_desc)
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.IsVariantTarget(
                desc, [first_vs, default_var_desc]),
            "Default variant must be targeted after create_multi_variants_component_from_nodes")

    def testDefaultIsAlphabeticallyFirst(self):
        """With three nodes the alphabetically-first name should be the default variant."""
        import AdskUsdComponentCreator
        from usd_component_creator_plugin import create_multi_variants_component_from_nodes
        for name in ('MangoNode', 'AvocadoNode', 'KiwiNode'):
            cmds.polyCube(name=name)
        paths = [cmds.ls(n, long=True)[0] for n in ('MangoNode', 'AvocadoNode', 'KiwiNode')]
        before = self._snapshotProxyShapes()
        create_multi_variants_component_from_nodes(paths)
        proxy = self._findNewProxyShape(before)
        self.assertIsNotNone(proxy)
        stage = mayaUsd.ufe.getStage(proxy)
        desc = self._getDescFromStage(stage)
        self.assertIsNotNone(desc, 'Could not construct ComponentDescription from stage')

        first_vs = next(iter(desc.GetVariantSets().values()))
        self.assertEqual(len(first_vs.GetVariants()), 3, "Three nodes → three variants")

        opts = AdskUsdComponentCreator.Options()
        opts.Validate()
        expected_name = AdskUsdComponentCreator.GenerateVariantNameFromObjectName(
            opts, 'AvocadoNode', ['AvocadoNode'], [])
        self.assertEqual(first_vs.default_variant, expected_name,
                         "AvocadoNode is alphabetically first and must be the default variant")



if __name__ == '__main__':
    fixturesUtils.runTests(globals())

