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
        self._StartTest('MayaSurfaces')

        # Flat shading requires V3 lighting API:
        if int(os.getenv("MAYA_LIGHTAPI_VERSION")) >= 3:
            panel = mayaUtils.activeModelPanel()
            cmds.modelEditor(panel, edit=True, displayLights="flat")
            self._StartTest('MayaSurfaces_flat')
            cmds.modelEditor(panel, edit=True, displayLights="default")

        self._StartTest('MayaSurfaces_untextured', False)

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


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
