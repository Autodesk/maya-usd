#!/usr/bin/env mayapy
#
# Copyright 2022 Animal Logic
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
import testUtils

from maya import cmds

import os

class testVP2RenderDelegateTextureLoading(imageUtils.ImageDiffingTestCase):
    """
    Test texture loading in sync and async mode.
    """

    @classmethod
    def setUpClass(cls):
        input_path = fixturesUtils.setUpClass(
            __file__, initializeStandalone=False, loadPlugin=False
        )

        cls._baseline_dir = os.path.join(
            input_path, "VP2RenderDelegateTextureLoadingTest", "baseline"
        )

        cls._test_dir = os.path.abspath(".")

        cls._optVarName = "mayaUsd_DisableAsyncTextureLoading"
        # Save optionVar preference
        cls._hasDisabledAsync = cmds.optionVar(exists=cls._optVarName)
        if cls._hasDisabledAsync:
            cls._prevDisableAsync = cmds.optionVar(q=cls._optVarName)

        mayaUtils.loadPlugin("mayaUsdPlugin")

    @classmethod
    def tearDownClass(cls):
        # Restore user optionVar
        if cls._hasDisabledAsync:
            cmds.optionVar(iv=(cls._optVarName, cls._prevDisableAsync))
        else:
            cmds.optionVar(remove=cls._optVarName)

    def assertSnapshotClose(self, imageName):
        baseline_image = os.path.join(self._baseline_dir, imageName)
        snapshot_image = os.path.join(self._test_dir, imageName)
        imageUtils.snapshot(snapshot_image, width=768, height=768)
        return self.assertImagesClose(baseline_image, snapshot_image)

    def testTextureLoadingSync(self):
        cmds.file(force=True, new=True)

        # Make sure the sync mode is ON (disable async loading)
        cmds.optionVar(iv=(self._optVarName, 1))

        cmds.xform("persp", t=(2, 2, 5.8))
        cmds.xform("persp", ro=[0, 0, 0], ws=True)

        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, lights=False, displayLights="default")

        testFile = testUtils.getTestScene("multipleMaterialsAssignment",
                                          "MultipleMaterialsAssignment.usda")

        # Default purpose mode is "proxy"
        shapeNode, _ = mayaUtils.createProxyFromFile(testFile)
        cmds.select(cl=True)
        self.assertSnapshotClose("TextureLoading_Proxy_Sync.png")

        # Switch purpose to "render"
        cmds.setAttr("{}.drawProxyPurpose".format(shapeNode), 0)
        cmds.setAttr("{}.drawRenderPurpose".format(shapeNode), 1)

        cmds.select(cl=True)
        self.assertSnapshotClose("TextureLoading_Render_Sync.png")

    def testTextureLoadingAsync(self):
        cmds.file(force=True, new=True)
        # Make sure the async mode is ON (enable async loading)
        cmds.optionVar(iv=(self._optVarName, 0))

        cmds.xform("persp", t=(2, 2, 5.8))
        cmds.xform("persp", ro=[0, 0, 0], ws=True)

        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, lights=False, displayLights="default")

        testFile = testUtils.getTestScene("multipleMaterialsAssignment",
                                          "MultipleMaterialsAssignment.usda")

        # Force all idle tasks to finish
        cmds.flushIdleQueue()

        # Default purpose mode is "proxy"
        shapeNode, _ = mayaUtils.createProxyFromFile(testFile)
        cmds.select(cl=True)
        self.assertSnapshotClose("TextureLoading_Proxy_Async.png")

        # Switch purpose to "render"
        cmds.setAttr("{}.drawProxyPurpose".format(shapeNode), 0)
        cmds.setAttr("{}.drawRenderPurpose".format(shapeNode), 1)

        # Force all idle tasks to finish
        cmds.flushIdleQueue()

        cmds.select(cl=True)
        self.assertSnapshotClose("TextureLoading_Render_Async.png")


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
