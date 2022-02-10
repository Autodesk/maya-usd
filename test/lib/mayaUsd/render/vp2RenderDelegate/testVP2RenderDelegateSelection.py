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
import unittest
import usdUtils
import testUtils

from mayaUsd import lib as mayaUsdLib
from mayaUsd import ufe as mayaUsdUfe

from maya import cmds
import maya.api.OpenMayaRender as omr

from pxr import Usd

import ufe

import os


class testVP2RenderDelegateSelection(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate when using per-instance
    inherited data on instances.
    """

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegateSelectionTest', 'baseline')

        cls._testDir = os.path.abspath('.')

    @classmethod
    def tearDownClass(cls):
        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, useDefaultMaterial=False)

    def assertSnapshotClose(self, imageName):
        cmds.undoInfo(stateWithoutFlush=False) # disable the undo queue during the snapshot
        baselineImage = os.path.join(self._baselineDir, imageName)
        snapshotImage = os.path.join(self._testDir, imageName)
        imageUtils.snapshot(snapshotImage, width=960, height=540)
        retVal = self.assertImagesClose(baselineImage, snapshotImage)
        cmds.undoInfo(stateWithoutFlush=True)
        return retVal
    
    def _stringToUfeItem(self, pathString):
        ufePath = ufe.PathString.path(pathString)
        ufeItem = ufe.Hierarchy.createItem(ufePath)
        return ufeItem

    def _createPrim(self, underItem, primType, expectedPath):
        '''
        Helper that creates a prim of the given type under the given item, and return its item.
        '''
        # create a proxy shape and add a Cone prim

        proxyShapeContextOps = ufe.ContextOps.contextOps(underItem)
        proxyShapeContextOps.doOp(['Add New Prim', primType])

        primPath = ufe.Path([
            ufe.PathString.path("|stage|stageShape").segments[0],
            ufe.PathSegment(expectedPath, mayaUsdUfe.getUsdRunTimeId(), "/")
            ])
        return ufe.PathString.string(primPath)

    def _selectionTest(self, scene, objectA, objectB, proxyDagPath, displayMode):

        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        self.assertSnapshotClose('%sunselected_%s.png' % (scene, displayMode))

        # Select cube
        cmds.select(objectA)
        self.assertSnapshotClose('%sobjectA_%s.png' % (scene, displayMode))

        # Select cylinder
        cmds.select(objectB, add=True)
        self.assertSnapshotClose('%sobjectA_and_objectB_%s.png' % (scene, displayMode))

        # Undo, Redo
        cmds.undo() # Undo remove capsule from isolate select
        self.assertSnapshotClose('%sundoObjectB_%s.png' % (scene, displayMode))
        cmds.redo() # Redo remove capsule from isolate select
        self.assertSnapshotClose('%sredoObjectB_%s.png' % (scene, displayMode))

        # Proxy
        cmds.select(proxyDagPath)
        self.assertSnapshotClose('%sgatewayNode_%s.png' % (scene, displayMode))

        cmds.select(clear=True)
        self.assertSnapshotClose('%sclear_%s.png' % (scene, displayMode))

    def testSelection(self):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")
        panel = mayaUtils.activeModelPanel()
        usdaFile = testUtils.getTestScene("setsCmd", "5prims.usda")
        proxyDagPath, sphereStage = mayaUtils.createProxyFromFile(usdaFile)

        cmds.move(-4, -24, 0, "persp")
        cmds.rotate(90, 0, 0, "persp")

        usdCube = proxyDagPath + ",/Cube1"
        usdCylinder = proxyDagPath + ",/Cylinder1"

        # Test smooth shaded
        cmds.modelEditor('modelPanel4', e=True, displayAppearance='smoothShaded', displayLights='default')
        cmds.modelEditor('modelPanel4', e=True, wireframeOnShaded=False, displayLights='default')
        self._selectionTest('', usdCube, usdCylinder, proxyDagPath, 'smoothShaded')

        # Test smooth shaded
        cmds.modelEditor('modelPanel4', e=True, displayAppearance='smoothShaded', displayLights='default')
        cmds.modelEditor('modelPanel4', e=True, wireframeOnShaded=True, displayLights='default')
        self._selectionTest('', usdCube, usdCylinder, proxyDagPath, 'wireframeOnShaded')

        # Test smooth shaded
        cmds.modelEditor('modelPanel4', e=True, displayAppearance='wireframe', displayLights='default')
        cmds.modelEditor('modelPanel4', e=True, wireframeOnShaded=False, displayLights='default')
        self._selectionTest('', usdCube, usdCylinder, proxyDagPath, 'wireframe')

    def testInstancedSelection(self):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")
        panel = mayaUtils.activeModelPanel()
        usdaFile = testUtils.getTestScene("instances", "perInstanceInheritedData.usda")
        proxyDagPath, sphereStage = mayaUtils.createProxyFromFile(usdaFile)
        usdball01 = proxyDagPath + ",/root/group/ball_01"
        usdball03 = proxyDagPath + ",/root/group/ball_03"

        cmds.move(8.5, -20, 0, "persp")
        cmds.rotate(90, 0, 0, "persp")

        # Test smooth shaded
        cmds.modelEditor('modelPanel4', e=True, displayAppearance='smoothShaded', displayLights='default')
        cmds.modelEditor('modelPanel4', e=True, wireframeOnShaded=False, displayLights='default')
        self._selectionTest('instance_', usdball01, usdball03, proxyDagPath, 'smoothShaded')

        # Test smooth shaded
        cmds.modelEditor('modelPanel4', e=True, displayAppearance='smoothShaded', displayLights='default')
        cmds.modelEditor('modelPanel4', e=True, wireframeOnShaded=True, displayLights='default')
        self._selectionTest('instance_', usdball01, usdball03, proxyDagPath, 'wireframeOnShaded')

        # Test smooth shaded
        cmds.modelEditor('modelPanel4', e=True, displayAppearance='wireframe', displayLights='default')
        cmds.modelEditor('modelPanel4', e=True, wireframeOnShaded=False, displayLights='default')
        self._selectionTest('instance_', usdball01, usdball03, proxyDagPath, 'wireframe')



if __name__ == '__main__':
    fixturesUtils.runTests(globals())
