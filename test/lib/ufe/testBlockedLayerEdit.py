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
import usdUtils

from pxr import Sdf

from maya import cmds
from maya import standalone

import mayaUsd
from mayaUsd import ufe as mayaUsdUfe

import ufe
import unittest

class BlockedLayerEditTestCase(unittest.TestCase):

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

    def testTranslateMutedLayer(self):
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

        # create a sub-layer.
        rootLayer = stage.GetRootLayer()
        subLayerA = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="LayerA")[0]

        # check to see the if the sublayers was added
        addedLayers = [subLayerA]
        self.assertEqual(rootLayer.subLayerPaths, addedLayers)

        # set the edit target to LayerA.
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerA)

        # mute the layer
        stage.MuteLayer(subLayerA)

        # Check that our helper function is returning the right thing
        self.assertFalse(mayaUsdUfe.isEditTargetLayerModifiable(stage))

        # Get translate attribute and make sure its empty
        translateAttr = cylinderPrim.GetAttribute('xformOp:translate')
        self.assertIsNotNone(translateAttr)

        tranlateBeforeEdit = translateAttr.Get()

        # create a transform3d and translate
        cylinderPath = ufe.Path([
                     mayaUtils.createUfePathSegment("|mayaUsdTransform|shape"),
                     usdUtils.createUfePathSegment("/pCylinder1")])
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)

        cylinderT3d = ufe.Transform3d.transform3d(cylinderItem)
        if cylinderT3d is not None:
            cylinderT3d.translate(5.0, 6.0, 7.0)

        # check that the translate operation didn't change anything
        tranlateAfterEdit = translateAttr.Get()
        self.assertEqual(tranlateBeforeEdit, tranlateAfterEdit)

    def testTranslateLockedLayer(self):
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

        # create a sub-layer.
        rootLayer = stage.GetRootLayer()
        subLayerA = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="LayerA")[0]

        # check to see the if the sublayers was added
        addedLayers = [subLayerA]
        self.assertEqual(rootLayer.subLayerPaths, addedLayers)

        # set the edit target to LayerA.
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerA)

        # set permission to edit to false
        layerA = Sdf.Find(subLayerA)
        layerA.SetPermissionToEdit(False)

        # Check that our helper function is returning the right thing
        self.assertFalse(mayaUsdUfe.isEditTargetLayerModifiable(stage))

        # Get translate attribute and make sure its empty
        translateAttr = cylinderPrim.GetAttribute('xformOp:translate')
        self.assertIsNotNone(translateAttr)

        tranlateBeforeEdit = translateAttr.Get()

        # create a transform3d and translate
        cylinderPath = ufe.Path([
                     mayaUtils.createUfePathSegment("|mayaUsdTransform|shape"),
                     usdUtils.createUfePathSegment("/pCylinder1")])
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)

        cylinderT3d = ufe.Transform3d.transform3d(cylinderItem)
        if cylinderT3d is not None:
            cylinderT3d.translate(5.0, 6.0, 7.0)

        # check that the translate operation didn't change anything
        tranlateAfterEdit = translateAttr.Get()
        self.assertEqual(tranlateBeforeEdit, tranlateAfterEdit)

if __name__ == '__main__':
    unittest.main(verbosity=2)
