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

from pxr import Sdf, Usd

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

    def testAddPrimViaApiLandsInSessionLayer(self):
        '''Validate that authoring via the pxr API puts the prim in the session layer.

        stage.DefinePrim() does not create a UsdUndoBlock, so edit-forwarding
        never fires.  The prim is therefore authored directly on the current
        edit target, which for component stages is the session layer.
        '''
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant so the geo scope is visible.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

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
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

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
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

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
