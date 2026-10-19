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

from maya import cmds
from maya import standalone

import mayaUtils
import mayaUsd
import mayaUsd_createStageWithNewLayer

import usdUtils

from pxr import Sdf

import unittest

class SaveEditorLockedLayerTest(unittest.TestCase):
    '''
    Test that a layer locked through the layer editor command (the path used by the
    Layer Editor UI) is still locked after saving and reloading the scene.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testEditorLockedLayerSurvivesSaveInUSD(self):
        self._runTest(1)

    def testEditorLockedLayerSurvivesSaveInMaya(self):
        self._runTest(2)

    def testEditorLockOnShapeSharingLeafNameWithAnotherShape(self):
        firstPath = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        secondPath = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Give both shapes the same leaf name, under different parents.
        leafName = firstPath.split('|')[-1]
        secondParent = secondPath.rsplit('|', 1)[0]
        cmds.rename(secondPath, leafName)
        secondPath = cmds.ls(secondParent + '|' + leafName, long=True)[0]
        self.assertNotEqual(firstPath, secondPath)
        self.assertEqual(secondPath.split('|')[-1], leafName)

        stage = mayaUsd.lib.GetPrim(secondPath).GetStage()
        subLayer = usdUtils.addNewLayerToLayer(stage.GetRootLayer(), anonymous=True)

        cmds.mayaUsdLayerEditor(
            subLayer.identifier, edit=True, lockLayer=(1, 0, secondPath))

        self.assertEqual(cmds.getAttr(secondPath + '.lockedLayers'), [subLayer.identifier])
        self.assertFalse(cmds.getAttr(firstPath + '.lockedLayers'))

    def _runTest(self, saveLocation):
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        rootLayer = stage.GetRootLayer()
        subLayer = usdUtils.addNewLayerToLayer(rootLayer, anonymous=True)
        self.assertFalse(mayaUsd.lib.isLayerLocked(subLayer))

        # lockLayer flag: (lockType 1 = locked, includeSublayers 0, proxy shape path).
        cmds.mayaUsdLayerEditor(
            subLayer.identifier, edit=True, lockLayer=(1, 0, psPathStr))
        self.assertTrue(mayaUsd.lib.isLayerLocked(subLayer))

        # The lock state is stored on the proxy shape, which is what persists it in the scene.
        self.assertEqual(cmds.getAttr(psPathStr + '.lockedLayers'), [subLayer.identifier])

        tempMayaFile = 'saveEditorLockedLayer.ma'
        cmds.optionVar(intValue=(
            mayaUsd.lib.OptionVarTokens.SerializedUsdEditsLocation, saveLocation))
        cmds.file(rename=tempMayaFile)
        cmds.file(save=True, force=True, type='mayaAscii')

        stage = None
        rootLayer = None
        subLayer = None
        cmds.file(new=True, force=True)
        cmds.file(tempMayaFile, open=True)

        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        rootLayer = stage.GetRootLayer()
        self.assertEqual(len(rootLayer.subLayerPaths), 1)
        subLayer = Sdf.Layer.FindOrOpen(rootLayer.subLayerPaths[0])
        self.assertIsNotNone(subLayer)

        self.assertTrue(mayaUsd.lib.isLayerLocked(subLayer))


if __name__ == '__main__':
    unittest.main(verbosity=2)
