#!/usr/bin/env mayapy
#
# Copyright 2017 Pixar
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

import os
import unittest

from pxr import Gf
from pxr import Usd
from pxr import UsdShade

from maya import OpenMaya
from maya import cmds
from maya import standalone

import fixturesUtils

class testUsdImportShadingModeDisplayColor(unittest.TestCase):

    RED_COLOR = Gf.Vec3f(1.0, 0.0, 0.0)

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.readOnlySetUpClass(__file__)
        cmds.file(new=True, force=True)

        # Import from USD.
        usdFilePath = os.path.join(inputPath, "UsdImportShadingModeDisplayColor", "RedCube.usda")
        cmds.usdImport(file=usdFilePath, shadingMode=[['displayColor', 'default'], ])

        cls._stage = Usd.Stage.Open(usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStageOpens(self):
        """
        Tests that the USD stage being imported opens successfully.
        """
        self.assertTrue(self._stage)

    def _GetMayaMesh(self, meshName):
         selectionList = OpenMaya.MSelectionList()
         selectionList.add(meshName)
         mObj = OpenMaya.MObject()
         selectionList.getDependNode(0, mObj)

         meshFn = OpenMaya.MFnMesh(mObj)
         self.assertTrue(meshFn)

         return meshFn

    def testImportDisplayColorShading(self):
        """
        Tests that importing a USD Mesh prim with a simple shading setup
        results in the correct shading on the Maya mesh.
        """
        meshFn = self._GetMayaMesh('|RedCube|Geom|Cube|CubeShape')

        # Validate the shadingEngine connected to the mesh.
        objectArray = OpenMaya.MObjectArray()
        indexArray = OpenMaya.MIntArray()
        meshFn.getConnectedShaders(0, objectArray, indexArray)
        self.assertEqual(objectArray.length(), 1)

        shadingEngineObj = objectArray[0]
        shadingEngineDepFn = OpenMaya.MFnDependencyNode(shadingEngineObj)
        self.assertEqual(shadingEngineDepFn.name(),
            'USD_Materials:RedLambertSG')

        # Validate the lambert plugged in as the surfaceShader of the
        # shadingEngine.
        surfaceShaderPlug = shadingEngineDepFn.findPlug('surfaceShader', True)
        plugArray = OpenMaya.MPlugArray()
        surfaceShaderPlug.connectedTo(plugArray, True, False)
        self.assertEqual(plugArray.length(), 1)

        shaderObj = plugArray[0].node()
        shaderLambertFn = OpenMaya.MFnLambertShader(shaderObj)
        self.assertEqual(shaderLambertFn.name(),
            'RedLambertSG_lambert')

        mayaColor = shaderLambertFn.color()
        mayaColor = Gf.Vec3f(mayaColor.r, mayaColor.g, mayaColor.b)
        self.assertTrue(Gf.IsClose(mayaColor, self.RED_COLOR, 1e-6))


if __name__ == '__main__':
    unittest.main(verbosity=2)
