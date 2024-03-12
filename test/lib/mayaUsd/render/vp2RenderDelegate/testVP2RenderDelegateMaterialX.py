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

import fixturesUtils
import imageUtils
import mayaUtils
import usdUtils
import testUtils

from mayaUsd import lib as mayaUsdLib
from mayaUsd import ufe as mayaUsdUfe

from maya import cmds

from pxr import Usd

import ufe

import os
import unittest


class testVP2RenderDelegateMaterialX(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate of a MaterialX scene.
    """

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegateMaterialXTest', 'baseline')

        cls._testDir = os.path.abspath('.')

    def assertSnapshotClose(self, imageName, w=960, h=540):
        baselineImage = os.path.join(self._baselineDir, imageName)
        snapshotImage = os.path.join(self._testDir, imageName)
        imageUtils.snapshot(snapshotImage, width=w, height=h)
        return self.assertImagesClose(baselineImage, snapshotImage)

    def _StartTest(self, testName, textured=True):
        mayaUtils.loadPlugin("mayaUsdPlugin")
        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, displayTextures=textured)

        self._testName = testName
        testFile = testUtils.getTestScene("MaterialX", self._testName + ".usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        self.assertSnapshotClose('%s_render.png' % self._testName)

    def _GetPrim(self, mayaPathString, usdPathString):
        mayaPathSegment = mayaUtils.createUfePathSegment(mayaPathString)
        usdPathSegment = usdUtils.createUfePathSegment(usdPathString)
        ufePath = ufe.Path([mayaPathSegment, usdPathSegment])
        ufeItem = ufe.Hierarchy.createItem(ufePath)
        return usdUtils.getPrimFromSceneItem(ufeItem)

    def testUVStreamManagement(self):
        """Test that a scene without primvar readers renders correctly if it
           uses indexed UV streams"""
        cmds.file(force=True, new=True)
        cmds.move(2, -2, 1.5, 'persp')
        self._StartTest('MtlxUVStreamTest')

    def testMayaSurfaces(self):
        cmds.file(force=True, new=True)
        cmds.move(6, -6, 6, 'persp')
        cmds.rotate(60, 0, 45, 'persp')

        # OCIO will clamp colors before converting them, giving slightly different
        # results for the triplanar projection:
        suffix = ""
        if os.getenv('MAYA_HAS_COLOR_MANAGEMENT_SUPPORT_API', 'FALSE') == 'TRUE':
            suffix = "_ocio"

        # MaterialX 1.38.8 has a new triplanar node with superior blending
        if os.getenv('MATERIALX_VERSION', '1.38.0') >= '1.38.8':
            suffix += "_blended"

        mayaUtils.loadPlugin("mayaUsdPlugin")
        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, displayTextures=True)

        testFile = testUtils.getTestScene("MaterialX", "MayaSurfaces.usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        self.assertSnapshotClose('%s_render.png' % ("MayaSurfaces" + suffix))

        # Flat shading requires V3 lighting API:
        if int(os.getenv("MAYA_LIGHTAPI_VERSION")) >= 3:
            panel = mayaUtils.activeModelPanel()
            cmds.modelEditor(panel, edit=True, displayLights="flat")
            self.assertSnapshotClose('%s_render.png' % ('MayaSurfaces_flat' + suffix))
            cmds.modelEditor(panel, edit=True, displayLights="default")

        cmds.modelEditor(panel, edit=True, displayTextures=False)
        self.assertSnapshotClose('MayaSurfaces_untextured_render.png')

    def testResetToDefaultValues(self):
        """When deleting an authored attribute, make sure the shader reverts to the default unauthored value."""
        cmds.file(force=True, new=True)

        light = cmds.directionalLight(rgb=(1, 1, 1))
        transform = cmds.listRelatives(light, parent=True)[0]
        cmds.xform(transform, ro=(-30, -6, -75), ws=True)
        cmds.setAttr(light+".intensity", 10)
        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, lights=False, displayLights="all", displayTextures=True)

        self._StartTest('delete_attr_test')

        toDelete = [
            ("/mtl/standard_surface1/standard_surface1", "inputs:base"),
            ("/mtl/standard_surface1/standard_surface1", "inputs:specular_color"),
            ("/mtl/standard_surface1/ifequal1", "inputs:value1"),
            ("/mtl/standard_surface1/place2d1", "inputs:scale"),
            ("/mtl/standard_surface1/image1", "inputs:file"),
        ]
        for primPath, attrName in toDelete:
            prim = self._GetPrim('|stage|stageShape', primPath)
            prim.RemoveProperty(attrName)
            self.assertSnapshotClose('delete_attr_test_%s.png' % attrName.split(":")[1])

    @unittest.skipIf(os.getenv('MATERIALX_VERSION', '1.38.0') < '1.38.4', 'Test has a glTf PBR surface only found in MaterialX 1.38.4 and later.')
    def testTransparency(self):
        mayaUtils.loadPlugin("mayaUsdPlugin")
        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, displayTextures=True)

        # Too much differences between Linux and Windows otherwise
        cmds.setAttr("hardwareRenderingGlobals.multiSampleEnable", True)

        testFile = testUtils.getTestScene("MaterialX", "transparencyScene.ma")
        cmds.file(testFile, force=True, open=True)
        cmds.move(0, 7, -1.5, 'persp')
        cmds.rotate(-90, 0, 0, 'persp')
        self.assertSnapshotClose('transparencyScene.png', 960, 960)

    def testDemoQuads(self):
        cmds.file(force=True, new=True)

        cmds.move(0, 8, 0, 'persp')
        cmds.rotate(-90, 0, 0, 'persp')

        # Add a light to see bump mapping:
        light = cmds.directionalLight(rgb=(1, 1, 1))
        transform = cmds.listRelatives(light, parent=True)[0]
        cmds.xform(transform, ro=(105, 25, 130), ws=True)
        cmds.setAttr(light+".intensity", 2)
        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, lights=True, displayLights="all", displayTextures=True)

        self._StartTest('DemoQuads')

    def testWithEnabledMaterialX(self):
        """Make sure the absence of MAYAUSD_VP2_USE_ONLY_PREVIEWSURFACE env var has an effect."""
        cmds.file(force=True, new=True)

        light = cmds.directionalLight(rgb=(1, 1, 1))
        transform = cmds.listRelatives(light, parent=True)[0]
        cmds.xform(transform, ro=(-30, -6, -75), ws=True)
        cmds.setAttr(light+".intensity", 10)
        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, lights=False, displayLights="all", displayTextures=True)

        # Pyramid is red under preview surface, green as MaterialX, and
        # blue as display colors. We want red here.
        self._StartTest('ShadedPyramid')

    def testUDIMs(self):
        cmds.file(force=True, new=True)

        cmds.move(0, 6, 0, 'persp')
        cmds.rotate(-90, 0, 0, 'persp')

        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, displayTextures=True)

        self._StartTest('grid_with_udims')

    def testMayaPlace2dTexture(self):
        mayaUtils.loadPlugin("mayaUsdPlugin")
        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, displayTextures=True)

        # Too much differences between Linux and Windows otherwise
        cmds.setAttr("hardwareRenderingGlobals.multiSampleEnable", True)

        testFile = testUtils.getTestScene("MaterialX", "place2dTextureShowcase.ma")
        cmds.file(testFile, force=True, open=True)
        cmds.move(0, 7, -1.5, 'persp')
        cmds.rotate(-90, 0, 0, 'persp')
        self.assertSnapshotClose('place2dTextureShowcase_Maya_render.png', 960, 960)
        usdFilePath = os.path.join(self._testDir, "place2dTextureShowcase.usda")
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo=['MaterialX'],
            materialsScopeName='Materials')
        xform, shape = mayaUtils.createProxyFromFile(usdFilePath)
        cmds.move(0, 0, 1, xform)
        self.assertSnapshotClose('place2dTextureShowcase_USD_render.png', 960, 960)

        cmds.delete(xform)
        xform = cmds.mayaUSDImport(file=usdFilePath, shadingMode=[["useRegistry","MaterialX"]])
        cmds.move(0, 0, 1, xform)
        self.assertSnapshotClose('place2dTextureShowcase_Import_render.png', 960, 960)

        cmds.setAttr("hardwareRenderingGlobals.multiSampleEnable", True)

    def testMayaNodesExport(self):
        """Test a scene that will contain test samples for MaterialX exported nodes"""
        mayaUtils.loadPlugin("mayaUsdPlugin")
        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, displayTextures=True)

        # Too much differences between Linux and Windows otherwise
        cmds.setAttr("hardwareRenderingGlobals.multiSampleEnable", True)

        testFile = testUtils.getTestScene("MaterialX", "mtlxNodesShowcase.ma")
        cmds.file(testFile, force=True, open=True)
        cmds.move(0, 7, -1.5, 'persp')
        cmds.rotate(-90, 0, 0, 'persp')
        self.assertSnapshotClose('mtlxNodesShowcase_Maya_render.png', 960, 960)
        usdFilePath = os.path.join(self._testDir, "mtlxNodesShowcase.usda")
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo=['MaterialX'],
            materialsScopeName='Materials')
        xform, shape = mayaUtils.createProxyFromFile(usdFilePath)
        cmds.move(0, 0, 1, xform)
        self.assertSnapshotClose('mtlxNodesShowcase_USD_render.png', 960, 960)

        cmds.delete(xform)
        xform = cmds.mayaUSDImport(file=usdFilePath, shadingMode=[["useRegistry","MaterialX"]])
        cmds.move(0, 0, 1, xform)
        self.assertSnapshotClose('mtlxNodesShowcase_Import_render.png', 960, 960)

        cmds.setAttr("hardwareRenderingGlobals.multiSampleEnable", True)

    @unittest.skipUnless(os.getenv('MAYA_HAS_COLOR_MANAGEMENT_SUPPORT_API', 'FALSE') == 'TRUE', 'Test requires OCIO API in Maya SDK.')
    def testOCIOIntegration(self):
        """Test that we can color manage using Maya OCIO fragments."""
        cmds.file(new=True, force=True)
        # This config has file rules for all the new textures:
        if (Usd.GetVersion() >= (0, 23, 11)):
            # USD starting at 23.11 we no longer requires file rules to get OCIO results. Metadata will be usable.
            configFile = testUtils.getTestScene("MaterialX", "no-rule-studio-config-v1.0.0_aces-v1.3_ocio-v2.0.ocio")
        else:
            configFile = testUtils.getTestScene("MaterialX", "studio-config-v1.0.0_aces-v1.3_ocio-v2.0.ocio")
        cmds.colorManagementPrefs(edit=True, configFilePath=configFile)

        # Import the Maya data so we can compare:
        mayaFile = testUtils.getTestScene("MaterialX", "color_management.ma")
        cmds.file(mayaFile, i=True, type="mayaAscii")

        # And a few USD stages that have USD and MaterialX data in need of color management:
        usdCases = (("USD", 0.51), ("MTLX", -0.51), ("MTLX_Raw", -1.02))
        for suffix, offset in usdCases:
            testFile = testUtils.getTestScene("MaterialX", "color_management_{}.usda".format(suffix))
            proxyNode = mayaUtils.createProxyFromFile(testFile)[0]

            proxyXform = "|".join(proxyNode.split("|")[:-1])
            cmds.setAttr(proxyXform + ".translateZ", offset)
            cmds.setAttr(proxyXform + ".scaleX", 0.5)

        # Position our camera:
        cmds.setAttr("persp.translateX", 0)
        cmds.setAttr("persp.translateY", 4)
        cmds.setAttr("persp.translateZ", -0.25)
        cmds.setAttr("persp.rotateX", -90)
        cmds.setAttr("persp.rotateY", 0)
        cmds.setAttr("persp.rotateZ", 0)    

        # Turn on textured display and focus in on the subject
        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, e=1, displayTextures=1)

        # Snapshot and assert similarity
        self.assertSnapshotClose('OCIO_Integration.png')

        # Disable OCIO:
        cmds.colorManagementPrefs(edit=True, cmEnabled=False)

        # Snapshot and assert similarity
        self.assertSnapshotClose('OCIO_Integration_disabled.png')

        # Re-enable OCIO:
        cmds.colorManagementPrefs(edit=True, cmEnabled=True)

        # Snapshot and assert similarity
        self.assertSnapshotClose('OCIO_Integration_enabled.png')

        # Change working colorspace:
        cmds.colorManagementPrefs(edit=True, renderingSpaceName="Linear P3-D65")

        # Snapshot and assert similarity
        self.assertSnapshotClose('OCIO_Integration_p3_d65.png')

    @unittest.skipUnless(os.getenv('MAYA_HAS_COLOR_MANAGEMENT_SUPPORT_API', 'FALSE') == 'TRUE', 'Test requires OCIO API in Maya SDK.')
    def testOCIOIntegrationSourceColorSpaces(self):
        """Test that the code properly parses explicit color spaces in the fileTexture 
           and UsdUVTexture nodes. Still using Maya OCIO fragments."""
        cmds.file(new=True, force=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")

        def connectUVNode(uv_node, file_node):
            for att_name in (".coverage", ".translateFrame", ".rotateFrame",
                            ".mirrorU", ".mirrorV", ".stagger", ".wrapU",
                            ".wrapV", ".repeatUV", ".offset", ".rotateUV",
                            ".noiseUV", ".vertexUvOne", ".vertexUvTwo",
                            ".vertexUvThree", ".vertexCameraOne"):
                cmds.connectAttr(uv_node + att_name, file_node + att_name, f=True)
            cmds.connectAttr(uv_node + ".outUV", file_node + ".uvCoord", f=True)
            cmds.connectAttr(uv_node + ".outUvFilterSize", file_node + ".uvFilterSize", f=True)

        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, displayTextures=True)

        # Too much differences between Linux and Windows otherwise
        cmds.setAttr("hardwareRenderingGlobals.multiSampleEnable", True)

        # Position our camera:
        cmds.setAttr("persp.translateX", -0.5)
        cmds.setAttr("persp.translateY", 6)
        cmds.setAttr("persp.translateZ", 0)
        cmds.setAttr("persp.rotateX", -90)
        cmds.setAttr("persp.rotateY", 0)
        cmds.setAttr("persp.rotateZ", 0)    

        # Build the scene to test OCIO export options.
        testDataFiles = (
            ("_ACEScg", "ACEScg"),
            ("_ADX10", "Log film scan (ADX10)"),
            ("_lin_p3d65", "scene-linear DCI-P3 D65"),
            ("_srgb_texture", "sRGB")
        )

        posX = -2.02
        for fileSuffix, colorSpace in testDataFiles:
            plane_xform = cmds.polyPlane(ch=False, cuv=1, sx=1, sy=1)[0]
            cmds.setAttr(plane_xform + ".translateX", posX)
            posX += 1.01

            material_node = cmds.shadingNode("usdPreviewSurface", asShader=True,
                                            name="usdPreviewSurface{}".format(fileSuffix))

            material_sg = cmds.sets(renderable=True, noSurfaceShader=True,
                                    empty=True, name=material_node+"SG")
            cmds.connectAttr(material_node+".outColor",
                            material_sg+".surfaceShader", force=True)
            cmds.sets(plane_xform, e=True, forceElement=material_sg)

            cmds.setAttr(material_node + ".roughness", 1)
            cmds.setAttr(material_node + ".diffuseColor", 0, 0, 0, type="double3")

            file_node = cmds.shadingNode("file", asTexture=True,
                                        isColorManaged=True,
                                        name="file{}".format(fileSuffix))
            uv_node = cmds.shadingNode("place2dTexture", asUtility=True,
                                    name="place2dTexture{}".format(fileSuffix))

            connectUVNode(uv_node, file_node)

            cmds.connectAttr(file_node + ".outColor",
                            material_node + ".emissiveColor", f=True)

            txfile = testUtils.getTestScene("MaterialX", "textures", "color_palette{}.exr".format(fileSuffix))
            cmds.setAttr(file_node+".fileTextureName", txfile, type="string")
            cmds.setAttr(file_node+".colorSpace", colorSpace, type="string")
            cmds.setAttr(file_node+".ignoreColorSpaceFileRules", 1)
            cmds.setAttr(file_node+".filterType", 0)
            cmds.setAttr(file_node + ".defaultColor", 0.5, 0.25, 0.125, type="double3")

        usdFilePath = os.path.join(self._testDir, "explicit_ocio_usd.usda")
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo=['UsdPreviewSurface'],
            materialsScopeName='mtl')

        mtlxFilePath = os.path.join(self._testDir, "explicit_ocio_mtlx.usda")
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=mtlxFilePath,
            shadingMode='useRegistry', convertMaterialsTo=['MaterialX'],
            materialsScopeName='mtl')

        xform = mayaUtils.createProxyFromFile(usdFilePath)[0]
        cmds.move(0, 0, 1.01, xform)
        xform = mayaUtils.createProxyFromFile(mtlxFilePath)[0]
        cmds.move(0, 0, -1.01, xform)

        self.assertSnapshotClose('OCIO_Explicit.png')
    
    def testDoubleSided(self):
        """
        Test that backfaces get culled if the doubleSided attribute
        is enabled on USD prims, and that they don't if it is not.
        """
        cmds.file(new=True, force=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")

        testFile = testUtils.getTestScene("doubleSided", "MaterialX_StandardSurface.usda")
        stageShapeNode, stage = mayaUtils.createProxyFromFile(testFile)

        mayaUtils.setBasicCamera(20)
        self.assertSnapshotClose('doubleSided_enabled_front.png')
        mayaUtils.setBasicCamera(3)
        self.assertSnapshotClose('doubleSided_enabled_back.png')

        cubePrim = stage.GetPrimAtPath("/Cube1")
        cubePrim.GetAttribute('doubleSided').Set(False)

        mayaUtils.setBasicCamera(20)
        self.assertSnapshotClose('doubleSided_disabled_front.png')
        mayaUtils.setBasicCamera(3)
        self.assertSnapshotClose('doubleSided_disabled_back.png')

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
