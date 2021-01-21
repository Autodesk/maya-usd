#!/usr/bin/env mayapy
#
# Copyright 2020 Apple Inc. All rights reserved.
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

import fixturesUtils
from maya import cmds
from maya import standalone
from pxr import Usd
from pxr import UsdShade


def findTransformPrim(root):
    for prim in root.GetAllChildren():
        if not prim.IsA(UsdShade.Shader):
            continue

        shader = UsdShade.Shader(prim)
        shader_type = shader.GetIdAttr().Get()
        if shader_type == "UsdTransform2d":
            return prim

        transform = findTransformPrim(prim)
        if transform:
            return transform


class testUsdExportUVTransforms(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.temp_dir = fixturesUtils.setUpClass(__file__)
        cls.maya_file = os.path.join(cls.temp_dir, "UsdExportUVTransforms", "UsdExportUVTransforms.ma")
        cmds.file(cls.maya_file, force=True, open=True)

        usd_file_path = os.path.join(cls.temp_dir, "UsdExportUVTransforms.usda")
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usd_file_path,
                           shadingMode='useRegistry')

        cls.stage = Usd.Stage.Open(usd_file_path)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportStandaloneTransform2dDefault(self):
        """
        Tests that shaders without UV transforms do not get a resulting UsdTransform2D prim created
        """
        root = self.stage.GetPrimAtPath("/PxrUsdTransform2dExportTest/Looks/pxrUsdPreviewSurface_DefaultSG")
        transform = findTransformPrim(root)
        self.assertFalse(transform, "Found a UsdTransform2D when there should be None")

    def testExportStandaloneTransform2dTransformed(self):
        """
        Test that a material with transform 2D values exports with a UsdTransform2D shader and the
        correct values
        """

        root = self.stage.GetPrimAtPath("/PxrUsdTransform2dExportTest/Looks/pxrUsdPreviewSurface_TransformedSG")
        transform = findTransformPrim(root)
        self.assertTrue(transform, "Missing UsdTransform2D shader")

        shader = UsdShade.Shader(transform)
        translation_input = shader.GetInput('translation')
        rotation_input = shader.GetInput('rotation')
        scale_input = shader.GetInput('scale')

        expected_translation_value = (0.10104963, 0.19860554)
        expected_rotation_value = 0.2
        expected_scale_value = (0.8, 1)

        self.assertAlmostEqual(translation_input.Get(), expected_translation_value)

        self.assertAlmostEqual(rotation_input.Get(), expected_rotation_value)

        self.assertEqual(scale_input.Get(), expected_scale_value)


if __name__ == '__main__':
    unittest.main(verbosity=2)
