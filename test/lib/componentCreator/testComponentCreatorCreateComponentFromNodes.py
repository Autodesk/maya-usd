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

class CreateComponentFromNodesTestCase(_ComponentCreatorTestBase, unittest.TestCase):
    """
    Tests for usd_component_creator_plugin.create_component.create_component_from_nodes.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def setUp(self):
        self._setUpCC()

    def testEmptyNodesIsNoOp(self):
        """create_component_from_nodes([]) must not create a proxy shape or raise."""
        from usd_component_creator_plugin import create_component_from_nodes
        before = self._snapshotProxyShapes()
        create_component_from_nodes([])
        self.assertEqual(self._snapshotProxyShapes(), before,
                         "No proxy shape should be created for an empty node list")

    def testGeometryInStage(self):
        """The exported geometry should appear under /root/geo/<nodeName> in the stage."""
        from usd_component_creator_plugin import create_component_from_nodes
        cmds.polyCube(name='pCube1')
        path = cmds.ls('pCube1', long=True)[0]
        before = self._snapshotProxyShapes()
        create_component_from_nodes([path])
        proxy = self._findNewProxyShape(before)
        self.assertIsNotNone(proxy)
        stage = mayaUsd.ufe.getStage(proxy)
        geo_prim = stage.GetPrimAtPath('/root/geo/pCube1')
        self.assertTrue(geo_prim.IsValid(),
                        "pCube1 prim should exist under /root/geo in the component stage")

    def testHasVariantSet(self):
        """The resulting component should have at least one variant set with one variant."""
        from usd_component_creator_plugin import create_component_from_nodes
        cmds.polyCube(name='pCube1')
        path = cmds.ls('pCube1', long=True)[0]
        before = self._snapshotProxyShapes()
        create_component_from_nodes([path])
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

    def testDefaultAndTargetVariantIsSet(self):
        """The first variant set must have a non-empty default_variant after creation."""
        from usd_component_creator_plugin import create_component_from_nodes
        import AdskUsdComponentCreator
        cmds.polyCube(name='pCube1')
        path = cmds.ls('pCube1', long=True)[0]
        before = self._snapshotProxyShapes()
        create_component_from_nodes([path])
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



if __name__ == '__main__':
    fixturesUtils.runTests(globals())

