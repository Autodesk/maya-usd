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

from maya import cmds
from maya import standalone

import ufe

from pxr import Sdf

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
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))
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
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))
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
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))
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
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))
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
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))
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
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))
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
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))
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

    def testDeleteRestrictionHierarchyWithChildrenInSessionLayer(self):
        '''Test delete restriction - we *do* allow removal of a prim with child/children defined in the session layer'''

        # open tree.ma scene in testSamples
        mayaUtils.openTreeScene()

        # create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # add child defined on session layer
        mayaPathSegment = mayaUtils.createUfePathSegment('|Tree_usd|Tree_usdShape')
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))
        self.assertTrue(stage)
        
        stage.SetEditTarget(stage.GetSessionLayer())
        stage.DefinePrim('/TreeBase/newChild', 'Xform')
        stage.SetEditTarget(stage.GetRootLayer())
        
        # delete two USD prims and Maya's shape
        ufeObs.reset()
        cmds.delete('|Tree_usd|Tree_usdShape,/TreeBase')
        self.assertEqual(ufeObs.nbDeleteNotif() , 2)
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
        
if __name__ == '__main__':
    unittest.main(verbosity=2)
