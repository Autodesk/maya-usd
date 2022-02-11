#!/usr/bin/env python

#
# Copyright 2019 Autodesk
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

import fixturesUtils
import mayaUtils
import testUtils
import ufeUtils
import usdUtils

from maya import cmds
from maya import standalone
from maya.internal.ufeSupport import ufeCmdWrapper as ufeCmd

from pxr import Sdf

import mayaUsd.ufe

import ufe

import unittest

def firstSubLayer(context, routingData):
    prim = context.get('prim')
    if prim is None:
        print('Prim not in context')
        return
    if len(prim.GetStage().GetRootLayer().subLayerPaths)==0:
        return 
    routingData['layer'] = prim.GetStage().GetRootLayer().subLayerPaths[0]

class DuplicateCmdTestCase(unittest.TestCase):
    '''Verify the Maya delete command, for multiple runtimes.

    UFE Feature : SceneItemOps
    Maya Feature : duplicate
    Action : Duplicate objects in the scene.
    Applied On Selection :
        - Multiple Selection [Mixed, Non-Maya].  Maya-only selection tested by
          Maya.
    Undo/Redo Test : Yes
    Expect Results To Test :
        - Duplicate objects in the scene.
    Edge Cases :
        - None.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        ''' Called initially to set up the Maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

        # Create some extra Maya nodes
        cmds.polySphere()

        # Clear selection to start off
        cmds.select(clear=True)

    def testDuplicate(self):
        '''Duplicate Maya and USD objects.'''

        # Select two objects, one Maya, one USD.
        spherePath = ufe.Path(mayaUtils.createUfePathSegment("|pSphere1"))
        sphereItem = ufe.Hierarchy.createItem(spherePath)
        sphereHierarchy = ufe.Hierarchy.hierarchy(sphereItem)
        worldItem = sphereHierarchy.parent()

        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment(
                "|transform1|proxyShape1"),
            usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)
        ball35Hierarchy = ufe.Hierarchy.hierarchy(ball35Item)
        propsItem = ball35Hierarchy.parent()

        worldHierarchy = ufe.Hierarchy.hierarchy(worldItem)
        worldChildrenPre = worldHierarchy.children()
        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)
        propsChildrenPre = propsHierarchy.children()

        ufe.GlobalSelection.get().append(sphereItem)
        ufe.GlobalSelection.get().append(ball35Item)

        # Set the edit target to the layer in which Ball_35 is defined (has a
        # primSpec, in USD terminology).  Otherwise, duplication will not find
        # a source primSpec to copy.  Layers are the (anonymous) session layer,
        # the root layer, then the Assembly_room_set sublayer.  Trying to find
        # the layer by name is not practical, as it requires the full path
        # name, which potentially differs per run-time environment.
        ball35Prim = usdUtils.getPrimFromSceneItem(ball35Item)
        stage = ball35Prim.GetStage()

        layer = stage.GetLayerStack()[2]
        stage.SetEditTarget(layer)

        cmds.duplicate()

        # The duplicate command doesn't return duplicated non-Maya UFE objects.
        # They are in the selection, in the same order as the sources.
        snIter = iter(ufe.GlobalSelection.get())
        sphereDupItem = next(snIter)
        sphereDupName = str(sphereDupItem.path().back())
        ball35DupItem = next(snIter)
        ball35DupName = str(ball35DupItem.path().back())

        worldChildren = worldHierarchy.children()
        propsChildren = propsHierarchy.children()

        self.assertEqual(len(worldChildren)-len(worldChildrenPre), 1)
        self.assertEqual(len(propsChildren)-len(propsChildrenPre), 1)

        self.assertIn(sphereDupItem, worldChildren)
        self.assertIn(ball35DupItem, propsChildren)

        cmds.undo()

        # The duplicated items should no longer appear in the child list of
        # their parents.

        def childrenNames(children):
            return [str(child.path().back()) for child in children]

        worldHierarchy = ufe.Hierarchy.hierarchy(worldItem)
        worldChildren = worldHierarchy.children()
        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)
        propsChildren = propsHierarchy.children()

        worldChildrenNames = childrenNames(worldChildren)
        propsChildrenNames = childrenNames(propsChildren)

        self.assertNotIn(sphereDupName, worldChildrenNames)
        self.assertNotIn(ball35DupName, propsChildrenNames)

        # The duplicated items shoudl reappear after a redo
        cmds.redo()

        snIter = iter(ufe.GlobalSelection.get())
        sphereDupItem = next(snIter)
        ball35DupItem = next(snIter)

        worldChildren = worldHierarchy.children()
        propsChildren = propsHierarchy.children()

        self.assertEqual(len(worldChildren)-len(worldChildrenPre), 1)
        self.assertEqual(len(propsChildren)-len(propsChildrenPre), 1)

        self.assertIn(sphereDupItem, worldChildren)
        self.assertIn(ball35DupItem, propsChildren)

        cmds.undo()

        # The duplicated items should not be assigned to the name of a
        # deactivated USD item.

        cmds.select(clear=True)

        # Deactivate the even numbered props:
        evenPropsChildrenPre = propsChildrenPre[0:35:2]
        for propChild in evenPropsChildrenPre:
            ufe.GlobalSelection.get().append(propChild)
            sceneItem = usdUtils.getPrimFromSceneItem(propChild)
            sceneItem.SetActive(False)

        worldHierarchy = ufe.Hierarchy.hierarchy(worldItem)
        worldChildren = worldHierarchy.children()
        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)
        propsChildren = propsHierarchy.children()
        propsChildrenPostDel = propsHierarchy.children()

        # Duplicate Ball_1
        ufe.GlobalSelection.get().append(propsChildrenPostDel[0])

        cmds.duplicate()

        snIter = iter(ufe.GlobalSelection.get())
        ballDupItem = next(snIter)
        ballDupName = str(ballDupItem.path().back())

        self.assertNotIn(ballDupItem, propsChildrenPostDel)
        self.assertNotIn(ballDupName, propsChildrenNames)
        self.assertEqual(ballDupName, "Ball_36")

        cmds.undo()  # undo duplication

    def testDuplicateLoadedAndUnloaded(self):
        '''Duplicate a USD object when the object payload is loaded or unloaded under an unloaded ancestor.'''

        # Test helpers
        def getItem(path):
            '''Get the UFE scene item and USD prim for an item under a USD path'''
            fullPath = ufe.Path([
                mayaUtils.createUfePathSegment(
                    "|transform1|proxyShape1"),
                usdUtils.createUfePathSegment(path)])
            item = ufe.Hierarchy.createItem(fullPath)
            prim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(fullPath))
            return item, prim

        def executeContextCmd(ufeItem, subCmd):
            '''Execute a context-menu command, supports among other things Load and Unload.'''
            contextOps = ufe.ContextOps.contextOps(ufeItem)
            cmd = contextOps.doOpCmd([subCmd])
            self.assertIsNotNone(cmd)
            ufeCmd.execute(cmd)

        def loadItem(ufeItem):
            '''Load the payload of a scene item.'''
            executeContextCmd(ufeItem, 'Load')

        def loadItemWithDescendants(ufeItem):
            '''Load the payload of a scene item and its descendants.'''
            executeContextCmd(ufeItem, 'Load with Descendants')

        def unloadItem(ufeItem):
            '''Unload the payload of a scene item.'''
            executeContextCmd(ufeItem, 'Unload')

        def duplicate(ufeItem):
            '''Duplicate a scene item and return the UFE scene item of the new item.'''
            # Set the edit target to the layer in which Ball_35 is defined (has a
            # primSpec, in USD terminology).  Otherwise, duplication will not find
            # a source primSpec to copy.  Layers are the (anonymous) session layer,
            # the root layer, then the Assembly_room_set sublayer.  Trying to find
            # the layer by name is not practical, as it requires the full path
            # name, which potentially differs per run-time environment.
            prim = usdUtils.getPrimFromSceneItem(ufeItem)
            stage = prim.GetStage()

            layer = stage.GetLayerStack()[2]
            stage.SetEditTarget(layer)

            ufe.GlobalSelection.get().clear()
            ufe.GlobalSelection.get().append(ufeItem)
            cmds.duplicate()

            # The duplicate command doesn't return duplicated non-Maya UFE objects.
            # They are in the selection, in the same order as the sources.
            sel = ufe.GlobalSelection.get()
            self.assertEqual(1, len(sel))
            return sel.front()

        # Retrieve the ancestor props and one child ball item.
        propsItem, propsPrim = getItem("/Room_set/Props")
        ball35Item, ball35Prim = getItem("/Room_set/Props/Ball_35")
        ball7Item, ball7Prim = getItem("/Room_set/Props/Ball_7")

        # Unload the Props and verify everything is unloaded.
        # Note: items without payload are considered loaded, so only check balls.
        unloadItem(propsItem)

        self.assertFalse(ball35Prim.IsLoaded())
        self.assertFalse(ball7Prim.IsLoaded())

        # Duplicate the ball 35 and verify the new ball is unloaded
        # because the original was unloaded due to the ancestor being unloaded.
        ball35DupItem = duplicate(ball35Item)
        ball35DupPath = ball35DupItem.path()
        ball35DupPrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(ball35DupPath))

        self.assertFalse(ball35Prim.IsLoaded())
        self.assertFalse(ball7Prim.IsLoaded())
        self.assertFalse(ball35DupPrim.IsLoaded())

        # Explicitly load the ball 35 and verify its load status.
        loadItem(ball35Item)

        self.assertTrue(ball35Prim.IsLoaded())
        self.assertFalse(ball7Prim.IsLoaded())

        # Duplicate the ball 35 and verify the new ball is loaded even though
        # the props ancestor unloaded rule would normally make in unloaded.
        ball35DupItem = duplicate(ball35Item)
        ball35DupPath = ball35DupItem.path()
        ball35DupPrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(ball35DupPath))

        self.assertTrue(ball35Prim.IsLoaded())
        self.assertFalse(ball7Prim.IsLoaded())
        self.assertTrue(ball35DupPrim.IsLoaded())

        # Load the props items and its descendants and verify the status of the balls.
        loadItemWithDescendants(propsItem)

        self.assertTrue(ball35Prim.IsLoaded())
        self.assertTrue(ball7Prim.IsLoaded())

        # Duplicate the ball 35 and verify the new ball is loaded since
        # everything is marked as loaded.
        ball35DupItem = duplicate(ball35Item)
        ball35DupPath = ball35DupItem.path()
        ball35DupPrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(ball35DupPath))

        self.assertTrue(ball35Prim.IsLoaded())
        self.assertTrue(ball7Prim.IsLoaded())
        self.assertTrue(ball35DupPrim.IsLoaded())

        # Unload the ball 35 items and verify the status of the balls.
        unloadItem(ball35Item)

        self.assertFalse(ball35Prim.IsLoaded())
        self.assertTrue(ball7Prim.IsLoaded())

        # Duplicate the ball 35 and verify the new ball is unloaded even though
        # normally it would be loaded since the ancestor is loaded.
        ball35DupItem = duplicate(ball35Item)
        ball35DupPath = ball35DupItem.path()
        ball35DupPrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(ball35DupPath))

        self.assertFalse(ball35Prim.IsLoaded())
        self.assertTrue(ball7Prim.IsLoaded())
        self.assertFalse(ball35DupPrim.IsLoaded())

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2022, 'Requires Maya fixes only available in Maya 2022 or greater.')
    def testSmartTransformDuplicate(self):
        '''Test smart transform option of duplicate command.'''
        torusFile = testUtils.getTestScene("groupCmd", "torus.usda")
        torusDagPath, torusStage = mayaUtils.createProxyFromFile(torusFile)
        usdTorusPathString = torusDagPath + ",/pTorus1"

        cmds.duplicate(usdTorusPathString)
        cmds.move(10, 0, 0, r=True)
        smartDup = cmds.duplicate(smartTransform=True)

        usdTorusItem = ufeUtils.createUfeSceneItem(torusDagPath, '/pTorus3')
        torusT3d = ufe.Transform3d.transform3d(usdTorusItem)
        transVector = torusT3d.inclusiveMatrix().matrix[-1]

        correctResult = [20, 0, 0, 1]

        self.assertEqual(correctResult, transVector)

    def testEditRouter(self):
        '''Test edit router functionality.'''

        cmds.file(new=True, force=True)
        import mayaUsd_createStageWithNewLayer

        # Create the following hierarchy:
        #
        # ps
        #  |_ A
        #      |_ B
        #
        # We A and duplicate it.

        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/A', 'Xform')
        stage.DefinePrim('/A/B', 'Xform')

        psPath = ufe.PathString.path(psPathStr)
        psPathSegment = psPath.segments[0]
        aPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/A')])
        a = ufe.Hierarchy.createItem(aPath)
        bPath = aPath + ufe.PathComponent('B')
        b = ufe.Hierarchy.createItem(bPath)

        # Add a sub-layer, where the parent edit should write to.
        subLayerId = cmds.mayaUsdLayerEditor(stage.GetRootLayer().identifier, edit=True, addAnonymous="aSubLayer")[0]

        mayaUsd.lib.registerEditRouter('duplicate', firstSubLayer)

        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(a)

        cmds.duplicate()

        sublayer01 = Sdf.Find(subLayerId)
        self.assertIsNotNone(sublayer01)
        self.assertIsNotNone(sublayer01.GetPrimAtPath('/A1/B'))

if __name__ == '__main__':
    unittest.main(verbosity=2)
