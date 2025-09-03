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
import ufeUtils

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
    
    def getConnectionHandler(self):
        rid = ufe.RunTimeMgr.instance().getId('USD')
        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(rid)
        self.assertIsNotNone(connectionHandler)
        return connectionHandler

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'nodeDefHandler is only available in UFE v4 or greater')
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

        nodeDef = nodeDefHandler.definition("ND_noise2d_color3")
        self.assertEqual(nodeDef.classification(nodeDef.nbClassifications() - 1), "MaterialX")


    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'filename and float2 are no longer generic starting with Ufe v4')
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
        self.assertEqual(inputs[4].type(), "EnumString")
        self.assertEqual(inputs[4].defaultValue(), "periodic")
        self.assertEqual(inputs[5].name(), "vaddressmode")
        self.assertEqual(inputs[5].type(), "EnumString")
        self.assertEqual(inputs[5].defaultValue(), "periodic")
        self.assertEqual(inputs[6].name(), "filtertype")
        self.assertEqual(inputs[6].type(), "EnumString")
        self.assertEqual(inputs[6].defaultValue(), "linear")
        self.assertEqual(inputs[7].name(), "framerange")
        self.assertEqual(inputs[7].type(), "String")
        self.assertEqual(inputs[7].defaultValue(), "")
        self.assertEqual(inputs[8].name(), "frameoffset")
        self.assertEqual(inputs[8].type(), "Int")
        self.assertEqual(inputs[8].defaultValue(), "0")
        self.assertEqual(inputs[9].name(), "frameendaction")
        self.assertEqual(inputs[9].type(), "EnumString")
        self.assertEqual(inputs[9].defaultValue(), "constant")
        self.assertEqual(len(outputs), 1)
        self.assertEqual(outputs[0].name(), "out")
        self.assertEqual(outputs[0].type(), "ColorFloat3")

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Improvements to nodeDef only available in UFE v4 or greater')
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

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'nodeDefHandler is only available in UFE V4 or greater')
    def testNodeCreation(self):
        import mayaUsd_createStageWithNewLayer
        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePathStr = '|stage1|stageShape1'
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()
        nodeDefHandler = self.getNodeDefHandler()
        connectionHandler = self.getConnectionHandler()

        # Create a simple hierarchy.
        scopePrim = stage.DefinePrim('/mtl', 'Scope')
        scopePathStr = proxyShapePathStr + ",/mtl"
        self.assertIsNotNone(scopePrim)
        materialPrim = stage.DefinePrim('/mtl/Material1', 'Material')
        materialPathStr = scopePathStr + "/Material1"
        self.assertIsNotNone(materialPrim)
        nodeGraphPrim = stage.DefinePrim('/mtl/Material1/NodeGraph1', 'NodeGraph')
        nodeGraphPathStr = materialPathStr + "/NodeGraph1"
        self.assertIsNotNone(nodeGraphPrim)

        # Construct a surface shader node inside the existing material.
        type = "ND_UsdPreviewSurface_surfaceshader"
        nodeDef = nodeDefHandler.definition(type)
        self.assertIsNotNone(nodeDef)
        materialSceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(materialPathStr))
        self.assertIsNotNone(materialSceneItem)
        command = nodeDef.createNodeCmd(materialSceneItem, ufe.PathComponent("UsdPreviewSurface"))
        self.assertIsNotNone(command)
        command.execute()
        self.assertIsNotNone(command.insertedChild)
        # Verify it is created in the right place and has no connections
        self.assertEqual(ufe.PathString.string(command.insertedChild.path()), materialPathStr + "/UsdPreviewSurface1")
        connections = connectionHandler.sourceConnections(materialSceneItem)
        self.assertIsNotNone(connections)
        self.assertEqual(len(connections.allConnections()), 0)

        # Construct a surface shader node at the mtl scope
        scopeSceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(scopePathStr))
        self.assertIsNotNone(scopeSceneItem)
        command = nodeDef.createNodeCmd(scopeSceneItem, ufe.PathComponent("UsdPreviewSurface"))
        self.assertIsNotNone(command)
        command.execute()
        self.assertIsNotNone(command.insertedChild)
        # Verify that a new material is created, matching the shader name, and the connection set
        self.assertEqual(ufe.PathString.string(command.insertedChild.path()), scopePathStr + "/UsdPreviewSurface1/UsdPreviewSurface1")
        newMaterialSceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(scopePathStr + "/UsdPreviewSurface1"))
        connections = connectionHandler.sourceConnections(newMaterialSceneItem)
        self.assertIsNotNone(connections)
        self.assertEqual(len(connections.allConnections()), 1)

        # Construct a non surface shader node at the mtl scope
        type = "ND_image_color3"
        nodeDef = nodeDefHandler.definition(type)
        self.assertIsNotNone(nodeDef)
        scopeSceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(scopePathStr))
        self.assertIsNotNone(scopeSceneItem)
        command = nodeDef.createNodeCmd(scopeSceneItem, ufe.PathComponent("image"))
        self.assertIsNotNone(command)
        command.execute()
        self.assertIsNotNone(command.insertedChild)
        # Verify that a new material is created, but without connections this time as it is not a surface
        self.assertEqual(ufe.PathString.string(command.insertedChild.path()), scopePathStr + "/image1/image1")
        newMaterialSceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(scopePathStr + "/image1"))
        connections = connectionHandler.sourceConnections(newMaterialSceneItem)
        self.assertIsNotNone(connections)
        self.assertEqual(len(connections.allConnections()), 0)

    @unittest.skipIf(os.getenv('UFE_HAS_NATIVE_TYPE_METADATA', 'NOT-FOUND') not in ('1', "TRUE"), 'Test only available if UFE AttributeDef \'NativeType\' metadata exists.')
    def testNativeTypeMetadata(self):
        type = "ND_constant_color3"
        nodeDefHandler = self.getNodeDefHandler()
        nodeDef = nodeDefHandler.definition(type)
        self.assertIsNotNone(nodeDef)
        valueInput = nodeDef.input("value")
        self.assertIsNotNone(valueInput)
        self.assertTrue(valueInput.hasMetadata(ufe.AttributeDef.kNativeType))
        self.assertEqual(valueInput.type(), "ColorFloat3")
        self.assertEqual(valueInput.getMetadata(ufe.AttributeDef.kNativeType), ufe.Value("color3f"))

        type = "ND_surfacematerial"
        nodeDef = nodeDefHandler.definition(type)
        self.assertIsNotNone(nodeDef)
        surfaceShaderInput = nodeDef.input("surfaceshader")
        self.assertIsNotNone(surfaceShaderInput)
        self.assertTrue(surfaceShaderInput.hasMetadata(ufe.AttributeDef.kNativeType))
        self.assertEqual(surfaceShaderInput.type(), "Generic")
        self.assertEqual(surfaceShaderInput.getMetadata(ufe.AttributeDef.kNativeType), ufe.Value("terminal"))

