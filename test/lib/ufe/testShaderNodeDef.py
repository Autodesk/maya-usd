#!/usr/bin/env python

#
# Copyright 2022 Autodesk
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

import fixturesUtils
import mayaUtils

from maya import standalone

import ufe
import mayaUsd.ufe

import os
import unittest

class ShaderNodeDefTestCase(unittest.TestCase):

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        self.assertTrue(self.pluginsLoaded)

    def getNodeDefHandler(self):
        rid = ufe.RunTimeMgr.instance().getId('USD')
        nodeDefHandler = ufe.RunTimeMgr.instance().nodeDefHandler(rid)
        self.assertIsNotNone(nodeDefHandler)
        return nodeDefHandler

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4001', 'nodeDefHandler is only available in UFE preview version 0.4.1 and greater')
    def testDefinitions(self):
        nodeDefHandler = self.getNodeDefHandler()
        nodeDefs = nodeDefHandler.definitions("Shader")
        nodeDefTypes = [nodeDef.type() for nodeDef in nodeDefs]
        # A list of various mtlx nodes
        mtlxDefs = ["ND_image_color3",
                    "ND_tiledimage_float",
                    "ND_triplanarprojection_vector3",
                    "ND_constant_color3",
                    "ND_ramplr_color3",
                    "ND_noise2d_color3",
                    "ND_noise3d_float",
                    "ND_fractal3d_color4",
                    "ND_cellnoise3d_float",
                    "ND_worleynoise2d_vector3",
                    "ND_worleynoise3d_float",
                    "ND_position_vector3",
                    "ND_normal_vector3",
                    "ND_tangent_vector3",
                    "ND_texcoord_vector2",
                    "ND_add_color3",
                    "ND_subtract_vector2",
                    "ND_multiply_float",
                    "ND_divide_vector4FA",
                    "ND_modulo_vector3",
                    "ND_invert_color4",
                    "ND_absval_float",
                    "ND_floor_color3",
                    "ND_ceil_vector3",
                    "ND_power_color4",
                    "ND_cos_float",
                    "ND_clamp_float",
                    "ND_normalize_vector3",
                    "ND_dotproduct_vector3",
                    "ND_transpose_matrix33",
                    "ND_determinant_matrix44",
                    "ND_smoothstep_color3",
                    "ND_rgbtohsv_color3"]
        for mtlxDef in mtlxDefs:
            self.assertTrue(mtlxDef in nodeDefTypes)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4001', 'nodeDefHandler is only available in UFE preview version 0.4.1 and greater')
    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') >= '4015', 'filename and float2 are no longer generic starting with Ufe 0.4.15')
    def testDefinitionByType(self):
        type = "ND_image_color3"
        nodeDefHandler = self.getNodeDefHandler()
        nodeDef = nodeDefHandler.definition(type)
        self.assertIsNotNone(nodeDef)
        inputs = nodeDef.inputs()
        outputs = nodeDef.outputs()
        self.assertEqual(nodeDef.type(), type)
        self.assertEqual(len(inputs), 10)
        self.assertEqual(inputs[0].name(), "file")
        self.assertEqual(inputs[0].type(), "Generic")
        self.assertEqual(inputs[0].defaultValue(), "")
        self.assertEqual(inputs[1].name(), "layer")
        self.assertEqual(inputs[1].type(), "String")
        self.assertEqual(inputs[1].defaultValue(), "")
        self.assertEqual(inputs[2].name(), "default")
        self.assertEqual(inputs[2].type(), "ColorFloat3")
        self.assertEqual(inputs[2].defaultValue(), "(0, 0, 0)")
        self.assertEqual(inputs[3].name(), "texcoord")
        self.assertEqual(inputs[3].type(), "Generic")
        self.assertEqual(inputs[3].defaultValue(), "")
        self.assertEqual(inputs[4].name(), "uaddressmode")
        self.assertEqual(inputs[4].type(), "String")
        self.assertEqual(inputs[4].defaultValue(), "periodic")
        self.assertEqual(inputs[5].name(), "vaddressmode")
        self.assertEqual(inputs[5].type(), "String")
        self.assertEqual(inputs[5].defaultValue(), "periodic")
        self.assertEqual(inputs[6].name(), "filtertype")
        self.assertEqual(inputs[6].type(), "String")
        self.assertEqual(inputs[6].defaultValue(), "linear")
        self.assertEqual(inputs[7].name(), "framerange")
        self.assertEqual(inputs[7].type(), "String")
        self.assertEqual(inputs[7].defaultValue(), "")
        self.assertEqual(inputs[8].name(), "frameoffset")
        self.assertEqual(inputs[8].type(), "Int")
        self.assertEqual(inputs[8].defaultValue(), "0")
        self.assertEqual(inputs[9].name(), "frameendaction")
        self.assertEqual(inputs[9].type(), "String")
        self.assertEqual(inputs[9].defaultValue(), "constant")
        self.assertEqual(len(outputs), 1)
        self.assertEqual(outputs[0].name(), "out")
        self.assertEqual(outputs[0].type(), "ColorFloat3")

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4015', 'filename and float2 are no longer generic starting with Ufe 0.4.15')
    def testDefinitionByType(self):
        type = "ND_image_color3"
        nodeDefHandler = self.getNodeDefHandler()
        nodeDef = nodeDefHandler.definition(type)
        self.assertIsNotNone(nodeDef)
        inputs = nodeDef.inputs()
        outputs = nodeDef.outputs()
        self.assertEqual(nodeDef.type(), type)
        self.assertEqual(len(inputs), 10)
        self.assertEqual(inputs[0].name(), "file")
        self.assertEqual(inputs[0].type(), "Filename")
        self.assertEqual(inputs[0].defaultValue(), "")
        self.assertEqual(inputs[1].name(), "layer")
        self.assertEqual(inputs[1].type(), "String")
        self.assertEqual(inputs[1].defaultValue(), "")
        self.assertEqual(inputs[2].name(), "default")
        self.assertEqual(inputs[2].type(), "ColorFloat3")
        self.assertEqual(inputs[2].defaultValue(), "(0, 0, 0)")
        self.assertEqual(inputs[3].name(), "texcoord")
        self.assertEqual(inputs[3].type(), "Float2")
        self.assertEqual(inputs[3].defaultValue(), "")
        self.assertEqual(inputs[4].name(), "uaddressmode")
        self.assertEqual(inputs[4].type(), "String")
        self.assertEqual(inputs[4].defaultValue(), "periodic")
        self.assertEqual(inputs[5].name(), "vaddressmode")
        self.assertEqual(inputs[5].type(), "String")
        self.assertEqual(inputs[5].defaultValue(), "periodic")
        self.assertEqual(inputs[6].name(), "filtertype")
        self.assertEqual(inputs[6].type(), "String")
        self.assertEqual(inputs[6].defaultValue(), "linear")
        self.assertEqual(inputs[7].name(), "framerange")
        self.assertEqual(inputs[7].type(), "String")
        self.assertEqual(inputs[7].defaultValue(), "")
        self.assertEqual(inputs[8].name(), "frameoffset")
        self.assertEqual(inputs[8].type(), "Int")
        self.assertEqual(inputs[8].defaultValue(), "0")
        self.assertEqual(inputs[9].name(), "frameendaction")
        self.assertEqual(inputs[9].type(), "String")
        self.assertEqual(inputs[9].defaultValue(), "constant")
        self.assertEqual(len(outputs), 1)
        self.assertEqual(outputs[0].name(), "out")
        self.assertEqual(outputs[0].type(), "ColorFloat3")

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4010', 'Improvements to nodeDef only available in UFE preview version 0.4.8 and greater')
    def testClassificationsAndMetadata(self):
        type = "ND_image_color3"
        nodeDefHandler = self.getNodeDefHandler()
        nodeDef = nodeDefHandler.definition(type)
        self.assertIsNotNone(nodeDef)

        # Full input API:
        inputNames = nodeDef.inputNames()
        self.assertEqual(len(inputNames), 10)
        self.assertIn("file", inputNames)
        self.assertTrue(nodeDef.hasInput("file"))
        self.assertFalse(nodeDef.hasInput("DefinitelyNotAnInput"))

        # Metadata API:
        self.assertTrue(nodeDef.hasMetadata("role"))
        self.assertEqual(nodeDef.getMetadata("role"), ufe.Value("texture"))
        self.assertFalse(nodeDef.hasMetadata("DefinitelyNotAKnownMetadata"))

        # Classifications API:
        self.assertGreater(nodeDef.nbClassifications(), 0)
        self.assertEqual(nodeDef.classification(0), "image")

        # AttributeDef Metadata API:
        fileInput = nodeDef.input("file")
        self.assertTrue(fileInput.hasMetadata("__SDR__isAssetIdentifier"))
        self.assertFalse(fileInput.hasMetadata("DefinitelyNotAKnownMetadata"))
        nodeDef = nodeDefHandler.definition("ND_add_float")
        output = nodeDef.output("out")
        self.assertEqual(output.getMetadata("__SDR__defaultinput"), ufe.Value("in1"))



