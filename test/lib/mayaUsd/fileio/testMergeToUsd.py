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
from usdUtils import createSimpleXformScene, mayaUsd_createStageWithNewLayer, createLayeredStage, createSimpleXformSceneInCurrentLayer
from ufeUtils import ufeFeatureSetVersion

import mayaUsd.lib

import mayaUtils
import mayaUsd.ufe

from pxr import UsdGeom, Gf, Sdf, Usd

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

    @unittest.skipUnless(ufeFeatureSetVersion() >= 3, 'Test only available in UFE v3 or greater.')
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

        # Edit will re-allocate anything that was under pulled prim due to deactivation
        # Grab new references for /A/B prim
        bUsdPrim = mayaUsd.ufe.ufePathToPrim(bUsdUfePathStr)
        bXlateOp = UsdGeom.Xformable(bUsdPrim).GetOrderedXformOps()[0]
        
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

    @unittest.skipUnless(ufeFeatureSetVersion() >= 3, 'Test only available in UFE v3 or greater.')
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

        # Make a selection before merge edits back to USD.
        cmds.select('persp')
        previousSn = cmds.ls(sl=True, ufe=True, long=True)

        # Merge edits back to USD.
        cmds.mayaUsdMergeToUsd(aMayaPathStr)

        def verifyMergeToUsd():
            # Edit will re-allocate anything that was under pulled prim due to deactivation
            # Grab new references for /A/B prim
            bUsdPrim = mayaUsd.ufe.ufePathToPrim(bUsdUfePathStr)
            bXlateOp = UsdGeom.Xformable(bUsdPrim).GetOrderedXformOps()[0]

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

            # Selection is on the restored USD object.
            sn = cmds.ls(sl=True, ufe=True, long=True)
            self.assertEqual(len(sn), 1)
            self.assertEqual(sn[0], aUsdUfePathStr)

        verifyMergeToUsd()

        cmds.undo()

        def verifyMergeIsUndone():
            # Maya nodes are back.
            for mayaPathStr in [aMayaPathStr, bMayaPathStr]:
                try:
                    om.MSelectionList().add(mayaPathStr)
                except Exception:
                    self.assertTrue(False, "Selecting node should not have raise an exception")
            # Selection is restored.
            self.assertEqual(cmds.ls(sl=True, ufe=True, long=True), previousSn)

        verifyMergeIsUndone()

        cmds.redo()

        verifyMergeToUsd()

    @unittest.skipUnless(ufeFeatureSetVersion() >= 3, 'Test only available in UFE v3 or greater.')
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

    @unittest.skipUnless(ufeFeatureSetVersion() >= 3, 'Test only available in UFE v3 or greater.')
    def testMergeToUsdToParentLayer(self):
        '''Merge edits on a USD transform back to USD targeting a parent layer.'''

        # Create a multi-layered scene with prim on the lowest layer.
        # We should receive three layers: root and two additional sub-layers.
        (psPathStr, psPath, ps, layers) = createLayeredStage(2)
        self.assertEqual(3, len(layers))

        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.SetEditTarget(layers[-1])

        (aXlateOp, aXlation, aUsdUfePathStr, aUsdUfePath, aUsdItem,
         bXlateOp, bXlation, bUsdUfePathStr, bUsdUfePath, bUsdItem) = createSimpleXformSceneInCurrentLayer(psPathStr, ps)

        # To merge back to USD, we must edit as Maya first.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(bUsdUfePathStr))

        bMayaItem = ufe.GlobalSelection.get().front()
        (bMayaPath, bMayaPathStr, _, bMayaMatrix) = \
            setMayaTranslation(bMayaItem, om.MVector(10, 11, 12))

        psHier = ufe.Hierarchy.hierarchy(ps)

        # Before merging, set the edit target to the top non-root layer
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.SetEditTarget(layers[1])

        # Merge edits back to USD.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.mergeToUsd(bMayaPathStr))

        # Check that edits have been preserved in USD.
        bUsdMatrix = bXlateOp.GetOpTransform(
            mayaUsd.ufe.getTime(bUsdUfePathStr))
        mayaValues = [v for v in bMayaMatrix]
        usdValues = [v for row in bUsdMatrix for v in row]
    
        assertVectorAlmostEqual(self, mayaValues, usdValues)

        # Check that edits have been set as "overs"
        primSpecA = layers[1].GetPrimAtPath("/A")
        self.assertEqual(primSpecA.specifier, Sdf.SpecifierOver)
        primSpecB = layers[1].GetPrimAtPath("/A/B")
        self.assertEqual(primSpecB.specifier, Sdf.SpecifierOver)

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

    def testPrimvarsNormalsMergeToUsd(self):
        '''Merge edits when the original USD had primvars normals vs the Maya-generated normals.'''

        # Create a simple scene with a prim with a rotateY attribute.
        testFile = getTestScene("normals", "primvars_normals.usda")
        testDagPath, stage = mayaUtils.createProxyFromFile(testFile)
        usdSpherePathString = testDagPath + ",/pSphere1"


        # Check that the primvars:normals and primvaras:normals:indices are present,
        # and check a few others.
        sphereBottomPrim = stage.GetPrimAtPath("/pSphere1")
        self.assertTrue(sphereBottomPrim.HasAttribute("primvars:normals"))
        self.assertTrue(sphereBottomPrim.HasAttribute("primvars:normals:indices"))
        self.assertTrue(sphereBottomPrim.HasAttribute("primvars:st"))
        self.assertTrue(sphereBottomPrim.HasAttribute("primvars:st:indices"))
        sphereBottomPrim = None

        # Edit as maya and do nothing.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(usdSpherePathString))

        sphereMayaItem = ufe.GlobalSelection.get().front()

        # Merge edits back to USD.
        with mayaUsd.lib.OpUndoItemList():
            sphereMayaPath = sphereMayaItem.path()
            sphereMayaPathStr = ufe.PathString.string(sphereMayaPath)
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.mergeToUsd(sphereMayaPathStr))

        # Check that the normal attribute was created and the primvars:normals and
        # primvaras:normals:indices are gone.
        sphereBottomPrim = stage.GetPrimAtPath("/pSphere1")
        self.assertTrue(sphereBottomPrim.HasAttribute("normals"))
        self.assertFalse(sphereBottomPrim.HasAttribute("primvars:normals"))
        self.assertFalse(sphereBottomPrim.HasAttribute("primvars:normals:indices"))
        self.assertTrue(sphereBottomPrim.HasAttribute("primvars:st"))
        self.assertTrue(sphereBottomPrim.HasAttribute("primvars:st:indices"))

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

    def testMergeWithoutMaterials(self):
        '''Merge edits on data and not merging the materials.'''

        # Create a stage from a file.
        testFile = getTestScene("cylinder", "cylinder.usda")
        testDagPath, stage = mayaUtils.createProxyFromFile(testFile)
        usdCylPathString = testDagPath + ",/pCylinder1"

        # Verify that the original scene does not have a look (material) prim.
        cylLooksPrim = stage.GetPrimAtPath("/pCylinder1/Looks")
        self.assertFalse(cylLooksPrim.IsValid())

        # Edit as maya and do nothing.
        cmds.mayaUsdEditAsMaya(usdCylPathString)

        # Merge back to USD.
        cylMayaItem = ufe.GlobalSelection.get().front()
        cylMayaPath = cylMayaItem.path()
        cylMayaPathStr = ufe.PathString.string(cylMayaPath)
        cmds.mayaUsdMergeToUsd(cylMayaPathStr)

        # Verify that the merged scene added a look (material) prim.
        cylLooksPrim = stage.GetPrimAtPath("/pCylinder1/Looks")
        self.assertTrue(cylLooksPrim.IsValid())

        # Undo merge to USD.
        cmds.undo()

        # Merge back to USD with export options that disable materials.
        cmds.mayaUsdMergeToUsd(cylMayaPathStr, exportOptions='shadingMode=none')

        # Verify that the merged scene still does not have a look (material) prim.
        cylLooksPrim = stage.GetPrimAtPath("/pCylinder1/Looks")
        self.assertFalse(cylLooksPrim.IsValid())

    def testMergeWithoutMeshes(self):
        '''
        Merge edits on data and not merging the meshes.
        Edit again and merge with meshes. This should work.

        (Previously it would not work because the first merge without meshes
        authored the root prim as a USD assembly which then preventing
        authoring meshes under it.)
        '''

        # Create an stage with an xform (no meshes).
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        psPath = ufe.PathString.path(psPathStr)
        ps = ufe.Hierarchy.createItem(psPath)
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        aPrim = stage.DefinePrim('/A', 'Xform')
        self.assertTrue(aPrim)
        aUsdUfePathStr = psPathStr + ',/A'

        # Edit as maya and add a sphere under the node.
        def editAndAddSphere():
            cmds.mayaUsdEditAsMaya(aUsdUfePathStr)

            editedMayaItem = ufe.GlobalSelection.get().front()
            editedMayaPath = editedMayaItem.path()
            editedMayaPathStr = ufe.PathString.string(editedMayaPath)

            sphere = cmds.polySphere(r=1, sx=20, sy=20)
            cmds.parent(sphere, "A")

            return editedMayaPathStr
        
        editedPath = editAndAddSphere()

        # Merge back to USD wihtout meshes.
        cmds.mayaUsdMergeToUsd(editedPath, exportOptions='excludeExportTypes=[Mesh]')

        # Verify that the merged prim has no sphere and is not an assembly.
        def verify(expectSphere=False):
            aPrim = stage.GetPrimAtPath("/A")
            self.assertTrue(aPrim.IsValid())
            aModel = Usd.ModelAPI(aPrim)
            aKind = aModel.GetKind()
            self.assertEqual(aKind, '')
            aSphere = stage.GetPrimAtPath("/A/pSphere1")
            if expectSphere:
                self.assertTrue(aSphere.IsValid())
            else:
                self.assertFalse(aSphere.IsValid())

        verify()

        # Edit again as maya and add a sphere under the node.
        editedPath = editAndAddSphere()

        # Merge back to USD, this time with meshes.
        cmds.mayaUsdMergeToUsd(editedPath)

        # Verify that the merged prim has a sphere.
        verify(expectSphere=True)


    def testMergeInSubVariant(self):
        '''Merge edits on data that is inside a variant of a parent prim and contains variants.'''

        # Create a stage from a file.
        testFile = getTestScene("multiVariants", "multi_variants.usda")
        testDagPath, stage = mayaUtils.createProxyFromFile(testFile)

        # UFE path to interesting prims in the USD file.
        #
        #    /RootPrim
        #      /PrimWithVariant {modelingVariant = A or B or C}
        #        /ModelA
        #          /ConeA { extraStuff = Cube or Cylinder}
        #            /ExtraCube

        # Root prim
        rootSdfPath = "/RootPrim"

        # This prim has "modelingVariant" variant set with variants "A", "B" and "C"
        parentWithVariantSdfPath = rootSdfPath + "/PrimWithVariant"
        parentWithVariantSetName = "modelingVariant"
        parentWithVariantName = "A"

        # This prim is in variant A. This is the prim that will be edited
        editedSdfPath = parentWithVariantSdfPath + "/ModelA"

        # This is a sub-prim of the edited prim and has "extraStuff" variant set
        # with variants "Cube" and "Cylinder"
        subWithVariantSdfPath = editedSdfPath + "/ConeA"
        subWithVariantSetName = "extraStuff"
        subWithVariantName = "Cube"

        # This is a sub-sub-prim in the sub-prim with variant,
        # inside its "Cube" variant.
        subsubSdfPath = subWithVariantSdfPath + "/ExtraCube"

        # Verify that the original scene has the correct variants selected.
        rootPrim = stage.GetPrimAtPath(rootSdfPath)
        self.assertIsNotNone(rootPrim)
        self.assertTrue(rootPrim.IsValid())
        
        def verifyParentVariant():
            parentWithVariantPrim = stage.GetPrimAtPath(parentWithVariantSdfPath)
            self.assertIsNotNone(parentWithVariantPrim)
            self.assertTrue(parentWithVariantPrim.IsValid())
            self.assertTrue(parentWithVariantPrim.HasVariantSets())
            parentWithVariantVariantSet = parentWithVariantPrim.GetVariantSets().GetVariantSet(parentWithVariantSetName)
            self.assertIsNotNone(parentWithVariantVariantSet)
            self.assertEqual(parentWithVariantName, parentWithVariantVariantSet.GetVariantSelection())

        def verifySubVariant():
            subWithVariantPrim = stage.GetPrimAtPath(subWithVariantSdfPath)
            self.assertIsNotNone(subWithVariantPrim)
            self.assertTrue(subWithVariantPrim.IsValid())
            self.assertTrue(subWithVariantPrim.HasVariantSets())
            subWithVariantVariantSet = subWithVariantPrim.GetVariantSets().GetVariantSet(subWithVariantSetName)
            self.assertIsNotNone(subWithVariantVariantSet)
            self.assertEqual(subWithVariantName, subWithVariantVariantSet.GetVariantSelection())

        verifyParentVariant()
        verifySubVariant()

        # Edit as maya and move the ConeA and ExtraCube.
        cmds.mayaUsdEditAsMaya(testDagPath + "," + editedSdfPath)

        editedMayaItem = ufe.GlobalSelection.get().front()
        editedMayaPath = editedMayaItem.path()
        editedMayaPathStr = ufe.PathString.string(editedMayaPath)

        subWithVariantMayaPathStr = editedMayaPathStr + "|ConeA"
        subWithVariantMayaPath = ufe.PathString.path(subWithVariantMayaPathStr)
        subWithVariantMayaItem = ufe.Hierarchy.createItem(subWithVariantMayaPath)
        (subWithVariantMayaPath, subWithVariantMayaPathStr, _, subWithVariantMayaMatrix) = \
            setMayaTranslation(subWithVariantMayaItem, om.MVector(2, 3, 4))

        subsubMayaPathStr = subWithVariantMayaPathStr + "|ExtraCube"
        subsubMayaPath = ufe.PathString.path(subsubMayaPathStr)
        subsubMayaItem = ufe.Hierarchy.createItem(subsubMayaPath)
        (subsubMayaPath, subsubMayaPathStr, _, subsubMayaMatrix) = \
            setMayaTranslation(subsubMayaItem, om.MVector(-4, -5, -6))

        # Merge back to USD.
        cmds.mayaUsdMergeToUsd(editedMayaPathStr)

        verifyParentVariant()
        verifySubVariant()

        # Switch parent with variant to variant "B" and verify
        # that the subWithVariant and subsub are gone.

        def switchParentWithVariant(newVariantName):
            parentWithVariantPrim = stage.GetPrimAtPath(parentWithVariantSdfPath)
            parentWithVariantVariantSets = parentWithVariantPrim.GetVariantSets()
            parentWithVariantVariantSets.SetSelection(parentWithVariantSetName, newVariantName)

        def verifySwitch():
            editedPrim = stage.GetPrimAtPath(editedSdfPath)
            self.assertFalse(editedPrim.IsValid())
            subWithVariantPrim = stage.GetPrimAtPath(subWithVariantSdfPath)
            self.assertFalse(subWithVariantPrim.IsValid())
            subsubPrim = stage.GetPrimAtPath(subsubSdfPath)
            self.assertFalse(subsubPrim.IsValid())

        switchParentWithVariant("B")
        verifySwitch()

    def testMergeComponentInVariants(self):
        '''
        Merge edits on a component that is kept in a separate file.
        That component is inside the variants declared in its parent
        prim.

        Verify that editing it and merging it inside variants, that is
        with the merge option ignoreVariants set to false, the edits will
        be authored inside the variants.
        '''

        # Create a stage from a file.
        testFile = getTestScene("multiVariants", "with-variants-root.usda")
        testDagPath, stage = mayaUtils.createProxyFromFile(testFile)

        # Create a new anonymous layer under the root that will receive the edits.
        newLayer = Sdf.Layer.CreateAnonymous()
        stage.GetRootLayer().subLayerPaths.append(newLayer.identifier)
        stage.SetEditTarget(newLayer)

        # UFE path to interesting prims in the USD file.
        #
        #    /group (Xform, variant sets: geo, geo_vis, selection: render_high, render, xformOp inside both variants)
        #      /GEO (Xform, inside both variants)
        #        /pCube1 (Mesh, inside both variants)

        # Root prim
        # This prim has variant set "geo" set to "render_high" and "geo_vis" set to "render"
        rootSdfPath = "/group"

        geoVariantSetName = "geo"
        geoVariantSelection = "render_high"

        visVariantSetName = "geo_vis"
        visRenderVariant = "render"
        visPreviewVariant = "preview"

        # This prim is in variants will be edited
        editedSdfPath = rootSdfPath + "/GEO"

        # Verify that the original scene has the correct variants selected.
        def verifyVariantSelections():
            rootPrim = stage.GetPrimAtPath(rootSdfPath)
            self.assertIsNotNone(rootPrim)
            self.assertTrue(rootPrim.IsValid())
            self.assertTrue(rootPrim.HasVariantSets())
            geoVariantSetVariantSet = rootPrim.GetVariantSets().GetVariantSet(geoVariantSetName)
            self.assertIsNotNone(geoVariantSetVariantSet)
            self.assertEqual(geoVariantSelection, geoVariantSetVariantSet.GetVariantSelection())
            visVariantSetVariantSet = rootPrim.GetVariantSets().GetVariantSet(visVariantSetName)
            self.assertIsNotNone(visVariantSetVariantSet)
            self.assertEqual(visRenderVariant, visVariantSetVariantSet.GetVariantSelection())

        verifyVariantSelections()

        # Edit as maya and move the GEO.
        cmds.mayaUsdEditAsMaya(testDagPath + "," + editedSdfPath)

        editedMayaItem = ufe.GlobalSelection.get().front()
        editedMayaPath = editedMayaItem.path()
        editedMayaPathStr = ufe.PathString.string(editedMayaPath)

        (editedMayaPath, editedMayaPathStr, _, editedMayaMatrix) = \
            setMayaTranslation(editedMayaItem, om.MVector(-12, -13, 14))

        # Merge back to USD.
        cmds.mayaUsdMergeToUsd(editedMayaPathStr)

        verifyVariantSelections()

        def verifyTranslation():
            editedPrim = stage.GetPrimAtPath(editedSdfPath)
            self.assertTrue(editedPrim.IsValid())

            xformOps = UsdGeom.Xformable(editedPrim).GetOrderedXformOps()
            self.assertIsNotNone(xformOps)
            self.assertEqual(len(xformOps), 1)
            usdMatrix = xformOps[0].GetOpTransform(Usd.TimeCode())

            usdValues = [v for row in usdMatrix for v in row]
            mayaValues = [v for v in editedMayaMatrix]
            assertVectorAlmostEqual(self, mayaValues, usdValues)

        verifyTranslation()

        # Switch root to variant "preview" and verify that the translation
        # on the GEO prim is gone.
        #
        # For the variant switch to work, we must put the edit target on
        # the root layer again.

        stage.SetEditTarget(stage.GetRootLayer())

        def switchRootVisVariant():
            rootPrim = stage.GetPrimAtPath(rootSdfPath)
            rootVariantSets = rootPrim.GetVariantSets()
            rootVariantSets.SetSelection(visVariantSetName, visPreviewVariant)

        def verifySwitch():
            editedPrim = stage.GetPrimAtPath(editedSdfPath)
            self.assertTrue(editedPrim.IsValid())

            xformOps = UsdGeom.Xformable(editedPrim).GetOrderedXformOps()
            self.assertListEqual(xformOps, [])

        switchRootVisVariant()
        verifySwitch()

    def testMergeComponentOutsideVariants(self):
        '''
        Merge edits on a component that is kept in a separate file.
        That component is *outside* the variants declared in its parent
        prim.

        Verify that editing it and merging it *outside* variants, that is
        with the merge option ignoreVariants set to true, the edits will
        be authored *outside* the variants.
        '''

        # Create a stage from a file.
        testFile = getTestScene("multiVariants", "no-variant-root.usda")
        testDagPath, stage = mayaUtils.createProxyFromFile(testFile)

        # Create a new anonymous layer under the root that will receive the edits.
        newLayer = Sdf.Layer.CreateAnonymous()
        stage.GetRootLayer().subLayerPaths.append(newLayer.identifier)
        stage.SetEditTarget(newLayer)

        # UFE path to interesting prims in the USD file.
        #
        #    /group (Xform, variant sets: geo, geo_vis, selection: render_high, render, xformOp inside both variants)
        #      /GEO (Xform, outside both variants)
        #        /pCube1 (Mesh, outside both variants)

        # Root prim
        # This prim has variant set "geo" set to "render_high" and "geo_vis" set to "render"
        rootSdfPath = "/group"

        geoVariantSetName = "geo"
        geoVariantSelection = "render_high"

        visVariantSetName = "geo_vis"
        visRenderVariant = "render"
        visPreviewVariant = "preview"

        # This prim is in variants will be edited
        editedSdfPath = rootSdfPath + "/GEO"

        # Verify that the original scene has the correct variants selected.
        def verifyVariantSelections():
            rootPrim = stage.GetPrimAtPath(rootSdfPath)
            self.assertIsNotNone(rootPrim)
            self.assertTrue(rootPrim.IsValid())
            self.assertTrue(rootPrim.HasVariantSets())
            geoVariantSetVariantSet = rootPrim.GetVariantSets().GetVariantSet(geoVariantSetName)
            self.assertIsNotNone(geoVariantSetVariantSet)
            self.assertEqual(geoVariantSelection, geoVariantSetVariantSet.GetVariantSelection())
            visVariantSetVariantSet = rootPrim.GetVariantSets().GetVariantSet(visVariantSetName)
            self.assertIsNotNone(visVariantSetVariantSet)
            self.assertEqual(visRenderVariant, visVariantSetVariantSet.GetVariantSelection())

        verifyVariantSelections()

        # Edit as maya and move the GEO.
        cmds.mayaUsdEditAsMaya(testDagPath + "," + editedSdfPath)

        editedMayaItem = ufe.GlobalSelection.get().front()
        editedMayaPath = editedMayaItem.path()
        editedMayaPathStr = ufe.PathString.string(editedMayaPath)

        (editedMayaPath, editedMayaPathStr, _, editedMayaMatrix) = \
            setMayaTranslation(editedMayaItem, om.MVector(-12, -13, 14))

        # Merge back to USD.
        cmds.mayaUsdMergeToUsd(editedMayaPathStr, ignoreVariants=1)

        verifyVariantSelections()

        def verifyTranslation():
            editedPrim = stage.GetPrimAtPath(editedSdfPath)
            self.assertTrue(editedPrim.IsValid())

            xformOps = UsdGeom.Xformable(editedPrim).GetOrderedXformOps()
            self.assertIsNotNone(xformOps)
            self.assertEqual(len(xformOps), 1)
            usdMatrix = xformOps[0].GetOpTransform(Usd.TimeCode())

            usdValues = [v for row in usdMatrix for v in row]
            mayaValues = [v for v in editedMayaMatrix]
            assertVectorAlmostEqual(self, mayaValues, usdValues)

        verifyTranslation()

        # Switch root to variant "preview" and verify that the translation
        # on the GEO is still there.
        #
        # For the variant switch to work, we must put the edit target on
        # the root layer again.

        stage.SetEditTarget(stage.GetRootLayer())

        def switchRootVisVariant():
            rootPrim = stage.GetPrimAtPath(rootSdfPath)
            rootVariantSets = rootPrim.GetVariantSets()
            rootVariantSets.SetSelection(visVariantSetName, visPreviewVariant)

        def verifySwitch():
            editedPrim = stage.GetPrimAtPath(editedSdfPath)
            self.assertTrue(editedPrim.IsValid())

            xformOps = UsdGeom.Xformable(editedPrim).GetOrderedXformOps()
            self.assertIsNotNone(xformOps)
            self.assertEqual(len(xformOps), 1)
            usdMatrix = xformOps[0].GetOpTransform(Usd.TimeCode())

            usdValues = [v for row in usdMatrix for v in row]
            mayaValues = [v for v in editedMayaMatrix]
            assertVectorAlmostEqual(self, mayaValues, usdValues)

        switchRootVisVariant()
        verifySwitch()

if __name__ == '__main__':
    unittest.main(verbosity=2)
