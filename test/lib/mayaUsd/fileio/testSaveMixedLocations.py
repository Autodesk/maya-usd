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
from testUtils import getTestScene

from maya import cmds
from maya import standalone

import mayaUtils
import mayaUsd
import mayaUsd_createStageWithNewLayer
import ufe

import usdUtils

from pxr import Usd, Sdf

import unittest

class SaveMixedLocationsTest(unittest.TestCase):
    '''
    Test saving a mix of external stage file and in Maya scene stage.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)
        cls.inputPath = fixturesUtils.setUpClass(__file__)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testSaveLayerInMaya(self):
        '''
        The goal is to create stage and modify a prim in it session layer and then create
        a stage from an external file. When saving to the Maya scene and reloading, the session
        layer edits should be preserved.
        '''
        # Create a stage.
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()

        # Add a prim in the root and in the sub-layer.
        with Usd.EditContext(stage, Usd.EditTarget(stage.GetRootLayer())):
            stage.DefinePrim('/InRoot', 'Sphere')

        with Usd.EditContext(stage, Usd.EditTarget(stage.GetSessionLayer())):
            ufePath = ufe.Path([
                mayaUtils.createUfePathSegment(psPathStr),
                usdUtils.createUfePathSegment('/InRoot')])
            ufeItem = ufe.Hierarchy.createItem(ufePath)

            # Select the point instance scene item.
            globalSelection = ufe.GlobalSelection.get()
            globalSelection.clear()
            globalSelection.append(ufeItem)
            cmds.move(5, 0, 0, absolute=True)
            globalSelection.clear()
            ufePath = None
            ufeItem = None

        def verifyPrims(stage):
            self.assertIsNotNone(stage)
            inRoot = stage.GetPrimAtPath("/InRoot")
            self.assertTrue(inRoot)
            xformOp = inRoot.GetAttribute('xformOp:translate')
            self.assertTrue(xformOp)
            self.assertEqual(xformOp.Get(), (5.0, 0.0, 0.0))
            inRoot = None
            xformOp = None

        verifyPrims(stage)

        usdFilePath = getTestScene("cylinder", "cylinder.usda")
        cylinderShape, cylinderStage = mayaUtils.createProxyFromFile(usdFilePath)
        self.assertIsNotNone(cylinderStage)
        cylinderShape = None
        cylinderStage = None

        verifyPrims(stage)

        # Save the file. Make sure the edit will go where requested by saveOnDisk.
        # saveLocation 1 means on-disk, 2 in the Maya scene.
        saveLocation = 2
        tempMayaFile = 'saveMixedLocationsTest.ma'
        cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', saveLocation))
        cmds.file(rename=tempMayaFile)
        cmds.file(save=True, force=True, type='mayaAscii')

        # Clear and reopen the file.
        stage = None
        cmds.file(new=True, force=True)
        cmds.file(tempMayaFile, open=True)

        layers = Sdf.Layer.GetLoadedLayers()
        for layer in layers:
            if 'cylinder' in layer.identifier:
                continue
            if 'anon' in layer.identifier:
                if 'session' in layer.identifier:
                    sessionLayer = layer
                else:
                    rootLayer = layer

        self.assertIsNotNone(rootLayer)
        self.assertIsNotNone(sessionLayer)
        stage = Usd.Stage.Open(rootLayer, sessionLayer)
        self.assertIsNotNone(stage)

        # Verify the two objects are still present.
        # Note: the test fails to repro the issue.
        verifyPrims(stage)


if __name__ == '__main__':
    unittest.main(verbosity=2)
