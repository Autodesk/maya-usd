#!/usr/bin/env python

#
# Copyright 2020 Autodesk
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
import platform
import unittest

import testUtils
from ufeUtils import ufeFeatureSetVersion

from maya import cmds
import maya.mel as mel

import mayaUsd.lib
import mayaUsd.ufe

import ufe

class MayaUsdCreateStageCommandsTestCase(unittest.TestCase):
    """Test the MEL commands that are used to create a USD stage."""

    @classmethod
    def setUpClass(cls):
        cmds.loadPlugin('mayaUsdPlugin')

    def samefile(self, path1, path2):
        if platform.system() == 'Windows':
            return os.path.normcase(os.path.normpath(path1)) == os.path.normcase(os.path.normpath(path2))
        else:
            return os.path.samefile(path1, path2)

    def testCreateStageWithNewLayer(self):
        # Create a proxy shape with empty stage to start with.
        import mayaUsd_createStageWithNewLayer
        shapeNode = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Verify that we got a proxy shape object.
        mayaSel = cmds.ls(sl=True)
        self.assertEqual(1, len(mayaSel))
        nt = cmds.nodeType(shapeNode)
        self.assertEqual('mayaUsdProxyShape', nt)

        # Verify that the shape node is connected to time.
        self.assertTrue(cmds.isConnected('time1.outTime', shapeNode+'.time'))

    def testCreateStageFromFile(self):
        # Open top_layer.ma and make sure to have USD stage paths as absolute
        mayaSceneFilePath = testUtils.getTestScene("ballset", "StandaloneScene", "top_layer.ma")
        cmds.file(mayaSceneFilePath, force=True, open=True)
        cmds.optionVar(iv=('mayaUsd_MakePathRelativeToSceneFile', 0))

        # We cannot directly call the 'mayaUsdCreateStageFromFile'
        # as it opens a file dialog to choose the scene. So instead
        # we can call what it does once the file is choose.
        # Note: on ballFilePath we replace \ with / to stop the \ as
        #       being interpreted.
        ballFilePath = os.path.normpath(testUtils.getTestScene('ballset', 'StandaloneScene', 'top_layer.usda')).replace('\\', '/')
        mel.eval('source \"mayaUsd_createStageFromFile.mel\"')
        shapeNode = mel.eval('mayaUsd_createStageFromFilePath(\"'+ballFilePath+'\")')
        mayaSel = cmds.ls(sl=True)
        self.assertEqual(1, len(mayaSel))
        nt = cmds.nodeType(shapeNode)
        self.assertEqual('mayaUsdProxyShape', nt)

        # Verify that the we have a good stage.
        stage = mayaUsd.ufe.getStage(shapeNode)
        self.assertTrue(stage)

        # This stage has five layers.
        layerStack = stage.GetLayerStack()
        self.assertEqual(5, len(layerStack))

        # Finally, verify that we can get at least one of the ball prims.
        ballPrim = stage.GetPrimAtPath('/Room_set/Props/Ball_1')
        self.assertTrue(ballPrim.IsValid())

        # Verify that the shape node has the correct file path.
        filePathAttr = cmds.getAttr(shapeNode+'.filePath')
        self.assertTrue(self.samefile(filePathAttr, ballFilePath))

        # Verify that the shape node is connected to time.
        self.assertTrue(cmds.isConnected('time1.outTime', shapeNode+'.time'))

        # Now switch to having USD stage paths as relative to Maya scene file
        cmds.optionVar(iv=('mayaUsd_MakePathRelativeToSceneFile', 1))
        
        # Create the same stage and verify that now it's open as relative
        shapeNodeRel = mel.eval('mayaUsd_createStageFromFilePath(\"'+ballFilePath+'\")')
        filePathAttrRel = cmds.getAttr(shapeNodeRel+'.filePath')
        self.assertEqual(filePathAttrRel, 'top_layer.usda')
        
        # Restore mayaUsd_MakePathRelativeToSceneFile
        cmds.optionVar(iv=('mayaUsd_MakePathRelativeToSceneFile', 0))

    @unittest.skipUnless(ufeFeatureSetVersion() >= 4, 'Test only available in UFE v4 or greater.')
    def testCreateStageWithCommand(self):
        '''
        Create a stage with a new layer using the command exposed by a Python wrapper.
        '''

        stageUfePathStr = mayaUsd.ufe.createStageWithNewLayer("")
        self.assertIsNotNone(stageUfePathStr)

        stageUfePath = ufe.PathString.path(stageUfePathStr)
        stageUfeSceneItem = ufe.Hierarchy.createItem(stageUfePath)
        self.assertIsNotNone(stageUfeSceneItem)
        stageUfeSceneItem = None

        # Create a poly-sphere and copy it to the stage.
        # We had a bug where undo would crash when undoing past this, so this is the goal
        # of having this here when testing undo/redo below.
        cmds.CreatePolygonSphere()
        sphereNode = cmds.ls(sl=True,l=True)[0]
        self.assertIsNotNone(sphereNode)
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.duplicate(
                sphereNode, stageUfePathStr))

        # Note: commands execute other commands. To get to the point where
        #       the stage no longer exist, we need 6 undo.
        undoCountToGetRidOfStage = 6
        for _ in range(undoCountToGetRidOfStage):
            cmds.undo()
        stageUfeSceneItem = ufe.Hierarchy.createItem(stageUfePath)
        self.assertIsNone(stageUfeSceneItem)

        for _ in range(undoCountToGetRidOfStage):
            cmds.redo()
        stageUfeSceneItem = ufe.Hierarchy.createItem(stageUfePath)
        self.assertIsNotNone(stageUfeSceneItem)
