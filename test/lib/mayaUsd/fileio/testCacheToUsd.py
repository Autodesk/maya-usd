#!/usr/bin/env python

#
# Copyright 2022 Autodesk
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
from ufeUtils import createItem, createHierarchy

import mayaUsd.lib

from mayaUsd.lib import cacheToUsd

import mayaUtils
import usdUtils
import ufeUtils
import mayaUsd.ufe
import mayaUsdAddMayaReference
import mayaUsdMayaReferenceUtils as mayaRefUtils

from pxr import Usd, UsdGeom, Sdf, Pcp, Gf

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as om

import ufe

import unittest

from testUtils import assertVectorAlmostEqual, getTestScene

import os
import shutil

def createMayaRefPrimSiblingCache(testCase, cacheParent, cacheParentPathStr):
    '''Create a Maya reference prim for the sibling cache test case.'''
    testCase.assertFalse(cacheParent.HasVariantSets())
    mayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
        cacheParentPathStr, testCase.mayaSceneStr, testCase.kDefaultNamespace)

    # The sibling cache is a new child of its parent, thus the parent has no
    # variant data.
    return [mayaRefPrim, None, None, None, None, False]

def createMayaRefPrimSiblingCacheWithRelativePath(testCase, cacheParent, cacheParentPathStr):
    options = createMayaRefPrimSiblingCache(testCase, cacheParent, cacheParentPathStr)
    options[-1] = True
    return options

def checkSiblingCacheParent(testCase, cacheParentChildren, vs, vn):
    '''Verify the cache parent after caching in the sibling cache test case.'''
    # After caching the cache parent has the original child and its sibling
    # cache.
    testCase.assertTrue(len(cacheParentChildren), 2)

def createMayaRefPrimVariantCache(testCase, cacheParent, cacheParentPathStr):
    '''Create a Maya reference prim for the variant cache test case.'''
    # Add a variant set to the cache parent.
    variantSetName = 'animation'
    variantName = 'Rig'
    groupPrim = None
    autoEdit = False

    # Author a Maya reference prim to the rig variant.
    mayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
        cacheParentPathStr, testCase.mayaSceneStr, testCase.kDefaultNamespace,
        'Reference1', groupPrim, (variantSetName, variantName), autoEdit)

    refParent = mayaRefPrim.GetParent()
    vs = refParent.GetVariantSets()
    variantSet = vs.GetVariantSet(variantSetName)

    return [mayaRefPrim, variantSetName, variantSet, variantName, 'Cache', False]

def createMayaRefPrimVariantCacheWithRelativePath(testCase, cacheParent, cacheParentPathStr):
    options = createMayaRefPrimVariantCache(testCase, cacheParent, cacheParentPathStr)
    options[-1] = True
    return options

def checkVariantCacheParent(testCase, cacheParentChildren, variantSet, cacheVariantName):
    '''Verify the cache parent after caching in the variant cache test case.'''
    # The variant set in the cache parent now has the cache variant
    # selected, and its only child is called the cache prim name.
    testCase.assertEqual(variantSet.GetVariantSelection(), cacheVariantName)
    testCase.assertTrue(len(cacheParentChildren), 1)

