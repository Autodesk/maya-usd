#!/usr/bin/env python

#
# Copyright 2024 Autodesk
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

from pxr import Usd, Sdf, UsdGeom

import unittest

class SaveUpAxisAndUnitsTest(unittest.TestCase):
    '''
    Test saving a layer and reloading the scene to verify they have up-axis and units metadata.
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

    def testSaveLayerInUSD(self):
        # Save the file. Make sure the USD edits will go to a USD file.
        self._runTestSaveLayer(1)

    def testSaveLayerInMaya(self):
        # Save the file. Make sure the USD edits will go to the Maya file.
        self._runTestSaveLayer(2)

    def _runTestSaveLayer(self, saveLocation):
        '''
        The goal is to create an anonymous sub-layer, verify the up-axis and units
        metadata is present when reloaded.
        '''
        # Create a stage with a sub-layer.
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        rootLayer = stage.GetRootLayer()
        subLayer = usdUtils.addNewLayerToLayer(rootLayer, anonymous=True)

        # Add a prim in the root and in the sub-layer.
        with Usd.EditContext(stage, Usd.EditTarget(rootLayer)):
            stage.DefinePrim('/InRoot', 'Sphere')

        with Usd.EditContext(stage, Usd.EditTarget(subLayer)):
            stage.DefinePrim('/InSub', 'Cube')

        def verifyPrims(stage):
            inRoot = stage.GetPrimAtPath("/InRoot")
            self.assertTrue(inRoot)
            inRoot = None
            inSub = stage.GetPrimAtPath("/InSub")
            self.assertTrue(inSub)
            inSub = None

        verifyPrims(stage)

        # Save the file. Make sure the edit will go where requested by saveLocation.
        tempMayaFile = 'saveLockedAnonLayer.ma'
        cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', saveLocation))
        cmds.file(rename=tempMayaFile)
        cmds.file(save=True, force=True, type='mayaAscii')

        # Clear and reopen the file.
        stage = None
        rootLayer = None
        subLayer = None
        cmds.file(new=True, force=True)
        cmds.file(tempMayaFile, open=True)

        # Retrieve the stage, root and sub-layer.
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        rootLayer = stage.GetRootLayer()
        self.assertEqual(len(rootLayer.subLayerPaths), 1)
        subLayerPath = rootLayer.subLayerPaths[0]
        self.assertIsNotNone(subLayerPath)
        self.assertTrue(subLayerPath)
        subLayer = Sdf.Layer.FindOrOpen(subLayerPath)
        self.assertIsNotNone(subLayer)

        self.assertTrue(stage.HasAuthoredMetadata(UsdGeom.Tokens.metersPerUnit))
        self.assertEqual(UsdGeom.GetStageMetersPerUnit(stage), 0.01)
        self.assertTrue(stage.HasAuthoredMetadata(UsdGeom.Tokens.upAxis))
        self.assertEqual(UsdGeom.GetStageUpAxis(stage), UsdGeom.Tokens.y)

        # Verify the two objects are still present.
        verifyPrims(stage)

if __name__ == '__main__':
    unittest.main(verbosity=2)
