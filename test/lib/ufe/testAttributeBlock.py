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

from pxr import UsdGeom, Vt, Gf

from maya import cmds
from maya import standalone

import mayaUsd
from mayaUsd import ufe as mayaUsdUfe

import ufe
import os
import unittest

class AttributeBlockTestCase(unittest.TestCase):

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

    def testSingleAttributeBlocking(self):
        ''' Authoring attribute(s) in weaker layer(s) are not permitted if there exist opinion(s) in stronger layer(s).'''

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

        # author a new radius value
        self.assertTrue(cylinderPrim.HasAttribute('doubleSided'))
        doubleSidedAttr = cylinderPrim.GetAttribute('doubleSided')

        # authoring new attribute edit is expected to be allowed.
        self.assertTrue(mayaUsdUfe.isAttributeEditAllowed(doubleSidedAttr))
        doubleSidedAttr.Set(0)

        # create a sub-layer.
        rootLayer = stage.GetRootLayer()
        subLayerA = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="LayerA")[0]

        # check to see the if the sublayers was added
        addedLayers = [subLayerA]
        self.assertEqual(rootLayer.subLayerPaths, addedLayers)

        # set the edit target to LayerA.
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerA)

        # doubleSidedAttr is not allowed to change since there is an opinion in a stronger layer
        self.assertFalse(mayaUsdUfe.isAttributeEditAllowed(doubleSidedAttr))

    def testTransformationAttributeBlocking(self):
        '''Authoring transformation attribute(s) in weaker layer(s) are not permitted if there exist opinion(s) in stronger layer(s).'''

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

         # create 3 sub-layers ( LayerA, LayerB, LayerC ) and set the edit target to LayerB.
        rootLayer = stage.GetRootLayer()
        subLayerC = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="SubLayerC")[0]
        subLayerB = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="SubLayerB")[0]
        subLayerA = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="SubLayerA")[0]

        # check to see the sublayers added
        addedLayers = [subLayerA, subLayerB, subLayerC]
        self.assertEqual(rootLayer.subLayerPaths, addedLayers)

        # set the edit target to LayerB
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerB)

        # create a transform3d
        cylinderPath = ufe.Path([
                     mayaUtils.createUfePathSegment("|mayaUsdTransform|shape"), 
                     usdUtils.createUfePathSegment("/pCylinder1")])
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)

        cylinderT3d = ufe.Transform3d.transform3d(cylinderItem)

        # create a xformable
        cylinderXformable = UsdGeom.Xformable(cylinderPrim)

        # writing to "transform op order" is expected to be allowed.
        self.assertTrue(mayaUsdUfe.isAttributeEditAllowed(cylinderXformable.GetXformOpOrderAttr()))

        # do any transform editing.
        cylinderT3d = ufe.Transform3d.transform3d(cylinderItem)
        cylinderT3d.scale(2.5, 2.5, 2.5)
        cylinderT3d.translate(4.0, 2.0, 3.0)

        # check the "transform op order" stack.
        self.assertEqual(cylinderXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray(('xformOp:translate', 'xformOp:scale')))

        # check if translate attribute is editable
        translateAttr = cylinderPrim.GetAttribute('xformOp:translate')
        self.assertIsNotNone(translateAttr)

        # authoring new transformation edit is expected to be allowed.
        self.assertTrue(mayaUsdUfe.isAttributeEditAllowed(translateAttr))
        cylinderT3d.translate(5.0, 6.0, 7.0)
        self.assertEqual(translateAttr.Get(), Gf.Vec3d(5.0, 6.0, 7.0))

        # set the edit target to a weaker layer (LayerC)
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerC)

        # authoring new transformation edit is not allowed.
        self.assertFalse(mayaUsdUfe.isAttributeEditAllowed(translateAttr))

        # set the edit target to a stronger layer (LayerA)
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerA)

        # authoring new transformation edit is allowed.
        self.assertTrue(mayaUsdUfe.isAttributeEditAllowed(translateAttr))
        cylinderT3d.rotate(0.0, 90.0, 0.0)

        # check the "transform op order" stack.
        self.assertEqual(cylinderXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray(('xformOp:translate','xformOp:rotateXYZ', 'xformOp:scale')))

if __name__ == '__main__':
    unittest.main(verbosity=2)
