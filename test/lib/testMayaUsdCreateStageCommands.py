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
from maya import cmds
import maya.mel as mel
import mayaUtils

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
        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Verify that we got a proxy shape object.
        mayaSel = cmds.ls(sl=True)
        self.assertEqual(1, len(mayaSel))
        shapeNode = mayaSel[0]
        nt = cmds.nodeType(shapeNode)
        self.assertEqual('mayaUsdProxyShape', nt)

        # Verify that the shape node is connected to time.
        self.assertTrue(cmds.isConnected('time1.outTime', shapeNode+'.time'))

    def testCreateStageFromFile(self):
        # We cannot directly call the 'mayaUsdCreateStageFromFile'
        # as it opens a file dialog to choose the scene. So instead
        # we can call what it does once the file is choose.
        # Note: on ballFilePath we replace \ with / to stop the \ as
        #       being interpreted.
        ballFilePath = os.path.normpath(mayaUtils.getTestScene('ballset', 'StandaloneScene', 'top_layer.usda')).replace('\\', '/')
        mel.eval('source \"mayaUsd_createStageFromFile.mel\"')
        mel.eval('mayaUsd_createStageFromFilePath(\"'+ballFilePath+'\")')
        mayaSel = cmds.ls(sl=True)
        self.assertEqual(1, len(mayaSel))
        shapeNode = mayaSel[0]
        nt = cmds.nodeType(shapeNode)
        self.assertEqual('mayaUsdProxyShape', nt)

        # Verify that the shape node has the correct file path.
        filePathAttr = cmds.getAttr(shapeNode+'.filePath')
        self.assertTrue(self.samefile(filePathAttr, ballFilePath))

        # Verify that the shape node is connected to time.
        self.assertTrue(cmds.isConnected('time1.outTime', shapeNode+'.time'))
