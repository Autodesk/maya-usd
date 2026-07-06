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

class AddToComponentFromUsdPrimsTestCase(_ComponentCreatorTestBase, unittest.TestCase):
    """
    Tests for usd_component_creator_plugin.create_component.add_to_component_from_usd_prims.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def setUp(self):
        self._setUpCC()
        # Clear the variant editor state so there is no lingering component from a
        # previous test.
        #from usd_component_creator_plugin import update_variant_editor_window
        #update_variant_editor_window(None, force_update=True)

    def _createInitialComponent(self, node_name='pCube1'):
        """Create a polyCube, build a single-node component, and return (proxy, desc)."""
        from usd_component_creator_plugin import create_component_from_usd_prims

        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        self._input_stage = mayaUsd.ufe.getStage(proxyShape)
        path = Sdf.Path(f'/{node_name}')
        self._input_stage.DefinePrim(path, 'Cube')

        before = self._snapshotProxyShapes()
        create_component_from_usd_prims([f'{proxyShape},{path}'])
        proxy = self._findNewProxyShape(before)
        desc = self._getActiveDesc()
        return proxy, desc, proxyShape

    def testNoComponentDescReturnsFalse(self):
        """add_to_component_from_usd_prims with no desc and empty variant editor returns False."""
        from usd_component_creator_plugin import add_to_component_from_usd_prims
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        self._input_stage = mayaUsd.ufe.getStage(proxyShape)
        path = Sdf.Path('/Cube1')
        self._input_stage.DefinePrim(path, 'Cube')
        result = add_to_component_from_usd_prims([f'{proxyShape},{path}'], [('model', 'extra')],
                                             is_replacing=False, component_desc=None)
        self.assertFalse(result,
                         "add_to_component_from_usd_prims must return False when no component "
                         "description is available")

    def testSanityReturnsTrueNoNewProxy(self):
        """add_to_component_from_usd_prims returns True and reuses the existing proxy shape."""
        from usd_component_creator_plugin import add_to_component_from_usd_prims
        proxy, desc, proxyShape = self._createInitialComponent('pCube1')
        self.assertIsNotNone(desc, 'Could not get initial ComponentDescription')
        first_vs = next(iter(desc.GetVariantSets().values()))

        path2 = Sdf.Path('/pCube2')
        self._input_stage.DefinePrim(path2, 'Cube')
        before = self._snapshotProxyShapes()

        # Add pCube2 to the existing default variant (not a new one)
        result = add_to_component_from_usd_prims(
            [f'{proxyShape},{path2}'],
            [(first_vs.name, first_vs.default_variant)],
            is_replacing=False,
            component_desc=desc)
        self.assertTrue(result, "add_to_component_from_usd_prims should return True on success")
        new_proxy = self._findNewProxyShape(before)
        self.assertIsNone(new_proxy, "No new proxy shape should be created when adding to an existing component")

    def testAddWithPurpose(self):
        """add_to_component_from_usd_prims with purpose specified."""
        from usd_component_creator_plugin import add_to_component_from_usd_prims
        proxy, desc, proxyShape = self._createInitialComponent('pCube1')
        self.assertIsNotNone(desc, 'Could not get initial ComponentDescription')
        first_vs = next(iter(desc.GetVariantSets().values()))

        path2 = Sdf.Path('/pCube2')
        self._input_stage.DefinePrim(path2, 'Cube')
        before = self._snapshotProxyShapes()

        # Add pCube2 to the existing default variant (not a new one)
        result = add_to_component_from_usd_prims(
            [f'{proxyShape},{path2}'],
            [(first_vs.name, first_vs.default_variant)],
            is_replacing=False,
            component_desc=desc,
            purpose='guide')
        self.assertTrue(result, "add_to_component_from_usd_prims should return True on success")
        new_proxy = self._findNewProxyShape(before)
        self.assertIsNone(new_proxy, "No new proxy shape should be created when adding to an existing component")

        stage = mayaUsd.ufe.getStage(proxy)

        cube_path = Sdf.Path('/root/geo/pCube1')
        cube_prim = stage.GetPrimAtPath(cube_path)        
        self.assertTrue(cube_prim.IsValid(), f"{cube_path} should be a valid prim")

        purposed_path = Sdf.Path('/root/geo/guide/pCube2')
        purposed_prim = stage.GetPrimAtPath(purposed_path)        
        self.assertTrue(purposed_prim.IsValid(), f"{purposed_path} should be a valid prim after add_to_component_from_usd_prims with purpose")

    def testDefaultVariantUnchanged(self):
        """Adding a node to an existing variant must not change the default_variant name."""
        from usd_component_creator_plugin import add_to_component_from_usd_prims
        proxy, desc, proxyShape = self._createInitialComponent('pCube1')
        self.assertIsNotNone(desc, 'Could not get initial ComponentDescription')
        first_vs = next(iter(desc.GetVariantSets().values()))
        original_default = first_vs.default_variant
        self.assertTrue(original_default, "There must be a default variant before the add")

        path2 = Sdf.Path('/pCube2')
        self._input_stage.DefinePrim(path2, 'Cube')

        result = add_to_component_from_usd_prims(
            [f'{proxyShape},{path2}'],
            [(first_vs.name, original_default)],
            is_replacing=False,
            component_desc=desc)
        self.assertTrue(result)

        updated_desc = self._getActiveDesc()
        self.assertIsNotNone(updated_desc, 'Could not get updated ComponentDescription')
        updated_vs = next(iter(updated_desc.GetVariantSets().values()))
        self.assertEqual(updated_vs.default_variant, original_default,
                         "default_variant must not change after add_to_component_from_usd_prims "
                         "(is_default_variant=False)")

    def testDefaultVariantUnchangedWhenAddingToNonDefault(self):
        """Adding a node to a non-default variant must not change the default_variant name."""
        import AdskUsdComponentCreator
        from usd_component_creator_plugin import (
            add_to_component_from_usd_prims,
            create_multi_variants_component_from_usd_prims,
        )
        # Create a 2-variant component: 'AppleNode' is alphabetically first -> default variant.
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        self._input_stage = mayaUsd.ufe.getStage(proxyShape)
        
        cubePath = Sdf.Path('/ZebraNode')
        self._input_stage.DefinePrim(cubePath, 'Cube')
        conePath = Sdf.Path('/AppleNode')
        self._input_stage.DefinePrim(conePath, 'Cone')

        paths = [f'{proxyShape},{cubePath}', f'{proxyShape},{conePath}']

        before = self._snapshotProxyShapes()
        create_multi_variants_component_from_usd_prims(paths)
        proxy = self._findNewProxyShape(before)
        self.assertIsNotNone(proxy)
        desc = self._getActiveDesc()
        self.assertIsNotNone(desc, 'Could not get ComponentDescription after multi-variant create')

        first_vs = next(iter(desc.GetVariantSets().values()))
        original_default = first_vs.default_variant
        self.assertTrue(original_default, "There must be a default variant")

        # Pick the non-default variant to add to
        non_default_name = next(
            n for n in first_vs.GetVariants().keys() if n != original_default)

        path_extra = Sdf.Path('/CubeExtra')
        self._input_stage.DefinePrim(path_extra, 'Cube')
 
        result = add_to_component_from_usd_prims(
            [f'{proxyShape},{path_extra}'],
            [(first_vs.name, non_default_name)],
            is_replacing=False,
            component_desc=desc)
        self.assertTrue(result)

        updated_desc = self._getActiveDesc()
        self.assertIsNotNone(updated_desc, 'Could not get updated ComponentDescription')
        updated_vs = next(iter(updated_desc.GetVariantSets().values()))
        self.assertEqual(updated_vs.default_variant, original_default,
                         "default_variant must not change when adding to a non-default variant "
                         "(is_default_variant=False)")

    def testTargetedVariantUnchangedAfterAdd(self):
        """Adding a node to an existing variant must not change the targeted variant."""
        import AdskUsdComponentCreator
        from usd_component_creator_plugin import add_to_component_from_usd_prims
        proxy, desc, proxyShape = self._createInitialComponent('pCube1')
        self.assertIsNotNone(desc, 'Could not get initial ComponentDescription')
        first_vs = next(iter(desc.GetVariantSets().values()))
        default_name = first_vs.default_variant
        self.assertTrue(default_name)
        default_var_desc = first_vs.GetVariantDescription(default_name)
        self.assertIsNotNone(default_var_desc)

        # Verify: after create, default variant IS targeted
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.IsVariantTarget(
                desc, [first_vs, default_var_desc]),
            "Default variant must be targeted after create_component_from_usd_prims")

        # Add a second node to the same existing default variant
        path2 = Sdf.Path('/pCube2')
        self._input_stage.DefinePrim(path2, 'Cube')

        result = add_to_component_from_usd_prims(
            [f'{proxyShape},{path2}'],
            [(first_vs.name, default_name)],
            is_replacing=False,
            component_desc=desc)
        self.assertTrue(result)

        updated_desc = self._getActiveDesc()
        if updated_desc is None:
            stage = mayaUsd.ufe.getStage(proxy)
            updated_desc = self._getDescFromStage(stage)
        self.assertIsNotNone(updated_desc, 'Could not obtain updated ComponentDescription')

        updated_vs = next(iter(updated_desc.GetVariantSets().values()))
        updated_default_var_desc = updated_vs.GetVariantDescription(default_name)
        self.assertIsNotNone(updated_default_var_desc)
        # Check target did not change
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.IsVariantTarget(
                updated_desc, [updated_vs, updated_default_var_desc]))



if __name__ == '__main__':
    fixturesUtils.runTests(globals())

