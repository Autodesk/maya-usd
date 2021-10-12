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


class testVP2RenderDelegateIsolateSelect(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate when using per-instance
    inherited data on instances.
    """

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegateIsolateSelectTest', 'baseline')

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

    def testIsolateSelect(self):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")
        panel = mayaUtils.activeModelPanel()
        usdaFile = testUtils.getTestScene("setsCmd", "5prims.usda")
        proxyDagPath, sphereStage = mayaUtils.createProxyFromFile(usdaFile)
        usdCube = proxyDagPath + ",/Cube1"
        usdCylinder = proxyDagPath + ",/Cylinder1"
        usdCapsule = proxyDagPath + ",/Capsule1"
        usdCone = proxyDagPath + ",/Cone1"
        usdXform = proxyDagPath + ",/Xform1"

        cmds.move(-4, -24, 0, "persp")
        cmds.rotate(90, 0, 0, "persp")

        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        self.assertSnapshotClose('unselected.png')

        # Turn on isolate select for cube
        cmds.select(usdCube)
        cmds.isolateSelect("modelPanel4", state=1)
        self.assertSnapshotClose('cube.png')

        # Replace isolate select cube with cylinder
        cmds.select(usdCylinder)
        cmds.isolateSelect("modelPanel4", loadSelected=True)
        self.assertSnapshotClose('cylinder.png')

        # Add capsule to isolate select
        cmds.select(usdCapsule)
        cmds.isolateSelect("modelPanel4", addSelected=True)
        self.assertSnapshotClose('cylinderAndCapsule.png')

        # Remove capsule from isolate select
        cmds.isolateSelect("modelPanel4", removeSelected=True)
        self.assertSnapshotClose('cylinderAfterCapsuleRemove.png')

        # Undo, Redo
        cmds.undo() # Undo remove capsule from isolate select
        self.assertSnapshotClose('undoCapsuleRemove.png')
        cmds.redo() # Redo remove capsule from isolate select
        self.assertSnapshotClose('redoCapsuleRemove.png')
        cmds.undo() # Undo remove capsule from isolate select
        cmds.undo() # Undo add capsule to isolate select
        self.assertSnapshotClose('undoCapsuleAdd.png')

        # Turn off isolate select
        cmds.isolateSelect("modelPanel4", state=0)
        self.assertSnapshotClose('isolateSelectOff.png')

        # Create an isolate select set, then add something directly to it
        cmds.isolateSelect("modelPanel4", state=1)
        isolateSelectSet = "modelPanel4ViewSelectedSet"
        cmds.sets(usdCube, add=isolateSelectSet)
        cmds.isolateSelect("modelPanel4", update=True)
        self.assertSnapshotClose('capsuleAndCube.png')

        # The flags addDagObject and removeDagObject don't
        # work with USD items.

        # Add the cone to the isolate select
        # different from addSelected because it filters out components
        cmds.select(usdCone)
        cmds.isolateSelect("modelPanel4", addSelectedObjects=True)
        self.assertSnapshotClose('capsuleAndCubeAndCone.png')

        # Translate Xform1 and reparent Cube1 under Xform1
        cmds.select(usdXform)
        cmds.move( 0, 0, 1, relative=True)
        cmds.select(clear=True)
        cmds.parent(usdCube, usdXform, relative=True)
        cmds.isolateSelect("modelPanel4", update=True)
        usdCube = usdXform + "/Cube1"
        self.assertSnapshotClose('reparentedCube.png')

        # Reparent Cube1 back
        cmds.parent(usdCube, proxyDagPath, relative=True)
        cmds.isolateSelect("modelPanel4", update=True)
        usdCube = proxyDagPath + ",/Cube1"
        self.assertSnapshotClose('reparentedCubeBack.png')

        #reparent the proxy shape
        locatorShape = cmds.createNode("locator")
        locator = "|" + cmds.listRelatives(locatorShape, parent=True)[0]
        cmds.move( 0, 0, 5, locator)
        cmds.parent("|stage", locator, relative=True)
        usdCube = locator + usdCube
        self.assertSnapshotClose('reparentedProxyShape.png')


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
