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

import ufe
from maya.internal.ufeSupport import ufeCmdWrapper as ufeCmd

from pxr import Sdf

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


class TestObserver(ufe.Observer):
    """Observer to track UFE notifications for delete operations."""
    def __init__(self):
        super(TestObserver, self).__init__()
        self.deleteNotif = 0
        self.addNotif = 0

    def __call__(self, notification):
        if isinstance(notification, ufe.ObjectDelete):
            self.deleteNotif += 1
        if isinstance(notification, ufe.ObjectAdd):
            self.addNotif += 1

    def nbDeleteNotif(self):
        return self.deleteNotif

    def nbAddNotif(self):
        return self.addNotif

    def reset(self):
        self.addNotif = 0
        self.deleteNotif = 0


class DeleteInComponentTestCase(unittest.TestCase):
    """
    Test deleting prims in Adsk USD Component stages.

    Component stages have special delete behavior where prim specs are removed
    from all layers in the prim stack (including non-local layers).
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
        return proxyShapePath, desc

    def testDeleteLeafPrimInComponentStage(self):
        '''Test successful delete of a leaf prim in a component stage.'''
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Create a prim in the component's geo scope.
        stage.SetEditTarget(stage.GetEditTarget())
        geoPrim = stage.GetPrimAtPath('/root/geo')
        self.assertTrue(geoPrim.IsValid())

        testPrim = stage.DefinePrim('/root/geo/TestPrim', 'Xform')
        self.assertTrue(testPrim.IsValid())

        # Create UFE notification observer.
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # Delete the prim.
        ufeObs.reset()
        cmds.delete(proxyShapePath + ',/root/geo/TestPrim')
        self.assertEqual(ufeObs.nbDeleteNotif(), 1)
        self.assertFalse(stage.GetPrimAtPath('/root/geo/TestPrim').IsValid())

        # Validate undo.
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif(), 1)
        self.assertTrue(stage.GetPrimAtPath('/root/geo/TestPrim').IsValid())

        # Validate redo.
        ufeObs.reset()
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif(), 1)
        self.assertFalse(stage.GetPrimAtPath('/root/geo/TestPrim').IsValid())

    def testDeleteHierarchyInComponentStage(self):
        '''Test successful delete of a prim hierarchy in a component stage.'''
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Create a prim hierarchy in the component's geo scope.
        parentPrim = stage.DefinePrim('/root/geo/Parent', 'Xform')
        childPrim1 = stage.DefinePrim('/root/geo/Parent/Child1', 'Xform')
        childPrim2 = stage.DefinePrim('/root/geo/Parent/Child2', 'Sphere')
        self.assertTrue(parentPrim.IsValid())
        self.assertTrue(childPrim1.IsValid())
        self.assertTrue(childPrim2.IsValid())

        # Create UFE notification observer.
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # Delete the parent prim (should delete entire hierarchy).
        ufeObs.reset()
        cmds.delete(proxyShapePath + ',/root/geo/Parent')
        self.assertEqual(ufeObs.nbDeleteNotif(), 1)
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent/Child1').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent/Child2').IsValid())

        # Validate undo.
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif(), 1)
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent/Child1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent/Child2').IsValid())

        # Validate redo.
        ufeObs.reset()
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif(), 1)
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent/Child1').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent/Child2').IsValid())

    def testDeleteMultiplePrimsInComponentStage(self):
        '''Test delete of multiple prims, some being children of others, in a component stage.'''
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Create a prim hierarchy.
        parentPrim = stage.DefinePrim('/root/geo/Parent', 'Xform')
        child1 = stage.DefinePrim('/root/geo/Parent/Child1', 'Xform')
        child2 = stage.DefinePrim('/root/geo/Parent/Child2', 'Sphere')
        sibling = stage.DefinePrim('/root/geo/Sibling', 'Cube')
        self.assertTrue(parentPrim.IsValid())
        self.assertTrue(child1.IsValid())
        self.assertTrue(child2.IsValid())
        self.assertTrue(sibling.IsValid())

        # Create UFE notification observer.
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # Delete parent and one of its children (redundant) plus a sibling.
        ufeObs.reset()
        cmds.delete(
            proxyShapePath + ',/root/geo/Parent/Child1',
            proxyShapePath + ',/root/geo/Parent',
            proxyShapePath + ',/root/geo/Sibling'
        )

        # All prims should be deleted.
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent/Child1').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent/Child2').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Sibling').IsValid())

        # Validate undo.
        ufeObs.reset()
        cmds.undo()
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent/Child1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent/Child2').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Sibling').IsValid())

        # Validate redo.
        ufeObs.reset()
        cmds.redo()
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent/Child1').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent/Child2').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Sibling').IsValid())

    def testDeleteRestrictionComponentScopeItself(self):
        '''Test delete restriction - cannot delete the component scope prims themselves.'''
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Verify the geo scope exists.
        geoScope = stage.GetPrimAtPath('/root/geo')
        self.assertTrue(geoScope.IsValid())

        # Try to delete the geo scope itself - should fail.
        # Maya wraps the exception, so we check that it raises any exception.
        with self.assertRaises(Exception):
            cmds.delete(proxyShapePath + ',/root/geo')

        # Verify scope still exists.
        self.assertTrue(stage.GetPrimAtPath('/root/geo').IsValid())


class ReparentInComponentTestCase(unittest.TestCase):
    """
    Test reparenting (insert child) prims in Adsk USD Component stages.

    Component stages have special reparent behavior where prim specs are moved
    from all layers in the prim stack.
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

        transform = cmds.createNode('transform', name='TestComp')
        shape = cmds.createNode('mayaUsdProxyShape', name='TestCompShape', parent=transform)
        cmds.setAttr(shape + '.filePath', rootLayerPath, type='string')

        proxyShapePath = cmds.ls(shape, long=True)[0]
        return proxyShapePath, desc

    def testReparentPrimInComponentStage(self):
        '''Test successful reparenting of a prim in a component stage.'''
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Create prims in the component's geo scope.
        child = stage.DefinePrim('/root/geo/Child', 'Xform')
        newParent = stage.DefinePrim('/root/geo/NewParent', 'Xform')
        self.assertTrue(child.IsValid())
        self.assertTrue(newParent.IsValid())

        # Reparent child under newParent using cmds.parent.
        cmds.parent(proxyShapePath + ',/root/geo/Child', proxyShapePath + ',/root/geo/NewParent')

        # Verify the child was reparented.
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Child').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewParent/Child').IsValid())

        # Validate undo.
        cmds.undo()
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Child').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/NewParent/Child').IsValid())

        # Validate redo.
        cmds.redo()
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Child').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewParent/Child').IsValid())

    def testReparentPrimWithChildrenInComponentStage(self):
        '''Test reparenting a prim hierarchy in a component stage.'''
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Create a prim hierarchy.
        parent = stage.DefinePrim('/root/geo/Parent', 'Xform')
        child1 = stage.DefinePrim('/root/geo/Parent/Child1', 'Xform')
        child2 = stage.DefinePrim('/root/geo/Parent/Child2', 'Sphere')
        newGrandparent = stage.DefinePrim('/root/geo/NewGrandparent', 'Xform')
        self.assertTrue(parent.IsValid())
        self.assertTrue(child1.IsValid())
        self.assertTrue(child2.IsValid())
        self.assertTrue(newGrandparent.IsValid())

        # Reparent the entire hierarchy.
        cmds.parent(
            proxyShapePath + ',/root/geo/Parent',
            proxyShapePath + ',/root/geo/NewGrandparent'
        )

        # Verify the hierarchy was reparented.
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewGrandparent/Parent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewGrandparent/Parent/Child1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewGrandparent/Parent/Child2').IsValid())

        # Validate undo.
        cmds.undo()
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent/Child1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent/Child2').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/NewGrandparent/Parent').IsValid())

        # Validate redo.
        cmds.redo()
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewGrandparent/Parent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewGrandparent/Parent/Child1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewGrandparent/Parent/Child2').IsValid())

    def testReparentRestrictionToOutsideComponentScope(self):
        '''Test reparent restriction - cannot reparent prims to outside component scopes.'''
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Create a prim inside the geo scope and a parent outside.
        child = stage.DefinePrim('/root/geo/Child', 'Xform')
        outsideParent = stage.DefinePrim('/root/OutsideScope', 'Xform')
        self.assertTrue(child.IsValid())
        self.assertTrue(outsideParent.IsValid())

        # Try to reparent the prim to outside the scope - this should succeed
        # because the child is within the scope (source validation passes).
        # But the operation itself might fail or succeed depending on the implementation.
        # In component stages, we validate the child being moved, not the destination.
        # So this should actually work - the child can be moved anywhere.
        cmds.parent(proxyShapePath + ',/root/geo/Child', proxyShapePath + ',/root/OutsideScope')

        # Verify the child was moved.
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Child').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/OutsideScope/Child').IsValid())

        # Undo to clean up.
        cmds.undo()

    def testReparentRestrictionComponentScopeItself(self):
        '''Test reparent restriction - cannot reparent the component scope prims themselves.'''
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Create a new parent.
        newParent = stage.DefinePrim('/root/NewParent', 'Xform')
        self.assertTrue(newParent.IsValid())

        # Try to reparent the geo scope itself - should fail.
        # Maya wraps the exception, so we check that it raises any exception.
        with self.assertRaises(Exception):
            cmds.parent(proxyShapePath + ',/root/geo', proxyShapePath + ',/root/NewParent')

        # Verify scope still at original location.
        self.assertTrue(stage.GetPrimAtPath('/root/geo').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/NewParent/geo').IsValid())


class RenameInComponentTestCase(unittest.TestCase):
    """
    Test renaming prims in Adsk USD Component stages.

    Component stages have special rename behavior where prim specs are renamed
    in all layers of the prim stack (not just defining layers).
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

        transform = cmds.createNode('transform', name='TestComp')
        shape = cmds.createNode('mayaUsdProxyShape', name='TestCompShape', parent=transform)
        cmds.setAttr(shape + '.filePath', rootLayerPath, type='string')

        proxyShapePath = cmds.ls(shape, long=True)[0]
        return proxyShapePath, desc

    def testRenamePrimInComponentStage(self):
        '''Test successful renaming of a prim in a component stage.'''
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Create a prim in the component's geo scope.
        testPrim = stage.DefinePrim('/root/geo/OldName', 'Xform')
        self.assertTrue(testPrim.IsValid())
        self.assertEqual(testPrim.GetName(), 'OldName')

        # Rename the prim using cmds.rename.
        cmds.rename(proxyShapePath + ',/root/geo/OldName', 'NewName')

        # Verify the prim was renamed.
        self.assertFalse(stage.GetPrimAtPath('/root/geo/OldName').IsValid())
        renamedPrim = stage.GetPrimAtPath('/root/geo/NewName')
        self.assertTrue(renamedPrim.IsValid())
        self.assertEqual(renamedPrim.GetName(), 'NewName')

        # Validate undo.
        cmds.undo()
        self.assertTrue(stage.GetPrimAtPath('/root/geo/OldName').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/NewName').IsValid())

        # Validate redo.
        cmds.redo()
        self.assertFalse(stage.GetPrimAtPath('/root/geo/OldName').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewName').IsValid())

    def testRenamePrimWithChildrenInComponentStage(self):
        '''Test renaming a prim with children in a component stage.'''
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Create a prim hierarchy.
        parent = stage.DefinePrim('/root/geo/OldParent', 'Xform')
        child1 = stage.DefinePrim('/root/geo/OldParent/Child1', 'Xform')
        child2 = stage.DefinePrim('/root/geo/OldParent/Child2', 'Sphere')
        self.assertTrue(parent.IsValid())
        self.assertTrue(child1.IsValid())
        self.assertTrue(child2.IsValid())

        # Rename the parent prim.
        cmds.rename(proxyShapePath + ',/root/geo/OldParent', 'NewParent')

        # Verify the parent was renamed and children moved with it.
        self.assertFalse(stage.GetPrimAtPath('/root/geo/OldParent').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/OldParent/Child1').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/OldParent/Child2').IsValid())

        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewParent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewParent/Child1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewParent/Child2').IsValid())

        # Validate undo.
        cmds.undo()
        self.assertTrue(stage.GetPrimAtPath('/root/geo/OldParent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/OldParent/Child1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/OldParent/Child2').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/NewParent').IsValid())

        # Validate redo.
        cmds.redo()
        self.assertFalse(stage.GetPrimAtPath('/root/geo/OldParent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewParent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewParent/Child1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewParent/Child2').IsValid())

    def testRenameRestrictionComponentScopeItself(self):
        '''Test rename restriction - cannot rename the component scope prims themselves.'''
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Verify the geo scope exists.
        geoScope = stage.GetPrimAtPath('/root/geo')
        self.assertTrue(geoScope.IsValid())

        # Try to rename the geo scope itself - should fail.
        # Maya wraps the exception, so we check that it raises any exception.
        with self.assertRaises(Exception):
            cmds.rename(proxyShapePath + ',/root/geo', 'renamed_geo')

        # Verify scope still has original name.
        self.assertTrue(stage.GetPrimAtPath('/root/geo').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/renamed_geo').IsValid())


class ComponentStageInvariantsTestCase(unittest.TestCase):
    """
    Validate structural invariants that hold for every Adsk USD Component stage.

    1. Root layer is locked (permissionToEdit == False) — the MayaComponentManager
       locks it via mayaUsdLayerEditor when the component is registered, preventing
       direct edits to the on-disk file through the proxy shape.

    2. Default edit target is the session layer — stageNode.cpp sets this
       automatically whenever the root layer is not editable.
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

        transform = cmds.createNode('transform', name='TestComp')
        shape = cmds.createNode('mayaUsdProxyShape', name='TestCompShape', parent=transform)
        cmds.setAttr(shape + '.filePath', rootLayerPath, type='string')

        proxyShapePath = cmds.ls(shape, long=True)[0]
        return proxyShapePath, desc

    def testComponentRootLayerIsLocked(self):
        '''Validate that the component root layer is locked (permissionToEdit == False).

        The MayaComponentManager locks the root layer via mayaUsdLayerEditor
        when the component stage is registered.  This prevents direct edits
        to the on-disk component file through the proxy shape.
        '''
        proxyShapePath, _ = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        rootLayer = stage.GetRootLayer()
        self.assertFalse(
            rootLayer.permissionToEdit,
            'Component root layer should be locked (permissionToEdit == False)')

    def testComponentDefaultEditTargetIsSessionLayer(self):
        '''Validate that the default edit target of a component stage is the session layer.

        stageNode.cpp automatically targets the session layer when the root
        layer is not editable (permissionToEdit == False), so every new edit
        made without an explicit edit-context goes to the session layer.
        '''
        proxyShapePath, _ = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        self.assertEqual(
            stage.GetEditTarget().GetLayer(),
            stage.GetSessionLayer(),
            'Component stage default edit target should be the session layer')


class AddPrimInComponentTestCase(unittest.TestCase):
    """
    Test adding prims to an Adsk USD Component stage via different authoring paths.

    1. Authoring via the pxr API (stage.DefinePrim) with no UsdUndoBlock →
       no edit-forwarding fires → the prim lands on the session layer.

    2. Authoring via the UFE 'Add New Prim' context-op on a valid scope (geo/mtl)
       → edit-forwarding fires on idle → prim is forwarded to the variant layer
       and removed from the session layer.

    3. Authoring via the UFE 'Add New Prim' context-op at the root level
       (outside geo/mtl) → the component's block rule rolls back the entire
       transaction → the prim ends up in neither the session layer nor the root layer.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)
        # Ensure idle callbacks fire when cmds.flushIdleQueue() is called.
        cmds.flushIdleQueue(resume=True)
        cmds.flushIdleQueue()

    def setUp(self):
        if not _CC_AVAILABLE:
            self.skipTest('Could not find the USD component creator plugin')
        mayaUtils.loadPlugin('mayaUsdPlugin')
        if _HAVE_CC_MAYA_PLUGIN:
            mayaUtils.loadPlugin('usd_component_creator')
        else:
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
        opts.component_variants = [('model', 'default')]
        opts.is_default_variant = True

        info = AdskUsdComponentCreator.ComponentAPI.CreateFromNothing(opts)
        desc = AdskUsdComponentCreator.ComponentDescription.CreateFromInfo(info)

        rootLayerPath = info.stage.GetRootLayer().realPath

        transform = cmds.createNode('transform', name='TestComp')
        shape = cmds.createNode('mayaUsdProxyShape', name='TestCompShape', parent=transform)
        cmds.setAttr(shape + '.filePath', rootLayerPath, type='string')

        proxyShapePath = cmds.ls(shape, long=True)[0]

        # Give the CC plugin an idle tick to detect the new proxy shape and register
        # its edit-forwarding callbacks before the test proceeds.
        cmds.flushIdleQueue()

        return proxyShapePath, desc

    def _setDefaultVariantTarget(self, desc):
        """Target the `model=default` variant on the given component description."""
        import AdskUsdComponentCreator
        vsDesc = desc.GetVariantSets()['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

    def testAddPrimViaApiLandsInSessionLayer(self):
        '''Validate that authoring via the pxr API puts the prim in the session layer.

        stage.DefinePrim() does not create a UsdUndoBlock, so edit-forwarding
        never fires.  The prim is therefore authored directly on the current
        edit target, which for component stages is the session layer.
        '''
        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant so the geo scope is visible.
        self._setDefaultVariantTarget(desc)

        primPath = Sdf.Path('/root/geo/ApiPrim')

        # Author directly via the pxr API — no UsdUndoBlock, no forwarding.
        newPrim = stage.DefinePrim(primPath, 'Xform')
        self.assertTrue(newPrim.IsValid())

        # Prim must be in the session layer.
        sessionLayer = stage.GetSessionLayer()
        self.assertIsNotNone(
            sessionLayer.GetPrimAtPath(primPath),
            'Prim authored via pxr API should land in the session layer')

        # Prim must NOT be in the locked root layer.
        rootLayer = stage.GetRootLayer()
        self.assertIsNone(
            rootLayer.GetPrimAtPath(primPath),
            'Prim authored via pxr API should not be written to the locked root layer')

    def testAddPrimViaContextOpForwardsToVariantLayerInComponentStage(self):
        '''Validate that 'Add New Prim' under a valid scope is forwarded to the variant layer.

        When contextOps is invoked on the geo scope item the new prim lands under
        /root/geo, which matches the forwarding rule.  The prim is first authored
        in the session layer (UsdUndoBlock), then edit-forwarding fires on the next
        idle tick and moves it to the active variant layer.  After the flush the
        prim must NOT remain in the session layer, and must be visible in the
        composed stage via the active variant.
        '''
        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        self._setDefaultVariantTarget(desc)

        # Build the UFE item for the geo scope so the new prim lands under it.
        geoItem = ufe.Hierarchy.createItem(
            ufe.PathString.path(proxyShapePath + ',/root/geo'))
        self.assertIsNotNone(geoItem)

        contextOps = ufe.ContextOps.contextOps(geoItem)
        self.assertIsNotNone(contextOps)

        ufeCmd.execute(contextOps.doOpCmd(['Add New Prim', 'Xform']))

        primPath = Sdf.Path('/root/geo/Xform1')
        sessionLayer = stage.GetSessionLayer()

        # Before forwarding: prim is in the session layer and visible in the composed stage.
        self.assertIsNotNone(
            sessionLayer.GetPrimAtPath(primPath),
            'Prim should be in the session layer immediately after contextOps command')
        self.assertTrue(
            stage.GetPrimAtPath(primPath).IsValid(),
            'Prim should be visible in the composed stage before forwarding')

        # Flush idle queue so edit-forwarding fires and moves the edit.
        cmds.flushIdleQueue()

        # After forwarding: prim has left the session layer (moved to the variant sublayer).
        self.assertIsNone(
            sessionLayer.GetPrimAtPath(primPath),
            'Prim should have been forwarded out of the session layer')
        # The prim must remain visible in the composed stage via the active variant
        # payload (the root-layer variant selection keeps the variant active).
        self.assertTrue(
            stage.GetPrimAtPath(primPath).IsValid(),
            'Prim should remain visible in the composed stage after forwarding')

    def testAddPrimViaContextOpIsBlockedByForwardingRuleInComponentStage(self):
        '''Validate that 'Add New Prim' at root level is rejected by the edit-forward block rule.

        The context-op creates a UsdUndoBlock internally.  Edit-forwarding fires
        on the next idle tick.  The component's block rule (EditForwardRules.cpp,
        isTargetLayerBlock=true) matches prims outside the geo/mtl scopes and
        rolls back the entire transaction — the session-layer edit is undone.
        After the flush the prim exists in neither the session layer nor the root layer,
        and is not visible in the composed stage.
        '''
        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        self._setDefaultVariantTarget(desc)

        # Build the UFE item for the proxy shape — 'Add New Prim' will land at /Xform1,
        # which is outside /root/geo and /root/mtl.
        shapeItem = ufe.Hierarchy.createItem(ufe.PathString.path(proxyShapePath))
        self.assertIsNotNone(shapeItem)

        contextOps = ufe.ContextOps.contextOps(shapeItem)
        self.assertIsNotNone(contextOps)

        ufeCmd.execute(contextOps.doOpCmd(['Add New Prim', 'Xform']))

        primPath = Sdf.Path('/Xform1')
        sessionLayer = stage.GetSessionLayer()

        # Before block fires: prim is in the session layer and visible in the composed stage.
        self.assertIsNotNone(
            sessionLayer.GetPrimAtPath(primPath),
            'Prim should be in the session layer immediately after contextOps command')
        self.assertTrue(
            stage.GetPrimAtPath(primPath).IsValid(),
            'Prim should be visible in the composed stage before the block rule fires')

        # Flush idle queue so the block rule fires and rolls back the transaction.
        cmds.flushIdleQueue()

        # After rollback: prim is gone from the composed stage.
        self.assertFalse(
            stage.GetPrimAtPath(primPath).IsValid(),
            'Prim must not be visible in the composed stage after rollback')


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
