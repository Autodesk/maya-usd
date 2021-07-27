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

import os
import platform
import unittest

import testUtils

from maya import cmds
import maya.mel as mel
import fixturesUtils

class MayaUsdInteractiveLayerEditorCommandsTestCase(unittest.TestCase):
    """Test interactive commands that need the UI of the layereditor."""

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__, initializeStandalone=False, loadPlugin=False)
        cls._baselineDir = os.path.join(inputPath,'VP2RenderDelegatePrimPathTest', 'baseline')
        cls._testDir = os.path.abspath('.')
        cmds.loadPlugin('mayaUsdPlugin')
        mel.eval('mayaUsdLayerEditorWindow mayaUsdLayerEditor')

    def samefile(self, path1, path2):
        if platform.system() == 'Windows':
            return os.path.normcase(os.path.normpath(path1)) == os.path.normcase(os.path.normpath(path2))
        else:
            return os.path.samefile(path1, path2)

    def testCreateStageFromFileWithInvalidUsd(self):
        # We cannot directly call the 'mayaUsdCreateStageFromFile'
        # as it opens a file dialog to choose the scene. So instead
        # we can call what it does once the file is choose.
        # Note: on ballFilePath we replace \ with / to stop the \ as
        #       being interpreted.
        ballFilePath = os.path.normpath(testUtils.getTestScene('ballset', 'StandaloneScene', 'invalid_layer.usda')).replace('\\', '/')
        mel.eval('source \"mayaUsd_createStageFromFile.mel\"')
        shapeNode = mel.eval('mayaUsd_createStageFromFilePath(\"'+ballFilePath+'\")')
        mayaSel = cmds.ls(sl=True)
        self.assertEqual(1, len(mayaSel))
        nt = cmds.nodeType(shapeNode)
        self.assertEqual('mayaUsdProxyShape', nt)

        # Verify that the shape node has the correct file path.
        filePathAttr = cmds.getAttr(shapeNode+'.filePath')
        self.assertTrue(self.samefile(filePathAttr, ballFilePath))

        # Verify that the shape node is connected to time.
        self.assertTrue(cmds.isConnected('time1.outTime', shapeNode+'.time'))

if __name__ == '__main__':
    fixturesUtils.runTests(globals())