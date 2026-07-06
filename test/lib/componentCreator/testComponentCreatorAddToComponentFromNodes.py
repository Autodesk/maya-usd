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

class AddToComponentFromNodesTestCase(_ComponentCreatorTestBase, unittest.TestCase):
    """
    Tests for usd_component_creator_plugin.create_component.add_to_component_from_nodes.
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
        from usd_component_creator_plugin import create_component_from_nodes
        cmds.polyCube(name=node_name)
        path = cmds.ls(node_name, long=True)[0]
        before = self._snapshotProxyShapes()
        create_component_from_nodes([path])
        proxy = self._findNewProxyShape(before)
        desc = self._getActiveDesc()
        return proxy, desc

    def testNoComponentDescReturnsFalse(self):
        """add_to_component_from_nodes with no desc and empty variant editor returns False."""
        from usd_component_creator_plugin import add_to_component_from_nodes
        cmds.polyCube(name='pCube1')
        path = cmds.ls('pCube1', long=True)[0]
        result = add_to_component_from_nodes([path], [('model', 'extra')],
                                             is_replacing=False, component_desc=None)
        self.assertFalse(result,
                         "add_to_component_from_nodes must return False when no component "
                         "description is available")

    def testSanityReturnsTrueNoNewProxy(self):
        """add_to_component_from_nodes returns True and reuses the existing proxy shape."""
        from usd_component_creator_plugin import add_to_component_from_nodes
        proxy, desc = self._createInitialComponent('pCube1')
        self.assertIsNotNone(desc, 'Could not get initial ComponentDescription')
        first_vs = next(iter(desc.GetVariantSets().values()))

        cmds.polyCube(name='pCube2')
        path2 = cmds.ls('pCube2', long=True)[0]
        before = self._snapshotProxyShapes()

        # Add pCube2 to the existing default variant (not a new one)
        result = add_to_component_from_nodes(
            [path2],
            [(first_vs.name, first_vs.default_variant)],
            is_replacing=False,
            component_desc=desc)
        self.assertTrue(result, "add_to_component_from_nodes should return True on success")
        new_proxy = self._findNewProxyShape(before)
        self.assertIsNone(new_proxy, "No new proxy shape should be created when adding to an existing component")

    def testDefaultVariantUnchanged(self):
        """Adding a node to an existing variant must not change the default_variant name."""
        from usd_component_creator_plugin import add_to_component_from_nodes
        proxy, desc = self._createInitialComponent('pCube1')
        self.assertIsNotNone(desc, 'Could not get initial ComponentDescription')
        first_vs = next(iter(desc.GetVariantSets().values()))
        original_default = first_vs.default_variant
        self.assertTrue(original_default, "There must be a default variant before the add")

        cmds.polyCube(name='pCube2')
        path2 = cmds.ls('pCube2', long=True)[0]

        result = add_to_component_from_nodes(
            [path2],
            [(first_vs.name, original_default)],
            is_replacing=False,
            component_desc=desc)
        self.assertTrue(result)

        updated_desc = self._getActiveDesc()
        self.assertIsNotNone(updated_desc, 'Could not get updated ComponentDescription')
        updated_vs = next(iter(updated_desc.GetVariantSets().values()))
        self.assertEqual(updated_vs.default_variant, original_default,
                         "default_variant must not change after add_to_component_from_nodes "
                         "(is_default_variant=False)")

    def testDefaultVariantUnchangedWhenAddingToNonDefault(self):
        """Adding a node to a non-default variant must not change the default_variant name."""
        import AdskUsdComponentCreator
        from usd_component_creator_plugin import (
            add_to_component_from_nodes,
            create_multi_variants_component_from_nodes,
        )
        # Create a 2-variant component: 'AppleNode' is alphabetically first -> default variant.
        for name in ('ZebraNode', 'AppleNode'):
            cmds.polyCube(name=name)
        paths = [cmds.ls(n, long=True)[0] for n in ('ZebraNode', 'AppleNode')]
        before = self._snapshotProxyShapes()
        create_multi_variants_component_from_nodes(paths)
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

        cmds.polyCube(name='pCubeExtra')
        path_extra = cmds.ls('pCubeExtra', long=True)[0]

        result = add_to_component_from_nodes(
            [path_extra],
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
        from usd_component_creator_plugin import add_to_component_from_nodes
        proxy, desc = self._createInitialComponent('pCube1')
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
            "Default variant must be targeted after create_component_from_nodes")

        # Add a second node to the same existing default variant
        cmds.polyCube(name='pCube2')
        path2 = cmds.ls('pCube2', long=True)[0]
        result = add_to_component_from_nodes(
            [path2],
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

