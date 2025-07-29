#!/usr/bin/env python

#
# Copyright 2024 Autodesk
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
import ufeUtils, usdUtils, mayaUtils
import mayaUsd
import mayaUsd_createStageWithNewLayer

from maya import standalone
import maya.cmds as cmds

import ufe

import unittest


class ClipboardHandlerTestCase(unittest.TestCase):
    '''Verify ClipboardHandler interface.'''

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
        ''' Called initially to set up the maya test environment '''
        self.assertTrue(self.pluginsLoaded)

    def testClipboardCopyPaste(self):
        '''Basic test for the Clipboard copy/paste support.'''

        # Verify that the method to set the clipboard format exists.
        self.assertTrue(callable(getattr(mayaUsd.ufe, 'setClipboardFileFormat', None)))

        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/Xform1', 'Xform')
        stage.DefinePrim('/Xform1/Sphere1', 'Sphere')

        xformItem = ufeUtils.createItem(psPathStr + ',/Xform1')
        sphereItem = ufeUtils.createItem(psPathStr + ',/Xform1/Sphere1')
        self.assertIsNotNone(xformItem)
        self.assertIsNotNone(sphereItem)

        xformHier = ufe.Hierarchy.hierarchy(xformItem)
        self.assertIsNotNone(xformHier)

        # Verify that we have a clipboard handler for USD runtime.
        ch = ufe.ClipboardHandler.clipboardHandler(sphereItem.runTimeId())
        self.assertIsNotNone(ch)

        # Store how many children the Xform1 currently has (should be 1).
        nbChildren = len(xformHier.children())

        # Initialize the clipboard copy.
        ufe.ClipboardHandler.preCopy()

        # We should not have any items to paste now.
        self.assertFalse(ch.hasItemsToPaste_())

        # Copy the sphere item and test that we have an item to paste.
        copyCmd = ch.copyCmd_(sphereItem)
        self.assertIsNotNone(copyCmd)
        copyCmd.execute()
        self.assertTrue(ch.hasItemsToPaste_())

        # We should still have the sphere item.
        copiedSphereItem = ufeUtils.createItem(psPathStr + ',/Xform1/Sphere1')
        self.assertIsNotNone(copiedSphereItem)

        # Clear the selection before pasting.
        ufe.GlobalSelection.get().clear()

        # Paste (sphere from copy) into the xform1, it will get renamed
        # to Sphere1. The Xform1 parent should have one more child.
        pasteCmd = ch.pasteCmd_(xformItem)
        self.assertIsNotNone(pasteCmd)
        pasteCmd.execute()
        self.assertEqual(nbChildren+1, len(xformHier.children()))
        pastedSphereItem = ufeUtils.createItem(psPathStr + ',/Xform1/Sphere2')
        self.assertIsNotNone(pastedSphereItem)

        # Verify that we have the correct pasted item in the paste command.
        pastedItems = pasteCmd.targetItems()
        self.assertIsNotNone(pastedItems)
        self.assertEqual(1, len(pastedItems))
        pastedTargetSphere = pastedItems[0]
        self.assertEqual(pastedSphereItem, pastedTargetSphere)

        # Verify that the paste succeeded and no items failed.
        pasteInfo = pasteCmd.getPasteInfos()
        self.assertIsNotNone(pasteInfo)
        # The paste target should be the parent item (Xform1) we pasted into.
        self.assertEqual(xformItem.path(), pasteInfo[0].pasteTarget)
        # The successful pasted item should be the single pasted target from above
        self.assertEqual(1, len(pasteInfo[0].successfulPastes))
        self.assertEqual(pastedSphereItem.path(), pasteInfo[0].successfulPastes[0])
        # No paste failures.
        self.assertEqual(0, len(pasteInfo[0].failedPastes))

        # Verify that the pasted item is selected.
        sn = ufe.GlobalSelection.get()
        self.assertEqual(len(sn), 1)
        self.assertTrue(sn.contains(pastedSphereItem.path()))

        # Undo the paste, copied sphere item should be gone and child count
        # should be back to original.
        pasteCmd.undo()
        self.assertEqual(nbChildren, len(xformHier.children()))
        pastedSphereItem = ufeUtils.createItem(psPathStr + ',/Xform1/Sphere2')
        self.assertIsNone(pastedSphereItem)

        # The pasted sphere should no longer be selected, and the selection should
        # be empty (since there was nothing selected before pasting).
        sn = ufe.GlobalSelection.get()
        self.assertEqual(len(sn), 0)

        # Redo the paste, pasted sphere will be back and child count should increase
        # by 1 again.
        pasteCmd.redo()
        self.assertEqual(nbChildren+1, len(xformHier.children()))
        pastedSphereItem = ufeUtils.createItem(psPathStr + ',/Xform1/Sphere2')
        self.assertIsNotNone(pastedSphereItem)

        # Pasted sphere should again be selected.
        sn = ufe.GlobalSelection.get()
        self.assertEqual(len(sn), 1)
        self.assertTrue(sn.contains(pastedSphereItem.path()))

    def testClipboardCutPaste(self):
        '''Basic test for the Clipboard cut/paste support.'''

        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/Xform1', 'Xform')
        stage.DefinePrim('/Xform1/Sphere1', 'Sphere')

        xformItem = ufeUtils.createItem(psPathStr + ',/Xform1')
        sphereItem = ufeUtils.createItem(psPathStr + ',/Xform1/Sphere1')
        self.assertIsNotNone(xformItem)
        self.assertIsNotNone(sphereItem)

        xformHier = ufe.Hierarchy.hierarchy(xformItem)
        self.assertIsNotNone(xformHier)

        # Verify that we have a clipboard handler for USD runtime.
        ch = ufe.ClipboardHandler.clipboardHandler(sphereItem.runTimeId())
        self.assertIsNotNone(ch)

        # Store how many children the Xform1 currently has (should be 1).
        nbChildren = len(xformHier.children())

        # Initialize the clipboard cut.
        ufe.ClipboardHandler.preCut()

        # We should not have any items to paste now.
        self.assertFalse(ch.hasItemsToPaste_())

        # We should be able to cut the sphere.
        self.assertTrue(ch.canBeCut_(sphereItem))

        # Cut the sphere item and test that we have an item to paste.
        cutCmd = ch.cutCmd_(sphereItem)
        self.assertIsNotNone(cutCmd)
        cutCmd.execute()
        self.assertTrue(ch.hasItemsToPaste_())

        # We should no longer have the sphere item and the parent xform
        # should have 1 less child
        self.assertEqual(0, len(xformHier.children()))
        cutSphereItem = ufeUtils.createItem(psPathStr + ',/Xform1/Sphere1')
        self.assertIsNone(cutSphereItem)

        # Undo the cut, which will bring back the sphere.
        cutCmd.undo()
        cutSphereItem = ufeUtils.createItem(psPathStr + ',/Xform1/Sphere1')
        self.assertIsNotNone(cutSphereItem)

        # And redo the cut which means the sphere will be gone again.
        cutCmd.redo()
        cutSphereItem = ufeUtils.createItem(psPathStr + ',/Xform1/Sphere1')
        self.assertIsNone(cutSphereItem)

        # Paste (sphere from cut) into the xform1, it should retain the
        # same name. The Xform1 parent should have the same number of children.
        xformItem = ufeUtils.createItem(psPathStr + ',/Xform1')
        self.assertIsNotNone(xformItem)
        pasteCmd = ch.pasteCmd_(xformItem)
        self.assertIsNotNone(pasteCmd)
        pasteCmd.execute()
        self.assertEqual(nbChildren, len(xformHier.children()))
        pastedSphereItem = ufeUtils.createItem(psPathStr + ',/Xform1/Sphere1')
        self.assertIsNotNone(pastedSphereItem)

        # Verify that we have the correct pasted item in the paste command.
        pastedItems = pasteCmd.targetItems()
        self.assertIsNotNone(pastedItems)
        self.assertEqual(1, len(pastedItems))
        pastedTargetSphere = pastedItems[0]
        self.assertEqual(pastedSphereItem, pastedTargetSphere)

        # Verify that the paste succeeded and no items failed.
        pasteInfo = pasteCmd.getPasteInfos()
        self.assertIsNotNone(pasteInfo)
        # The paste target should be the parent item (Xform1) we pasted into.
        self.assertEqual(xformItem.path(), pasteInfo[0].pasteTarget)
        # The successful pasted item should be the single pasted target from above
        self.assertEqual(1, len(pasteInfo[0].successfulPastes))
        self.assertEqual(pastedSphereItem.path(), pasteInfo[0].successfulPastes[0])
        # No paste failures.
        self.assertEqual(0, len(pasteInfo[0].failedPastes))

        # Undo the paste, sphere item should be gone and the child count
        # should be back down by 1.
        pasteCmd.undo()
        self.assertEqual(0, len(xformHier.children()))
        pastedSphereItem = ufeUtils.createItem(psPathStr + ',/Xform1/Sphere1')
        self.assertIsNone(pastedSphereItem)

        # Redo the paste, sphere will be back and child count should be
        # back to original.
        pasteCmd.redo()
        self.assertEqual(nbChildren, len(xformHier.children()))
        pastedSphereItem = ufeUtils.createItem(psPathStr + ',/Xform1/Sphere1')
        self.assertIsNotNone(pastedSphereItem)

    def testClipboardCutRestrictions(self):
        '''Basic test for the Clipboard cut restrictions.'''

        # Create a proxy shape stage and prims to start with.
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/Xform1', 'Xform')
        stage.DefinePrim('/Xform1/Sphere1', 'Sphere')

        # The Sphere prim should be cutable.
        sphereItem = ufeUtils.createItem(psPathStr + ',/Xform1/Sphere1')
        self.assertIsNotNone(sphereItem)
        self.assertTrue(ufe.ClipboardHandler.canBeCut(sphereItem))

        # Create a new layer and set it as the edit target.
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        rootLayer = stage.GetRootLayer()
        subLayerA = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=False, addAnonymous="SubLayerA")[0]
        self.assertIsNotNone(subLayerA)
        cmds.mayaUsdEditTarget(psPathStr, edit=True, editTarget=subLayerA)
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, subLayerA)

        # Now the Sphere prim should no longer be cutable.
        self.assertFalse(ufe.ClipboardHandler.canBeCut(sphereItem))

        # And trying to cut it should not work and there should be no
        # items in the clipboard.
        rid = mayaUsd.ufe.getUsdRunTimeId()
        ufe.ClipboardHandler.preCut()
        self.assertFalse(ufe.ClipboardHandler.hasItemsToPaste(rid))
        cmds.select(psPathStr + ',/Xform1/Sphere1')
        # The cut command should be empty since the cut on Sphere is restricted
        # and its the only item in the selection (to cut).
        # Because the returned command is empty the clipboard handler will throw.
        self.assertRaises(RuntimeError, ufe.ClipboardHandler.cutCmd, ufe.GlobalSelection.get())
        self.assertFalse(ufe.ClipboardHandler.hasItemsToPaste(rid))

        # The Sphere prim should still be there.
        self.assertTrue(stage.GetPrimAtPath('/Xform1/Sphere1').IsValid())

        # Create a new prim in this layer which will then be cutable.
        stage.DefinePrim('/Xform1/Cube1', 'Cube')
        cubeItem = ufeUtils.createItem(psPathStr + ',/Xform1/Cube1')
        self.assertIsNotNone(cubeItem)
        self.assertTrue(ufe.ClipboardHandler.canBeCut(cubeItem))

        # Select both the cutable and non-cutable prims and cut. The
        # cut should succeed by cutting only the cutable prim and only
        # place it in the clipboard (to paste).
        cmds.select(psPathStr + ',/Xform1/Sphere1', psPathStr + ',/Xform1/Cube1')
        ufe.ClipboardHandler.preCut()
        self.assertFalse(ufe.ClipboardHandler.hasItemsToPaste(rid))
        cutCmd = ufe.ClipboardHandler.cutCmd(ufe.GlobalSelection.get())
        self.assertIsNotNone(cutCmd)
        cutCmd.execute()
        self.assertTrue(ufe.ClipboardHandler.hasItemsToPaste(rid))

    def testClipboardCutRemovedFromSelection(self):
        '''Test that when we cut a selected item it is removed from selection list.'''

        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/Xform1', 'Xform')
        stage.DefinePrim('/Xform1/Sphere1', 'Sphere')

        # Select the sphere and cut it.
        cmds.select(psPathStr + ',/Xform1/Sphere1')
        cutCmd = ufe.ClipboardHandler.cutCmd(ufe.GlobalSelection.get())
        self.assertIsNotNone(cutCmd)
        cutCmd.execute()

        # There sphere prim should not be selected.
        self.assertTrue(ufe.GlobalSelection.get().empty())

    def testClipboardCopyPasteWithVariant(self):
        '''Basic test for the Clipboard copy/paste of prim with a variant.'''

        # Open Variant.ma scene in testSamples
        mayaUtils.openVariantSetScene()

        # Get the stage
        mayaPathSegment = mayaUtils.createUfePathSegment('|Variant_usd|Variant_usdShape')
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))

        # First check that we have a Variant.
        objectPrim = stage.GetPrimAtPath('/objects')
        self.assertTrue(objectPrim.HasVariantSets())

        # Get the VariantSets for the object prim.
        objectVariantSet = objectPrim.GetVariantSets()
        self.assertIsNotNone(objectVariantSet)

        # Get clipboard handler for object prim.
        objectItem = ufeUtils.createItem('|Variant_usd|Variant_usdShape,/objects')
        self.assertIsNotNone(objectItem)
        ch = ufe.ClipboardHandler.clipboardHandler(objectItem.runTimeId())
        self.assertIsNotNone(ch)

        # Create a prim to paste into.
        stage.DefinePrim('/Xform1', 'Xform')
        xformItem = ufeUtils.createItem('|Variant_usd|Variant_usdShape,/Xform1')
        self.assertIsNotNone(xformItem)

        # Initialize the clipboard copy.
        ufe.ClipboardHandler.preCopy()

        # We should not have any items to paste now.
        self.assertFalse(ch.hasItemsToPaste_())

        # Copy the ojbect and paste it to the xform.
        copyCmd = ch.copyCmd_(objectItem)
        self.assertIsNotNone(copyCmd)
        copyCmd.execute()
        self.assertTrue(ch.hasItemsToPaste_())

        # Paste (object from copy) into the xform1.
        pasteCmd = ch.pasteCmd_(xformItem)
        self.assertIsNotNone(pasteCmd)
        pasteCmd.execute()
        pastedObjectItem = ufeUtils.createItem('|Variant_usd|Variant_usdShape,/Xform1/objects')
        self.assertIsNotNone(pastedObjectItem)

        # The pasted object should have the same variant sets as the original.
        pastedPrim = usdUtils.getPrimFromSceneItem(pastedObjectItem)
        self.assertIsNotNone(pastedPrim)
        pastedVariantSet = pastedPrim.GetVariantSets()
        self.assertIsNotNone(pastedVariantSet)
        self.assertEqual(objectVariantSet.GetNames(), pastedVariantSet.GetNames())
        for name in objectVariantSet.GetNames():
            self.assertTrue(pastedVariantSet.HasVariantSet(name))
            self.assertEqual(objectVariantSet.GetVariantSelection(name), pastedVariantSet.GetVariantSelection(name))

    def testClipboardMultiplePaste(self):
        '''Test pasting multiple times.'''
        
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/Xform1', 'Xform')
        stage.DefinePrim('/Xform1/Sphere1', 'Sphere')

        # Copy the sphere item and test that we have an item to paste.
        cmds.select(psPathStr + ',/Xform1/Sphere1')
        ufe.ClipboardHandler.preCopy()
        copyCmd = ufe.ClipboardHandler.copyCmd(ufe.GlobalSelection.get())
        self.assertIsNotNone(copyCmd)
        copyCmd.execute()

        # Create new Xforms for pasting into.
        stage.DefinePrim('/Xform2', 'Xform')
        stage.DefinePrim('/Xform3', 'Xform')

        # First select the xform and continously paste into the
        # global selection, which will be changing for each paste.
        # The paste command selects the newly pasted items. But there
        # is special "paste as sibling" logic.
        cmds.select(psPathStr + ',/Xform2')

        # Paste into the new xform multiple times.
        nbPastes = 10
        for _ in range(nbPastes):
            # Paste target is global selection each time which is changing.
            pasteCmd = ufe.ClipboardHandler.pasteCmd(ufe.GlobalSelection.get())
            self.assertIsNotNone(pasteCmd)
            pasteCmd.execute()  # This pastes and select

        # The xform2 should now have X number of children.
        xformItem = ufeUtils.createItem(psPathStr + ',/Xform2')
        self.assertIsNotNone(xformItem)
        xformHier = ufe.Hierarchy.hierarchy(xformItem)
        self.assertIsNotNone(xformHier)
        self.assertEqual(nbPastes, len(xformHier.children()))

        # Select a different target and paste into that.
        cmds.select(psPathStr + ',/Xform3')
        pasteCmd = ufe.ClipboardHandler.pasteCmd(ufe.GlobalSelection.get())
        self.assertIsNotNone(pasteCmd)
        pasteCmd.execute()

        # That new target should now have one child and the other target
        # still the same X number.
        self.assertEqual(nbPastes, len(xformHier.children()))
        xformItem = ufeUtils.createItem(psPathStr + ',/Xform3')
        xformHier = ufe.Hierarchy.hierarchy(xformItem)
        self.assertEqual(1, len(xformHier.children()))

    def testClipboardErrorConditions(self):
        '''Test the clipboard handler error conditions.'''

        # Create a proxy shape stage and prims to start with.
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/Xform1', 'Xform')
        stage.DefinePrim('/Xform1/Sphere1', 'Sphere')

        # The Sphere prim should be cutable.
        #sphereItem = ufeUtils.createItem(psPathStr + ',/Xform1/Sphere1')
        #self.assertIsNotNone(sphereItem)
        #self.assertTrue(ufe.ClipboardHandler.canBeCut(sphereItem))

        # Trying to cut/copy/paste with empty selection will throw.
        emptySel = ufe.Selection()
        self.assertRaisesRegex(ValueError, 'Empty clipboard selection for cut operation.',
                               ufe.ClipboardHandler.cutCmd, emptySel)
        self.assertRaisesRegex(ValueError, 'Empty clipboard selection for copy operation.',
                               ufe.ClipboardHandler.copyCmd, emptySel)
        self.assertRaisesRegex(ValueError, 'Empty clipboard selection for paste operation.',
                               ufe.ClipboardHandler.pasteCmd, emptySel)

        # The cut/copy/paste methods will throw if given invalid scene item.
        rid = mayaUsd.ufe.getUsdRunTimeId()
        ch = ufe.ClipboardHandler.clipboardHandler(rid)
        self.assertIsNotNone(ch)
        self.assertRaisesRegex(ValueError, 'Cannot cut null item.', ch.cutCmd_, None)
        self.assertRaisesRegex(ValueError, 'Cannot copy null item.', ch.copyCmd_, None)
        self.assertRaisesRegex(ValueError, 'Cannot paste into null parent item.', ch.pasteCmd, None)

        # There is no Maya clipboardHandler registered.
        self.assertIsNone(ufe.ClipboardHandler.clipboardHandler(mayaUsd.ufe.getMayaRunTimeId()))

        # Trying to directly use clipboardHandler with Maya object selected will throw.
        cmds.polyCube()
        mayaSel = ufe.GlobalSelection.get()
        self.assertRaisesRegex(RuntimeError, "No clipboard handler for runtime 'Maya-DG'.",
                               ufe.ClipboardHandler.cutCmd, mayaSel)
        self.assertRaisesRegex(RuntimeError, "No clipboard handler for runtime 'Maya-DG'.",
                               ufe.ClipboardHandler.copyCmd, mayaSel)
        self.assertRaisesRegex(RuntimeError, "No clipboard handler for runtime 'Maya-DG'.",
                               ufe.ClipboardHandler.pasteCmd, mayaSel)

        # Calling paste when there is nothing to paste will throw.
        cmds.select(psPathStr + ',/Xform1/Sphere1')
        ufe.ClipboardHandler.preCopy()
        self.assertFalse(ch.hasItemsToPaste_())
        # Using clipboardHandler directly.
        pasteCmd = ch.pasteCmd(ufe.GlobalSelection.get())
        self.assertRaisesRegex(RuntimeError, 'Failed to load Clipboard stage.', pasteCmd.execute)
        # Using static method.
        pasteCmd = ufe.ClipboardHandler.pasteCmd(ufe.GlobalSelection.get())
        self.assertRaisesRegex(RuntimeError, 'Failed to load Clipboard stage.', pasteCmd.execute)

    def testClipboardCopyNodeGraphOutputConnection(self):
        '''
        Regression test for EMSUSD-2681. There was a bug where connections to compound outputs
        didn't get copied correctly. Repro steps are simple:
        - Create a NodeGraph and add an output port
        - Create an add node inside the NodeGraph
        - Connect the add node to the NodeGraph output
        - Copy/Paste the NodeGraph
        - The connection was missing in the copied NodeGraph
        '''

        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        
        # Create a NodeGraph and add an output attribute.
        stage.DefinePrim('/Material1', 'Material')
        stage.DefinePrim('/Material1/NodeGraph1', 'NodeGraph')
        nodeGraphItem = ufeUtils.createItem(psPathStr + ',/Material1/NodeGraph1')
        nodeGraphOutput = ufe.Attributes.attributes(nodeGraphItem).addAttribute("outputs:out", ufe.Attribute.kColorFloat3)

        # Create a shader within the NodeGraph.
        nodeDef = ufe.NodeDef.definition(nodeGraphItem.runTimeId(), 'ND_add_color3')
        childItem = nodeDef.createNode(nodeGraphItem, ufe.PathComponent('add1'))
        childOutput = ufe.Attributes.attributes(childItem).attribute('outputs:out')

        # Connect the shader to the compound output
        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(nodeGraphItem.runTimeId())
        connection = connectionHandler.connect(childOutput, nodeGraphOutput)
        self.assertIsNotNone(connection)

        # Copy the NodeGraph
        ch = ufe.ClipboardHandler.clipboardHandler(nodeGraphItem.runTimeId())
        ufe.ClipboardHandler.preCopy()
        copyCmd = ch.copyCmd_(nodeGraphItem)
        copyCmd.execute()

        # Clear the selection before pasting.
        ufe.GlobalSelection.get().clear()

        # Paste the node graph.
        pasteCmd = ch.pasteCmd_(ufe.Hierarchy.createItem(nodeGraphItem.path().pop()))
        pasteCmd.execute()
        self.assertIsNotNone(pasteCmd.targetItems())
        self.assertEqual(1, len(pasteCmd.targetItems()))
        pastedNodeGraph = pasteCmd.targetItems()[0]

        # Verify that the connection was copied.
        self.assertEqual(1, len(connectionHandler.sourceConnections(pastedNodeGraph).allConnections()))


    def testClipboardCopyConnections(self):
        '''Regression test to ensure connections between duplicated items get copied.'''

        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        
        # Create a NodeGraph and add an output attribute.
        stage.DefinePrim('/Material1', 'Material')
        stage.DefinePrim('/Material2', 'Material')
        materialItem1 = ufeUtils.createItem(psPathStr + ',/Material1')
        materialItem2 = ufeUtils.createItem(psPathStr + ',/Material2')

        # Create two shaders within the NodeGraph.
        nodeDef = ufe.NodeDef.definition(materialItem1.runTimeId(), 'ND_add_color3')
        srcItem = nodeDef.createNode(materialItem1, ufe.PathComponent('src1'))
        dstItem = nodeDef.createNode(materialItem1, ufe.PathComponent('dst1'))

        # Connect the shaders.
        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(materialItem1.runTimeId())
        srcOutput = ufe.Attributes.attributes(srcItem).attribute('outputs:out')
        dstInput = ufe.Attributes.attributes(dstItem).attribute('inputs:in1')
        connection = connectionHandler.connect(srcOutput, dstInput)
        self.assertIsNotNone(connection)

        # Copy both shaders.
        ch = ufe.ClipboardHandler.clipboardHandler(materialItem1.runTimeId())
        ufe.ClipboardHandler.preCopy()
        copyCmd = ch.copyCmd_(ufe.Selection([srcItem, dstItem]))
        copyCmd.execute()

        # Clear the selection before pasting.
        ufe.GlobalSelection.get().clear()

        # Paste both shaders.
        pasteCmd = ch.pasteCmd_(materialItem2)
        pasteCmd.execute()
        self.assertIsNotNone(pasteCmd.targetItems())
        self.assertEqual(2, len(pasteCmd.targetItems()))
        pastedSrcItem = ufe.Hierarchy.createItem(materialItem2.path() + srcItem.nodeName())
        pastedDstItem = ufe.Hierarchy.createItem(materialItem2.path() + dstItem.nodeName())

        # Verify that the connection was copied.
        self.assertEqual(1, len(connectionHandler.sourceConnections(pastedDstItem).allConnections()))
        pastedConnection = connectionHandler.sourceConnections(pastedDstItem).allConnections()[0]
        self.assertEqual(pastedConnection.src.path, pastedSrcItem.path())

if __name__ == '__main__':
    unittest.main(verbosity=2)
