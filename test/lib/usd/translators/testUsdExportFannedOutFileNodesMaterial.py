#!/usr/bin/env mayapy
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

from pxr import Usd
from pxr import UsdShade
from pxr import Sdr

from maya import cmds
from maya import standalone

import mayaUsd.lib as mayaUsdLib

import os
import unittest

import fixturesUtils
try:
    import mayaUtils
except ImportError:
    pass
    
class testExportFannedOutFileNodesMaterial(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)
        mayaFile = os.path.join(inputPath, 'UsdExportFannedOutFileNodesMaterialTest',
            'FannedOutFileNodesMaterialTest.ma')
        cmds.file(mayaFile, force=True, open=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()
        
    @unittest.skipUnless("mayaUtils" in globals() and mayaUtils.mayaMajorVersion() >= 2023 and Usd.GetVersion() > (0, 21, 2), 'Requires MaterialX support.')
    def testMaterialScopeResolution(self):
        # New default value as per USD Asset WG:
        self.assertEqual(mayaUsdLib.JobExportArgs.GetDefaultMaterialsScopeName(), "mtl")

        # This override is low-priority
        os.environ["MAYAUSD_MATERIALS_SCOPE_NAME"] = "MyMaterialScope"
        self.assertEqual(mayaUsdLib.JobExportArgs.GetDefaultMaterialsScopeName(), "MyMaterialScope")

        # The USD_FORCE_DEFAULT_MATERIALS_SCOPE_NAME override has higher priority, but is read
        # at startup and can not be turned on/off for this test. It was used on all tests that
        # relied on "Looks" being the default name instead of "mtl"

        os.environ.pop("MAYAUSD_MATERIALS_SCOPE_NAME")
        self.assertEqual(mayaUsdLib.JobExportArgs.GetDefaultMaterialsScopeName(), "mtl")

    @unittest.skipUnless("mayaUtils" in globals() and mayaUtils.mayaMajorVersion() >= 2023 and Usd.GetVersion() > (0, 21, 2), 'Requires MaterialX support.')
    def testExportedUsdShadeNodeTypes(self):
        '''
        Tests that all node ids are what we expect:
        '''
        
        usdFilePath = os.path.abspath('FannedOutFileNodesMaterialTest.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', 
            convertMaterialsTo=['MaterialX', 'UsdPreviewSurface'],
            materialsScopeName='Materials')

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)
        
        base_path = "/pCube{0}/Materials/{1}SG/{2}/{1}"
        to_test = [
            # pCube1 has lambert, known to UsdPreviewSurface
            (1, "lambert4", "UsdPreviewSurface", "UsdPreviewSurface"),
            # pCube2 has standard_surface, known to UsdPreviewSurface and MaterialX
            (2, "standardSurface2", "UsdPreviewSurface", "UsdPreviewSurface"),
            (2, "standardSurface2", "MaterialX", "ND_standard_surface_surfaceshader"),
            # pCube3 has UsdPreviewSurface, known to UsdPreviewSurface and MaterialX
            (3, "usdPreviewSurface1", "UsdPreviewSurface", "UsdPreviewSurface"),
            (3, "usdPreviewSurface1", "MaterialX", "ND_UsdPreviewSurface_surfaceshader"),
            # pCube4 has lambert, known to UsdPreviewSurface
            (4, "lambert2", "UsdPreviewSurface", "UsdPreviewSurface"),
            # pCube5 has lambert, known to UsdPreviewSurface
            (5, "lambert3", "UsdPreviewSurface", "UsdPreviewSurface"),
        ]

        allowedNodes = {
            "lambert4" : ("file13", "file14", "file15"),
            "standardSurface2" :  ("file16", "file17", "file18"),
            "usdPreviewSurface1" :  ("file7", "file8", "file9"),
            "lambert2" :  ("file11", "file12"),
            "lambert3" :  ("file10", "file11")}
            
        disallowedNodes = {
            "lambert2" :  ("file10"),
            "lambert3" :  ("file12")}

        knownFileShaders = ["UsdUVTexture", "ND_image_color4"]

        for prim_idx, shd_name, shd_scope, id_attr in to_test:
            prim_path = base_path.format(prim_idx, shd_name, shd_scope)
            
            prim = stage.GetPrimAtPath(prim_path)
            self.assertTrue(prim, prim_path)
            for child in prim.GetParent().GetAllChildren():
                if not prim.IsA(UsdShade.Shader):
                    continue
                # When child name is MayaNG_MaterialX, it is a NodeGraph itself and we 
                # iterate through its children
                if (child.GetName() == "MayaNG_MaterialX"):
                    for nodeGraphChild in child.GetAllChildren():
                        shader = UsdShade.Shader(nodeGraphChild)
                        if (shader.GetShaderId() in knownFileShaders):
                            self.assertIn(nodeGraphChild.GetName(), allowedNodes[prim.GetName()])
                            if (prim.GetName() in disallowedNodes):
                                self.assertNotIn(nodeGraphChild.GetName(), disallowedNodes[prim.GetName()])
                else:
                    shader = UsdShade.Shader(child)
                    if (shader.GetShaderId() in knownFileShaders):
                        self.assertIn(child.GetName(), allowedNodes[prim.GetName()])
                        if (prim.GetName() in disallowedNodes):
                            self.assertNotIn(child.GetName(), disallowedNodes[prim.GetName()])
                
if __name__ == '__main__':
    unittest.main(verbosity=2)
