#!/usr/bin/env python

#
# Copyright 2024 Animal Logic
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
import unittest
import shutil

from pxr import Usd, Sdf

from maya import cmds
from maya import standalone

import mayaUsd

import fixturesUtils
import testUtils
import mayaUtils


class NonLocalEditTargetLayer(unittest.TestCase):
    '''
    Test saving and restoring non local edit target layer.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

        cls.testDataDir = os.path.join(inputPath, 'NonLocalEditTargetLayerTest')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def _runTestNonLocalEditTargetLayer(self, saveLocation):
        # Localize the test data to the temp location to avoid
        # saving mode 1 (save all edits back to usd files) to write back changes
        # to original USD file, that would prevent from re-running the unit test
        with testUtils.TemporaryDirectory(prefix='NonLocalEditTargetLayerTest', ignore_errors=True) as testDir:
            shutil.copytree(self.testDataDir, os.path.join(testDir, 'data'))

            rootLayerPath = os.path.join(testDir, 'data', 'root_layer.usda')
            # create new stage
            proxyShapePath, stage = mayaUtils.createProxyFromFile(rootLayerPath)

            # the "sphere" variant prim should be loaded
            spherePrim = stage.GetPrimAtPath('/root/group/sphere/group/sphere')
            self.assertTrue(spherePrim.IsValid())
            prim = stage.GetPrimAtPath('/root/group/sphere/group')
            self.assertTrue(spherePrim.IsValid())
            # Find the nested pcp node
            primNode = prim.GetPrimIndex().rootNode.children[0].children[0]
            self.assertTrue(primNode)
            # For this particular case, the layer should be the deepest "geo.usda" layer
            targetLayer = primNode.layerStack.identifier.rootLayer
            self.assertTrue(targetLayer)
            targetLayerId = targetLayer.identifier
            # Verify target layer that it is **not** in stage layer stack
            self.assertNotIn(targetLayer, stage.GetLayerStack())
            # Verify current edit target layer is not the target layer
            self.assertNotEqual(stage.GetEditTarget().GetLayer(), targetLayer)
            # Change edit target layer to the non-local edit target layer
            stage.SetEditTarget(Usd.EditTarget(targetLayer, primNode))
            # Verify edit target layer
            self.assertEqual(stage.GetEditTarget().GetLayer(), targetLayer)
            # Dirty the non-local edit target layer, the path in nested layer
            # will be mapped like this on the stage level:
            #    /root/group/sphere/group/newXform
            newXform = Sdf.CreatePrimInLayer(targetLayer, '/root/group/newXform')
            newXform.specifier = Sdf.SpecifierDef
            newXform.typeName = 'Xform'

            self.assertTrue(stage.GetPrimAtPath('/root/group/sphere/group/newXform'))
            self.assertTrue(targetLayer.dirty)

            # Save and reopen the maya file
            cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', saveLocation))

            tempMayaFile = os.path.join(testDir, 'SaveNonLocalEditTargetLayer.ma')
            cmds.file(rename=tempMayaFile)
            cmds.file(save=True, force=True, type='mayaAscii')

            # Reopen the saved scene
            cmds.file(force=True, new=True)
            cmds.file(tempMayaFile, open=True, force=True)
            self.assertTrue(cmds.ls(proxyShapePath))

            stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
            self.assertTrue(stage)

            # Verify edit target layer id
            self.assertEqual(stage.GetEditTarget().GetLayer().identifier, targetLayerId)

            # Verify the prim that created in the edit target layer
            self.assertTrue(stage.GetPrimAtPath('/root/group/sphere/group/newXform'))
            return stage

    def testNonLocalEditTargetLayerInUSD(self):
        '''
        Test saving and restoring non local edit target layer, dirty layers are saved in USD.
        '''
        self._runTestNonLocalEditTargetLayer(1)

    def testNonLocalEditTargetLayerInMaya(self):
        '''
        Test saving and restoring non local edit target layer, dirty layers are saved in Maya.
        '''
        stage = self._runTestNonLocalEditTargetLayer(2)
        # Saving mode 2 writes the dirty layers in Maya scene, thus the USD layers
        # are still in *dirty* status after reopening
        self.assertTrue(stage.GetEditTarget().GetLayer().dirty)


if __name__ == '__main__':
    unittest.main(verbosity=2)
