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

        # Undo the paste, copied sphere item should be gone and child count
        # should be back to original.
        pasteCmd.undo()
        self.assertEqual(nbChildren, len(xformHier.children()))
        pastedSphereItem = ufeUtils.createItem(psPathStr + ',/Xform1/Sphere2')
        self.assertIsNone(pastedSphereItem)

        # Redo the paste, pasted sphere will be back and child count should increase
        # by 1 again.
        pasteCmd.redo()
        self.assertEqual(nbChildren+1, len(xformHier.children()))
        pastedSphereItem = ufeUtils.createItem(psPathStr + ',/Xform1/Sphere2')
        self.assertIsNotNone(pastedSphereItem)

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

    def testClipboardCopyPasteWithVariant(self):
        '''Basic test for the Clipboard copy/paste of prim with a variant.'''

        # Open Variant.ma scene in testSamples
        mayaUtils.openVariantSetScene()

        # Get the stage
        mayaPathSegment = mayaUtils.createUfePathSegment('|Variant_usd|Variant_usdShape')
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

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

if __name__ == '__main__':
    unittest.main(verbosity=2)
