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
import mayaUtils
import ufeUtils
import usdUtils
import testUtils

import mayaUsd.ufe
import usdUfe
from mayaUsd import lib as mayaUsdLib

from maya import cmds
from maya import standalone

import ufe

from pxr import Sdf, UsdShade, Usd

import os
import unittest


def childrenNames(children):
    return [str(child.path().back()) for child in children]

class TestObserver(ufe.Observer):
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

class DeleteCmdTestCase(unittest.TestCase):
    '''Verify the Maya delete command, for multiple runtimes.

    UFE Feature : SceneItemOps
    Maya Feature : delete
    Action : Remove object from scene.
    Applied On Selection / Command Arguments:
        - Multiple Selection [Mixed, Non-Maya].  Maya-only selection tested by
          Maya.
    Undo/Redo Test : Yes
    Expect Results To Test :
        - Delete removes object from scene.
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
        
        cmds.file(new=True, force=True)

    def testDeleteLeafPrims(self):
        '''Test successful delete of leaf prims'''
        
        # open tree.ma scene in testSamples
        mayaUtils.openTreeScene()

        # Create some extra Maya nodes
        cmds.polySphere()

        # clear selection to start off
        cmds.select(clear=True)

        # create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # validate the default edit target to be the Rootlayer.
        mayaPathSegment = mayaUtils.createUfePathSegment('|Tree_usd|Tree_usdShape')
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))
        self.assertTrue(stage)
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())
        
        # delete two USD prims and Maya's shape
        ufeObs.reset()
        cmds.delete('|Tree_usd|Tree_usdShape,/TreeBase/leavesXform/leaves', '|Tree_usd|Tree_usdShape,/TreeBase/trunk', '|pSphere1|pSphereShape1')
        self.assertEqual(ufeObs.nbDeleteNotif() , 3)
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/trunk'))
        
        # validate undo
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif() , 3)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
        
        # validate redo
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif() , 3)
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/trunk'))
        
    def testDeleteHierarchy(self):
        '''Test successful delete of a prim with children'''
        
        # open tree.ma scene in testSamples
        mayaUtils.openTreeScene()

        # create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # validate the default edit target to be the Rootlayer.
        mayaPathSegment = mayaUtils.createUfePathSegment('|Tree_usd|Tree_usdShape')
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))
        self.assertTrue(stage)
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())
        
        # delete two USD prims and Maya's shape
        ufeObs.reset()
        cmds.delete('|Tree_usd|Tree_usdShape,/TreeBase')
        self.assertEqual(ufeObs.nbDeleteNotif() , 1)
        self.assertFalse(stage.GetPrimAtPath('/TreeBase'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/trunk'))
        
        # validate undo
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif() , 1)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
        
        # validate redo
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif() , 1)
        self.assertFalse(stage.GetPrimAtPath('/TreeBase'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/trunk'))
 
    def testDeleteHierarchyMultiSelect(self):
        '''Test successful delete of a multiple prims, some being children of others'''
        
        # open tree.ma scene in testSamples
        mayaUtils.openTreeScene()

        # create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # validate the default edit target to be the Rootlayer.
        mayaPathSegment = mayaUtils.createUfePathSegment('|Tree_usd|Tree_usdShape')
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))
        self.assertTrue(stage)
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())
        
        # delete two USD prims and Maya's shape
        ufeObs.reset()
        cmds.delete('|Tree_usd|Tree_usdShape,/TreeBase/leavesXform/leaves', '|Tree_usd|Tree_usdShape,/TreeBase', '|Tree_usd|Tree_usdShape,/TreeBase/trunk')
        self.assertEqual(ufeObs.nbDeleteNotif() , 2)
        self.assertFalse(stage.GetPrimAtPath('/TreeBase'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/trunk'))
        
        # validate undo
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif() , 2)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
        
        # validate redo
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif() , 2)
        self.assertFalse(stage.GetPrimAtPath('/TreeBase'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/trunk'))
 
    def testDeleteRestrictionDifferentLayer(self):
        '''Test delete restriction - we don't allow removal of a prim when edit target is different than layer introducing the prim'''
        
        # open tree.ma scene in testSamples
        mayaUtils.openTreeScene()

        # create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # retrieve the stage
        mayaPathSegment = mayaUtils.createUfePathSegment('|Tree_usd|Tree_usdShape')
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))
        self.assertTrue(stage)

        newLayerName = 'Layer_1'
        usdFormat = Sdf.FileFormat.FindByExtension('usd')
        newLayer = Sdf.Layer.New(usdFormat, newLayerName)
        stage.GetRootLayer().subLayerPaths.append(newLayer.identifier)

        stage.SetEditTarget(newLayer)
        self.assertEqual(stage.GetEditTarget().GetLayer(), newLayer)
        
        # delete two USD prims and Maya's shape
        ufeObs.reset()
        cmds.delete('|Tree_usd|Tree_usdShape,/TreeBase/leavesXform/leaves', '|Tree_usd|Tree_usdShape,/TreeBase/trunk')
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
        
        # validate undo
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
        
        # validate redo
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
 
    def testDeleteRestrictionVariantPrim(self):
        '''Test delete restriction - we don't allow removal of a prim defined in a variant'''
        
        # open Variant.ma scene in testSamples
        mayaUtils.openVariantSetScene()
        
        # create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # validate the default edit target to be the Rootlayer.
        mayaPathSegment = mayaUtils.createUfePathSegment('|Variant_usd|Variant_usdShape')
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))
        self.assertTrue(stage)
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())
        
        # delete two USD prims and Maya's shape
        ufeObs.reset()
        cmds.delete('|Variant_usd|Variant_usdShape,/objects/Geom')
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/objects/Geom'))
        
        # validate undo
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/objects/Geom'))
        
        # validate redo
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/objects/Geom'))

    def testDeleteRestrictionReferencedPrim(self):
        '''Test delete restriction - we don't allow removal of a prim defined in a referenced layer'''
        
        # open appleBite.ma scene in testSamples
        mayaUtils.openAppleBiteScene()

        # create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # validate the default edit target to be the Rootlayer.
        mayaPathSegment = mayaUtils.createUfePathSegment('|Asset_flattened_instancing_and_class_removed_usd|Asset_flattened_instancing_and_class_removed_usdShape')
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))
        self.assertTrue(stage)
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())
        
        # delete two USD prims and Maya's shape
        ufeObs.reset()
        cmds.delete('|Asset_flattened_instancing_and_class_removed_usd|Asset_flattened_instancing_and_class_removed_usdShape,/apple/payload/geo')
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/apple/payload/geo'))
        
        # validate undo
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/apple/payload/geo'))
        
        # validate redo
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/apple/payload/geo'))

    def testDeleteRestrictionHierarchyWithChildrenOnDifferentLayer(self):
        '''Test delete restriction - we don't allow removal of a prim with child/children defined on a different layer'''

        # open tree.ma scene in testSamples
        mayaUtils.openTreeScene()

        # create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # add child defined on a new layer
        mayaPathSegment = mayaUtils.createUfePathSegment('|Tree_usd|Tree_usdShape')
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))
        self.assertTrue(stage)

        newLayerName = 'Layer_1'
        usdFormat = Sdf.FileFormat.FindByExtension('usd')
        newLayer = Sdf.Layer.New(usdFormat, newLayerName)
        stage.GetRootLayer().subLayerPaths.append(newLayer.identifier)

        stage.SetEditTarget(newLayer)
        stage.DefinePrim('/TreeBase/newChild', 'Xform')
        stage.SetEditTarget(stage.GetRootLayer())
        
        # delete two USD prims and Maya's shape
        ufeObs.reset()
        cmds.delete('|Tree_usd|Tree_usdShape,/TreeBase')
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/newChild'))
        
        # validate undo
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/newChild'))
        
        # validate redo
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/newChild'))

    @unittest.skipUnless(Usd.GetVersion() >= (0, 23, 8), 'Variable expressions in sublayers requires OpenUSD version 23.08 and above')
    def testDeleteRestrictionWithExpressionVariableSubLayer(self):
        '''
        Test delete restriction - we don't allow removal of a prim from a weaker
        layer when the prim is in the stronger layer.
        With sub layer paths using expression variables.
        '''
        psPathStr, _, varLayer, weakLayer = usdUtils.createStageWithExpressionVariableSubLayer()
        stage = mayaUsd.ufe.getStage(psPathStr)

        stage.SetEditTarget(varLayer)
        stage.DefinePrim('/A', 'Xform')
        self.assertTrue(stage.GetPrimAtPath('/A'))

        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # not allowed from the weaker layer.
        stage.SetEditTarget(weakLayer)
        cmds.delete(psPathStr + ',/A')

        self.assertEqual(ufeObs.nbDeleteNotif(), 0)
        self.assertTrue(stage.GetPrimAtPath('/A'))
        self.assertFalse(weakLayer.GetPrimAtPath('/A'))

        # allowed from the layer that owns the prim spec.
        ufeObs.reset()

        stage.SetEditTarget(varLayer)
        cmds.delete(psPathStr + ',/A')

        self.assertEqual(ufeObs.nbDeleteNotif(), 1)
        self.assertFalse(stage.GetPrimAtPath('/A'))
        self.assertFalse(varLayer.GetPrimAtPath('/A'))

    def testDeleteRestrictionHierarchyWithChildrenInSessionLayer(self):
        '''Test delete restriction - we *do* allow removal of a prim with child/children defined in the session layer'''

        # open tree.ma scene in testSamples
        mayaUtils.openTreeScene()

        # create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # add child defined on session layer
        mayaPathSegment = mayaUtils.createUfePathSegment('|Tree_usd|Tree_usdShape')
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))
        self.assertTrue(stage)
        
        stage.SetEditTarget(stage.GetSessionLayer())
        stage.DefinePrim('/TreeBase/newChild', 'Xform')
        stage.SetEditTarget(stage.GetRootLayer())
        
        # Delete the /TreeBase prim, at the usd root.
        ufeObs.reset()
        cmds.delete('|Tree_usd|Tree_usdShape,/TreeBase')
        self.assertEqual(ufeObs.nbDeleteNotif() , 1)
        self.assertFalse(stage.GetPrimAtPath('/TreeBase'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/trunk'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/newChild'))
        
        # validate undo
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif() , 1)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/newChild'))
        
        # validate redo
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif() , 1)
        self.assertFalse(stage.GetPrimAtPath('/TreeBase'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/trunk'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/newChild'))

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test requires delete prim, its connections and connected properties feature only available on Ufe v4 and later')
    def testDeleteAndRemoveConnections(self):
        '''Test deleting a prim and its connections'''
        
        # Load a stage with a compound.
        testFile = testUtils.getTestScene('MaterialX', 'multiple_connections.usda')
        testPath,shapeStage = mayaUtils.createProxyFromFile(testFile)
        self.assertTrue(shapeStage)
        self.assertTrue(testPath)
      
        ufeItem = ufeUtils.createUfeSceneItem(testPath,'/Material1/UsdPreviewSurface1')
        ufeItemFractal = ufeUtils.createUfeSceneItem(testPath,'/Material1/fractal3d1')
        ufeItemPreview = ufeUtils.createUfeSceneItem(testPath,'/Material1/UsdPreviewSurface2')
        ufeItemMaterial = ufeUtils.createUfeSceneItem(testPath,'/Material1')
        ufeItemSurface1 = ufeUtils.createUfeSceneItem(testPath,'/Material1/surface1')
        ufeItemSurface2 = ufeUtils.createUfeSceneItem(testPath,'/Material1/surface2')
        ufeItemSurface3 = ufeUtils.createUfeSceneItem(testPath,'/Material1/surface3')
        ufeItemCompound = ufeUtils.createUfeSceneItem(testPath,'/Material1/compound')

        self.assertIsNotNone(ufeItem)
        self.assertIsNotNone(ufeItemFractal)
        self.assertIsNotNone(ufeItemPreview)
        self.assertIsNotNone(ufeItemMaterial)
        self.assertIsNotNone(ufeItemSurface1)
        self.assertIsNotNone(ufeItemSurface2)
        self.assertIsNotNone(ufeItemSurface3)
        self.assertIsNotNone(ufeItemCompound)

        # Test the connections before deleting the node.
        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeItemFractal.runTimeId())
        self.assertIsNotNone(connectionHandler)

        connections = connectionHandler.sourceConnections(ufeItemFractal)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        connections = connectionHandler.sourceConnections(ufeItemPreview)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        connections = connectionHandler.sourceConnections(ufeItemMaterial)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 2)

        connections = connectionHandler.sourceConnections(ufeItemSurface3)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # Delete the node.
        usdItemToDeletePathString = testPath + ',/Material1/UsdPreviewSurface1'
        cmds.delete(usdItemToDeletePathString)
        
        # Test the connections after deleting the node.
        connections = connectionHandler.sourceConnections(ufeItemFractal)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        connections = connectionHandler.sourceConnections(ufeItemPreview)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        connections = connectionHandler.sourceConnections(ufeItemMaterial)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        connections = connectionHandler.sourceConnections(ufeItemSurface3)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # Test the properties.
        parentPrim = shapeStage.GetPrimAtPath('/Material1')
        surface1Prim = shapeStage.GetPrimAtPath('/Material1/surface1')
        previewSurface2Prim = shapeStage.GetPrimAtPath('/Material1/UsdPreviewSurface2')
        fractal3d1Prim = shapeStage.GetPrimAtPath('/Material1/fractal3d1')
        self.assertTrue(parentPrim)
        self.assertTrue(surface1Prim)
        self.assertTrue(previewSurface2Prim)
        self.assertTrue(fractal3d1Prim)
        self.assertTrue(parentPrim.HasProperty('inputs:clearcoat'))
        self.assertTrue(parentPrim.HasProperty('outputs:displacement1'))
        self.assertFalse(surface1Prim.HasProperty('outputs:out'))
        self.assertFalse(fractal3d1Prim.HasProperty('inputs:amplitude'))
        self.assertFalse(previewSurface2Prim.HasProperty('inputs:diffuseColor'))

        # Delete the compound.
        usdItemToDeletePathString = testPath + ',/Material1/compound'
        cmds.delete(usdItemToDeletePathString)

        connections = connectionHandler.sourceConnections(ufeItemMaterial)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        connections = connectionHandler.sourceConnections(ufeItemSurface3)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        # Test the properties.
        surface2Prim = shapeStage.GetPrimAtPath('/Material1/surface2')
        surface3Prim = shapeStage.GetPrimAtPath('/Material1/surface3')
        self.assertTrue(surface2Prim)
        self.assertTrue(surface3Prim)
        self.assertTrue(parentPrim.HasProperty('inputs:clearcoat1'))
        self.assertTrue(parentPrim.HasProperty('outputs:displacement'))
        self.assertFalse(surface2Prim.HasProperty('outputs:out'))
        self.assertFalse(surface3Prim.HasProperty('inputs:bsdf'))

    def testDeleteAndRemoveReference(self):
        '''Test deleting a prim and its references'''

        # open treeRef.ma scene in testSamples
        mayaUtils.openTreeRefScene()

        # validate the default edit target to be the Rootlayer.
        mayaPathSegment = mayaUtils.createUfePathSegment('|TreeRef_usd|TreeRef_usdShape')
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(mayaPathSegment)))
        self.assertTrue(stage)
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leaf_ref_1').HasAuthoredReferences())
        
        # delete the greenLeaf and check the internal references were cleaned:
        cmds.delete('|TreeRef_usd|TreeRef_usdShape,/TreeBase/leavesXform/greenLeaf')
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leaf_ref_1').HasAuthoredReferences())
        # Ref2 has an "external" reference, but to this file. It will not be
        # cleaned up because external, baut maybe it should have since the
        # referenced file happens to be the root layer?
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leaf_ref_2').HasAuthoredReferences())
        
        # validate undo
        cmds.undo()
        ref1 = stage.GetPrimAtPath('/TreeBase/leaf_ref_1')
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leaf_ref_1').HasAuthoredReferences())
        
        # validate redo
        cmds.redo()
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leaf_ref_1').HasAuthoredReferences())

        # Try deleting leavesXform hierarchy, which also deletes greenLeaf and
        # should clean-up the reference.
        cmds.undo()
        cmds.delete('|TreeRef_usd|TreeRef_usdShape,/TreeBase/leavesXform')
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leaf_ref_1').HasAuthoredReferences())

        # validate undo
        cmds.undo()
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leaf_ref_1').HasAuthoredReferences())
        
        # validate redo
        cmds.redo()
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leaf_ref_1').HasAuthoredReferences())

    def testDeleteAndInheritMaterial(self):
        '''Test deleting a material cleans up all places where it was assigned'''

        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('tree', 'treeMat.usda')
        pathString,stage = mayaUtils.createProxyFromFile(testFile)
        self.assertTrue(stage)
        self.assertTrue(pathString)
        matAPI = UsdShade.MaterialBindingAPI(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))

        self.assertEqual(matAPI.ComputeBoundMaterial()[0].GetPrim().GetPath(), '/mtlFoliage/GreenMat')

        cmds.delete(pathString + ',/mtlFoliage/GreenMat')
        self.assertEqual(matAPI.ComputeBoundMaterial()[0].GetPrim().GetPath(), '/mtl/BrownMat')

        cmds.undo()
        self.assertEqual(matAPI.ComputeBoundMaterial()[0].GetPrim().GetPath(), '/mtlFoliage/GreenMat')

        cmds.redo()
        self.assertEqual(matAPI.ComputeBoundMaterial()[0].GetPrim().GetPath(), '/mtl/BrownMat')

        cmds.undo()
        cmds.delete(pathString + ',/mtlFoliage')
        self.assertEqual(matAPI.ComputeBoundMaterial()[0].GetPrim().GetPath(), '/mtl/BrownMat')

        cmds.undo()
        self.assertEqual(matAPI.ComputeBoundMaterial()[0].GetPrim().GetPath(), '/mtlFoliage/GreenMat')

        cmds.redo()
        self.assertEqual(matAPI.ComputeBoundMaterial()[0].GetPrim().GetPath(), '/mtl/BrownMat')

    def testDeleteRemovesTargetsInLocalVariant(self):
        '''Test deleting a prim removes the relationship targets pointing to it that are
        authored inside a variant of a local layer.'''

        cmds.file(new=True, force=True)
        psPathStr, _, _ = usdUtils.createSimpleStage()
        stage = mayaUsd.ufe.getStage(psPathStr)

        # /Model/Looks/Mat is defined outside of the variant, the selected variant binds
        # a mesh to it.
        model = stage.DefinePrim('/Model', 'Xform')
        material = UsdShade.Material.Define(stage, '/Model/Looks/Mat')
        lookSet = model.GetVariantSets().AddVariantSet('look')
        lookSet.AddVariant('red')
        lookSet.SetVariantSelection('red')
        with lookSet.GetVariantEditContext():
            geom = stage.DefinePrim('/Model/Geom', 'Mesh')
            UsdShade.MaterialBindingAPI.Apply(geom).Bind(material)

        def bindingTargets():
            rel = stage.GetPrimAtPath('/Model/Geom').GetRelationship('material:binding')
            return rel.GetTargets() if rel else []

        self.assertEqual(bindingTargets(), [Sdf.Path('/Model/Looks/Mat')])

        cmds.delete(psPathStr + ',/Model/Looks/Mat')
        self.assertFalse(stage.GetPrimAtPath('/Model/Looks/Mat'))
        self.assertEqual(bindingTargets(), [])

        cmds.undo()
        self.assertTrue(stage.GetPrimAtPath('/Model/Looks/Mat'))
        self.assertEqual(bindingTargets(), [Sdf.Path('/Model/Looks/Mat')])

        cmds.redo()
        self.assertEqual(bindingTargets(), [])

    def testDeleteKeepsTargetsDeletedByEditTarget(self):
        '''Test deleting a prim keeps the edit target deleting the relationship targets
        authored in other local layers, when nothing is left targeted.'''

        cmds.file(new=True, force=True)
        psPathStr, _, _ = usdUtils.createSimpleStage()
        stage = mayaUsd.ufe.getStage(psPathStr)
        stage.DefinePrim('/hidden', 'Xform')
        stage.DefinePrim('/target', 'Xform')
        viewer = stage.DefinePrim('/viewer', 'Xform')

        # A weaker local layer targets /hidden, the edit target deletes that target and
        # targets /target instead.
        subLayer = usdUtils.addNewLayerToStage(stage, anonymous=True)
        subRel = Sdf.RelationshipSpec(Sdf.CreatePrimInLayer(subLayer, '/viewer'), 'lookAt')
        subRel.targetPathList.prependedItems.append(Sdf.Path('/hidden'))
        lookAt = viewer.GetRelationship('lookAt')
        lookAt.RemoveTarget('/hidden')
        lookAt.AddTarget('/target')

        def lookAtTargets():
            return stage.GetPrimAtPath('/viewer').GetRelationship('lookAt').GetTargets()

        self.assertEqual(lookAtTargets(), [Sdf.Path('/target')])

        cmds.delete(psPathStr + ',/target')
        self.assertFalse(stage.GetPrimAtPath('/target'))
        self.assertEqual(lookAtTargets(), [])

        cmds.undo()
        self.assertTrue(stage.GetPrimAtPath('/target'))
        self.assertEqual(lookAtTargets(), [Sdf.Path('/target')])

        cmds.redo()
        self.assertEqual(lookAtTargets(), [])

    def testDeleteRemovesAssetTargetToLocalPrim(self):
        '''Test deleting a prim defined locally under a referenced prim removes a
        relationship target authored in the asset that points to it. The asset does not
        define the target, so composition keeps mapping the target to the deleted prim.
        The asset itself must not be edited.'''

        cmds.file(new=True, force=True)

        # The asset targets a child it defines and one it does not define.
        assetLayer, assetTargets = usdUtils.createAssetTargetingUndefinedChild()

        psPathStr, _, _ = usdUtils.createSimpleStage()
        stage = mayaUsd.ufe.getStage(psPathStr)
        model = stage.DefinePrim('/Model', 'Xform')
        model.GetReferences().AddReference(assetLayer.identifier)
        # The target is only defined locally, so it can be deleted.
        stage.DefinePrim('/Model/target', 'Xform')

        def verifyTargets(targets):
            lookAt = stage.GetPrimAtPath('/Model').GetRelationship('lookAt')
            self.assertEqual(lookAt.GetTargets(), targets)
            assetLookAt = assetLayer.GetRelationshipAtPath('/asset.lookAt')
            self.assertEqual(list(assetLookAt.targetPathList.explicitItems), assetTargets)

        allTargets = [Sdf.Path('/Model/target'), Sdf.Path('/Model/other')]
        verifyTargets(allTargets)

        cmds.delete(psPathStr + ',/Model/target')
        self.assertFalse(stage.GetPrimAtPath('/Model/target'))
        verifyTargets([Sdf.Path('/Model/other')])

        cmds.undo()
        self.assertTrue(stage.GetPrimAtPath('/Model/target'))
        verifyTargets(allTargets)

        cmds.redo()
        verifyTargets([Sdf.Path('/Model/other')])

    def testDeleteRefusedKeepsTargets(self):
        '''Test that a delete refused by the command restrictions leaves the relationship
        targets pointing to the prim unchanged.'''

        cmds.file(new=True, force=True)
        psPathStr, _, _ = usdUtils.createSimpleStage()
        stage = mayaUsd.ufe.getStage(psPathStr)
        stage.DefinePrim('/target', 'Xform')
        stage.DefinePrim('/viewer', 'Xform').CreateRelationship('lookAt').AddTarget('/target')

        # A weaker layer also defines the prim, so deleting it from the edit target is
        # refused: it would orphan that opinion.
        subLayer = usdUtils.addNewLayerToStage(stage, anonymous=True)
        Sdf.CreatePrimInLayer(subLayer, '/target').specifier = Sdf.SpecifierDef

        cmds.delete(psPathStr + ',/target')
        self.assertTrue(stage.GetPrimAtPath('/target'))
        self.assertEqual(
            stage.GetPrimAtPath('/viewer').GetRelationship('lookAt').GetTargets(),
            [Sdf.Path('/target')])

    def testDeleteWarnsAboutTargetInStrongerLayer(self):
        '''Test deleting a prim leaves a relationship target authored in a local layer
        stronger than the edit target as it is, with a warning. The edit target cannot
        override a stronger layer, and other layers are not edited.'''

        cmds.file(new=True, force=True)
        psPathStr, _, _ = usdUtils.createSimpleStage()
        stage = mayaUsd.ufe.getStage(psPathStr)
        rootLayer = stage.GetRootLayer()
        stage.DefinePrim('/viewer', 'Xform').CreateRelationship('lookAt').AddTarget('/target')

        # The deleted prim is defined in a weaker layer, which is the edit target.
        subLayer = usdUtils.addNewLayerToStage(stage, anonymous=True)
        stage.SetEditTarget(subLayer)
        stage.DefinePrim('/target', 'Xform')

        def verifyNotEdited():
            lookAt = stage.GetPrimAtPath('/viewer').GetRelationship('lookAt')
            self.assertEqual(lookAt.GetTargets(), [Sdf.Path('/target')])
            rootSpec = rootLayer.GetRelationshipAtPath('/viewer.lookAt')
            self.assertEqual(usdUtils.addedPaths(rootSpec.targetPathList), [Sdf.Path('/target')])
            self.assertIsNone(subLayer.GetPrimAtPath('/viewer'))

        verifyNotEdited()

        with usdUtils.WarningRecorder() as recorder:
            cmds.delete(psPathStr + ',/target')
        self.assertFalse(stage.GetPrimAtPath('/target'))
        verifyNotEdited()
        self.assertTrue(
            any('/viewer.lookAt' in warning and rootLayer.GetDisplayName() in warning
                for warning in recorder.warnings),
            recorder.warnings)

        cmds.undo()
        self.assertTrue(stage.GetPrimAtPath('/target'))
        verifyNotEdited()

    def testDeleteRemovesTargetInSessionLayer(self):
        '''Test deleting a prim removes a relationship target authored in the session
        layer, there. The session layer is stronger than the edit target, so it cannot be
        overridden from it.'''

        cmds.file(new=True, force=True)
        psPathStr, _, _ = usdUtils.createSimpleStage()
        stage = mayaUsd.ufe.getStage(psPathStr)
        rootLayer = stage.GetRootLayer()
        sessionLayer = stage.GetSessionLayer()
        stage.DefinePrim('/target', 'Xform')
        stage.DefinePrim('/other', 'Xform')
        viewer = stage.DefinePrim('/viewer', 'Xform')
        with Usd.EditContext(stage, sessionLayer):
            lookAt = viewer.CreateRelationship('lookAt')
            lookAt.AddTarget('/target')
            lookAt.AddTarget('/other')

        def verifyTargets(paths):
            lookAt = stage.GetPrimAtPath('/viewer').GetRelationship('lookAt')
            self.assertEqual(lookAt.GetTargets(), paths)
            sessionSpec = sessionLayer.GetRelationshipAtPath('/viewer.lookAt')
            self.assertEqual(usdUtils.addedPaths(sessionSpec.targetPathList), paths)
            self.assertIsNone(rootLayer.GetPropertyAtPath('/viewer.lookAt'))

        allTargets = [Sdf.Path('/target'), Sdf.Path('/other')]
        verifyTargets(allTargets)

        with usdUtils.WarningRecorder() as recorder:
            cmds.delete(psPathStr + ',/target')
        self.assertFalse(stage.GetPrimAtPath('/target'))
        verifyTargets([Sdf.Path('/other')])
        self.assertFalse(
            any('/viewer.lookAt' in warning for warning in recorder.warnings),
            recorder.warnings)

        cmds.undo()
        self.assertTrue(stage.GetPrimAtPath('/target'))
        verifyTargets(allTargets)

    def testDeleteRemovesAllCompositionArcsOfPrim(self):
        '''Test deleting a prim removes the internal references and the inherits pointing
        to it when a prim has both, and when they are authored in explicit lists.'''

        cmds.file(new=True, force=True)
        psPathStr, _, _ = usdUtils.createSimpleStage()
        stage = mayaUsd.ufe.getStage(psPathStr)
        layer = stage.GetEditTarget().GetLayer()
        stage.DefinePrim('/source', 'Xform')
        stage.DefinePrim('/base', 'Xform')

        # A prim with both an internal reference and an inherit.
        user = stage.DefinePrim('/user', 'Xform')
        user.GetReferences().AddInternalReference('/source')
        user.GetInherits().AddInherit('/base')

        # A prim with both in explicit lists.
        explicitSpec = Sdf.CreatePrimInLayer(layer, '/explicitUser')
        explicitSpec.specifier = Sdf.SpecifierDef
        explicitSpec.referenceList.ClearEditsAndMakeExplicit()
        explicitSpec.referenceList.explicitItems.append(Sdf.Reference(primPath='/source'))
        explicitSpec.inheritPathList.ClearEditsAndMakeExplicit()
        explicitSpec.inheritPathList.explicitItems.append(Sdf.Path('/base'))

        def verifyArcs(sourcePaths, basePaths):
            userSpec = layer.GetPrimAtPath('/user')
            self.assertEqual(
                [ref.primPath for ref in userSpec.referenceList.prependedItems], sourcePaths)
            self.assertEqual(list(userSpec.inheritPathList.prependedItems), basePaths)
            explicitSpec = layer.GetPrimAtPath('/explicitUser')
            self.assertEqual(
                [ref.primPath for ref in explicitSpec.referenceList.explicitItems], sourcePaths)
            self.assertEqual(list(explicitSpec.inheritPathList.explicitItems), basePaths)

        verifyArcs([Sdf.Path('/source')], [Sdf.Path('/base')])

        cmds.delete(psPathStr + ',/base')
        self.assertFalse(stage.GetPrimAtPath('/base'))
        verifyArcs([Sdf.Path('/source')], [])

        cmds.delete(psPathStr + ',/source')
        self.assertFalse(stage.GetPrimAtPath('/source'))
        verifyArcs([], [])

        cmds.undo()
        cmds.undo()
        verifyArcs([Sdf.Path('/source')], [Sdf.Path('/base')])

    def testDeleteUsingEditRouterIsRefusedWhenRoutedLayerIsNotEditable(self):
        '''Test deleting a prim using an EditRouter that routes the deletion to a layer
        that cannot be edited edits nothing, the targets in the edit target included.'''

        cmds.file(new=True, force=True)

        # The asset has no target to its child, so it has no path to clean.
        assetLayer = Sdf.Layer.CreateAnonymous('asset.usda')
        assetStage = Usd.Stage.Open(assetLayer)
        assetStage.SetDefaultPrim(assetStage.DefinePrim('/asset', 'Xform'))
        assetStage.DefinePrim('/asset/a', 'Xform')

        psPathStr, _, _ = usdUtils.createSimpleStage()
        stage = mayaUsd.ufe.getStage(psPathStr)
        stage.DefinePrim('/Model', 'Xform').GetReferences().AddReference(assetLayer.identifier)
        stage.DefinePrim('/viewer', 'Xform').CreateRelationship('lookAt').AddTarget('/Model/a')

        def editRouterCallback(context, routingData):
            # Route the deletion to the asset layer, through the reference.
            prim = context.get('prim')
            if prim is None:
                return
            for arc in Usd.PrimCompositionQuery(prim).GetCompositionArcs():
                node = arc.GetTargetNode()
                if node.layerStack.identifier.rootLayer == assetLayer:
                    routingData['editTarget'] = Usd.EditTarget(assetLayer, node)
                    return

        mayaUsdLib.registerEditRouter('delete', editRouterCallback)
        assetLayer.SetPermissionToEdit(False)
        try:
            try:
                cmds.delete(psPathStr + ',/Model/a')
            except RuntimeError:
                pass
            self.assertTrue(stage.GetPrimAtPath('/Model/a'))
            self.assertEqual(
                stage.GetPrimAtPath('/viewer').GetRelationship('lookAt').GetTargets(),
                [Sdf.Path('/Model/a')])
        finally:
            assetLayer.SetPermissionToEdit(True)
            mayaUsdLib.restoreAllDefaultEditRouters()

    def testCleanReferencedPathInAllLayers(self):
        '''Test that, when cleaning all layers as the delete command does for component
        stages, the connection and relationship target authored inside a payload that point
        to a deleted prim are removed in place. This does not need the USD Component
        Creator, as it calls the function used by the delete command directly.'''

        stage, rootLayer, geoLayer, mtlLayer = usdUtils.createComponentLikeStage('Mat')

        def deleteInAllLayers(primPath):
            # As the delete command does for component stages: clean the paths, then
            # remove the prim from every layer of its prim stack.
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(usdUfe.cleanReferencedPathInAllLayers(prim))
            with Sdf.ChangeBlock():
                for spec in prim.GetPrimStack():
                    del spec.nameParent.nameChildren[spec.name]

        def surfaceConnections():
            material = UsdShade.Material(stage.GetPrimAtPath('/root/mtl/Mat'))
            return material.GetSurfaceOutput().GetAttr().GetConnections()

        def bindingTargets():
            rel = stage.GetPrimAtPath('/root/geo/Mesh').GetRelationship('material:binding')
            return rel.GetTargets() if rel else []

        self.assertEqual(surfaceConnections(), [Sdf.Path('/root/mtl/Mat/surf.outputs:surface')])
        self.assertEqual(bindingTargets(), [Sdf.Path('/root/mtl/Mat')])

        # Deleting the shader removes the connection to it, in the material layer.
        deleteInAllLayers('/root/mtl/Mat/surf')
        self.assertFalse(stage.GetPrimAtPath('/root/mtl/Mat/surf'))
        self.assertEqual(surfaceConnections(), [])
        # The output is left without connection or value, so its spec is removed too.
        self.assertIsNone(mtlLayer.GetAttributeAtPath('/asset/mtl/Mat.outputs:surface'))

        # Deleting the material removes the binding to it, in the geo layer.
        deleteInAllLayers('/root/mtl/Mat')
        self.assertFalse(stage.GetPrimAtPath('/root/mtl/Mat'))
        self.assertEqual(bindingTargets(), [])
        # The binding is left without target, so its spec is removed too.
        self.assertIsNone(geoLayer.GetRelationshipAtPath('/asset/geo/Mesh.material:binding'))

        # Nothing is authored in the root layer.
        self.assertIsNone(rootLayer.GetPrimAtPath('/root/geo'))
        self.assertIsNone(rootLayer.GetPrimAtPath('/root/mtl'))

    def testCleanReferencedPathInAllLayersWithReadOnlyLayer(self):
        '''Test that, when cleaning all layers as the delete command does for component
        stages, nothing is edited if one of the layers to edit cannot be edited. Otherwise
        the paths would be left half removed when the delete fails.'''

        stage, rootLayer, geoLayer, mtlLayer = usdUtils.createComponentLikeStage('Mat')

        def cleanMaterialPaths():
            prim = stage.GetPrimAtPath('/root/mtl/Mat')
            return usdUfe.cleanReferencedPathInAllLayers(prim)

        def verifyNotEdited():
            bindingSpec = geoLayer.GetRelationshipAtPath('/asset/geo/Mesh.material:binding')
            self.assertEqual(
                usdUtils.addedPaths(bindingSpec.targetPathList), [Sdf.Path('/asset/mtl/Mat')])
            outputSpec = mtlLayer.GetAttributeAtPath('/asset/mtl/Mat.outputs:surface')
            self.assertEqual(
                usdUtils.addedPaths(outputSpec.connectionPathList),
                [Sdf.Path('/asset/mtl/Mat/surf.outputs:surface')])

        # The geo layer only holds the binding to the deleted material.
        geoLayer.SetPermissionToEdit(False)
        self.assertFalse(cleanMaterialPaths())
        verifyNotEdited()
        geoLayer.SetPermissionToEdit(True)

        # The material layer holds the deleted material and the connection to its shader.
        mtlLayer.SetPermissionToEdit(False)
        self.assertFalse(cleanMaterialPaths())
        verifyNotEdited()
        mtlLayer.SetPermissionToEdit(True)

        # The root layer only holds an over of the deleted material.
        Sdf.CreatePrimInLayer(rootLayer, '/root/mtl/Mat')
        rootLayer.SetPermissionToEdit(False)
        self.assertFalse(cleanMaterialPaths())
        verifyNotEdited()

    def testDeleteRestrictionMutedLayer(self):
        '''
        Test delete restriction - we don't allow removal of a prim
        when there are opinions on a muted layer.
        '''
        
        # Create a stage with a xform prim named A
        cmds.file(new=True, force=True)
        import mayaUsd_createStageWithNewLayer

        proxyShapePathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()
        self.assertTrue(stage)
        stage.DefinePrim('/A', 'Xform')

        # Add two new layers
        usdFormat = Sdf.FileFormat.FindByExtension('usd')
        topLayer = Sdf.Layer.New(usdFormat, 'Layer_1')
        stage.GetRootLayer().subLayerPaths.append(topLayer.identifier)

        usdFormat = Sdf.FileFormat.FindByExtension('usd')
        bottomLayer = Sdf.Layer.New(usdFormat, 'Layer_2')
        stage.GetRootLayer().subLayerPaths.append(bottomLayer.identifier)

        # Create a sphere on the bottom layer
        stage.SetEditTarget(bottomLayer)
        self.assertEqual(stage.GetEditTarget().GetLayer(), bottomLayer)
        spherePrim = stage.DefinePrim('/A/ball', 'Sphere')
        self.assertIsNotNone(spherePrim)

        # Author an opinion on the top layer
        stage.SetEditTarget(topLayer)
        self.assertEqual(stage.GetEditTarget().GetLayer(), topLayer)
        spherePrim.GetAttribute('radius').Set(4.)
        
        # Set target to bottom layer and mute the top layer
        stage.SetEditTarget(bottomLayer)
        self.assertEqual(stage.GetEditTarget().GetLayer(), bottomLayer)
        # Note: mute by passing through the stage, otherwise the stage won't get recomposed
        stage.MuteLayer(topLayer.identifier)

        # Try to delete the prim with muted opinion: it should fail
        with self.assertRaises(RuntimeError):
            cmds.delete('%s,/A/ball' % proxyShapePathStr)
        self.assertTrue(stage.GetPrimAtPath('/A/ball'))
    
    def testDeletePrimUsingEditRouter(self):
        '''Test deleting prims using edit target layer from an EditRouter.'''

        def editRouterCallback(context, routingData):
            prim = context.get('prim')
            if prim is None:
                return
            
            stage = prim.GetStage()
            targetLayer = None     
            editTarget = None 
            
            # Get non-local referenced layer for edit target layer.
            for layer in stage.GetUsedLayers():
                if 'subCRef' in layer.identifier:
                    targetLayer = layer 
                    break 
            
            query = Usd.PrimCompositionQuery(prim)
            for arc in query.GetCompositionArcs():
                if (arc.GetTargetNode().layerStack.identifier.rootLayer == targetLayer
                    and not arc.GetTargetNode().path.ContainsPrimVariantSelection()):
                    editTarget = Usd.EditTarget(targetLayer, arc.GetTargetNode())
                    break
            
            routingData['layer'] = editTarget.GetLayer().identifier
            routingData['editTarget'] = editTarget
        
        cmds.file(new=True, force=True)

        # We use an example usd file from attributeBlock which has non-local referenced layers.
        testFile = testUtils.getTestScene('attributeBlock', 'root.usda')
        pathString, stage = mayaUtils.createProxyFromFile(testFile)

        self.assertTrue(stage)
        self.assertTrue(pathString)

        primA = stage.GetPrimAtPath('/root/a')
        self.assertTrue(primA.IsValid())
        
        primPathString = pathString + ',/root/a'
        
        # Delete a prim defined within an non-local layer without using EditRouter.
        cmds.delete(primPathString)
        
        # The deletion should fail so the prim should still be valid
        primA = stage.GetPrimAtPath('/root/a')
        self.assertTrue(primA.IsValid())
        
        # Register an EditRouter callback for prim deletion.
        mayaUsdLib.registerEditRouter('delete', editRouterCallback)
        
        cmds.delete(primPathString)
        
        # The prim should be invalid becaue we deleted it from the layer sent by the EditRouter.
        primA = stage.GetPrimAtPath('/root/a')
        self.assertFalse(primA.IsValid())
        
        # Restore EditRouter callbacks to default.
        mayaUsdLib.restoreAllDefaultEditRouters()

    def testDeleteUsingEditRouterRemovesTargetsInReferencedLayer(self):
        '''Test deleting a prim using an EditRouter that routes the deletion to a
        referenced layer removes the relationship targets pointing to the prim where they
        are authored, in that layer, and authors nothing in the local layers.'''

        cmds.file(new=True, force=True)

        # The asset defines two children and targets both.
        assetLayer = Sdf.Layer.CreateAnonymous('asset.usda')
        assetStage = Usd.Stage.Open(assetLayer)
        assetRoot = assetStage.DefinePrim('/asset', 'Xform')
        assetStage.SetDefaultPrim(assetRoot)
        assetStage.DefinePrim('/asset/a', 'Xform')
        assetStage.DefinePrim('/asset/b', 'Xform')
        assetLookAt = assetRoot.CreateRelationship('lookAt')
        assetLookAt.AddTarget('/asset/a')
        assetLookAt.AddTarget('/asset/b')

        psPathStr, _, _ = usdUtils.createSimpleStage()
        stage = mayaUsd.ufe.getStage(psPathStr)
        rootLayer = stage.GetRootLayer()
        stage.DefinePrim('/Model', 'Xform').GetReferences().AddReference(assetLayer.identifier)

        def editRouterCallback(context, routingData):
            # Route the deletion to the asset layer, through the reference.
            prim = context.get('prim')
            if prim is None:
                return
            for arc in Usd.PrimCompositionQuery(prim).GetCompositionArcs():
                node = arc.GetTargetNode()
                if node.layerStack.identifier.rootLayer == assetLayer:
                    routingData['editTarget'] = Usd.EditTarget(assetLayer, node)
                    return

        def verifyTargets(names):
            lookAt = stage.GetPrimAtPath('/Model').GetRelationship('lookAt')
            self.assertEqual(
                lookAt.GetTargets(), [Sdf.Path('/Model').AppendChild(n) for n in names])
            assetSpec = assetLayer.GetRelationshipAtPath('/asset.lookAt')
            self.assertEqual(
                usdUtils.addedPaths(assetSpec.targetPathList),
                [Sdf.Path('/asset').AppendChild(n) for n in names])
            self.assertIsNone(rootLayer.GetPropertyAtPath('/Model.lookAt'))

        verifyTargets(['a', 'b'])

        mayaUsdLib.registerEditRouter('delete', editRouterCallback)
        try:
            cmds.delete(psPathStr + ',/Model/a')
            self.assertFalse(stage.GetPrimAtPath('/Model/a'))
            self.assertIsNone(assetLayer.GetPrimAtPath('/asset/a'))
            verifyTargets(['b'])

            cmds.undo()
            self.assertTrue(stage.GetPrimAtPath('/Model/a'))
            verifyTargets(['a', 'b'])

            cmds.redo()
            verifyTargets(['b'])
        finally:
            mayaUsdLib.restoreAllDefaultEditRouters()

    def testDeleteUsingEditRouterRemovesTargetsInRoutedLayer(self):
        '''Test deleting a prim using an EditRouter that routes the deletion to a local
        layer other than the edit target removes the relationship targets pointing to the
        prim where they are authored, in that layer, instead of overriding them from the
        edit target.'''

        cmds.file(new=True, force=True)
        psPathStr, _, _ = usdUtils.createSimpleStage()
        stage = mayaUsd.ufe.getStage(psPathStr)
        rootLayer = stage.GetRootLayer()

        # The prims and the relationship are authored in a weaker layer, the edit target
        # stays the root layer.
        subLayer = usdUtils.addNewLayerToStage(stage, anonymous=True)
        with Usd.EditContext(stage, subLayer):
            stage.DefinePrim('/target', 'Xform')
            stage.DefinePrim('/other', 'Xform')
            lookAt = stage.DefinePrim('/viewer', 'Xform').CreateRelationship('lookAt')
            lookAt.AddTarget('/target')
            lookAt.AddTarget('/other')
        self.assertEqual(stage.GetEditTarget().GetLayer(), rootLayer)

        def verifyTargets(paths):
            lookAt = stage.GetPrimAtPath('/viewer').GetRelationship('lookAt')
            self.assertEqual(lookAt.GetTargets(), paths)
            subSpec = subLayer.GetRelationshipAtPath('/viewer.lookAt')
            self.assertEqual(usdUtils.addedPaths(subSpec.targetPathList), paths)
            self.assertIsNone(rootLayer.GetPrimAtPath('/viewer'))

        allTargets = [Sdf.Path('/target'), Sdf.Path('/other')]
        verifyTargets(allTargets)

        mayaUsdLib.registerStageLayerEditRouter('delete', stage, subLayer)
        try:
            cmds.delete(psPathStr + ',/target')
            self.assertFalse(stage.GetPrimAtPath('/target'))
            verifyTargets([Sdf.Path('/other')])

            cmds.undo()
            self.assertTrue(stage.GetPrimAtPath('/target'))
            verifyTargets(allTargets)

            cmds.redo()
            verifyTargets([Sdf.Path('/other')])
        finally:
            mayaUsdLib.restoreAllDefaultEditRouters()



if __name__ == '__main__':
    unittest.main(verbosity=2)
