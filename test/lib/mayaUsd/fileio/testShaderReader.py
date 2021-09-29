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

from pxr import Gf
from pxr import Sdf
from pxr import Tf
from pxr import Vt
from pxr import UsdGeom

from maya import cmds
from maya.api import OpenMaya
from maya import standalone

import fixturesUtils, os

import unittest

class shaderReaderTest(mayaUsdLib.ShaderReader):
    CanImportCalled = False
    IsConverterCalled = False
    ReadCalled = False
    GetCreatedObjectCalled = False
    NotCalled = False

    @classmethod
    def CanImport(cls, args, materialConversion):
        shaderReaderTest.CanImportCalled = True
        return cls.ContextSupport.Fallback

    def Read(self, context):
        shaderReaderTest.ReadCalled = True
        return True

    def GetMayaPlugForUsdAttrName(self, usdAttrName, mayaObject):
        print("shaderReaderTest.GetMayaPlugForUsdAttrName called")
        return OpenMaya.MPlug()

    def GetMayaNameForUsdAttrName(self, usdAttrName):
        print("shaderReaderTest.GetMayaNameForUsdAttrName called")
        return ""

    def PostConnectSubtree(self, context):
        print("shaderReaderTest.PostConnectSubtree called")
        return

    def IsConverter(self, downstreamSchema, downstreamOutputName):
        shaderReaderTest.IsConverterCalled = True
        if (self._mayaNodeTypeName == "x"):
            return False
        else:
            print("This code path is not Python friendly, need to refactor before activating.")
            exit(1)
            downstreamSchema = mayaUsdLib.UsdShadeShader()
            downstreamOutputName = "?"
            return True

    def SetDownstreamReader(self, downstreamReader):
        print("shaderReaderTest.SetDownstreamReader called")
        return

    def GetCreatedObject(self, context, prim):
        shaderReaderTest.GetCreatedObjectCalled = True
        return OpenMaya.MObject()


class testShaderReader(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testSimpleShaderReader(self):
        mayaUsdLib.ShaderReader.Register(shaderReaderTest, "PxrMayaMarble", "x", "y")
#        mayaUsdLib.ShaderReader.Register(shaderReaderTest, "PxrMayaLambert", "w", "y")
        
        usdFilePath = os.path.join(testShaderReader.inputPath, '..', '..', 'usd', 'translators','UsdImportRfMShadersTest',
            'MarbleCube.usda')

        cmds.usdImport(file=usdFilePath, shadingMode=['useRegistry','rendermanForMaya' ])

        self.assertTrue(shaderReaderTest.CanImportCalled)
        self.assertTrue(shaderReaderTest.IsConverterCalled)
        self.assertTrue(shaderReaderTest.ReadCalled)
        self.assertTrue(shaderReaderTest.GetCreatedObjectCalled)
        self.assertFalse(shaderReaderTest.NotCalled)

if __name__ == '__main__':
    unittest.main(verbosity=2)
