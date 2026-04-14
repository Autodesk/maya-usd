#!/usr/bin/env python

#
# Copyright 2026 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import unittest

import mayaUtils
import fixturesUtils
import mayaUsd.lib
import mayaUsd.ufe
import tempfile
from maya import cmds

import os
import sys

def _isCCAvailable():
    # Check to see if the AdskUsdComponentCreator exist in python path. If so it means
    # we have the Component Creator installed with this MayaUsd.
    for pp in sys.path:
        if not os.path.exists(pp):
            continue
        with os.scandir(path=pp) as entries:
            for entry in entries:
                if entry.is_dir() and entry.name == 'AdskUsdComponentCreator':
                    return True
    return False

# The USD component creator Maya plugin will only be there in certain circumstances so
# only try and load it if the CC Maya plugin actually exists.
def haveMayaCCPlugin():
    if 'MAYA_PLUG_IN_PATH' in os.environ:
        pluginPath = os.environ['MAYA_PLUG_IN_PATH']
        for pp in pluginPath.split(os.path.pathsep):
            if os.path.exists(pp) and ('usd_component_creator.py' in os.listdir(pp)):
                return True
        return False

_CC_AVAILABLE = _isCCAvailable()
_HAVE_CC_MAYA_PLUGIN = haveMayaCCPlugin()

class LoadComponentCreatorPluginTestCase(unittest.TestCase):
    """
    Verify that the USD component creator plugin can be loaded.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def testPluginLoadable(self):
        forceTest = os.environ.get('MAYAUSD_FORCE_CC_TEST', '0') == '1'
        if not _CC_AVAILABLE:
            if forceTest:
                self.fail('AdskUsdComponentCreator was not found but MAYAUSD_FORCE_CC_TEST is set.')
            else:
                self.skipTest('Could not find the AdskUsdComponentCreator.')

        # Both cases rely on the MayaUsd plugin being loaded first.
        self.assertTrue(mayaUtils.loadPlugin('mayaUsdPlugin'))
        if _HAVE_CC_MAYA_PLUGIN:
            self.assertTrue(mayaUtils.loadPlugin('usd_component_creator'))
        else:
            # In Maya at startup the idle events are suspended during initialization to prevent
            # plugins from being loaded before the UI is initialized.
            #
            # In an interactive test the startup command (for the test) is called before Maya
            # resumes the idle event processing. So we need to force resume it here and flush
            # the idle queue to ensure that the MayaUsd plugin UI Creation script is called
            # which is where the Component Creator initialization is done.
            #
            cmds.flushIdleQueue(resume=True)
            cmds.flushIdleQueue()

            # The Component Creator Maya plugin was removed and replace with a direct CC
            # initialization. We make sure CC was init correctly.
            from usd_component_creator_plugin import is_initialized as is_initializedCC
            is_cc_init = is_initializedCC()
            self.assertTrue(is_cc_init, "Component Creator was not initialized but MAYAUSD_FORCE_CC_TEST is set.")



class _ComponentCreatorTestBase:
    """Shared helpers for component creator test cases."""

    def _setUpCC(self):
        """Load required plugins for CC tests.  Call from setUp()."""
        if not _CC_AVAILABLE:
            self.skipTest('Could not find the USD component creator plugin')
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

# ---------------------------------------------------------------------------
# Test: create_component_from_nodes
# ---------------------------------------------------------------------------

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
    
# ---------------------------------------------------------------------------
# Test: create_multi_variants_component_from_nodes
# ---------------------------------------------------------------------------

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


# ---------------------------------------------------------------------------
# Test: add_to_component_from_nodes
# ---------------------------------------------------------------------------

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

class DuplicateToComponentTestCase(unittest.TestCase):
    """
    Test duplicating Maya nodes to an Adsk USD Component stage via the
    'Duplicate as USD' command.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def setUp(self):
        if not _CC_AVAILABLE:
            self.skipTest('Could not find the USD component creator plugin')
        mayaUtils.loadPlugin('mayaUsdPlugin')
        if _HAVE_CC_MAYA_PLUGIN:
            mayaUtils.loadPlugin('usd_component_creator')
        else:
            # See comment in test above.
            cmds.flushIdleQueue(resume=True)
            cmds.flushIdleQueue()
        cmds.file(new=True, force=True)

    def _createComponent(self):
        """Creates a new Adsk USD Component and a proxy shape pointing to it.

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

        # Create a proxy shape and point it at the component's root layer.
        transform = cmds.createNode('transform', name='TestComp')
        shape = cmds.createNode('mayaUsdProxyShape', name='TestCompShape', parent=transform)
        cmds.setAttr(shape + '.filePath', rootLayerPath, type='string')

        proxyShapePath = cmds.ls(shape, long=True)[0]
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        return proxyShapePath, desc

    def testDuplicateAsUsdToComponentNoVariantTarget(self):
        '''Duplicate to an Adsk USD Component with no variant targeted — should fail with an error.'''

        cmds.polyCube(name='pCube1')
        cubePath = cmds.ls('pCube1', long=True)[0]

        proxyShapePath, _ = self._createComponent()

        # No variant target is set — duplicate should return False and emit a error.
        with mayaUsd.lib.OpUndoItemList():
            result = mayaUsd.lib.PrimUpdaterManager.duplicate(cubePath, proxyShapePath)
        self.assertFalse(result)

    def testDuplicateAsUsdToComponent(self):
        '''Duplicate a Maya mesh to an Adsk USD Component with a variant targeted.'''
        import AdskUsdComponentCreator

        cmds.polyCube(name='pCube1')
        cubePath = cmds.ls('pCube1', long=True)[0]

        proxyShapePath, desc = self._createComponent()

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Duplicate the Maya mesh to the component.
        with mayaUsd.lib.OpUndoItemList():
            result = mayaUsd.lib.PrimUpdaterManager.duplicate(cubePath, proxyShapePath)
        self.assertTrue(result)

        # The cube should now appear in the component's geo scope.
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage.GetPrimAtPath('/root/geo/pCube1').IsValid())

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