class CacheToUsdTestCase(unittest.TestCase):
    '''Test cache to USD: commit edits done to a Maya reference back into USD.
    '''

    kDefaultNamespace = 'simpleSphere'

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

        # Create a pure Maya scene to reference in.
        cls.mayaSceneStr = mayaUtils.createSingleSphereMayaScene()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def getCacheDir(self):
        return os.path.abspath('testCacheToUsd')

    def getCacheFileName(self, basename="cache.usda"):
        return os.path.join(self.getCacheDir(), basename)

    def removeCacheFiles(self):
        '''
        Remove the cache file if it exists. Ignore error if it does not exists.
        '''
        try:
            shutil.rmtree(self.getCacheDir())
        except Exception:
            pass

    def getRootLayerFileName(self):
        return os.path.abspath('testCacheToUsdRootLayer.usda')

    def removeRootLayerFile(self):
        '''
        Remove the root layer file if it exists. Ignore error if it does not exists.
        '''
        try:
            os.remove(self.getRootLayerFileName())
        except Exception:
            pass

    def setUp(self):
        # Start each test with a new scene with empty stage.
        cmds.file(new=True, force=True)

        # Make sure the cache file is removed in case a previous test failed mid-way.
        self.removeCacheFiles()
        self.removeRootLayerFile()

        import mayaUsd_createStageWithNewLayer
        self.proxyShapePathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        self.stage = mayaUsd.lib.GetPrim(self.proxyShapePathStr).GetStage()

    def tearDown(self):
        self.removeCacheFiles()

    def makeRootLayerNotAnonymous(self):
        self.stage.GetRootLayer().identifier = self.getRootLayerFileName()

    def verifyCacheFileDefaultPrim(self, cacheFilename, defaultPrimName):
        layer = Sdf.Layer.FindOrOpen(cacheFilename)
        self.assertIsNotNone(layer)
        self.assertTrue(layer.HasDefaultPrim())
        defPrim = layer.defaultPrim
        self.assertIsNotNone(defPrim)
        self.assertEqual(defPrim, defaultPrimName)

    def runTestCacheToUsd(self, createMayaRefPrimFn, checkCacheParentFn):
        '''Cache edits on a pulled Maya reference prim test engine.'''

        # Create a Maya reference prim using the defaults, under a
        # newly-created parent, without any variant sets.
        cacheParent = self.stage.DefinePrim('/CacheParent', 'Xform')
        cacheParentPathStr = self.proxyShapePathStr + ',/CacheParent'
        self.assertFalse(cacheParent.HasVariantSets())

        (mayaRefPrim, variantSetName, variantSet, refVariantName, cacheVariantName, relativePath) = \
            createMayaRefPrimFn(self, cacheParent, cacheParentPathStr)

        # The Maya reference prim is a child of the cache parent.
        cacheParentItem = createItem(cacheParentPathStr)
        mayaRefPrimPathStr = cacheParentPathStr + '/' + mayaRefPrim.GetName()
        mayaRefPrimItem = createItem(mayaRefPrimPathStr)
        cacheParentHier = ufe.Hierarchy.hierarchy(cacheParentItem)
        cacheParentChildren = cacheParentHier.children()

        self.assertTrue(len(cacheParentChildren), 1)
        self.assertEqual(cacheParentChildren[0], mayaRefPrimItem)

        # At this point no Maya reference node has been created.
        self.assertEqual(len(cmds.ls(type='reference')), 0)

        # testAddMayaReference tests Maya reference prim creation, so we do not
        # repeat these tests here.  Edit the Maya reference prim as Maya, which
        # will create and load the corresponding Maya reference node.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(
                mayaRefPrimPathStr))

        # A Maya reference node has been created, and it is loaded.
        rns = cmds.ls(type='reference')
        rn = rns[0]
        self.assertEqual(len(rns), 1)
        self.assertTrue(cmds.referenceQuery(rn, isLoaded=True))

        # The Maya reference prim has been replaced by a Maya transform, which
        # is a child of the cache parent.
        cacheParentChildren = cacheParentHier.children()
        self.assertTrue(len(cacheParentChildren), 1)
        mayaTransformItem = cacheParentChildren[0]
        self.assertEqual(mayaTransformItem.nodeType(), 'transform')

        # The child of the Maya transform is the top-level transform of the
        # referenced Maya scene.
        mayaTransformHier = createHierarchy(mayaTransformItem)
        mayaTransformChildren = mayaTransformHier.children()
        self.assertTrue(len(mayaTransformChildren), 1)
        sphereTransformItem = mayaTransformChildren[0]
        self.assertEqual(sphereTransformItem.nodeType(), 'transform')

        # As the sphereTransformItem is a pulled node, its path is a pure Maya
        # Dag path.  Confirm that it is a referenced node.
        sphereTransformPath = sphereTransformItem.path()
        self.assertEqual(sphereTransformPath.nbSegments(), 1)
        self.assertEqual(sphereTransformPath.runTimeId(), 1)
        sphereTransformPathStr = ufe.PathString.string(sphereTransformPath)
        sphereTransformObj = om.MSelectionList().add(sphereTransformPathStr).getDagPath(0).node()
        self.assertTrue(om.MFnDependencyNode(sphereTransformObj).isFromReferencedFile)

        # Make an edit in Maya.
        (_, _, _, mayaMatrix) = \
            setMayaTranslation(sphereTransformItem, om.MVector(4, 5, 6))

        # Cache to USD, using a payload composition arc.
        defaultExportOptions = cacheToUsd.getDefaultExportOptions()
        cacheFile = self.getCacheFileName()
        cachePrimName = 'cachePrimName'
        payloadOrReference = 'Payload'
        listEditType = 'Prepend'
        # In the sibling cache case variantSetName and cacheVariantName will be
        # None.
        cacheOptions = cacheToUsd.createCacheCreationOptions(
            defaultExportOptions, cacheFile, cachePrimName,
            payloadOrReference, listEditType, variantSetName, cacheVariantName, relativePath)

        # Before caching, the cache file does not exist.
        self.assertFalse(os.path.exists(cacheFile))

        # Cache edits back to USD.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.mergeToUsd(ufe.PathString.string(mayaTransformItem.path()), cacheOptions))

        # The Maya reference node has been unloaded.
        self.assertFalse(cmds.referenceQuery(rn, isLoaded=True))

        # The USD cache file has been saved to the file system.
        self.assertTrue(os.path.exists(cacheFile))

        # There is a new child under the cache parent, with the chosen cache
        # prim name.
        cacheParentChildren = cacheParentHier.children()
        checkCacheParentFn(self, cacheParentChildren, variantSet, cacheVariantName)
        cacheItem = next((c for c in cacheParentChildren if c.nodeName() == cachePrimName), None)
        self.assertIsNotNone(cacheItem)

        # The Maya transformation has been transferred to the USD cache, in the
        # sphere child of the cache item.
        cacheHier = createHierarchy(cacheItem)
        cacheChildren = cacheHier.children()
        sphereItem = next((c for c in cacheChildren if c.nodeType() == 'Mesh'), None)
        self.assertIsNotNone(sphereItem)

        sphereItemPathStr = ufe.PathString.string(sphereItem.path())
        spherePrim = mayaUsd.ufe.ufePathToPrim(sphereItemPathStr)
        sphereXformOps = UsdGeom.Xformable(spherePrim).GetOrderedXformOps()
        self.assertEqual(len(sphereXformOps), 1)
        sphereXformOp = sphereXformOps[0]
        usdMatrix = sphereXformOp.GetOpTransform(mayaUsd.ufe.getTime(sphereItemPathStr))

        mayaValues = [v for v in mayaMatrix]
        usdValues = [v for row in usdMatrix for v in row]
        
        assertVectorAlmostEqual(self, mayaValues, usdValues)

        # The cache prim has a payload composition arc.
        cachePrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(cacheItem.path()))
        query = Usd.PrimCompositionQuery(cachePrim)
        foundPayload = False
        for arc in query.GetCompositionArcs():
            if arc.GetArcType() == Pcp.ArcTypePayload:
                foundPayload = True
                break

        self.assertTrue(foundPayload)
        self.assertTrue(cachePrim.HasAuthoredPayloads())

        # If using relative path, verify the payload source is using
        # a relative file path.
        #
        # Note: the reason we need to look at the raw contents of the
        #       root layer is that no OpenUSD API allows us to peek
        #       at the raw payload file path:
        #
        #       - UsdPayloads as returned by UsdPrim::GetPayloads() do not
        #         allow to retrieve the individual payloads. The docs even
        #         says so and advise to use UsdPrimCompositionQuery.
        #       - UsdPrimCompositionQuery returns UsdPrimCompositionQueryArc
        #         which don't give access to the file path, only to the
        #         resolved SdfLayer.
        #       - Due to the way layer caching works in OpenUSD, asking for
        #         the layer identifier returns the absolute path.
        if relativePath:
            if self.stage.GetRootLayer().anonymous:
                self.assertNotIn('payload = @testCacheToUsd/cache.usda', self.stage.GetRootLayer().ExportToString())
                self.assertIn('payload = @' + cacheFile, self.stage.GetRootLayer().ExportToString())
                self.makeRootLayerNotAnonymous()
                mayaUsd.lib.Util.updatePostponedRelativePaths(self.stage.GetRootLayer())
                
            self.assertIn('payload = @testCacheToUsd/cache.usda', self.stage.GetRootLayer().ExportToString())

        self.verifyCacheFileDefaultPrim(cacheFile, cachePrimName)

    def testCacheToUsdSibling(self):
        self.makeRootLayerNotAnonymous()
        self.runTestCacheToUsd(createMayaRefPrimSiblingCache, checkSiblingCacheParent)

    def testCacheToUsdSiblingWithRelativePath(self):
        self.makeRootLayerNotAnonymous()
        self.runTestCacheToUsd(createMayaRefPrimSiblingCacheWithRelativePath, checkSiblingCacheParent)

    def testCacheToUsdVariant(self):
        self.makeRootLayerNotAnonymous()
        self.runTestCacheToUsd(createMayaRefPrimVariantCache, checkVariantCacheParent)

    def testCacheToUsdVariantWithRelativePath(self):
        self.makeRootLayerNotAnonymous()
        self.runTestCacheToUsd(createMayaRefPrimVariantCacheWithRelativePath, checkVariantCacheParent)

    def testCacheToUsdVariantWithRelativePathInAnonLayer(self):
        self.runTestCacheToUsd(createMayaRefPrimVariantCacheWithRelativePath, checkVariantCacheParent)

    def testAutoEditAndCache(self):
        '''Test editing then caching a Maya Reference.

        Add a Maya Reference using auto-edit, then cache the edits.
        '''
        self.makeRootLayerNotAnonymous()
        kDefaultPrimName = mayaRefUtils.defaultMayaReferencePrimName()

        # Since this is a brand new prim, it should not have variant sets.
        primTestDefault = self.stage.DefinePrim('/Test_Default', 'Xform')
        primPathStr = self.proxyShapePathStr + ',/Test_Default'
        self.assertFalse(primTestDefault.HasVariantSets())

        variantSetName = 'new_variant_set'
        variantName =  'new_variant'

        mayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            primPathStr,
            self.mayaSceneStr,
            self.kDefaultNamespace,
            variantSet=(variantSetName, variantName),
            mayaAutoEdit=True)

        # The prim should have variant set.
        self.assertTrue(primTestDefault.HasVariantSets())

        # Verify that a Maya Reference prim was created.
        self.assertTrue(mayaRefPrim.IsValid())
        self.assertEqual(str(mayaRefPrim.GetName()), kDefaultPrimName)
        self.assertEqual(mayaRefPrim, primTestDefault.GetChild(kDefaultPrimName))
        self.assertTrue(mayaRefPrim.GetPrimTypeInfo().GetTypeName(), 'MayaReference')

        attr = mayaRefPrim.GetAttribute('mayaAutoEdit')
        self.assertTrue(attr.IsValid())
        self.assertEqual(attr.Get(), True)

        # Cache to USD, using a payload composition arc.
        defaultExportOptions = cacheToUsd.getDefaultExportOptions()
        cacheFile = self.getCacheFileName()
        cachePrimName = 'cachePrimName'
        payloadOrReference = 'Payload'
        listEditType = 'Prepend'

        userArgs = cacheToUsd.createCacheCreationOptions(
            defaultExportOptions, cacheFile, cachePrimName, payloadOrReference,
            listEditType, variantSetName, variantName)

        # Merge Maya edits.
        aMayaItem = ufe.GlobalSelection.get().front()
        aMayaPath = aMayaItem.path()
        aMayaPathStr = ufe.PathString.string(aMayaPath)

        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.mergeToUsd(aMayaPathStr, userArgs))
        
        # Verify that the auto-edit is still on.
        attr = mayaRefPrim.GetAttribute('mayaAutoEdit')
        self.assertTrue(attr.IsValid())
        self.assertEqual(attr.Get(), True)

        self.verifyCacheFileDefaultPrim(cacheFile, cachePrimName)

    def verifyMergedRhinoRig(self, cachePrimName, primPathStr, mayaNamespace):
        # Verify a prim deep in the rig got multiple xform ops of the same type
        # and they got proper suffixes.
        foot_node_path_names = [
            'rhino_rig_030_rig_grp',
            'rhino_rig_030_world_cnt_001_grp',
            'rhino_rig_030_world_cnt_001_Shift',
            'rhino_rig_030_world_cnt_001',
            'rhino_rig_030_world_cnt_002_grp',
            'rhino_rig_030_world_cnt_002_Shift',
            'rhino_rig_030_world_cnt_002_cnt',
            'rhino_rig_030_rig_setup_grp',
            'rhino_rig_030_cn_legs_grp',
            'rhino_rig_030_rt_ft_leg_grp',
            'rhino_rig_030_control_grp',
            'rhino_rig_030_rt_ft_foot_001_cnt_grp',
            'rhino_rig_030_rt_ft_foot_001_cnt_Shift',
            'rhino_rig_030_rt_ft_foot_001_cnt'
        ]

        foot_prim_with_multiple_translate = '/'.join(
            [cachePrimName] + [mayaNamespace + "_" + node for node in foot_node_path_names]
        )

        cacheUfePathStr = primPathStr + "/" + foot_prim_with_multiple_translate
        cacheUfeItem = ufeUtils.createItem(cacheUfePathStr)
        self.assertIsNotNone(cacheUfeItem)
        cacheUfeHier = ufe.Hierarchy.hierarchy(cacheUfeItem)
        cacheUsdPrim = usdUtils.getPrimFromSceneItem(cacheUfeItem)
        self.assertTrue(cacheUsdPrim.IsValid())
        xformOp = cacheUsdPrim.GetAttribute('xformOpOrder')
        self.assertTrue(xformOp.IsValid())
        value = xformOp.Get()
        self.assertIn("xformOp:translate", value)
        self.assertIn("xformOp:rotateXYZ", value)

    def testEditAndMergeRigMayaRef(self):
        '''
        Test editing then merge a Maya Reference to a complex animated rig.
        In particular, the rig generates multiple translation xform on some
        prim, verify that we do get these multiple translations.
        '''
        self.makeRootLayerNotAnonymous()
        kDefaultPrimName = mayaRefUtils.defaultMayaReferencePrimName()

        # Since this is a brand new prim, it should not have variant sets.
        primTestDefault = self.stage.DefinePrim('/Test_Default', 'Xform')
        primPathStr = self.proxyShapePathStr + ',/Test_Default'
        self.assertFalse(primTestDefault.HasVariantSets())

        rigScene = getTestScene("rhino_rig", "anim_001.ma")
        mayaRefNamespace = "anim_001"

        mayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            primPathStr,
            rigScene,
            mayaRefNamespace,
            mayaAutoEdit=False)

        mayaRefPrimUfePathStr = primPathStr + "/" + mayaRefPrim.GetName()
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(mayaRefPrimUfePathStr))

        # The prim should not have any variant set.
        self.assertFalse(primTestDefault.HasVariantSets())

        # Verify that a Maya Reference prim was created.
        self.assertTrue(mayaRefPrim.IsValid())
        self.assertEqual(str(mayaRefPrim.GetName()), kDefaultPrimName)
        self.assertEqual(mayaRefPrim, primTestDefault.GetChild(kDefaultPrimName))
        self.assertTrue(mayaRefPrim.GetPrimTypeInfo().GetTypeName(), 'MayaReference')

        # Cache to USD, using a payload composition arc.
        defaultExportOptions = cacheToUsd.getDefaultExportOptions()
        cacheFile = self.getCacheFileName()
        cachePrimName = 'cachePrimName'
        payloadOrReference = 'Payload'
        listEditType = 'Prepend'
        variantSetName = None
        variantName = None

        userArgs = cacheToUsd.createCacheCreationOptions(
            defaultExportOptions, cacheFile, cachePrimName, payloadOrReference,
            listEditType, variantSetName, variantName)

        # Merge Maya edits.
        aMayaItem = ufe.GlobalSelection.get().front()
        aMayaPath = aMayaItem.path()
        aMayaPathStr = ufe.PathString.string(aMayaPath)
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.mergeToUsd(aMayaPathStr, userArgs))

        self.verifyMergedRhinoRig(cachePrimName, primPathStr, mayaRefNamespace)

    def testEditAndBatchMergeRigMayaRefs(self):
        '''
        Test editing then merge a Maya Reference to a complex animated rig.
        In particular, the rig generates multiple translation xform on some
        prim, verify that we do get these multiple translations.
        '''
        self.makeRootLayerNotAnonymous()

        defaultPrim = self.stage.DefinePrim('/Test_Default', 'Xform')
        primPathStr = self.proxyShapePathStr + ',' + str(defaultPrim.GetPath())

        rigScene = getTestScene("rhino_rig", "anim_001.ma")
        defaultExportOptions = cacheToUsd.getDefaultExportOptions()
        payloadOrReference = 'Payload'
        listEditType = 'Prepend'
        variantSetName = None
        variantName = None

        # Create and edit multiple (3) mayaReferences to anim_001.ma.
        # Also prepare options for Cache to USD, using a payload composition arc.
        rigMayaPathAndOptions = []
        rigCacheNames = []
        for idx in range(3):
            # Create the mayaReference prim, autoEdited.
            refPrimName = "MayaReference_" + str(idx)
            mayaRefNamespace = refPrimName + "_NS"

            mayaUsdAddMayaReference.createMayaReferencePrim(
                primPathStr,
                rigScene,
                mayaRefNamespace,
                mayaReferencePrimName=refPrimName,
                mayaAutoEdit=True)

            # Add option for later merge in a signel call to mergeToUsd.
            cachePrimName = refPrimName + "_Cached"
            cacheFile = self.getCacheFileName(refPrimName + ".usda")

            # Before caching, the cache file does not exist.
            self.assertFalse(os.path.exists(cacheFile))

            userArgs = cacheToUsd.createCacheCreationOptions(
                defaultExportOptions, cacheFile, cachePrimName, payloadOrReference,
                listEditType, variantSetName, variantName)

            mayaPathStr = ufe.PathString.string(ufe.GlobalSelection.get().front().path())

            rigMayaPathAndOptions.append((mayaPathStr, userArgs))
            rigCacheNames.append((cachePrimName, mayaRefNamespace))

        # Merge Maya edits in a single batch.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.mergeToUsd(rigMayaPathAndOptions))

        # Verify the merged items.
        for cachePrimName, mayaRefNamespace in rigCacheNames:
            self.verifyMergedRhinoRig(cachePrimName, primPathStr, mayaRefNamespace)

    def runTestMayaRefPrimTransform(self, createMayaRefPrimFn, checkCacheParentFn):
        '''
        Run a test transforming the Maya Reference prim, editing it in Maya,
        then merging back the result. The caching can be done in a variant or not.
        '''

        # The Maya reference prim is transformable.  Change its local
        # transformation and confirm that the transformation appears in the
        # corresponding Maya transform node.

        # Create a Maya reference prim using the defaults, under a
        # newly-created parent.
        self.makeRootLayerNotAnonymous()
        cacheParent = self.stage.DefinePrim('/CacheParent', 'Xform')
        cacheParentPathStr = self.proxyShapePathStr + ',/CacheParent'
        self.assertFalse(cacheParent.HasVariantSets())

        (mayaRefPrim, variantSetName, variantSet, refVariantName, cacheVariantName, relativePath) = \
            createMayaRefPrimFn(self, cacheParent, cacheParentPathStr)

        # Set an initial translation.
        editTarget = self.stage.GetEditTarget()
        if variantSet:
            variantSet.SetVariantSelection(refVariantName)
            editTarget = variantSet.GetVariantEditTarget(editTarget.GetLayer())

        with Usd.EditContext(self.stage, editTarget):
            xformable = UsdGeom.Xformable(mayaRefPrim)
            xlateOp = xformable.AddTranslateOp()
            xlation = Gf.Vec3d(1, 2, 3)
            xlateOp.Set(xlation)

        # The Maya reference prim is a child of the cache parent.  This is
        # already tested in testCacheToUsd{Sibling,Variant}.
        mayaRefPrimPathStr = cacheParentPathStr + '/' + mayaRefPrim.GetName()

        # testAddMayaReference tests Maya reference prim creation, so we do not
        # repeat these tests here.  Edit the Maya reference prim as Maya, which
        # will create and load the corresponding Maya reference node.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(
                mayaRefPrimPathStr))

        # The Maya reference prim has been replaced by a Maya transform, which
        # is a child of the cache parent.
        cacheParentHier = createHierarchy(cacheParentPathStr)
        cacheParentChildren = cacheParentHier.children()
        self.assertTrue(len(cacheParentChildren), 1)
        mayaTransformItem = cacheParentChildren[0]
        self.assertEqual(mayaTransformItem.nodeType(), 'transform')

        # The Maya transform has the same transform as the Maya reference prim.
        mayaTransformPathStr = ufe.PathString.string(mayaTransformItem.path())
        dagPath = om.MSelectionList().add(mayaTransformPathStr).getDagPath(0)
        fn= om.MFnTransform(dagPath)
        self.assertEqual(fn.translation(om.MSpace.kObject), om.MVector(*xlation))
        mayaMatrix = fn.transformation().asMatrix()
        usdMatrix = xlateOp.GetOpTransform(mayaUsd.ufe.getTime(mayaRefPrimPathStr))
        mayaValues = [v for v in mayaMatrix]
        usdValues = [v for row in usdMatrix for v in row]

        assertVectorAlmostEqual(self, mayaValues, usdValues)

        # Change the Maya transform translation, then cache to USD.  Confirm
        # that the Maya edit is transported back to the Maya reference prim.
        # Use any cache options, not the purpose of this test.
        fn.setTranslation(om.MVector(4, 5, 6), om.MSpace.kObject)

        defaultExportOptions = cacheToUsd.getDefaultExportOptions()
        cacheFile = self.getCacheFileName()
        cachePrimName = 'cachePrimName'
        payloadOrReference = 'Payload'
        listEditType = 'Prepend'

        cacheOptions = cacheToUsd.createCacheCreationOptions(
            defaultExportOptions, cacheFile, cachePrimName,
            payloadOrReference, listEditType, variantSetName, cacheVariantName, relativePath)

        # Before caching, the cache file does not exist.
        self.assertFalse(os.path.exists(cacheFile))

        # Cache edits back to USD.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.mergeToUsd(ufe.PathString.string(mayaTransformItem.path()), cacheOptions))

        # There is a new child under the cache parent, with the chosen cache
        # prim name.
        cacheParentChildren = cacheParentHier.children()
        checkCacheParentFn(self, cacheParentChildren, variantSet, cacheVariantName)

        # Maya reference prim should now have the updated transformation.
        if variantSetName:
            cacheParent.GetVariantSet(variantSetName).SetVariantSelection('Rig')

        with Usd.EditContext(self.stage, editTarget):
            xformable = UsdGeom.Xformable(mayaRefPrim)
            ops = xformable.GetOrderedXformOps()
            self.assertEqual(len(ops), 1)
            usdMatrix = ops[0].GetOpTransform(mayaUsd.ufe.getTime(mayaRefPrimPathStr))
            usdValues = [v for row in usdMatrix for v in row]
            assertVectorAlmostEqual(self, usdValues, [1, 0, 0, 0, 0, 1, 0, 0, 0, 0,
                                                    1, 0, 4, 5, 6, 1])

        self.verifyCacheFileDefaultPrim(cacheFile, cachePrimName)

    def testMayaRefPrimTransform(self):
        '''Test transforming the Maya Reference prim, editing it in Maya, then merging back the result.'''
        self.makeRootLayerNotAnonymous()
        self.runTestMayaRefPrimTransform(createMayaRefPrimSiblingCache, checkSiblingCacheParent)

    def testMayaRefPrimTransformToVariant(self):
        '''Test transforming the Maya Reference prim, editing it in Maya, then merging back the result.'''
        self.makeRootLayerNotAnonymous()
        self.runTestMayaRefPrimTransform(createMayaRefPrimVariantCache, checkVariantCacheParent)

if __name__ == '__main__':
    unittest.main(verbosity=2)
