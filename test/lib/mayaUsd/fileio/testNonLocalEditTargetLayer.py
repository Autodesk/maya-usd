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

    def _prepareStage(self, testDir):
        # Localize the test data to the temp location to avoid
        # saving mode 1 (save all edits back to usd files) to write back changes
        # to original USD file, that would prevent from re-running the unit test
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
        # For this particular case, the layer should be the deepest "nested_reference_geo.usda" layer
        targetLayer = primNode.layerStack.identifier.rootLayer
        self.assertTrue(targetLayer)
        self.assertEqual(targetLayer.realPath, os.path.join(testDir, 'data', 'nested_reference_geo.usda'))
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

        return proxyShapePath, stage

    def _saveAndReopen(self, sceneName, testDir, saveLocation, proxyShapePath):
        targetLayerPath = os.path.join(testDir, 'data', 'nested_reference_geo.usda')
        # Save and reopen the maya file
        cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', saveLocation))
        tempMayaFile = os.path.join(testDir, '{}.ma'.format(sceneName))
        cmds.file(rename=tempMayaFile)
        cmds.file(save=True, force=True, type='mayaAscii')

        # Verify
        cmds.file(force=True, new=True)
        cmds.file(tempMayaFile, open=True, force=True)
        self.assertTrue(cmds.ls(proxyShapePath))
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
        self.assertTrue(stage)
        # Verify edit target layer id
        # The layer real path should equal to the file path
        self.assertEqual(stage.GetEditTarget().GetLayer().realPath, targetLayerPath)

        return stage

    def _runTestUnmutedUnlocked(self, saveLocation):
        '''
        Test preserving non local edit target layer, parent of target layer is unmuted and unlocked.
        '''
        caseName = 'UnmutedUnlocked'
        prefixDir = 'NonLocalEditTargetLayerTest_{}_{}'.format(saveLocation, caseName)
        with testUtils.TemporaryDirectory(prefix=prefixDir, ignore_errors=True) as testDir:
            proxyShapePath, _ = self._prepareStage(testDir)

            # Case 1: no muting nor locking parent layer (nothing else to do here)
            # Save and reopen the maya file
            stage = self._saveAndReopen(caseName, testDir, saveLocation, proxyShapePath)

            # Verify the prim that created in the edit target layer
            self.assertTrue(stage.GetPrimAtPath('/root/group/sphere/group/newXform'))

            # Verify the layer was reloaded but not muted nor locked
            subLayer = Sdf.Layer.Find(os.path.join(testDir, 'data', 'outer_sub_layer.usda'))
            self.assertIsNotNone(subLayer)
            self.assertFalse(stage.IsLayerMuted(subLayer.identifier))
            self.assertFalse(mayaUsd.lib.isLayerLocked(subLayer))

            if saveLocation == 2:
                # Saving mode 2 writes the dirty layers in Maya scene, thus the USD layers
                # are still in *dirty* status after reopening
                self.assertTrue(stage.GetEditTarget().GetLayer().dirty)

    def _runTestUnmutedLocked(self, saveLocation):
        '''
        Test preserving non local edit target layer, parent of target layer is unmuted and locked.
        '''
        caseName = 'UnmutedLocked'
        prefixDir = 'NonLocalEditTargetLayerTest_{}_{}'.format(saveLocation, caseName)
        with testUtils.TemporaryDirectory(prefix=prefixDir, ignore_errors=True) as testDir:
            proxyShapePath, stage = self._prepareStage(testDir)

            # Case 2: lock parent layer, no muting
            subLayerPath = os.path.join(testDir, 'data', 'outer_sub_layer.usda')

            subLayer = Sdf.Layer.Find(subLayerPath)
            self.assertIn(subLayer, stage.GetLayerStack())
            mayaUsd.lib.lockLayer(proxyShapePath, subLayer)

            # Save and reopen the maya file
            stage = self._saveAndReopen(caseName, testDir, saveLocation, proxyShapePath)

            # Parent layer locked, the prim is still accessible
            self.assertTrue(stage.GetPrimAtPath('/root/group/sphere/group/newXform'))
            # Verify the layer was reloaded as locked and not muted
            subLayer = Sdf.Layer.Find(os.path.join(testDir, 'data', 'outer_sub_layer.usda'))
            self.assertIsNotNone(subLayer)
            self.assertFalse(stage.IsLayerMuted(subLayer.identifier))
            self.assertTrue(mayaUsd.lib.isLayerLocked(subLayer))

            if saveLocation == 2:
                # Saving mode 2 writes the dirty layers in Maya scene, thus the USD layers
                # are still in *dirty* status after reopening
                self.assertTrue(stage.GetEditTarget().GetLayer().dirty)

    def _runTestMutedUnlocked(self, saveLocation):
        '''
        Test preserving non local edit target layer, parent of target layer is muted and unlocked.
        '''
        caseName = 'MutedUnlocked'
        prefixDir = 'NonLocalEditTargetLayerTest_{}_{}'.format(saveLocation, caseName)
        with testUtils.TemporaryDirectory(prefix=prefixDir, ignore_errors=True) as testDir:
            proxyShapePath, stage = self._prepareStage(testDir)

            # Case 3: mute parent layer, no locking
            subLayerPath = os.path.join(testDir, 'data', 'outer_sub_layer.usda')
            subLayer = Sdf.Layer.Find(subLayerPath)
            stage.MuteLayer(subLayer.identifier)

            # Save and reopen the maya file
            stage = self._saveAndReopen(caseName, testDir, saveLocation, proxyShapePath)

            # Parent layer muted, the prim is not accessible
            self.assertFalse(stage.GetPrimAtPath('/root/group/sphere/group/newXform'))
            # Verify the layer was reloaded as muted and not locked
            self.assertIsNone(Sdf.Layer.Find(subLayerPath))
            self.assertTrue(stage.IsLayerMuted(subLayerPath))
            self.assertFalse(mayaUsd.lib.isLayerLocked(Sdf.Layer.FindOrOpen(subLayerPath)))
            # The dirty layer was muted before saving, the layer manager won't be able
            # to find the dirty layer (the nested layer) thus the content won't be saved.
            self.assertFalse(stage.GetEditTarget().GetLayer().dirty)

    def _runTestMutedLocked(self, saveLocation):
        '''
        Test preserving non local edit target layer, parent of target layer is muted and locked.
        '''
        caseName = 'MutedLocked'
        prefixDir = 'NonLocalEditTargetLayerTest_{}_{}'.format(saveLocation, caseName)
        with testUtils.TemporaryDirectory(prefix=prefixDir, ignore_errors=True) as testDir:
            proxyShapePath, stage = self._prepareStage(testDir)

            # Case 4: mute and lock parent layer
            subLayerPath = os.path.join(testDir, 'data', 'outer_sub_layer.usda')

            subLayer = Sdf.Layer.Find(subLayerPath)
            mayaUsd.lib.lockLayer(proxyShapePath, subLayer)
            stage.MuteLayer(subLayer.identifier)

            # Save and reopen the maya file
            stage = self._saveAndReopen(caseName, testDir, saveLocation, proxyShapePath)

            # Parent layer muted, the prim is not accessible
            self.assertFalse(stage.GetPrimAtPath('/root/group/sphere/group/newXform'))
            # Note: MayaUSD inserts the locking layer in local layer stack to keep a reference of it
            #       thus the layer can still be found even it had been muted
            subLayer = Sdf.Layer.Find(subLayerPath)
            self.assertIsNotNone(subLayer)
            # Verify the layer was reloaded as muted
            self.assertTrue(stage.IsLayerMuted(subLayer.identifier))
            # Verify the layer was reloaded as locked
            self.assertTrue(mayaUsd.lib.isLayerLocked(subLayer))
            # The dirty layer was muted before saving, the layer manager won't be able
            # to find the dirty layer (the nested layer) thus the content won't be saved.
            self.assertFalse(stage.GetEditTarget().GetLayer().dirty)

    def testSaveInUSDUnmutedUnlocked(self):
        '''
        Test preserving non local edit target layer, save dirty layers in USD, parent of target layer is unmuted and unlocked.
        '''
        self._runTestUnmutedUnlocked(1)

    def testSaveInUSDUnmutedLocked(self):
        '''
        Test preserving non local edit target layer, save dirty layers in USD, parent of target layer is unmuted and locked.
        '''
        self._runTestUnmutedLocked(1)

    def testSaveInUSDMutedUnlocked(self):
        '''
        Test preserving non local edit target layer, save dirty layers in USD, parent of target layer is muted and unlocked.
        '''
        self._runTestMutedUnlocked(1)

    def testSaveInUSDMutedLocked(self):
        '''
        Test preserving non local edit target layer, save dirty layers in USD, parent of target layer is muted and locked.
        '''
        self._runTestMutedLocked(1)

    def testSaveInMayaUnmutedUnlocked(self):
        '''
        Test preserving non local edit target layer, save dirty layers in Maya, parent of target layer is unmuted and unlocked.
        '''
        self._runTestUnmutedUnlocked(2)

    def testSaveInMayaUnmutedLocked(self):
        '''
        Test preserving non local edit target layer, save dirty layers in Maya, parent of target layer is unmuted and locked.
        '''
        self._runTestUnmutedLocked(2)

    def testSaveInMayaMutedUnlocked(self):
        '''
        Test preserving non local edit target layer, save dirty layers in Maya, parent of target layer is muted and unlocked.
        '''
        self._runTestMutedUnlocked(2)

    def testSaveInMayaMutedLocked(self):
        '''
        Test preserving non local edit target layer, save dirty layers in Maya, parent of target layer is muted and locked.
        '''
        self._runTestMutedLocked(2)


if __name__ == '__main__':
    unittest.main(verbosity=2)
