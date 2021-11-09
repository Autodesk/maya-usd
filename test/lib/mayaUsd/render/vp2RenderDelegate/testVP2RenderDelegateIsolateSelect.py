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
        cmds.isolateSelect(panel, state=1)
        self.assertSnapshotClose('cube.png')

        # Replace isolate select cube with cylinder
        cmds.select(usdCylinder)
        cmds.isolateSelect(panel, loadSelected=True)
        self.assertSnapshotClose('cylinder.png')

        # Add capsule to isolate select
        cmds.select(usdCapsule)
        cmds.isolateSelect(panel, addSelected=True)
        self.assertSnapshotClose('cylinderAndCapsule.png')

        # Remove capsule from isolate select
        cmds.isolateSelect(panel, removeSelected=True)
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
        cmds.isolateSelect(panel, state=0)
        self.assertSnapshotClose('isolateSelectOff.png')

        # Create an isolate select set, then add something directly to it
        cmds.isolateSelect(panel, state=1)
        isolateSelectSet = "modelPanel4ViewSelectedSet"
        cmds.sets(usdCube, add=isolateSelectSet)
        cmds.isolateSelect(panel, update=True)
        self.assertSnapshotClose('capsuleAndCube.png')

        # The flags addDagObject and removeDagObject don't
        # work with USD items.

        # Add the cone to the isolate select
        # different from addSelected because it filters out components
        cmds.select(usdCone)
        cmds.isolateSelect(panel, addSelectedObjects=True)
        self.assertSnapshotClose('capsuleAndCubeAndCone.png')

        # Translate Xform1 and reparent Cube1 under Xform1
        cmds.select(usdXform)
        cmds.move( 0, 0, 1, relative=True)
        cmds.select(clear=True)
        cmds.parent(usdCube, usdXform, relative=True)
        cmds.isolateSelect(panel, update=True)
        usdCube = usdXform + "/Cube1"
        self.assertSnapshotClose('reparentedCube.png')

        # Reparent Cube1 back
        cmds.parent(usdCube, proxyDagPath, relative=True)
        cmds.isolateSelect(panel, update=True)
        usdCube = proxyDagPath + ",/Cube1"
        self.assertSnapshotClose('reparentedCubeBack.png')

        #reparent the proxy shape
        locatorShape = cmds.createNode("locator")
        locator = "|" + cmds.listRelatives(locatorShape, parent=True)[0]
        cmds.move( 0, 0, 5, locator)
        cmds.parent("|stage", locator, relative=True)
        usdCube = locator + usdCube
        self.assertSnapshotClose('reparentedProxyShape.png')

        cmds.undo() #undo reparent so that _createPrim works
        usdCube = proxyDagPath + ",/Cube1"

        #Auto load new objects
        usdXformItem = self._stringToUfeItem(usdXform)
        usdXformCone = self._createPrim(usdXformItem, 'Cone', '/Xform1/Cone1')
        cmds.select(usdXformCone)
        cmds.move(-8.725, 0, 2)
        self.assertSnapshotClose('autoLoadNewObjects.png')

        #Auto load selected objects
        cmds.editor(panel, edit=True, unlockMainConnection=True)
        self.assertSnapshotClose('autoLoadSelected_xformCone.png')
        cmds.select(usdCube)
        self.assertSnapshotClose('autoLoadSelected_cube.png')
        cmds.select("|stage")
        self.assertSnapshotClose('autoLoadSelected_stage.png')
        cmds.select(usdCone)
        cmds.select(usdCapsule, add=True)
        cmds.select(usdCylinder, add=True)
        self.assertSnapshotClose('autoLoadSelected_coneCapsuleCyliner.png')
        cmds.editor(panel, edit=True, unlockMainConnection=False)

    def testInstancedIsolateSelect(self):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")
        panel = mayaUtils.activeModelPanel()
        usdaFile = testUtils.getTestScene("instances", "perInstanceInheritedData.usda")
        proxyDagPath, sphereStage = mayaUtils.createProxyFromFile(usdaFile)
        usdball01 = proxyDagPath + ",/root/group/ball_01"
        usdball02 = proxyDagPath + ",/root/group/ball_02"
        usdball03 = proxyDagPath + ",/root/group/ball_03"
        usdball04 = proxyDagPath + ",/root/group/ball_04"
        usdball05 = proxyDagPath + ",/root/group/ball_05"

        cmds.move(8.5, -20, 0, "persp")
        cmds.rotate(90, 0, 0, "persp")

        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()

        # Turn on isolate select for cube
        cmds.select(usdball01)
        cmds.isolateSelect(panel, state=1)
        self.assertSnapshotClose('ball01.png')

        # Add the hidden ball and another ball
        cmds.select(usdball02)
        cmds.select(usdball03, add=True)
        cmds.isolateSelect(panel, addSelectedObjects=True)
        self.assertSnapshotClose('ball01_ball02_ball03.png')

        # Remove the hidden ball
        cmds.select(usdball02)
        cmds.isolateSelect(panel, removeSelected=True)
        self.assertSnapshotClose('ball01_ball03.png')

        # Remove the first ball
        cmds.select(usdball01)
        cmds.isolateSelect(panel, removeSelected=True)
        self.assertSnapshotClose('ball03.png')

        #Auto load selected objects
        cmds.editor(panel, edit=True, unlockMainConnection=True)
        cmds.select(usdball02)
        self.assertSnapshotClose('autoLoadSelected_ball02.png')
        cmds.select(usdball01)
        cmds.select(usdball04, add=True)
        self.assertSnapshotClose('autoLoadSelected_ball01_ball04.png')
        cmds.select('|stage')
        self.assertSnapshotClose('autoLoadSelected_instance_stage.png')
        cmds.select(usdball05)
        cmds.select(usdball04, add=True)
        self.assertSnapshotClose('autoLoadSelected_ball04_ball05.png')
        cmds.editor(panel, edit=True, unlockMainConnection=False)



if __name__ == '__main__':
    fixturesUtils.runTests(globals())
