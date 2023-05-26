#!/usr/bin/env python

#
# Copyright 2023 Autodesk
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
import testTRSBase
from testUtils import assertVectorAlmostEqual

import mayaUsd_createStageWithNewLayer
import mayaUsd.ufe

from pxr import UsdGeom, Vt, Gf

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as om

import ufe

import unittest


class RotateCmdUndoRedoTestCase(testTRSBase.TRSTestCaseBase):
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
        # Load plugins
        self.assertTrue(self.pluginsLoaded)
        
    def runTestOpUndo(self, createTransformOp, attrSpec):
        '''Engine method for op undo testing.'''
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        
        proxyShapePath = ufe.PathString.path(proxyShape)
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'Sphere'])

        spherePath = ufe.PathString.path('%s,/Sphere1' % proxyShape)
        sphereItem = ufe.Hierarchy.createItem(spherePath)

        spherePrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(spherePath))
        sphereXformable = UsdGeom.Xformable(spherePrim)
        createTransformOp(self, sphereXformable)

        # Set the edit target to the session layer, then move.
        stage = mayaUsd.ufe.getStage(proxyShape)
        sessionLayer = stage.GetLayerStack()[0]
        stage.SetEditTarget(sessionLayer)

        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(sphereItem)

        cmds.rotate(0, 10, 0, relative=True, os=True)

        sphereT3d = ufe.Transform3d.transform3d(sphereItem)
        assertVectorAlmostEqual(self, sphereT3d.rotation().vector, [0, 10, 0])

        primSpec = stage.GetEditTarget().GetPrimSpecForScenePath('/Sphere1')

        # Writing to the session layer has created a primSpec in that layer,
        # with the xformOp:transform attrSpec.
        self.assertIsNotNone(primSpec)
        self.assertIn(attrSpec, primSpec.attributes)

        # Undo: primSpec in session layer should be gone.
        cmds.undo()

        assertVectorAlmostEqual(self, sphereT3d.rotation().vector, [0, 0, 0])

        primSpec = stage.GetEditTarget().GetPrimSpecForScenePath('/Sphere1')
        self.assertIsNone(primSpec)

        cmds.redo()

        assertVectorAlmostEqual(self, sphereT3d.rotation().vector, [0, 10, 0])

        # Undo after redo must also remove primSpec.
        cmds.undo()

        assertVectorAlmostEqual(self, sphereT3d.rotation().vector, [0, 0, 0])

        primSpec = stage.GetEditTarget().GetPrimSpecForScenePath('/Sphere1')
        self.assertIsNone(primSpec)

        # Performing change in main layer should also work.
        layer = stage.GetLayerStack()[1]
        stage.SetEditTarget(layer)

        cmds.rotate(0, 10, 0, relative=True, os=True)

        def checkTransform(expectedTranslation):
            # Check through both USD and UFE interfaces
            assertVectorAlmostEqual(
                self, sphereT3d.rotation().vector, expectedTranslation)
            expected = Gf.Matrix4d(1.0)
            expected.SetRotate(Gf.Rotation(Gf.Vec3d(0., 1., 0.), expectedTranslation[1]))
            actual = sphereXformable.GetLocalTransformation()
            self.assertTrue(Gf.IsClose(actual, expected, 1e-5))
            
        checkTransform([0, 10, 0])

        cmds.undo()

        checkTransform([0, 0, 0])

        cmds.redo()

        checkTransform([0, 10, 0])

    def testMatrixOpUndo(self):
        '''Undo of matrix op move must completely remove attr spec.'''

        def createMatrixTransformOp(testCase, sphereXformable):
            transformOp = sphereXformable.AddTransformOp()
            xform = Gf.Matrix4d(1.0)
            transformOp.Set(xform)
    
            testCase.assertEqual(
                sphereXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray([
                    "xformOp:transform"]))

        self.runTestOpUndo(createMatrixTransformOp, 'xformOp:transform')

    def testCommonAPIUndo(self):
        '''Undo of move with common API must completely remove attr spec.'''

        def createCommonAPI(testCase, sphereXformable):
            sphereXformable.AddTranslateOp(
                UsdGeom.XformOp.PrecisionFloat, "pivot")
            sphereXformable.AddTranslateOp(
                UsdGeom.XformOp.PrecisionFloat, "pivot", True)
    
            self.assertEqual(
                sphereXformable.GetXformOpOrderAttr().Get(), 
                Vt.TokenArray(("xformOp:translate:pivot",
                               "!invert!xformOp:translate:pivot")))
            self.assertTrue(UsdGeom.XformCommonAPI(sphereXformable))

        self.runTestOpUndo(createCommonAPI, 'xformOp:rotateXYZ')


if __name__ == '__main__':
    unittest.main(verbosity=2)
