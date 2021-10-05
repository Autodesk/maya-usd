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

from pxr import Usd, UsdShade

from maya import cmds
from maya.api import OpenMaya
from maya import standalone

import fixturesUtils, os

import unittest

class mtlxShaderReaderTest(mayaUsdLib.ShaderReader):
    IsConverterCalled = False
    ReadCalled = False
    GetCreatedObjectCalled = False
    GetMayaPlugForUsdAttrNameCalled = 0
    GetMayaPlugForUsdAttrNameEndCalled = 0

    NotCalled = False

    _setAlphaIsLuminance = False

    def IsConverter(self):
        mtlxShaderReaderTest.IsConverterCalled = True
        self._refinedOutputToken = ''
        prim = self._GetArgs().GetUsdPrim()
        shaderSchema = UsdShade.Shader(prim)
        if (not shaderSchema):
            return None

        shaderId = shaderSchema.GetIdAttr().Get()

        input = shaderSchema.GetInput('in')
        if (not input):
            return None

        (source, sourceOutputName, sourceType) = input.GetConnectedSource()
        if (not source):
            return None

        downstreamSchema = UsdShade.Shader(source.GetPrim())
        if (not downstreamSchema):
            return None

        # No refinement necessary for ND_convert_color3_vector3 and ND_normalmap.

        if ("ND_luminance_" in shaderId):
            # Luminance is an alpha output.
            self._setAlphaIsLuminance = True
            self._refinedOutputToken = 'outAlpha'
        elif ("ND_swizzle_" in shaderId):
            channelsAttr = shaderSchema.GetInput('channels')
            val = channelsAttr.Get(Usd.TimeCode.Default())
            if(val=='r' or val=='x'):
                self._refinedOutputToken = 'outColorR'
            elif(val=='g'):
                self._refinedOutputToken = 'outColorG'
            elif(val=='y'):
                if (shaderSchema.GetOutput('out').GetTypeName() == 'Float'):
                    self._refinedOutputToken = 'outAlpha'
                else:
                    self._refinedOutputToken = 'outColorG'
            elif(val=='b' or val=='z'):
                self._refinedOutputToken = 'outColorB'
            elif(val=='a' or val=='w'):
                self._refinedOutputToken = 'outAlpha'
            else:
                print("Unsupported swizzle" + val)
                # TF_CODING_ERROR("Unsupported swizzle");
            
#            } else if (channels.size() == 3) {
#                // Triple channel swizzles must go to outColor:
#                self._refinedOutputToken = 'outColor';
#            }
        self._downstreamPrim = source.GetPrim()
        return downstreamSchema, sourceOutputName

    def GetCreatedObject(self, context, prim):
        mtlxShaderReaderTest.GetCreatedObjectCalled = True
        if (self._downstreamReader):
            return self._downstreamReader.GetCreatedObject(context, self._downstreamPrim)
        return OpenMaya.MObject()

    def Read(self, context):
        mtlxShaderReaderTest.ReadCalled = True
        if (self._downstreamReader):
            return self._downstreamReader.Read(context)
        return False

    def GetMayaPlugForUsdAttrName(self, usdAttrName, mayaObject):
        mtlxShaderReaderTest.GetMayaPlugForUsdAttrNameCalled += 1
        mayaPlug = OpenMaya.MPlug()
        if (self._downstreamReader):
            mayaPlug = self._downstreamReader.GetMayaPlugForUsdAttrName(usdAttrName, mayaObject)

            if (mayaPlug.isNull or len(self._refinedOutputToken)==0):
                # Nothing to refine.
                return mayaPlug

            if (self._refinedOutputToken != 'outColor' and mayaUsdLib.ShadingUtil.GetStandardAttrName(mayaPlug, False) != 'outColor'):
                # Already refined. Do not refine twice.
                return mayaPlug

            depNodeFn = OpenMaya.MFnDependencyNode(mayaPlug.node())

            if (self._setAlphaIsLuminance):
                alphaIsLuminancePlug = depNodeFn.findPlug('alphaIsLuminance', True)
                alphaIsLuminancePlug.setValue(True)

            if (len(self._refinedOutputToken)>0):
                mayaPlug = depNodeFn.findPlug(self._refinedOutputToken, True)
            mtlxShaderReaderTest.GetMayaPlugForUsdAttrNameEndCalled += 1
        return mayaPlug


class testShaderReader(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testMaterialXShaderReader(self):
        mayaUsdLib.ShaderReader.Register(mtlxShaderReaderTest, "ND_luminance_color3_float", "w", "y")
        mayaUsdLib.ShaderReader.Register(mtlxShaderReaderTest, "ND_swizzle_color3_float", "w", "y")
        mayaUsdLib.ShaderReader.Register(mtlxShaderReaderTest, "ND_convert_color3_vector3", "w", "y")
        
        usdFilePath = os.path.join(testShaderReader.inputPath, '..', '..', 'usd', 'translators','UsdImportMaterialX',
            'UsdImportMaterialX.usda')

        cmds.usdImport(file=usdFilePath, shadingMode=['useRegistry','MaterialX' ])

        self.assertTrue(mtlxShaderReaderTest.IsConverterCalled)
        self.assertFalse(mtlxShaderReaderTest.ReadCalled)
        self.assertFalse(mtlxShaderReaderTest.GetCreatedObjectCalled)
        self.assertEqual(mtlxShaderReaderTest.GetMayaPlugForUsdAttrNameCalled,5)
        self.assertEqual(mtlxShaderReaderTest.GetMayaPlugForUsdAttrNameEndCalled,3)
        self.assertFalse(mtlxShaderReaderTest.NotCalled)

if __name__ == '__main__':
    unittest.main(verbosity=2)
