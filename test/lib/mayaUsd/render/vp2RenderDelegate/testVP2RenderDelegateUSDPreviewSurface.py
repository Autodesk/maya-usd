#!/usr/bin/env mayapy
#
# Copyright 2021 Autodesk
# Copyright 2021 Apple
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
import imageUtils

import mayaUtils
import usdUtils
import testUtils

from mayaUsd import lib as mayaUsdLib
from mayaUsd import ufe as mayaUsdUfe

from maya import cmds

import ufe

import os

class testVP2RenderDelegateUSDPreviewSurface(imageUtils.ImageDiffingTestCase):
    """
    Test various features of the USD Preview Surface implementation.
    """

    @classmethod
    def setUpClass(cls):
        input_path = fixturesUtils.setUpClass(
            __file__, initializeStandalone=False, loadPlugin=False
        )

        cls._baseline_dir = os.path.join(
            input_path, "VP2RenderDelegateUSDPreviewSurface", "baseline"
        )

        cls._test_dir = os.path.abspath(".")

    def assertSnapshotClose(self, imageName):
        baseline_image = os.path.join(self._baseline_dir, imageName)
        snapshot_image = os.path.join(self._test_dir, imageName)
        imageUtils.snapshot(snapshot_image, width=960, height=540)
        return self.assertImagesClose(baseline_image, snapshot_image)

    def testMetallicResponse(self):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")

        cmds.xform("persp", t=(0, 0, 50))
        cmds.xform("persp", ro=[0, 0, 0], ws=True)

        # Create Test Spheres
        for i in range(5):
            for j in range(5):
                sphere_xform = cmds.polySphere()[0]
                cmds.move(2.5 * (i - 2), 2.5 * (j - 2), 0, sphere_xform, absolute=True)

                material_node = cmds.shadingNode("usdPreviewSurface", asShader=True)
                material_sg = cmds.sets(
                    renderable=True,
                    noSurfaceShader=True,
                    empty=True,
                    name=material_node + "SG",
                )
                cmds.connectAttr(
                    material_node + ".outColor",
                    material_sg + ".surfaceShader",
                    force=True,
                )
                cmds.sets(sphere_xform, e=True, forceElement=material_sg)

                cmds.setAttr(
                    material_node + ".diffuseColor", 0.944, 0.776, 0.373, type="double3"
                )
                cmds.setAttr(material_node + ".useSpecularWorkflow", False)
                cmds.setAttr(material_node + ".metallic", i * 0.25)
                roughness = j * 0.25
                if j == 0:
                    roughness = 0.001
                cmds.setAttr(material_node + ".roughness", roughness)

        blue_light = cmds.directionalLight(rgb=(0, 0.6069, 1))
        blue_transform = cmds.listRelatives(blue_light, parent=True)[0]
        cmds.xform(blue_transform, ro=(-45, -45, 0), ws=True)

        orange_light = cmds.directionalLight(rgb=(1, 0.2703, 0))
        orange_transform = cmds.listRelatives(orange_light, parent=True)[0]
        cmds.xform(orange_transform, ro=(-45, 45, 0), ws=True)

        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, lights=False, displayLights="all")

        if int(os.getenv("MAYA_LIGHTAPI_VERSION")) == 2:
            self.assertSnapshotClose("testMetallicResponseLightAPI2.png")
        else:
            self.assertSnapshotClose("testMetallicResponseLightAPI1.png")

    def testShadowsAndSSAO(self):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")

        cmds.xform("persp", t=(10, 10, 10))
        cmds.xform("persp", ro=[-30, 45, 0], ws=True)

        testFile = testUtils.getTestScene("UsdPreviewSurface", "LightAPI_Test.usda")
        mayaUtils.createProxyFromFile(testFile)

        white_light = cmds.directionalLight(rgb=(1, 1, 1))
        white_transform = cmds.listRelatives(white_light, parent=True)[0]
        cmds.xform(white_transform, ro=(-35, 0, 0), ws=True)

        if int(os.getenv("MAYA_LIGHTAPI_VERSION")) == 2:
            light_api = "V2"
        else:
            light_api = "V1"

        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, lights=False, displayLights="all")
        cmds.setAttr("hardwareRenderingGlobals.ssaoEnable", False)
        cmds.modelEditor(panel, edit=True, shadows=False)

        self.assertSnapshotClose('LightAPI_{}.png'.format(light_api))

        cmds.setAttr("hardwareRenderingGlobals.ssaoEnable", True)
        self.assertSnapshotClose('SSAO_LightAPI_{}.png'.format(light_api))

        cmds.modelEditor(panel, edit=True, shadows=True)
        self.assertSnapshotClose('Shadow_LightAPI_{}.png'.format(light_api))

        cmds.setAttr("hardwareRenderingGlobals.ssaoEnable", False)
        cmds.modelEditor(panel, edit=True, shadows=False)

    def testUsdTexture2d(self):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")

        cmds.xform("persp", t=(0, 0, 2))
        cmds.xform("persp", ro=[0, 0, 0], ws=True)

        testFile = testUtils.getTestScene("UsdPreviewSurface", "UsdTransform2dTest.usda")
        mayaUtils.createProxyFromFile(testFile)

        self.assertSnapshotClose('UsdTransform2dTest.png')

    def testUseSpecularWorkflow(self):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")

        cmds.xform("persp", t=(0, 0, 10))
        cmds.xform("persp", ro=[0, 0, 0], ws=True)

        testFile = testUtils.getTestScene("UsdPreviewSurface", "UseSpecularWorkflowTest.usda")
        mayaUtils.createProxyFromFile(testFile)

        self.assertSnapshotClose('UseSpecularWorkflowTest.png')

    def testMetallicF0(self):
        """Tests that the specular F0 of a metallic surface is equal to its base color
        See Pixar USD commit https://github.com/PixarAnimationStudios/USD/commit/f11ab360"""
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")

        cmds.xform("persp", t=(24, 16, 0), ws=True)
        cmds.xform("persp", ro=[-35, 90, 0], ws=True)

        testFile = testUtils.getTestScene("UsdPreviewSurface", "F0_is_base.usda")
        mayaUtils.createProxyFromFile(testFile)

        # Need a point light at (0.36, 7.625, 8.111)
        white_light = cmds.pointLight(rgb=(1, 1, 1))
        white_transform = cmds.listRelatives(white_light, parent=True)[0]
        cmds.xform(white_transform, t=(0.36, 7.625, 8.111), ws=True)

        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, lights=False, displayLights="all")

        if int(os.getenv("MAYA_LIGHTAPI_VERSION")) == 2:
            light_api = "V2"
        else:
            light_api = "V1"

        self.assertSnapshotClose('F0_is_base_{}.png'.format(light_api))


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
