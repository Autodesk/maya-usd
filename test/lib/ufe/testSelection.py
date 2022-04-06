#!/usr/bin/env python

#
# Copyright 2020 Autodesk
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

import mayaUsd
import fixturesUtils
import mayaUtils
import ufeUtils
import usdUtils

from maya import cmds
from maya import standalone

import ufe

try:
    from maya.internal.ufeSupport import ufeSelectCmd
except ImportError:
    # Maya 2019 and 2020 don't have ufeSupport plugin, so use fallback.
    from ufeScripts import ufeSelectCmd

import os
import unittest


class SelectTestCase(unittest.TestCase):
    '''Verify UFE selection on a USD scene.'''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()
    
    @classmethod
    def tearDownClass(cls):
        cmds.file(new=True, force=True)

        standalone.uninitialize()

    def setUp(self):
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

        # Load a file that has the same scene in both the Maya Dag
        # hierarchy and the USD hierarchy.
        mayaUtils.openTestScene("parentCmd", "simpleSceneMayaPlusUSD_TRS.ma")

        # Create multiple scene items.  We will alternate between selecting a
        # Maya item and a USD item, 3 items each data model, one item at a
        # time, so we select 6 different items, one at a time.
        shapeSegment = mayaUtils.createUfePathSegment(
            "|mayaUsdProxy1|mayaUsdProxyShape1")
        ufeNames = ["cubeXform", "cylinderXform", "sphereXform"]
        mayaNames = ["pCube1", "pCylinder1", "pSphere1"]
        usdPaths = []
        for n in ["/" + o for o in ufeNames]:
            usdPaths.append(ufe.Path(
                [shapeSegment, usdUtils.createUfePathSegment(n)]))
        mayaPaths = []
        for n in ["|" + o for o in mayaNames]:
            mayaPaths.append(ufe.Path(mayaUtils.createUfePathSegment(n)))

        # Create a list of paths by alternating USD objects and Maya objects
        # Flatten zipped tuples using list comprehension double loop.
        zipped = zip(usdPaths, mayaPaths)
        paths = [j for i in zipped for j in i]

        # Create items for all paths.
        self.items = [ufe.Hierarchy.createItem(p) for p in paths]

        # Clear selection to start off
        cmds.select(clear=True)

    def runTestSelection(self, selectCmd):
        '''Run the replace selection test, using the argument command to
        replace the selection with a single scene item.'''

        # Clear the selection.
        globalSn = ufe.GlobalSelection.get()
        globalSn.clear()
        self.assertTrue(globalSn.empty())

        # Select all items in turn.
        for item in self.items:
            selectCmd(item)

        # Item in the selection should be the last item in our list.
        self.assertEqual(len(globalSn), 1)
        def snFront(sn):
            return next(iter(sn)) if \
                ufe.VersionInfo.getMajorVersion() == 1 else sn.front()

        self.assertEqual(snFront(globalSn), self.items[-1])

        # Check undo.  For this purpose, re-create the list of items in reverse
        # order.  Because we're already at the last item, we skip the last one
        # (i.e. last item is -1, so start at -2, and increment by -1).
        rItems = self.items[-2::-1]

        # Undo until the first element, checking the selection as we go.
        for i in rItems:
            cmds.undo()
            self.assertEqual(len(globalSn), 1)
            self.assertEqual(snFront(globalSn), i)

        # Check redo.
        fItems = self.items[1:]

        # Redo until the last element, checking the selection as we go.
        for i in fItems:
            cmds.redo()
            self.assertEqual(len(globalSn), 1)
            self.assertEqual(snFront(globalSn), i)

    def testUfeSelect(self):
        def selectCmd(item):
            sn = ufe.Selection()
            sn.append(item)
            ufeSelectCmd.replaceWith(sn)
        self.runTestSelection(selectCmd)

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testMayaSelect only available in UFE v2 or greater.')
    def testMayaSelect(self):
        # Maya PR 121 now has support for UFE path string in select command.
        def selectCmd(item):
            cmds.select(ufe.PathString.string(item.path()))
        self.runTestSelection(selectCmd)

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testMayaSelectFlags only available in UFE v2 or greater.')
    def testMayaSelectFlags(self):
        # Maya PR 121 now has support for UFE path string in select command.

        # Clear the selection.
        globalSn = ufe.GlobalSelection.get()
        globalSn.clear()
        self.assertTrue(globalSn.empty())

        # Incrementally add to the selection all items in turn.
        # Also testing undo/redo along the way.
        cnt = 0
        for item in self.items:
            cnt += 1
            cmds.select(ufe.PathString.string(item.path()), add=True)
            self.assertEqual(cnt, len(globalSn))
            self.assertTrue(globalSn.contains(item.path()))

            cmds.undo()
            self.assertEqual(cnt-1, len(globalSn))
            self.assertFalse(globalSn.contains(item.path()))

            cmds.redo()
            self.assertEqual(cnt, len(globalSn))
            self.assertTrue(globalSn.contains(item.path()))

        # Since we added all the items to the global selection, it
        # should have all of them.
        self.assertEqual(len(globalSn), len(self.items))

        # Ensure the global selection order is the same as our item order.
        itemIt = iter(self.items)
        for selIt in globalSn:
            self.assertEqual(selIt, next(itemIt))

        # Incrementally remove from the selection all items in turn.
        # Also testing undo/redo along the way.
        for item in self.items:
            cnt -= 1
            cmds.select(ufe.PathString.string(item.path()), deselect=True)
            self.assertEqual(cnt, len(globalSn))
            self.assertFalse(globalSn.contains(item.path()))

            cmds.undo()
            self.assertEqual(cnt+1, len(globalSn))
            self.assertTrue(globalSn.contains(item.path()))

            cmds.redo()
            self.assertEqual(cnt, len(globalSn))
            self.assertFalse(globalSn.contains(item.path()))

        # Since we removed all items from global selection it
        # should be empty now.
        self.assertTrue(globalSn.empty())

        # Incrementally toggle selection state of all items in turn.
        # Since they all start unselected, they will toggle to selected.
        # Also testing undo/redo along the way.
        globalSn.clear()
        self.assertTrue(globalSn.empty())
        cnt = 0
        for item in self.items:
            cnt += 1
            cmds.select(ufe.PathString.string(item.path()), toggle=True)
            self.assertEqual(cnt, len(globalSn))
            self.assertTrue(globalSn.contains(item.path()))

            cmds.undo()
            self.assertEqual(cnt-1, len(globalSn))
            self.assertFalse(globalSn.contains(item.path()))

            cmds.redo()
            self.assertEqual(cnt, len(globalSn))
            self.assertTrue(globalSn.contains(item.path()))

        # Since we toggled each item to selected, we should have all
        # of them on the global selection.
        self.assertEqual(len(globalSn), len(self.items))

        # Incrementally toggle selection state of all items in turn.
        # Since they all start selected, they will toggle to unselected.
        # Also testing undo/redo along the way.
        for item in self.items:
            cnt -= 1
            cmds.select(ufe.PathString.string(item.path()), toggle=True)
            self.assertEqual(cnt, len(globalSn))
            self.assertFalse(globalSn.contains(item.path()))

            cmds.undo()
            self.assertEqual(cnt+1, len(globalSn))
            self.assertTrue(globalSn.contains(item.path()))

            cmds.redo()
            self.assertEqual(cnt, len(globalSn))
            self.assertFalse(globalSn.contains(item.path()))

        # Since we toggled all items to unselected, the global selection
        # should be empty now.
        self.assertTrue(globalSn.empty())

        # Select all the items at once, replacing the existing selection.
        # Also testing undo/redo.
        pathStrings = [ufe.PathString.string(i.path()) for i in self.items]
        cmds.select(*pathStrings, replace=True)
        self.assertEqual(len(globalSn), len(self.items))

        # Ensure the global selection order is the same as our item order.
        itemIt = iter(self.items)
        for selIt in globalSn:
            self.assertEqual(selIt, next(itemIt))

        cmds.undo()
        self.assertEqual(0, len(globalSn))
        self.assertTrue(globalSn.empty())

        cmds.redo()
        self.assertEqual(len(globalSn), len(self.items))

        # Ensure the global selection order is the same as our item order.
        itemIt = iter(self.items)
        for selIt in globalSn:
            self.assertEqual(selIt, next(itemIt))

        # With all items selected (and in same order as item order)
        # "select -add" the first item which will move it to the end.
        first = self.items[0]
        self.assertEqual(globalSn.front(), first)
        cmds.select(ufe.PathString.string(first.path()), add=True)
        self.assertTrue(globalSn.contains(first.path()))
        self.assertEqual(globalSn.back(), first)

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2 and mayaUtils.mayaMajorVersion() > 2023, 'testMayaSelectAddFirst only available in UFE v2 or greater and Maya greter than 2023.')
    def testMayaSelectAddFirst(self):
        """
        Test the -addFirst flag of the select command with UFE.
        """
        # Clear the selection.
        globalSn = ufe.GlobalSelection.get()
        globalSn.clear()
        self.assertTrue(globalSn.empty())

        # Incrementally add to the selection all items in turn.
        # Also testing undo/redo along the way.
        cnt = 0
        for item in self.items:
            cnt += 1
            cmds.select(ufe.PathString.string(item.path()), addFirst=True)
            self.assertEqual(cnt, len(globalSn))
            self.assertTrue(globalSn.contains(item.path()))

            cmds.undo()
            self.assertEqual(cnt-1, len(globalSn))
            self.assertFalse(globalSn.contains(item.path()))

            cmds.redo()
            self.assertEqual(cnt, len(globalSn))
            self.assertTrue(globalSn.contains(item.path()))

        # Since we added all the items to the global selection, it
        # should have all of them.
        self.assertEqual(len(globalSn), len(self.items))

        # Ensure the global selection order is the opposite of our item order.
        reversedItems = list(self.items)
        reversedItems.reverse()
        for sel, expected in zip(globalSn, reversedItems):
            self.assertEqual(sel, expected)

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2023, 'Requires Maya fixes only avaiable in Maya 2023 or greater.')
    def testMayaSelectMuteLayer(self):
        '''Stale selection items must be removed on mute layer.'''
        
        # Create new stage
        import mayaUsd_createStageWithNewLayer
        proxyShapePathStr    = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath       = ufe.PathString.path(proxyShapePathStr)
        proxyShapeItem       = ufe.Hierarchy.createItem(proxyShapePath)
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        # Create a sub-layer.
        stage     = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()
        rootLayer = stage.GetRootLayer()
        subLayerId = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="Layer1")[0]

        # Set the edit target to new sub-layer.
        cmds.mayaUsdEditTarget(
            proxyShapePathStr, edit=True, editTarget=subLayerId)

        # Create a prim.  This will create the primSpec in the new sub-layer.
        proxyShapeContextOps.doOp(['Add New Prim', 'Capsule'])

        capsulePathStr = '%s,/Capsule1' % proxyShapePathStr
        capsulePath    = ufe.PathString.path(capsulePathStr)
        capsuleItem    = ufe.Hierarchy.createItem(capsulePath)
        
        # Select the prim.  This is the core of the test: on subtree invalidate,
        # the prim's UFE scene item should be removed from the selection.
        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(capsuleItem)

        self.assertTrue(sn.contains(capsulePath))

        # Mute sub-layer
        cmds.mayaUsdLayerEditor(
            subLayerId, edit=True, muteLayer=[1, proxyShapePathStr])

        # Should be nothing on the selection list.
        self.assertTrue(sn.empty())

        # Undo: capsule should be back on the selection list.
        cmds.undo()

        self.assertTrue(sn.contains(capsulePath))
        
        # Redo: selection list now empty.
        cmds.redo()

        self.assertTrue(sn.empty())

        cmds.undo()

        # Change attribute on the capsule, using the item from the selection,
        # which must be valid.
        self.assertTrue(len(sn), 1)
        capsuleItem = sn.front()
        capsuleAttrs = ufe.Attributes.attributes(capsuleItem)
        self.assertIsNotNone(capsuleAttrs)

        capsuleRadius = capsuleAttrs.attribute('radius')
        capsuleRadius.set(2)

        self.assertEqual(capsuleRadius.get(), 2)

        # Now mute the layer outside a Maya command.  Stale scene items must be
        # removed from the selection.
        self.assertTrue(len(sn), 1)
        self.assertTrue(sn.contains(capsulePath))

        stage.MuteLayer(subLayerId)

        self.assertTrue(sn.empty())

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2023, 'Requires Maya fixes only available in Maya 2023 or greater.')
    def testMayaSelectSwitchVariant(self):
        '''Stale selection items must be removed on variant switch.'''

        import mayaUsd_createStageWithNewLayer
        import maya.internal.ufeSupport.ufeCmdWrapper as ufeCmd

        # Create a scene with two variants.
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(proxyShape).GetStage()
        top = stage.DefinePrim('/Xform1', 'Xform')
        vset = top.GetVariantSets().AddVariantSet('modelingVariant')
        vset.AddVariant('cube')
        vset.AddVariant('sphere')
        vset.SetVariantSelection('cube')
        with vset.GetVariantEditContext():
            stage.DefinePrim('/Xform1/Cube', 'Cube')
        vset.SetVariantSelection('sphere')
        with vset.GetVariantEditContext():
            stage.DefinePrim('/Xform1/Sphere', 'Sphere')

        # The sphere is the sole child of Xform1.  Get an attribute from it,
        # select it.
        xformPath  = ufe.PathString.path('%s,/Xform1' % proxyShape)
        spherePath = ufe.PathString.path('%s,/Xform1/Sphere' % proxyShape)
        xformItem  = ufe.Hierarchy.createItem(xformPath)
        sphereItem = ufe.Hierarchy.createItem(spherePath)

        xformHier = ufe.Hierarchy.hierarchy(xformItem)
        xformChildren = xformHier.children()
        self.assertEqual(len(xformChildren), 1)
        self.assertEqual(xformChildren[0].path(), spherePath)
        sphereAttrs = ufe.Attributes.attributes(sphereItem)
        sphereRadius = sphereAttrs.attribute('radius')
        self.assertEqual(sphereRadius.get(), 1)

        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(sphereItem)

        self.assertEqual(len(sn), 1)

        # Switch variants using a command: the cube is now the sole child of
        # Xform1, we can get an attribute from the cube.  The selection must
        # now be empty.
        xformCtxOps = ufe.ContextOps.contextOps(xformItem)
        cmd = xformCtxOps.doOpCmd(['Variant Sets', 'modelingVariant', 'cube'])
        ufeCmd.execute(cmd)

        cubePath = ufe.PathString.path('%s,/Xform1/Cube' % proxyShape)
        cubeItem = ufe.Hierarchy.createItem(cubePath)

        xformChildren = xformHier.children()
        self.assertEqual(len(xformChildren), 1)
        self.assertEqual(xformChildren[0].path(), cubePath)
        cubeAttrs = ufe.Attributes.attributes(cubeItem)
        cubeRadius = cubeAttrs.attribute('size')
        self.assertEqual(cubeRadius.get(), 2)

        self.assertTrue(sn.empty())

        # Undo: selection is restored, seletion item is valid.
        cmds.undo()

        self.assertEqual(len(sn), 1)
        sphereItem = sn.front()
        self.assertEqual(sphereItem.path(), spherePath)
        sphereAttrs = ufe.Attributes.attributes(sphereItem)
        sphereRadius = sphereAttrs.attribute('radius')
        self.assertEqual(sphereRadius.get(), 1)
        xformChildren = xformHier.children()
        self.assertEqual(len(xformChildren), 1)
        self.assertEqual(xformChildren[0].path(), spherePath)

        # Redo: selection is cleared.
        cmds.redo()

        self.assertTrue(sn.empty())
        xformChildren = xformHier.children()
        self.assertEqual(len(xformChildren), 1)
        self.assertEqual(xformChildren[0].path(), cubePath)

        # Undo: selection restored to sphere.
        cmds.undo()

        self.assertEqual(len(sn), 1)
        sphereItem = sn.front()
        self.assertEqual(sphereItem.path(), spherePath)

        # Now set the variant outside a Maya command.  Stale scene items must be
        # removed from the selection.
        vset.SetVariantSelection('cube')

        self.assertTrue(sn.empty())

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2023, 'Requires Maya fixes only available in Maya 2023 or greater.')
    def testMayaSelectUndoPrimCreation(self):
        '''Test if the SceneItem's prim is still valid on selection after the prim creation is undone then redone'''

        # helper function to check if the current
        # selected SceneItem's prim is valid.
        def checkSelectedSceneItemPrim(expectedSceneItem):
            globalSn = ufe.GlobalSelection.get()
            self.assertEqual(len(globalSn), 1)
            sceneItem = globalSn.front()
            self.assertEqual(sceneItem, expectedSceneItem)
            prim = mayaUsd.ufe.getPrimFromRawItem(sceneItem.getRawAddress())
            self.assertTrue(prim)

        shapeNode,shapeStage = mayaUtils.createProxyAndStage()
        proxyShapePath       = ufe.PathString.path(shapeNode)
        proxyShapeItem       = ufe.Hierarchy.createItem(proxyShapePath)
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        with mayaUsd.lib.UsdUndoBlock():
            proxyShapeContextOps.doOp(['Add New Prim', 'Capsule'])

        with mayaUsd.lib.UsdUndoBlock():
            proxyShapeContextOps.doOp(['Add New Prim', 'Cube'])

        capsulePath    = ufe.PathString.path('%s,/Capsule1' % shapeNode)
        capsuleItem    = ufe.Hierarchy.createItem(capsulePath)
        cmds.select(ufe.PathString.string(capsuleItem.path()), replace=True)
        checkSelectedSceneItemPrim(capsuleItem)

        cubePath    = ufe.PathString.path('%s,/Cube1' % shapeNode)
        cubeItem    = ufe.Hierarchy.createItem(cubePath)
        cmds.select(ufe.PathString.string(cubeItem.path()), replace=True)
        checkSelectedSceneItemPrim(cubeItem)

        cmds.undo() # undo the selection of the cube
        cmds.undo() # undo the selection of the capsule
        cmds.undo() # undo the creation of the cube
        cmds.undo() # undo the creation of the capsule

        cmds.redo() # redo the creation of the capsule
        cmds.redo() # redo the creation of the cube
        cmds.redo() # redo the selection of the capsule
        checkSelectedSceneItemPrim(capsuleItem)
        cmds.redo() # redo the selection of the cube
        checkSelectedSceneItemPrim(cubeItem)
        cmds.undo() # undo the selection of the cube
        checkSelectedSceneItemPrim(capsuleItem)

if __name__ == '__main__':
    unittest.main(verbosity=2)
