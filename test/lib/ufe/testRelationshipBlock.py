#!/usr/bin/env python

#
# Copyright 2025 Autodesk
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

from pxr import Usd

from maya import cmds
from maya import standalone

import mayaUsd
from mayaUsd import ufe as mayaUsdUfe

import unittest

class RelationshipBlockTestCase(unittest.TestCase):

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

        # Clear selection to start off
        cmds.select(clear=True)

    def testRelationshipBlocking(self):
        ''' Adding or removing to a relationship in a weaker layer is not permitted if there exist opinion(s) in stronger layer(s) contradicting those actions.'''

        # create new stage
        cmds.file(new=True, force=True)

        # Open usdCylinder.ma scene in testSamples
        mayaUtils.openCylinderScene()

        # get the stage
        proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        proxyShapePath = proxyShapes[0]
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()

        # cylinder prim
        cylinderPrim = stage.GetPrimAtPath('/pCylinder1')
        self.assertIsNotNone(cylinderPrim)

        # create a new light to test the light linking collection
        light = stage.DefinePrim('/light', 'UsdLux.SphereLight')
        self.assertIsNotNone(light)
        
        # get the light linking collection
        collections = Usd.CollectionAPI.GetAllCollections(light)
        print(collections)

        lightLinkCollection = Usd.CollectionAPI.Get(light, 'lightLink')
        self.assertIsNotNone(lightLinkCollection)

        # Get the include relation of the illumination collection
        includesRel = lightLinkCollection.GetIncludesRel()

        # Check if we can add the cylinder to the lightLink collection include
        # list
        targets = [cylinderPrim.GetPath()]
        (result, error) = mayaUsdUfe.isRelationshipEditAllowed(includesRel, targets, [])
        self.assertTrue(result)
        self.assertEqual(error, '')
        self.assertEqual(len(targets), 1)

        # Check if we could remove the cylinder from the lightLink collection
        # include list
        targets = [cylinderPrim.GetPath()]
        (result, error) = mayaUsdUfe.isRelationshipEditAllowed(includesRel, [], targets)
        self.assertTrue(result)
        self.assertEqual(error, '')
        self.assertEqual(len(targets), 1)

        # Get the exclude of the illumination collection
        excludesRel = lightLinkCollection.GetExcludesRel()
        # Check if we can add the cylinder to the lightLink collection exclude
        # list
        targets = [cylinderPrim.GetPath()]
        (result, error) = mayaUsdUfe.isRelationshipEditAllowed(excludesRel, targets, [])
        self.assertTrue( result)
        self.assertEqual(error, '')
        self.assertEqual(len(targets), 1)

        # Check if we could remove the cylinder from the lightLink collection
        # exclude list
        targets = [cylinderPrim.GetPath()]
        (result, error) = mayaUsdUfe.isRelationshipEditAllowed(excludesRel, [], targets)
        self.assertTrue( result)
        self.assertEqual(error, '')
        self.assertEqual(len(targets), 1)

        targets = includesRel.GetTargets()
        self.assertEqual(len(targets), 0)

        # add the cylinder to the lightLink collection 
        includesRel.AddTarget(cylinderPrim.GetPath())
        targets = includesRel.GetTargets()
        self.assertEqual(len(targets), 1)

        # this should still be allowed
        targetsToAdd = [cylinderPrim.GetPath()]
        targetsToRemove = [cylinderPrim.GetPath()]
        (result, error) = mayaUsdUfe.isRelationshipEditAllowed(includesRel, targetsToAdd, targetsToRemove)
        self.assertTrue(result)
        self.assertEqual(error, '')
        self.assertEqual(len(targetsToAdd), 1)
        self.assertEqual(len(targetsToRemove), 1)

        # create a sub-layer.
        rootLayer = stage.GetRootLayer()
        subLayerA = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="LayerA")[0]

        # check to see the if the sublayers was added
        addedLayers = [subLayerA]
        self.assertEqual(rootLayer.subLayerPaths, addedLayers)

        # set the edit target to LayerA.
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerA)

        # this should still now be allowed - there is an opinion in the stronger layer
        targetsToAdd = [cylinderPrim.GetPath()]
        targetsToRemove = [cylinderPrim.GetPath()]
        (result, error) = mayaUsdUfe.isRelationshipEditAllowed(includesRel, [], targetsToRemove)
        self.assertFalse(result)
        self.assertNotEqual(error, '')
        self.assertIn("Cannot remove", error)
        self.assertEqual(len(targetsToRemove), 0)

        # this should still only be partially allowed - there is an opinion in
        # the stronger layer to add the cylinder to the collection, so we can't
        # remove it in the weaker layer, but we still can add it.
        targetsToAdd = [cylinderPrim.GetPath()]
        targetsToRemove = [cylinderPrim.GetPath()]
        (result, error) = mayaUsdUfe.isRelationshipEditAllowed(includesRel, targetsToAdd, targetsToRemove)
        self.assertTrue(result) # some actions are allowed
        self.assertNotEqual(error, '')
        self.assertIn("Cannot remove", error)
        self.assertNotIn("Cannot add", error)
        self.assertEqual(len(targetsToRemove), 0)
        self.assertEqual(len(targetsToAdd), 1)

if __name__ == '__main__':
    unittest.main(verbosity=2)
