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
import mayaUsd.ufe
import mayaUsdAddMayaReference
import mayaUsdMayaReferenceUtils as mayaRefUtils

from pxr import Usd, UsdGeom, Sdf, Pcp

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as om

import ufe

import unittest

from testUtils import assertVectorAlmostEqual, getTestScene

import os

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

    def setUp(self):
        # Start each test with a new scene with empty stage.
        cmds.file(new=True, force=True)
        import mayaUsd_createStageWithNewLayer
        self.proxyShapePathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        self.stage = mayaUsd.lib.GetPrim(self.proxyShapePathStr).GetStage()

    def testCacheToUsd(self):
        '''Cache edits on a pulled Maya reference prim back to USD.'''

        # Create a Maya reference prim using the defaults, under a
        # newly-created parent, without any variant sets.
        cacheParent = self.stage.DefinePrim('/CacheParent', 'Xform')
        cacheParentPathStr = self.proxyShapePathStr + ',/CacheParent'
        self.assertFalse(cacheParent.HasVariantSets())

        mayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            cacheParentPathStr, self.mayaSceneStr, self.kDefaultNamespace)

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

        # Cache to USD, using a sibling prim and a payload composition arc.
        defaultExportOptions = cacheToUsd.getDefaultExportOptions()
        cacheFile = 'testCacheToUsd.usda'
        cachePrimName = 'cachePrimName'
        payloadOrReference = 'Payload'
        listEditType = 'Prepend'
        cacheOptions = cacheToUsd.createCacheCreationOptions(
            defaultExportOptions, cacheFile, cachePrimName,
            payloadOrReference, listEditType)

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
        self.assertTrue(len(cacheParentChildren), 2)
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

if __name__ == '__main__':
    unittest.main(verbosity=2)
