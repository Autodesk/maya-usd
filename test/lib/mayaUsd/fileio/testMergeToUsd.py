#!/usr/bin/env python

#
# Copyright 2021 Autodesk
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

from mayaUtils import setMayaTranslation, setMayaRotation
from usdUtils import createSimpleXformScene, mayaUsd_createStageWithNewLayer

import mayaUsd.lib

import mayaUtils
import mayaUsd.ufe

from pxr import UsdGeom, Gf, Sdf

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as om

import ufe

import unittest

from testUtils import assertVectorAlmostEqual, getTestScene

import os

class MergeToUsdTestCase(unittest.TestCase):
    '''Test merge to USD: commit edits done as Maya data back into USD.

    Also known as Push to USD.
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
        cmds.file(new=True, force=True)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '3006', 'Test only available in UFE preview version 0.3.6 and greater')
    def testTransformMergeToUsd(self):
        '''Merge edits on a USD transform back to USD.'''

        # To merge back to USD, we must edit as Maya first.
        (ps, aXlateOp, _, aUsdUfePathStr, aUsdUfePath, aUsdItem,
         bXlateOp, _, bUsdUfePathStr, bUsdUfePath, bUsdItem) = \
            createSimpleXformScene()
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(aUsdUfePathStr))

        aMayaItem = ufe.GlobalSelection.get().front()
        (aMayaPath, aMayaPathStr, _, aMayaMatrix) = \
            setMayaTranslation(aMayaItem, om.MVector(4, 5, 6))

        bMayaPathStr   = aMayaPathStr + '|B'
        bMayaPath      = ufe.PathString.path(bMayaPathStr)
        bMayaItem      = ufe.Hierarchy.createItem(bMayaPath)
        (_, _, _, bMayaMatrix) = \
            setMayaTranslation(bMayaItem, om.MVector(10, 11, 12))

        # There should be a path mapping for the edit as Maya hierarchy.
        for (mayaItem, mayaPath, usdUfePath) in \
            zip([aMayaItem, bMayaItem], [aMayaPath, bMayaPath],
                [aUsdUfePath, bUsdUfePath]):
            mayaToUsd = ufe.PathMappingHandler.pathMappingHandler(mayaItem)
            self.assertEqual(mayaToUsd.fromHost(mayaPath), usdUfePath)

        psHier = ufe.Hierarchy.hierarchy(ps)

        # Merge edits back to USD.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.mergeToUsd(aMayaPathStr))

        # Check that edits have been preserved in USD.
        for (usdUfePathStr, mayaMatrix, xlateOp) in \
            zip([aUsdUfePathStr, bUsdUfePathStr], [aMayaMatrix, bMayaMatrix], 
                [aXlateOp, bXlateOp]):
            usdMatrix = xlateOp.GetOpTransform(
                mayaUsd.ufe.getTime(usdUfePathStr))
            mayaValues = [v for v in mayaMatrix]
            usdValues = [v for row in usdMatrix for v in row]
        
            assertVectorAlmostEqual(self, mayaValues, usdValues)

        # There no longer are any Maya to USD path mappings.
        for mayaPath in [aMayaPath, bMayaPath]:
            self.assertEqual(len(mayaToUsd.fromHost(mayaPath)), 0)

        # Hierarchy is restored: USD item is child of proxy shape, Maya item is
        # not.  Be careful to use the Maya path rather than the Maya item, which
        # should no longer exist.
        self.assertIn(aUsdItem, psHier.children())
        self.assertNotIn(aMayaPath, [child.path() for child in psHier.children()])

        # Maya nodes are removed.
        for mayaPathStr in [aMayaPathStr, bMayaPathStr]:
            with self.assertRaises(RuntimeError):
                om.MSelectionList().add(mayaPathStr)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '3006', 'Test only available in UFE preview version 0.3.6 and greater')
    def testTransformMergeToUsdUndoRedo(self):
        '''Merge edits on a USD transform back to USD and use undo redo.'''

        # To merge back to USD, we must edit as Maya first.
        (ps, aXlateOp, _, aUsdUfePathStr, aUsdUfePath, aUsdItem,
         bXlateOp, _, bUsdUfePathStr, bUsdUfePath, bUsdItem) = \
            createSimpleXformScene()

        cmds.mayaUsdEditAsMaya(aUsdUfePathStr)

        aMayaItem = ufe.GlobalSelection.get().front()
        (aMayaPath, aMayaPathStr, _, aMayaMatrix) = \
            setMayaTranslation(aMayaItem, om.MVector(4, 5, 6))

        bMayaPathStr   = aMayaPathStr + '|B'
        bMayaPath      = ufe.PathString.path(bMayaPathStr)
        bMayaItem      = ufe.Hierarchy.createItem(bMayaPath)
        (_, _, _, bMayaMatrix) = \
            setMayaTranslation(bMayaItem, om.MVector(10, 11, 12))

        # There should be a path mapping for the edit as Maya hierarchy.
        for (mayaItem, mayaPath, usdUfePath) in \
            zip([aMayaItem, bMayaItem], [aMayaPath, bMayaPath],
                [aUsdUfePath, bUsdUfePath]):
            mayaToUsd = ufe.PathMappingHandler.pathMappingHandler(mayaItem)
            self.assertEqual(
                ufe.PathString.string(mayaToUsd.fromHost(mayaPath)),
                ufe.PathString.string(usdUfePath))

        psHier = ufe.Hierarchy.hierarchy(ps)

        # Merge edits back to USD.
        cmds.mayaUsdMergeToUsd(aMayaPathStr)

        def verifyMergeToUsd():
            # Check that edits have been preserved in USD.
            for (usdUfePathStr, mayaMatrix, xlateOp) in \
                zip([aUsdUfePathStr, bUsdUfePathStr], [aMayaMatrix, bMayaMatrix], 
                    [aXlateOp, bXlateOp]):
                usdMatrix = xlateOp.GetOpTransform(
                    mayaUsd.ufe.getTime(usdUfePathStr))
                mayaValues = [v for v in mayaMatrix]
                usdValues = [v for row in usdMatrix for v in row]
            
                assertVectorAlmostEqual(self, mayaValues, usdValues)

            # There no longer are any Maya to USD path mappings.
            for mayaPath in [aMayaPath, bMayaPath]:
                self.assertEqual(len(mayaToUsd.fromHost(mayaPath)), 0)

            # Hierarchy is restored: USD item is child of proxy shape, Maya item is
            # not.  Be careful to use the Maya path rather than the Maya item, which
            # should no longer exist.
            self.assertIn(aUsdItem, psHier.children())
            self.assertNotIn(aMayaPath, [child.path() for child in psHier.children()])

            # Maya nodes are removed.
            for mayaPathStr in [aMayaPathStr, bMayaPathStr]:
                with self.assertRaises(RuntimeError):
                    om.MSelectionList().add(mayaPathStr)

        verifyMergeToUsd()

        cmds.undo()

        def verifyMergeIsUndone():
            # Maya nodes are back.
            for mayaPathStr in [aMayaPathStr, bMayaPathStr]:
                try:
                    om.MSelectionList().add(mayaPathStr)
                except:
                    self.assertTrue(False, "Selecting node should not have raise an exception")

        verifyMergeIsUndone()

        cmds.redo()

        verifyMergeToUsd()

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '3006', 'Test only available in UFE preview version 0.3.6 and greater')
    def testMergeToUsdToNonRootTargetInSessionLayer(self):
        '''Merge edits on a USD transform back to USD targeting a non-root destination path that
           does not exists in the destination layer.'''

        # To merge back to USD, we must edit as Maya first.
        (ps, aXlateOp, _, aUsdUfePathStr, aUsdUfePath, aUsdItem,
         bXlateOp, _, bUsdUfePathStr, bUsdUfePath, bUsdItem) = \
            createSimpleXformScene()

        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(bUsdUfePathStr))

        bMayaItem = ufe.GlobalSelection.get().front()
        (bMayaPath, bMayaPathStr, _, bMayaMatrix) = \
            setMayaTranslation(bMayaItem, om.MVector(10, 11, 12))

        psHier = ufe.Hierarchy.hierarchy(ps)

        # Merge edits back to USD.
        with mayaUsd.lib.OpUndoItemList():
            stage = mayaUsd.ufe.getStage(bUsdUfePathStr)
            stage.SetEditTarget(stage.GetSessionLayer())
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.mergeToUsd(bMayaPathStr))

        # Check that edits have been preserved in USD.
        bUsdMatrix = bXlateOp.GetOpTransform(
            mayaUsd.ufe.getTime(bUsdUfePathStr))
        mayaValues = [v for v in bMayaMatrix]
        usdValues = [v for row in bUsdMatrix for v in row]
    
        assertVectorAlmostEqual(self, mayaValues, usdValues)

    def testEquivalentTransformMergeToUsd(self):
        '''Merge edits on a USD transform back to USD when the new transform is equivalent.'''

        # Create a simple scene with a prim with a rotateY attribute.
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        psPath = ufe.PathString.path(psPathStr)
        ps = ufe.Hierarchy.createItem(psPath)
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        aPrim = stage.DefinePrim('/A', 'Xform')
        aXformable = UsdGeom.Xformable(aPrim)
        aRotOp = aXformable.AddRotateYOp()
        aRotOp.Set(5.)
        aUsdUfePathStr = psPathStr + ',/A'
        aUsdUfePath = ufe.PathString.path(aUsdUfePathStr)
        aUsdItem = ufe.Hierarchy.createItem(aUsdUfePath)

        # Edit as maya and set the rotation to 0, 5, 0 rotateXYZ, which is equivalent
        # to rotateY but has a different representation.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(aUsdUfePathStr))
            
        aMayaItem = ufe.GlobalSelection.get().front()
        (aMayaPath, aMayaPathStr, _, aMayaMatrix) = \
            setMayaRotation(aMayaItem, om.MVector(0, 5, 0))

        # Merge edits back to USD.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.mergeToUsd(aMayaPathStr))

        # Check that the rotateXYZ has *not* been added and the rotateY is still there.
        self.assertFalse(aPrim.HasAttribute("xformOp:rotateXYZ"))
        self.assertTrue(aPrim.HasAttribute("xformOp:rotateY"))
        self.assertTrue(aPrim.HasAttribute("xformOpOrder"))

        opOrder = aPrim.GetAttribute("xformOpOrder").Get()
        self.assertEqual(1, len(opOrder))
        self.assertEqual("xformOp:rotateY", opOrder[0])

    def testMergeToUsdReferencedPrim(self):
        '''Merge edits on a USD reference back to USD.'''

        # Create a simple scene with a Def prim with a USD reference.
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        psPath = ufe.PathString.path(psPathStr)
        ps = ufe.Hierarchy.createItem(psPath)
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        aPrim = stage.DefinePrim('/A', 'Xform')

        sphereFile = getTestScene("groupCmd", "sphere.usda")
        sdfRef = Sdf.Reference(sphereFile)
        primRefs = aPrim.GetReferences()
        primRefs.AddReference(sdfRef)
        self.assertTrue(aPrim.HasAuthoredReferences())

        aUsdUfePathStr = psPathStr + ',/A'
        aUsdUfePath = ufe.PathString.path(aUsdUfePathStr)
        aUsdItem = ufe.Hierarchy.createItem(aUsdUfePath)

        # Edit as maya and do nothing.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(aUsdUfePathStr))

        aMayaItem = ufe.GlobalSelection.get().front()

        # Merge edits back to USD.
        with mayaUsd.lib.OpUndoItemList():
            aMayaPath = aMayaItem.path()
            aMayaPathStr = ufe.PathString.string(aMayaPath)
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.mergeToUsd(aMayaPathStr))

        # Check that the reference is still there.
        self.assertTrue(aPrim.HasAuthoredReferences())

    def testMergeToUnchangedCylinder(self):
        '''Merge edits on unchanged data.'''

        # Create a stage from a file.
        testFile = getTestScene("cylinderSubset", "cylinder.usda")
        testDagPath, stage = mayaUtils.createProxyFromFile(testFile)
        usdCylPathString = testDagPath + ",/sphere_butNot_pCylinder1"

        cylBottomPrim = stage.GetPrimAtPath("/sphere_butNot_pCylinder1/bottom")
        bottomIndices = cylBottomPrim.GetAttribute("indices")
        self.assertEqual("int[]", bottomIndices.GetTypeName())

        # Edit as maya and do nothing.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(usdCylPathString))

        cylMayaItem = ufe.GlobalSelection.get().front()

        # Merge edits back to USD.
        with mayaUsd.lib.OpUndoItemList():
            cylMayaPath = cylMayaItem.path()
            cylMayaPathStr = ufe.PathString.string(cylMayaPath)
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.mergeToUsd(cylMayaPathStr))

        # Check that the indices are still valid.
        cylBottomPrim = stage.GetPrimAtPath("/sphere_butNot_pCylinder1/bottom")
        bottomIndices = cylBottomPrim.GetAttribute("indices")
        self.assertEqual("int[]", bottomIndices.GetTypeName())

if __name__ == '__main__':
    unittest.main(verbosity=2)
