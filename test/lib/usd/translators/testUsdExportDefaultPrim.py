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

    def testExportNamespaceDefaultPrim(self):
        '''Export to USD with a mesh inside a Maya namespace set as the default prim.'''
        cmds.namespace(add="hello")
        cmds.namespace(setNamespace=":hello")
        cubeNames = cmds.polyCube()
        cmds.namespace(setNamespace=":")
        self.assertEqual(len(cubeNames), 2)

        usdFile = os.path.abspath('UsdExportNamespaceDefaultPrim.usda')

        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none', defaultPrim=cubeNames[0])
        
        stage = Usd.Stage.Open(usdFile)
        defaultPrim = stage.GetDefaultPrim()
        self.assertTrue(defaultPrim)
        self.assertEqual(defaultPrim.GetName(), 'hello_pCube1')

    def testExportRootsNoDefaultPrim(self):
        '''Export to USD with the export roots argument and no default prim should select a root as default.'''
        cubeNames = cmds.polyCube()
        self.assertEqual(len(cubeNames), 2)

        usdFile = os.path.abspath('UsdExportRootsNoDefaultPrim.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none', exportRoots=['|' + cubeNames[0]])

        stage = Usd.Stage.Open(usdFile)
        defaultPrim = stage.GetDefaultPrim()
        self.assertTrue(defaultPrim)
        self.assertEqual(defaultPrim.GetName(), cubeNames[0])

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
            shadingMode='none', defaultPrim="None")
        
        stage = Usd.Stage.Open(usdFile)
        defaultPrim = stage.GetDefaultPrim()
        self.assertFalse(defaultPrim)

    def testExportMaterialScopeDefaultPrim(self):
        '''Export to USD with no default prim.'''
        usdFile = os.path.abspath('UsdExportMaterialScopeDefaultPrim.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='useRegistry', convertMaterialsTo=['UsdPreviewSurface'], defaultPrim='mtl',
            legacyMaterialScope=False)
        
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
            excludeExportTypes=['Meshes'], legacyMaterialScope=False)
        
        stage = Usd.Stage.Open(usdFile)
        defaultPrim = stage.GetDefaultPrim()
        self.assertTrue(defaultPrim)
        self.assertEqual(defaultPrim.GetName(), 'mtl')
        self.assertFalse(stage.GetPrimAtPath('/mtl/mtl'))

    def testAccessibilityInfoDefaultPrim(self):
        '''Export a prim with accessibility information'''
        usdFile = os.path.abspath("UsdExportAccessibilityInfoDefaultPrim.usda")

        label = "A Sphere"
        description = "A polygonal sphere without any smoothing"

        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
                       shadingMode='none', defaultPrim='pSphere1',
                       accessibilityLabel=label,
                       accessibilityDescription=description)

        stage = Usd.Stage.Open(usdFile)
        defaultPrim = stage.GetDefaultPrim()
        self.assertTrue(defaultPrim)

        usdVersion = Usd.GetVersion()
        if usdVersion >= (0, 25, 5):
            from pxr import UsdUI
            accessibilityAPIs = UsdUI.AccessibilityAPI.GetAll(defaultPrim)
            self.assertTrue(accessibilityAPIs)

            accessibilityAPI = accessibilityAPIs[0]
            labelAttr = accessibilityAPI.GetLabelAttr()
            descriptionAttr = accessibilityAPI.GetDescriptionAttr()

            self.assertTrue(labelAttr)
            self.assertTrue(descriptionAttr)
            self.assertEqual(labelAttr.Get(), label)
            self.assertEqual(descriptionAttr.Get(), description)

        # Also check with ad-hoc properties so that we can make sure that
        # both the ad-hoc version and API version are creating the right results.
        appliedSchemas = defaultPrim.GetMetadata("apiSchemas")
        self.assertTrue(appliedSchemas.HasItem('AccessibilityAPI:default'))

        labelAttr = defaultPrim.GetAttribute("accessibility:default:label")
        descriptionAttr = defaultPrim.GetAttribute("accessibility:default:description")

        self.assertTrue(labelAttr)
        self.assertTrue(descriptionAttr)
        self.assertEqual(labelAttr.Get(), label)
        self.assertEqual(descriptionAttr.Get(), description)

if __name__ == '__main__':
    unittest.main(verbosity=2)
