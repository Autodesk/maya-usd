#!/usr/bin/env mayapy
#
# Copyright 2026 Autodesk
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

from mayaUsd import ufe as mayaUsdUfe

import mayaUtils

from pxr import Sdf, Usd, UsdGeom

from maya import cmds

import os

PAYLOAD_FILTER = 'mayaUsdPayloadBoundsDisplayFilter'


class testVP2RenderDelegatePayloadBBox(imageUtils.ImageDiffingTestCase):
    """
    Tests the "USD Payload Bounding Boxes" viewport display filter.
    """

    @classmethod
    def setUpClass(cls):
        input_path = fixturesUtils.setUpClass(
            __file__, initializeStandalone=False, loadPlugin=False
        )

        cls._baseline_dir = os.path.join(
            input_path, "VP2RenderDelegatePayloadBBoxTest", "baseline"
        )

        cls._test_dir = os.path.abspath(".")

        # Resume running idle tasks
        cmds.flushIdleQueue(resume=True)

    @classmethod
    def tearDownClass(cls):
        pass

    def setUp(self):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")
        self.panel = 'modelPanel4'

    def assertSnapshotClose(self, imageName):
        baseline_image = os.path.join(self._baseline_dir, imageName)
        snapshot_image = os.path.join(self._test_dir, imageName)
        imageUtils.snapshot(snapshot_image, width=768, height=768)
        return self.assertImagesClose(baseline_image, snapshot_image)

    def _createUnloadedPayloadProxy(self):
        """Create a proxy with a prim whose payload is unloaded and has an extentsHint."""
        payloadPath = os.path.abspath('payloadBoundsPayload.usda')
        payloadStage = Usd.Stage.CreateNew(payloadPath)
        cube = UsdGeom.Cube.Define(payloadStage, '/Asset/Cube')
        cube.GetSizeAttr().Set(2.0)
        payloadStage.SetDefaultPrim(payloadStage.GetPrimAtPath('/Asset'))
        payloadStage.GetRootLayer().Save()
        del payloadStage

        rootPath = os.path.abspath('payloadBoundsRoot.usda')
        stage = Usd.Stage.CreateNew(rootPath)
        asset = UsdGeom.Xform.Define(stage, '/Asset')
        asset.GetPrim().GetPayloads().AddPayload(Sdf.Payload(payloadPath))
        UsdGeom.ModelAPI(asset.GetPrim()).SetExtentsHint(
            [(-1, -1, -1), (1, 1, 1)])
        stage.GetRootLayer().Save()
        del stage

        shapePath = mayaUtils.createProxyFromFile(rootPath)[0]
        proxyStage = mayaUsdUfe.getStage(shapePath)
        proxyStage.Unload('/Asset')
        self.assertFalse(proxyStage.GetPrimAtPath('/Asset').IsLoaded())

    def _filterOn(self, name):
        return bool(cmds.modelEditor(self.panel, query=True, queryPluginObjects=name))

    def testFilterRegisteredAndOnByDefault(self):
        self.assertTrue(self._filterOn(PAYLOAD_FILTER))

    def testToggleChangeViewport(self):
        self._createUnloadedPayloadProxy()
        self.assertTrue(self._filterOn(PAYLOAD_FILTER))
        
        panel = mayaUtils.activeModelPanel()
        cmds.modelEditor(panel, edit=True, lights=False, displayLights="default", displayTextures=False)

        # Force all idle tasks to finish
        cmds.flushIdleQueue()
        self.assertSnapshotClose("PayloadBBox_bbox_on.png")

        cmds.modelEditor(
            self.panel, edit=True, pluginObjects=(PAYLOAD_FILTER, False))

        # Force all idle tasks to finish
        cmds.flushIdleQueue()
        self.assertSnapshotClose("PayloadBBox_bbox_off.png")


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
