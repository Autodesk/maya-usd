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

class CreateComponentFromUsdPrimsTestCase(_ComponentCreatorTestBase, unittest.TestCase):
    """
    Tests for usd_component_creator_plugin.create_component.create_component_from_usd_prims.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def setUp(self):
        self._setUpCC()

    def testEmptyNodesIsNoOp(self):
        """create_component_from_nodes([]) must not create a proxy shape or raise."""
        from usd_component_creator_plugin import create_component_from_usd_prims

        before = self._snapshotProxyShapes()
        create_component_from_usd_prims([])
        self.assertEqual(self._snapshotProxyShapes(), before,
                         "No proxy shape should be created for an empty node list")

    def testGeometryInStage(self):
        """The exported geometry should appear under /root/geo/<nodeName> in the stage."""
        from usd_component_creator_plugin import create_component_from_usd_prims
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        input_stage = mayaUsd.ufe.getStage(proxyShape)
        path = Sdf.Path('/pCube1')
        input_stage.DefinePrim(path, 'Cube')

        before = self._snapshotProxyShapes()
        create_component_from_usd_prims([f'{proxyShape},{str(path)}'])
        proxy = self._findNewProxyShape(before)
        self.assertIsNotNone(proxy)
        stage = mayaUsd.ufe.getStage(proxy)

        cube_prim = stage.GetPrimAtPath('/root/geo/pCube1')
        self.assertTrue(cube_prim.IsValid(),
                        "pCube1 prim should exist under /root/geo in the component stage")


    def testHasVariantSet(self):
        """The resulting component should have at least one variant set with one variant."""
        from usd_component_creator_plugin import create_component_from_usd_prims
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        input_stage = mayaUsd.ufe.getStage(proxyShape)
        path = Sdf.Path('/pCube1')
        input_stage.DefinePrim(path, 'Cube')

        before = self._snapshotProxyShapes()
        create_component_from_usd_prims([f'{proxyShape},{str(path)}'])
        proxy = self._findNewProxyShape(before)
        self.assertIsNotNone(proxy)
        stage = mayaUsd.ufe.getStage(proxy)
        desc = self._getDescFromStage(stage)
        self.assertIsNotNone(desc, 'Could not construct ComponentDescription from stage')
        variant_sets = desc.GetVariantSets()
        self.assertEqual(len(variant_sets), 1,
                           "Component must have one variant set")
        first_vs = next(iter(variant_sets.values()))
        self.assertEqual(len(first_vs.GetVariants()), 1,
                           "Variant set must have one variant")
        
        cube_prim = stage.GetPrimAtPath('/root/geo/pCube1')
        self.assertTrue(cube_prim.IsValid(),
                        "pCube1 prim should exist under /root/geo in the component stage")


    def testDefaultAndTargetVariantIsSet(self):
        """The first variant set must have a non-empty default_variant after creation."""
        from usd_component_creator_plugin import create_component_from_usd_prims
        import AdskUsdComponentCreator
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        input_stage = mayaUsd.ufe.getStage(proxyShape)
        cubePath = Sdf.Path('/pCube1')
        input_stage.DefinePrim(cubePath, 'Cube')
        conePath = Sdf.Path('/pCone1')
        input_stage.DefinePrim(conePath, 'Cone')

        before = self._snapshotProxyShapes()
        create_component_from_usd_prims([f'{proxyShape},{str(cubePath)}', f'{proxyShape},{str(conePath)}'])
        proxy = self._findNewProxyShape(before)
        self.assertIsNotNone(proxy)
        stage = mayaUsd.ufe.getStage(proxy)
        desc = self._getDescFromStage(stage)
        self.assertIsNotNone(desc, 'Could not construct ComponentDescription from stage')

        first_vs = next(iter(desc.GetVariantSets().values()))
        self.assertTrue(first_vs.default_variant,
                        "Variant set must have a default variant defined after creation")
        default_var_desc = first_vs.GetVariantDescription(first_vs.default_variant)
        self.assertIsNotNone(default_var_desc,
                             "GetVariantDescription must return a descriptor for the default variant")
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.IsVariantTarget(
                desc, [first_vs, default_var_desc]),
            "Default variant must be the targeted variant after create_component_from_nodes "
            "(target_default_variant=True)")
        
        cube_prim = stage.GetPrimAtPath('/root/geo/pCube1')
        self.assertTrue(cube_prim.IsValid(),
                        "pCube1 prim should exist under /root/geo in the component stage")

        cone_prim = stage.GetPrimAtPath('/root/geo/pCone1')
        self.assertTrue(cone_prim.IsValid(),
                        "pCone1 prim should exist under /root/geo in the component stage")


    def testPrimsWithSameNameFroMultiStages(self):
        """Crate from multiple prims with the same name."""
        from usd_component_creator_plugin import create_component_from_usd_prims
        import AdskUsdComponentCreator

        cubePath = Sdf.Path('/pCube1')

        proxyShape1 = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        input_stage1 = mayaUsd.ufe.getStage(proxyShape1)
        input_stage1.DefinePrim(cubePath, 'Cube')

        proxyShape2 = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        input_stage2 = mayaUsd.ufe.getStage(proxyShape2)
        input_stage2.DefinePrim(cubePath, 'Cube')

        before = self._snapshotProxyShapes()
        create_component_from_usd_prims([f'{proxyShape1},{str(cubePath)}', f'{proxyShape2},{str(cubePath)}'])
        proxy = self._findNewProxyShape(before)
        self.assertIsNotNone(proxy)
        stage = mayaUsd.ufe.getStage(proxy)
        desc = self._getDescFromStage(stage)
        self.assertIsNotNone(desc, 'Could not construct ComponentDescription from stage')

        first_vs = next(iter(desc.GetVariantSets().values()))
        self.assertTrue(first_vs.default_variant,
                        "Variant set must have a default variant defined after creation")
        default_var_desc = first_vs.GetVariantDescription(first_vs.default_variant)
        self.assertIsNotNone(default_var_desc,
                             "GetVariantDescription must return a descriptor for the default variant")
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.IsVariantTarget(
                desc, [first_vs, default_var_desc]),
            "Default variant must be the targeted variant after create_component_from_nodes "
            "(target_default_variant=True)")
        
        cube_prim = stage.GetPrimAtPath('/root/geo/pCube1')
        self.assertTrue(cube_prim.IsValid(),
                        "pCube1 prim should exist under /root/geo in the component stage")

        cube_2_prim = stage.GetPrimAtPath('/root/geo/pCube2')
        self.assertTrue(cube_2_prim.IsValid(),
                        "pCube2 prim should exist under /root/geo in the component stage")


if __name__ == '__main__':
    fixturesUtils.runTests(globals())

