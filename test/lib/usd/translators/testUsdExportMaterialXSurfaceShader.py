#!/usr/bin/env mayapy
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

from pxr import Usd
from pxr import Sdf
from pxr import Sdr
from pxr import UsdShade

from maya import cmds
from maya import standalone

import mayaUsd.lib as mayaUsdLib

import maya.api.OpenMaya as om

import os
import unittest

import fixturesUtils

class testUsdExportMaterialXSurfaceShader(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls._inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportMaterialXStack(self):

        cmds.file(f=True, new=True)

        mayaFile = os.path.join(self._inputPath, 'UsdExportMaterialXSurfaceShader',
            'MaterialXStackExport.ma')
        cmds.file(mayaFile, force=True, open=True)

        # Export to USD.
        usdFilePath = os.path.abspath('MaterialXStackExport.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo=['MaterialX'],
            materialsScopeName='Materials', defaultPrim='None')

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        prim = stage.GetPrimAtPath("/Materials/Standard_Surface1SG")
        self.assertTrue(prim)
        material = UsdShade.Material(prim)
        self.assertTrue(material)

        # Validate the displacement shader
        dispOutput = material.GetDisplacementOutput("mtlx")
        self.assertTrue(dispOutput)
        dispSource = dispOutput.GetConnectedSources()[0]
        dispPrim = dispSource[0].source.GetPrim()
        self.assertEqual(dispPrim.GetName(), "displacement1")
        id = dispPrim.GetAttribute('info:id')
        self.assertEqual(id.Get(), 'ND_displacement_float')
        dispShader = UsdShade.Shader(dispPrim)
        dispIn = dispShader.GetInput('displacement')
        # Validate the hexagon shader, connected to the displacement
        hexPrim = dispIn.GetConnectedSources()[0][0].source.GetPrim()
        self.assertEqual(hexPrim.GetName(), 'hexagon1')
        self.assertEqual(hexPrim.GetAttribute('info:id').Get(), 'ND_hexagon_float')
        hexShader = UsdShade.Shader(hexPrim)
        self.assertEqual(hexShader.GetInput('radius').Get(), 1)

        # Validate the Surface shader
        surfOutput = material.GetSurfaceOutput("mtlx")
        self.assertTrue(surfOutput)
        surfSource = surfOutput.GetConnectedSources()[0]
        surfPrim = surfSource[0].source.GetPrim()
        self.assertEqual(surfPrim.GetName(), "Standard_Surface1")
        id = surfPrim.GetAttribute('info:id')
        self.assertEqual(id.Get(), 'ND_standard_surface_surfaceshader')
        surfShader = UsdShade.Shader(surfPrim)
        colorIn = surfShader.GetInput('base_color')
        # Validate the Node Graph
        nodeGraphPrim = colorIn.GetConnectedSources()[0][0].source.GetPrim()
        self.assertEqual(nodeGraphPrim.GetName(), 'compound1')
        nodeGraph = UsdShade.NodeGraph(nodeGraphPrim)
        # Validate the Checkerboard Shader
        checkboardPrim = nodeGraph.GetOutput('out').GetConnectedSources()[0][0].source.GetPrim()
        self.assertEqual(checkboardPrim.GetName(), 'checkboard1')
        self.assertEqual(checkboardPrim.GetAttribute('info:id').Get(), 'ND_checkerboard_color3')


if __name__ == '__main__':
    unittest.main(verbosity=2)
