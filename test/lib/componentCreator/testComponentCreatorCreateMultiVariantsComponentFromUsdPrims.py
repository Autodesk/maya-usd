import unittest

import fixturesUtils
import mayaUsd.ufe
import mayaUsd_createStageWithNewLayer
from pxr import Sdf

from testComponentCreatorBase import _ComponentCreatorTestBase

class CreateMultiVariantsComponentFromUsdPrimsTestCase(_ComponentCreatorTestBase, unittest.TestCase):
    """
    Tests for create_multi_variants_component_from_usd_prims.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def setUp(self):
        self._setUpCC()

    def testEmptyNodesIsNoOp(self):
        """create_multi_variants_component_from_nodes([]) must not create any proxy shape."""
        from usd_component_creator_plugin import create_multi_variants_component_from_usd_prims
        before = self._snapshotProxyShapes()

        create_multi_variants_component_from_usd_prims([])
        self.assertEqual(self._snapshotProxyShapes(), before,
                         "No proxy shape should be created for an empty node list")


    def testVariantCount(self):
        """Two input nodes should produce exactly two variants in the first variant set."""
        from usd_component_creator_plugin import create_multi_variants_component_from_usd_prims
        import AdskUsdComponentCreator

        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        input_stage = mayaUsd.ufe.getStage(proxyShape)
        cubePath = Sdf.Path('/pCube1')
        input_stage.DefinePrim(cubePath, 'Cube')
        conePath = Sdf.Path('/pCone1')
        input_stage.DefinePrim(conePath, 'Cone')

        paths = [f'{proxyShape},{str(cubePath)}', f'{proxyShape},{str(conePath)}']

        before = self._snapshotProxyShapes()
        create_multi_variants_component_from_usd_prims(paths)
        proxy = self._findNewProxyShape(before)
        self.assertIsNotNone(proxy)
        stage = mayaUsd.ufe.getStage(proxy)
        desc = self._getDescFromStage(stage)
        self.assertIsNotNone(desc, 'Could not construct ComponentDescription from stage')
        first_vs = next(iter(desc.GetVariantSets().values()))
        self.assertEqual(len(first_vs.GetVariants()), 2,
                         "Two nodes must produce exactly two variants")

        # Note: each prim is in a separate variant, so only one can be active at a time.
        #       The default is the alphabetically first variant, which corresponds to conePath,
        #       so we expect to see pCone1 in the stage and not pCube1.
        self.assertFalse(stage.GetPrimAtPath('/root/geo/pCube1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/pCone1').IsValid())

        AdskUsdComponentCreator.ComponentAPI.SetVariantTemporarySelection(desc, [first_vs, first_vs.GetVariantDescription('pCube1')], True)
        self.assertTrue(stage.GetPrimAtPath('/root/geo/pCube1').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/pCone1').IsValid())


    def testTargetedVariantIsDefault(self):
        """After creation the targeted variant must be the default variant."""
        import AdskUsdComponentCreator
        from usd_component_creator_plugin import create_multi_variants_component_from_usd_prims

        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        input_stage = mayaUsd.ufe.getStage(proxyShape)
        cubePath = Sdf.Path('/pCube1')
        input_stage.DefinePrim(cubePath, 'Cube')
        conePath = Sdf.Path('/pCone1')
        input_stage.DefinePrim(conePath, 'Cone')

        paths = [f'{proxyShape},{str(cubePath)}', f'{proxyShape},{str(conePath)}']

        before = self._snapshotProxyShapes()
        create_multi_variants_component_from_usd_prims(paths)
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

        stage = mayaUsd.ufe.getStage(proxy)

        # Note: each prim is in a separate variant, so only one can be active at a time.
        #       The default is the alphabetically first variant, which corresponds to conePath,
        #       so we expect to see pCone1 in the stage and not pCube1.
        self.assertFalse(stage.GetPrimAtPath('/root/geo/pCube1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/pCone1').IsValid())

        AdskUsdComponentCreator.ComponentAPI.SetVariantTemporarySelection(desc, [first_vs, first_vs.GetVariantDescription('pCube1')], True)
        self.assertTrue(stage.GetPrimAtPath('/root/geo/pCube1').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/pCone1').IsValid())


    def testDefaultIsAlphabeticallyFirst(self):
        """With three nodes the alphabetically-first name should be the default variant."""
        import AdskUsdComponentCreator
        from usd_component_creator_plugin import create_multi_variants_component_from_usd_prims

        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        input_stage = mayaUsd.ufe.getStage(proxyShape)
        cubePath = Sdf.Path('/MangoNode')
        input_stage.DefinePrim(cubePath, 'Cube')
        conePath = Sdf.Path('/AvocadoNode')
        input_stage.DefinePrim(conePath, 'Cone')
        kiwiPath = Sdf.Path('/KiwiNode')
        input_stage.DefinePrim(kiwiPath, 'Cube')

        paths = [f'{proxyShape},{str(cubePath)}', f'{proxyShape},{str(conePath)}', f'{proxyShape},{str(kiwiPath)}']

        before = self._snapshotProxyShapes()
        create_multi_variants_component_from_usd_prims(paths)
        proxy = self._findNewProxyShape(before)
        self.assertIsNotNone(proxy)
        stage = mayaUsd.ufe.getStage(proxy)
        desc = self._getDescFromStage(stage)
        self.assertIsNotNone(desc, 'Could not construct ComponentDescription from stage')

        first_vs = next(iter(desc.GetVariantSets().values()))
        self.assertEqual(len(first_vs.GetVariants()), 3, "Expected three variants from three nodes")

        opts = AdskUsdComponentCreator.Options()
        opts.Validate()
        expected_name = AdskUsdComponentCreator.GenerateVariantNameFromObjectName(
            opts, 'AvocadoNode', ['AvocadoNode'], [])
        self.assertEqual(first_vs.default_variant, expected_name,
                         "AvocadoNode is alphabetically first and must be the default variant")

        stage = mayaUsd.ufe.getStage(proxy)
        
        # Note: each prim is in a separate variant, so only one can be active at a time.
        self.assertTrue(stage.GetPrimAtPath('/root/geo/AvocadoNode').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/KiwiNode').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/MangoNode').IsValid())

        AdskUsdComponentCreator.ComponentAPI.SetVariantTemporarySelection(desc, [first_vs, first_vs.GetVariantDescription('KiwiNode')], True)
        self.assertTrue(stage.GetPrimAtPath('/root/geo/KiwiNode').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/AvocadoNode').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/MangoNode').IsValid())

        AdskUsdComponentCreator.ComponentAPI.SetVariantTemporarySelection(desc, [first_vs, first_vs.GetVariantDescription('MangoNode')], True)
        self.assertTrue(stage.GetPrimAtPath('/root/geo/MangoNode').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/AvocadoNode').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/KiwiNode').IsValid())


    def testPrimsWithSameNameFromMultiStages(self):
        """Prims with the same name from multiple stages result in one of them being renamed."""
        import AdskUsdComponentCreator
        from usd_component_creator_plugin import create_multi_variants_component_from_usd_prims

        cubePath = Sdf.Path('/pCube1')

        proxyShape1 = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        input_stage1 = mayaUsd.ufe.getStage(proxyShape1)
        input_stage1.DefinePrim(cubePath, 'Cube')

        proxyShape2 = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        input_stage2 = mayaUsd.ufe.getStage(proxyShape2)
        input_stage2.DefinePrim(cubePath, 'Cube')

        paths = [f'{proxyShape1},{str(cubePath)}', f'{proxyShape2},{str(cubePath)}']

        before = self._snapshotProxyShapes()
        create_multi_variants_component_from_usd_prims(paths)
        proxy = self._findNewProxyShape(before)
        self.assertIsNotNone(proxy)

        desc = self._getActiveDesc()
        if desc is None:
            stage = mayaUsd.ufe.getStage(proxy)
            desc = self._getDescFromStage(stage)
        self.assertIsNotNone(desc, 'Could not obtain ComponentDescription')

        self.assertEqual(len(desc.GetVariantSets()), 1)
        first_vs = next(iter(desc.GetVariantSets().values()))
        
        # TODO FIXME: this requires a new component creator, will enable after integration of 0.3.7

        # self.assertEqual(len(first_vs.GetVariants()), 2, "Expected two variants from two nodes")

        # stage = mayaUsd.ufe.getStage(proxy)

        # self.maxDiff = None
        # self.assertEqual(stage.GetRootLayer().ExportToString(), "")

        # # Prim added second should be renamed to avoid name collision.
        # self.assertTrue(stage.GetPrimAtPath('/root/geo/pCube1').IsValid())
        # self.assertFalse(stage.GetPrimAtPath('/root/geo/pCube2').IsValid())

        # AdskUsdComponentCreator.ComponentAPI.SetVariantTemporarySelection(desc, [first_vs, first_vs.GetVariantDescription('pCube2')], True)
        # self.assertTrue(stage.GetPrimAtPath('/root/geo/pCube2').IsValid())
        # self.assertFalse(stage.GetPrimAtPath('/root/geo/pCube1').IsValid())


if __name__ == '__main__':
    fixturesUtils.runTests(globals())

