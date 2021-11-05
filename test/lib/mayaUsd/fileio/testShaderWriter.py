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

import mayaUsd.lib as mayaUsdLib

from maya import cmds
from maya import standalone

import fixturesUtils, os

import unittest

class shaderWriterTest(mayaUsdLib.ShaderWriter):
    CanExportCalled = False
    WriteCalledCount = 0
    GetShadingAttributeNameForMayaAttrNameCalledWith = ""
    GetShadingAttributeForMayaAttrNameCalled = False
    NotCalled = False

    @classmethod
    def CanExport(cls, exportArgs):
        shaderWriterTest.CanExportCalled = True
        return mayaUsdLib.ShaderWriter.ContextSupport.Supported

    def Write(self, usdTime):
        shaderWriterTest.WriteCalledCount += 1

    def GetShadingAttributeNameForMayaAttrName(self, mayaAttrName):
        shaderWriterTest.GetShadingAttributeNameForMayaAttrNameCalledWith = mayaAttrName
        return mayaAttrName

    def GetShadingAttributeForMayaAttrName(self, mayaAttrName, typeName):
        shaderWriterTest.GetShadingAttributeForMayaAttrNameCalled = True
        # return default value so that GetShadingAttributeNameForMayaAttrName gets called
        return mayaUsdLib.ShaderWriter.GetShadingAttributeForMayaAttrName(self, mayaAttrName, typeName)

class testShaderWriter(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testSimpleShaderWriter(self):
        mayaUsdLib.ShaderWriter.Register(shaderWriterTest, "lambert")

        cmds.file(f=True, new=True)

        sphere_xform = cmds.polySphere()[0]

        # Need a lambert, with something connected to exercise the
        # full API. The code will survive the test writer not even
        # creating a UsdShade node for the lambert.
        material_node = cmds.shadingNode("lambert", asShader=True,
                                         name="Lanbert42")

        material_sg = cmds.sets(renderable=True, noSurfaceShader=True,
                                empty=True, name=material_node+"SG")
        cmds.connectAttr(material_node+".outColor",
                         material_sg+".surfaceShader", force=True)
        cmds.sets(sphere_xform, e=True, forceElement=material_sg)

        file_node = cmds.shadingNode("file", asTexture=True,
                                     isColorManaged=True)
        uv_node = cmds.shadingNode("place2dTexture", asUtility=True)

        for att_name in (".coverage", ".translateFrame", ".rotateFrame",
                         ".mirrorU", ".mirrorV", ".stagger", ".wrapU",
                         ".wrapV", ".repeatUV", ".offset", ".rotateUV",
                         ".noiseUV", ".vertexUvOne", ".vertexUvTwo",
                         ".vertexUvThree", ".vertexCameraOne"):
            cmds.connectAttr(uv_node + att_name, file_node + att_name, f=True)

        cmds.connectAttr(file_node + ".outColor",
                         material_node + ".color", f=True)

        cmds.setAttr(file_node+".fileTextureName", "unknown.png", type="string")

        # Export to USD. We select MaterialX because all the writer
        # there are reporting "Fallback", which allows our CanExport
        # to always win with "Supported" (also helps that the current
        # version of the MaterialX export does not support lambert).
        usdFilePath = os.path.join(self.temp_dir,'testShaderWriter.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo=['MaterialX'],
            materialsScopeName='Materials')

        self.assertTrue(shaderWriterTest.CanExportCalled)
        self.assertEqual(shaderWriterTest.WriteCalledCount,1)
        self.assertTrue(shaderWriterTest.GetShadingAttributeForMayaAttrNameCalled)
        self.assertEqual(shaderWriterTest.GetShadingAttributeNameForMayaAttrNameCalledWith, 'color')
        self.assertFalse(shaderWriterTest.NotCalled)

if __name__ == '__main__':
    unittest.main(verbosity=2)
