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
    _usdShaderIdLastValue = ""
    NotCalled = False

    @classmethod
    def CanExport(cls, exportArgs, materialConversionName):
        shaderWriterTest.CanExportCalled = True
        if(materialConversionName=="z"):
            return mayaUsdLib.ShaderWriter.ContextSupport.Supported
        else:
            return mayaUsdLib.ShaderWriter.ContextSupport.Unsupported

    def Write(self, usdTime):
        shaderWriterTest.WriteCalledCount += 1
        shaderWriterTest._usdShaderIdLastValue = self._usdShaderId

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

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testSimpleShaderWriter(self):
        mayaUsdLib.ShaderWriter.Register(shaderWriterTest, "marble", "x", "y")
        mayaUsdLib.ShaderWriter.Register(shaderWriterTest, "lambert", "x", "z")

        mayaFile = os.path.join(testShaderWriter.inputPath, '..', '..', 'usd', 'translators','UsdExportRfMShadersTest',
            'MarbleCube.ma')
        cmds.file(mayaFile, force=True, open=True)

        # Export to USD.
        usdFilePath = os.path.join(os.environ.get('MAYA_APP_DIR'),'testShaderWriter.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo='rendermanForMaya',
            materialsScopeName='Materials')

        self.assertTrue(shaderWriterTest.CanExportCalled)
        self.assertEqual(shaderWriterTest.WriteCalledCount,1)
        self.assertTrue(shaderWriterTest.GetShadingAttributeForMayaAttrNameCalled)
        self.assertEqual(shaderWriterTest.GetShadingAttributeNameForMayaAttrNameCalledWith, 'color')
        self.assertEqual(shaderWriterTest._usdShaderIdLastValue,'x')
        self.assertFalse(shaderWriterTest.NotCalled)

if __name__ == '__main__':
    unittest.main(verbosity=2)
