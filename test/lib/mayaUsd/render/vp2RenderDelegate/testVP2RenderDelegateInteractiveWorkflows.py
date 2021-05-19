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

from maya.internal.ufeSupport import ufeCmdWrapper as ufeCmd
from maya import cmds
import maya.api.OpenMayaRender as omr

from pxr import Usd

import ufe

import os


class testVP2RenderDelegateInteractiveWorkflows(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate when using per-instance
    inherited data on instances.
    """

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegateInteractiveWorkflowsTest', 'baseline')

        cls._testDir = os.path.abspath('.')

    def assertSnapshotClose(self, imageName):
        baselineImage = os.path.join(self._baselineDir, imageName)
        snapshotImage = os.path.join(self._testDir, imageName)
        imageUtils.snapshot(snapshotImage, width=960, height=540)
        return self.assertImagesClose(baselineImage, snapshotImage)

    def _StartTest(self, testName):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")
        self._testName = testName
        testFile = testUtils.getTestScene("consolidation", self._testName + ".usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        self.assertSnapshotClose('%s_unselected.png' % self._testName)

    def testUndoRedo(self):
        self._StartTest('capsule')

        mayaUtils.loadPlugin("ufeSupport")

        cmds.move(3, -3, 3, 'persp')
        cmds.rotate(60, 0, 45, 'persp')

        # modify the capsule's height, then undo and redo that operation and
        # make sure the viewport updates as expected.
        mayaPathSegment = mayaUtils.createUfePathSegment('|stage|stageShape')
        capsuleUsdPathSegment = usdUtils.createUfePathSegment('/Capsule1')
        capsulePath = ufe.Path([mayaPathSegment, capsuleUsdPathSegment])
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)
        capsuleAttrs = ufe.Attributes.attributes(capsuleItem)
        heightAttr = capsuleAttrs.attribute('height')

        # get the undo queue into a clean state with nothing on the queue
        # and no open chunks

        # disable and flush the undo queue
        cmds.undoInfo(state=False)

        # the undo queue could still have some open chunks which were in the
        # process of being created when I turned the undo queue off. For example,
        # this test gets run from the mel "python" command (see test.cmake), and
        # that chunk is currently open.
        # If I try to query the current chunk string to see if something IS open,
        # it is always the command I used to try to query the current chunk name!
        # Experimentally, I found have that there are typically two open chunks.
        # So just close two chunks.
        cmds.undoInfo(closeChunk=True)
        cmds.undoInfo(closeChunk=True)
        # flush those truncated chunks if they are on the undo queue. They shouldn't
        # be, because I already disabled the undo queue, but I am paranoid.
        cmds.flushUndo()

        # Now run the actual test I want to run. Enable the undo queue for each command
        # that I want on the queue, and disable the undo queue again, without flushing,
        # immediately after.

        cmds.undoInfo(stateWithoutFlush=True)
        ufeCmd.execute(heightAttr.setCmd(3))
        cmds.undoInfo(stateWithoutFlush=False)
        
        self.assertSnapshotClose('%s_set_height.png' % self._testName)
        
        cmds.undoInfo(stateWithoutFlush=True)
        cmds.undo()
        cmds.undoInfo(stateWithoutFlush=False)
        
        self.assertSnapshotClose('%s_undo_set_height.png' % self._testName)
        
        cmds.undoInfo(stateWithoutFlush=True)
        cmds.redo()
        cmds.undoInfo(stateWithoutFlush=False)
        
        self.assertSnapshotClose('%s_redo_set_height.png' % self._testName)
        
        # Now the test is over, turn the undo queue back on incase this Maya session
        # gets re-used for more tests.
        cmds.undoInfo(stateWithoutFlush=True)

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
