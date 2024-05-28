#!/usr/bin/env mayapy
#
# Copyright 2024 Autodesk
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

from maya import cmds
from maya import standalone

from pxr import Usd
import fixturesUtils

class testUsdExportMesh(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')

        cls.testFile = os.path.join(cls.inputPath, "UsdExportTypeTest", "UsdExportTypeTest.ma")
        cmds.file(cls.testFile, force=True, open=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportMeshDefaultPrim(self):
        '''Export to USD with a mesh set as the default prim.'''
        usdFile = os.path.abspath('UsdExportMeshDefaultPrim.usda')

        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none', defaultPrim='pSphere1')
        
        stage = Usd.Stage.Open(usdFile)
        defaultPrim = stage.GetDefaultPrim()
        self.assertTrue(defaultPrim)
        self.assertEqual(defaultPrim.GetName(), 'pSphere1')

    def testExportLightDefaultPrim(self):
        '''Export to USD with a light set as the default prim.'''
        usdFile = os.path.abspath('UsdExportLightDefaultPrim.usda')

        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none', defaultPrim='pointLight1')
        
        stage = Usd.Stage.Open(usdFile)
        defaultPrim = stage.GetDefaultPrim()
        self.assertTrue(defaultPrim)
        self.assertEqual(defaultPrim.GetName(), 'pointLight1')

    def testExportRootScopeDefaultPrim(self):
        '''Export to USD with a scope set as the default prim.'''
        usdFile = os.path.abspath('UsdExportScopeDefaultPrim.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none', rootPrim='testScope', defaultPrim = 'testScope')
        
        stage = Usd.Stage.Open(usdFile)
        defaultPrim = stage.GetDefaultPrim()
        self.assertTrue(defaultPrim)
        self.assertEqual(defaultPrim.GetName(), 'testScope')

    def testExportNoDefaultPrim(self):
        '''Export to USD with no default prim.'''
        usdFile = os.path.abspath('UsdExportScopeDefaultPrim.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none', defaultPrim=None)
        
        stage = Usd.Stage.Open(usdFile)
        defaultPrim = stage.GetDefaultPrim()
        self.assertFalse(defaultPrim)

    def testExportMaterialScopeDefaultPrim(self):
        '''Export to USD with no default prim.'''
        usdFile = os.path.abspath('UsdExportMaterialScopeDefaultPrim.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='useRegistry', convertMaterialsTo=['UsdPreviewSurface'], defaultPrim='mtl')
        
        stage = Usd.Stage.Open(usdFile)
        defaultPrim = stage.GetDefaultPrim()
        self.assertTrue(defaultPrim)
        self.assertEqual(defaultPrim.GetName(), 'mtl')
        self.assertFalse(stage.GetPrimAtPath('/mtl/mtl'))

    def testExportNoMeshMaterialScopeDefaultPrim(self):
        '''Export to USD with no default prim.'''
        usdFile = os.path.abspath('UsdExportNoMeshMaterialScopeDefaultPrim.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='useRegistry', convertMaterialsTo=['UsdPreviewSurface'], defaultPrim='mtl',
            excludeExportTypes=['Meshes'])
        
        stage = Usd.Stage.Open(usdFile)
        defaultPrim = stage.GetDefaultPrim()
        self.assertTrue(defaultPrim)
        self.assertEqual(defaultPrim.GetName(), 'mtl')
        self.assertFalse(stage.GetPrimAtPath('/mtl/mtl'))


if __name__ == '__main__':
    unittest.main(verbosity=2)
