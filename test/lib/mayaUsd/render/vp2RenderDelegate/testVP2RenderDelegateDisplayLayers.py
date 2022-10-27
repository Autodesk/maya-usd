#!/usr/bin/env mayapy
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
import imageUtils
import mayaUtils
import usdUtils
import testUtils

from mayaUsd import lib as mayaUsdLib
from mayaUsd import ufe as mayaUsdUfe

from pxr import Gf

from maya import cmds
from maya import mel
from maya.api import OpenMaya

import ufe
import sys
import os


class testVP2RenderDelegateDisplayLayers(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate when using usd cameras
    """

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegateDisplayLayersTest', 'baseline')

        cls._testDir = os.path.abspath('.')

    def assertSnapshotClose(self, imageName, camera=None):
        baselineImage = os.path.join(self._baselineDir, imageName)
        snapshotImage = os.path.join(self._testDir, imageName)
        imageUtils.snapshot(snapshotImage, width=960, height=540, camera=camera)
        return self.assertImagesClose(baselineImage, snapshotImage)

    def _StartTest(self, testName):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")
        self._testName = testName
        testFile = testUtils.getTestScene("consolidation", "colorConsolidation.usda")
        self._proxyDagPath, sphereStage = mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()

    def _GetMayaNode(self, nodeName):
        selectionList = OpenMaya.MSelectionList()
        selectionList.add(nodeName)
        return selectionList.getDependNode(0)

    def testDisplayLayersTexturing(self):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")

        cmds.xform("persp", t=(2, 2, 5.8))
        cmds.xform("persp", ro=[0, 0, 0], ws=True)

        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, displayTextures=True)

        testFile = testUtils.getTestScene("multipleMaterialsAssignment",
                                          "MultipleMaterialsAssignment.usda")
        proxyDagPath, usdStage = mayaUtils.createProxyFromFile(testFile)
        cmds.select(cl=True)

        # add the stage to a new display layer
        cmds.createDisplayLayer(name="layer1", noRecurse=True)
        displayLayer1 = OpenMaya.MFnDisplayLayer(self._GetMayaNode("layer1"))
        displayLayer1.add(proxyDagPath + ",/root")

        # verify untextured
        cmds.setAttr('layer1.drawInfo.texturing', False)
        self.assertSnapshotClose("displayLayers_untextured.png")

        # verify textured
        cmds.setAttr('layer1.drawInfo.texturing', True)
        self.assertSnapshotClose("displayLayers_textured.png")

    def testDisplayLayers(self):
        self._StartTest('displayLayers')
        cmds.modelEditor('modelPanel4', edit=True, grid=False)

        cmds.move(1, 21, -14, 'persp')
        cmds.rotate(-120, 0, 18, 'persp')
        self.assertSnapshotClose('%s_unselected.png' % self._testName)

        sphere1 = self._proxyDagPath + ",/pSphere1"
        sphere2 = self._proxyDagPath + ",/pSphere2"
        sphere3 = self._proxyDagPath + ",/pSphere3"
        sphere4 = self._proxyDagPath + ",/pSphere4"
        sphere5 = self._proxyDagPath + ",/pSphere5"

        cmds.select(sphere1, add=True)
        cmds.select(sphere2, add=True)
        cmds.createDisplayLayer(name="layer1", noRecurse=True)
        displayLayer1 = OpenMaya.MFnDisplayLayer(self._GetMayaNode("layer1"))

        self.assertTrue(displayLayer1.contains(sphere1))
        self.assertTrue(displayLayer1.contains(sphere2))
        self.assertFalse(displayLayer1.contains(sphere3))
        self.assertFalse(displayLayer1.contains(sphere4))
        self.assertFalse(displayLayer1.contains(sphere5))

        displayLayer1.add(sphere3)
        displayLayer1.add(sphere4)
        displayLayer1.remove(sphere1)

        self.assertFalse(displayLayer1.contains(sphere1))
        self.assertTrue(displayLayer1.contains(sphere2))
        self.assertTrue(displayLayer1.contains(sphere3))
        self.assertTrue(displayLayer1.contains(sphere4))
        self.assertFalse(displayLayer1.contains(sphere5))

        # invisible
        cmds.setAttr('layer1.drawInfo.visibility', False)
        self.assertSnapshotClose('%s_sphere234_hidden.png' % self._testName)

        # visible
        cmds.setAttr('layer1.drawInfo.visibility', True)
        self.assertSnapshotClose('%s_sphere234_visible.png' % self._testName)

        # hide on playback
        cmds.setAttr('layer1.drawInfo.hideOnPlayback', True)
        self.assertSnapshotClose('%s_sphere234_hideonplayback.png' % self._testName)
        cmds.setAttr('layer1.drawInfo.hideOnPlayback', False)

        # templated
        cmds.setAttr('layer1.drawInfo.displayType', 1)
        self.assertSnapshotClose('%s_sphere234_templated.png' % self._testName)
        cmds.setAttr('layer1.drawInfo.displayType', 0)

        # bbox
        cmds.setAttr('layer1.drawInfo.levelOfDetail', 1)
        self.assertSnapshotClose('%s_sphere234_bbox.png' % self._testName)
        cmds.setAttr('layer1.drawInfo.levelOfDetail', 0)

        # wireframe
        cmds.setAttr('layer1.drawInfo.shading', False)
        self.assertSnapshotClose('%s_sphere234_wireframe.png' % self._testName)

        # reference
        cmds.setAttr('layer1.drawInfo.displayType', 2)
        self.assertSnapshotClose('%s_sphere234_reference.png' % self._testName)
        cmds.setAttr('layer1.drawInfo.displayType', 0)

        # color index
        cmds.setAttr('layer1.drawInfo.color', 10)
        self.assertSnapshotClose('%s_sphere234_colored.png' % self._testName)
        cmds.setAttr('layer1.drawInfo.color', 0)

        # rgb color
        cmds.setAttr('layer1.drawInfo.overrideRGBColors', True)
        cmds.setAttr('layer1.drawInfo.overrideColorRGB', 1, 0, 0)
        cmds.setAttr('layer1.drawInfo.overrideColorA', 0.5)
        self.assertSnapshotClose('%s_sphere234_rgbcolor.png' % self._testName)
        cmds.setAttr('layer1.drawInfo.overrideRGBColors', False)
        cmds.setAttr('layer1.drawInfo.shading', True)
        cmds.setAttr('layer1.drawInfo.visibility', False)

        cmds.select(clear=True)
        cmds.createDisplayLayer(name="layer2", noRecurse=True)
        displayLayer2 = OpenMaya.MFnDisplayLayer(self._GetMayaNode("layer2"))
        cmds.select(clear=True)

        displayLayer2.add(sphere3)
        self.assertSnapshotClose('%s_sphere24_hidden.png' % self._testName)

        cmds.select(sphere1, add=True)
        cmds.select(sphere2, add=True)
        group1 = self._proxyDagPath + ",/" + cmds.group(name="group", world=True)
        #sphere1 and sphere2 no longer exist

        groupedSphere1 = group1 + "/pSphere1"
        groupedSphere2 = group1 + "/pSphere2"
        self.assertFalse(displayLayer1.contains(sphere2))
        self.assertFalse(displayLayer2.contains(sphere2))
        self.assertTrue(displayLayer1.contains(groupedSphere2))
        self.assertFalse(displayLayer1.contains(groupedSphere1))
        displayLayer1.remove(sphere4)
        self.assertSnapshotClose('%s_groupedSphere2_hidden_.png' % self._testName)

        #layer2 has sphere3 in it
        #layer1 has groupedSphere2 in it
        self.assertTrue(displayLayer1.containsAncestorInclusive(groupedSphere2))
        self.assertTrue(displayLayer2.containsAncestorInclusive(sphere3))
        self.assertFalse(displayLayer1.containsAncestorInclusive(group1))
        displayLayer1.add(group1)
        self.assertTrue(displayLayer1.containsAncestorInclusive(groupedSphere1))
        self.assertTrue(displayLayer1.containsAncestorInclusive(groupedSphere2))
        self.assertTrue(displayLayer1.containsAncestorInclusive(group1))
        self.assertSnapshotClose('%s_group1_hidden_.png' % self._testName)

        displayLayer2.add(self._proxyDagPath)
        self.assertFalse(displayLayer2.contains(sphere1))
        self.assertFalse(displayLayer2.contains(sphere2))
        self.assertTrue(displayLayer2.contains(sphere3))
        self.assertFalse(displayLayer2.contains(sphere4))
        self.assertFalse(displayLayer2.contains(sphere5))
        self.assertTrue(displayLayer2.containsAncestorInclusive(sphere1)) # returned false
        self.assertTrue(displayLayer2.containsAncestorInclusive(sphere2)) # returned false
        self.assertTrue(displayLayer2.containsAncestorInclusive(sphere3))
        self.assertTrue(displayLayer2.containsAncestorInclusive(sphere4)) # returned false
        self.assertTrue(displayLayer2.containsAncestorInclusive(sphere5)) # returned false
        
        cmds.setAttr('layer1.drawInfo.visibility', True)
        cmds.setAttr('layer2.drawInfo.visibility', False)
        self.assertSnapshotClose('%s_proxyShape_hidden_.png' % self._testName)
        
        layer1Members =  displayLayer1.getMembers()
        self.assertEqual(layer1Members.length(), 2)
        layer1MemberStrings = layer1Members.getSelectionStrings()
        self.assertFalse(layer1MemberStrings[0] == sphere2 or layer1MemberStrings[1] == sphere2)
        self.assertTrue(layer1MemberStrings[0] == group1 or layer1MemberStrings[1] == group1)
        self.assertTrue(layer1MemberStrings[0] == groupedSphere2 or layer1MemberStrings[1] == groupedSphere2)

    def testPrimInTemplatedDisplayLayer(self):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")
        self._testName = 'templatedDisplayLayer'
        x = cmds.polyPlane(width=3.048, height=3.048, sx=4, sy=4, ax=(0, 0, 1))
        cmds.setAttr('lambert1.color', 0.55, 0.55, 0.55)
        cmds.select(x)
        usdFile = os.path.join(self._testDir, 'plane.usd')
        cmds.mayaUSDExport(file=usdFile, selection=True, shadingMode='none',
            exportDisplayColor=True)
        proxyShape = cmds.createNode('mayaUsdProxyShape', name='usdProxyShape')
        proxyTransform = cmds.listRelatives(proxyShape, parent=True,
            fullPath=True)[0]
        cmds.xform(proxyTransform, translation=(3.048, 0, 0))
        cmds.setAttr('%s.filePath' % proxyShape, usdFile, type='string')
        cmds.setAttr('%s.primPath' % proxyShape, '/pPlane1', type='string')
        
        # Create a display layer with the selected objects.
        cmds.select([x[0], "|usdProxy|usdProxyShape,/pPlane1"], replace=True)
        cmds.createDisplayLayer(name="layer1", noRecurse=True)

        # Set the display layer mode to templated.
        cmds.setAttr("layer1.displayType", 1)

        # Change the active template color and take a snapshot 
        # to see if it affected the USD object.
        cmds.displayRGBColor("templateActive", 0.9, 0.3, 0.7)

        self.assertSnapshotClose('%s_active.png' % self._testName, camera='front')


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
