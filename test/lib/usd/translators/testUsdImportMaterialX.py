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

from maya import cmds
from maya import standalone

import os
import unittest

import fixturesUtils

try:
    from pxr import UsdMtlx
    USD_HAS_MATERIALX_PLUGIN=True
except ImportError:
    USD_HAS_MATERIALX_PLUGIN=False


class testUsdImportMaterialX(unittest.TestCase):
    """Test importing an asset with MaterialX shading:
        - As exported from Maya
        - As converted from UsdMtlx module
    """

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.readOnlySetUpClass(__file__)
        usdFile = os.path.join(cls.inputPath, "UsdImportMaterialX", "UsdImportMaterialX.usda")
        options = ["shadingMode=[[useRegistry,MaterialX]]",
                   "primPath=/"]
        cmds.file(usdFile, i=True, type="USD Import",
                  ignoreVersion=True, ra=True, mergeNamespacesOnClash=False,
                  namespace="Test", pr=True, importTimeRange="combine",
                  options=";".join(options))

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testImportMaterialX(self):
        """
        Tests that the materials on imported spheres are what we expect.
        """

        # pPlatonic1Shape has a fully textured standardSurface:
        sgs = cmds.listConnections("Test:pPlatonic1Shape", type="shadingEngine")
        shaders = cmds.listConnections(sgs[0] + ".surfaceShader", d=False)
        node_type = cmds.nodeType(shaders[0])
        self.assertEqual(shaders[0], "Test:standardSurface2")
        self.assertEqual(node_type, "standardSurface")

        # Check texture connections:
        expected = set([
            ('Test:standardSurface2.baseColor', 'Test:file1.outColor'),
            ('Test:standardSurface2.emission', 'Test:file2.outColorB'),
            ('Test:standardSurface2.metalness', 'Test:file2.outColorG'),
            ('Test:standardSurface2.specularRoughness', 'Test:file2.outColorR'),
            ('Test:standardSurface2.normalCamera', 'Test:file3.outColor'),
        ])
        cnx = cmds.listConnections(shaders[0], destination=False,
                                   plugs=True, connections=True)
        cnx = set([(cnx[2*i], cnx[2*i+1]) for i in range(int(len(cnx)/2))])
        self.assertEqual(cnx, expected)

        # Check texture file names:
        expected = [
            ["Test:file1", "Brazilian_rosewood_pxr128.png"],
            ["Test:file2", "RGB.png"],
            ["Test:file3", "normalSpiral.png"],
        ]
        for file_node, tx_name in expected:
            attr = file_node + ".fileTextureName"
            self.assertTrue(cmds.getAttr(attr).endswith(tx_name))

        # pPlatonic2Shape has a Jade standardSurface that was converted from
        # .mtlx to .usda using the UsdMtlx module of USD.
        #
        # It has an interesting way of setting the parameters on the
        # standardSurface node by linking all parameters back to the material.
        sgs = cmds.listConnections("Test:pPlatonic2Shape", type="shadingEngine")
        shaders = cmds.listConnections(sgs[0] + ".surfaceShader", d=False)
        node_type = cmds.nodeType(shaders[0])
        # Can't check the name. It can change depending on whether USD has Mtlx
        self.assertEqual(node_type, "standardSurface")
        # Test one set value
        self.assertAlmostEqual(cmds.getAttr(shaders[0] + ".specularIOR"), 2.418)
        # Test one default value
        self.assertAlmostEqual(cmds.getAttr(shaders[0] + ".coatIOR"), 1.5)

    @unittest.skipUnless(USD_HAS_MATERIALX_PLUGIN, 'testImportMaterialXSidecar is available only if USD is compiled with MaterialX support.')
    def testImportMaterialXSidecar(self):
        # pPlatonic3Shape has a Gold material coming from a .mtlx subLayer. It
        # requires a USD compiled with MaterialX to import correctly.
        sgs = cmds.listConnections("Test:pPlatonic3Shape", type="shadingEngine")
        shaders = cmds.listConnections(sgs[0] + ".surfaceShader", d=False)
        node_type = cmds.nodeType(shaders[0])
        # Can't check the name. It can change depending on whether USD has Mtlx
        self.assertEqual(node_type, "standardSurface")
        # Test one set value
        self.assertAlmostEqual(cmds.getAttr(shaders[0] + ".specularRoughness"), 0.02)
        # Test one default value
        self.assertAlmostEqual(cmds.getAttr(shaders[0] + ".coatIOR"), 1.5)

if __name__ == '__main__':
    unittest.main(verbosity=2)
