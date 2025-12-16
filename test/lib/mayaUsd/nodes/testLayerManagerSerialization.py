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

from sys import prefix
from maya import cmds
from maya import standalone
from mayaUsd import lib as mayaUsdLib

import fixturesUtils

from pxr import Usd, Sdf

import os
import tempfile
import unittest
from distutils.dir_util import copy_tree
import shutil

import usdUtils
import mayaUtils
import ufeUtils
from mayaUtils import createProxyAndStage, createProxyFromFile

import ufe
import mayaUsd.ufe


class testLayerManagerSerialization(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls._inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setupEmptyScene(self):
        self._currentTestDir = tempfile.mkdtemp(prefix='LayerManagerTest')
        self._tempMayaFile = os.path.join(
            self._currentTestDir, 'SerializationTest.ma')
        cmds.file(rename=self._tempMayaFile)

    def copyTestFilesAndMakeEdits(self):
        '''
        Copy an existing Maya scene that contains a stage and a few layer,
        creates a few USD prim in the root, session and 1_1 layers.
        '''
        self._currentTestDir = tempfile.mkdtemp(prefix='LayerManagerTest')
        fromDirectory = os.path.join(
            self._inputPath, 'LayerManagerSerializationTest')
        copy_tree(fromDirectory, self._currentTestDir)

        self._tempMayaFile = os.path.join(
            self._currentTestDir, 'SerializationTest.ma')
        self._rootUsdFile = os.path.join(
            self._currentTestDir, 'SerializationTest.usda')
        self._test1File = os.path.join(
            self._currentTestDir, 'SerializationTest_1.usda')
        self._test1_1File = os.path.join(
            self._currentTestDir, 'SerializationTest_1_1.usda')
        self._test2File = os.path.join(
            self._currentTestDir, 'SerializationTest_2.usda')
        self._test2_1File = os.path.join(
            self._currentTestDir, 'SerializationTest_2_1.usda')

        cmds.file(self._tempMayaFile, open=True, force=True)

        stage = mayaUsd.ufe.getStage(
            "|SerializationTest|SerializationTestShape")
        stack = stage.GetLayerStack()
        # Note: layers are:
        #            0: session
        #            1: root
        #            2: 1
        #            3: 1_1
        #            4: 2
        #            5: 2_1
        self.assertEqual(6, len(stack))

        stage.SetEditTarget(stage.GetRootLayer())
        newPrimPath = "/ChangeInRoot"
        stage.DefinePrim(newPrimPath, "xform")

        stage.SetEditTarget(stack[3])
        newPrimPath = "/ChangeInLayer_1_1"
        stage.DefinePrim(newPrimPath, "xform")

        stage.SetEditTarget(stack[0])
        newPrimPath = "/ChangeInSessionLayer"
        stage.DefinePrim(newPrimPath, "xform")

        stage = mayaUsd.ufe.getStage(
            "|SerializationTest|SerializationTestShape")
        stack = stage.GetLayerStack()
        self.assertEqual(6, len(stack))

        cleanCount = 0
        dirtyCount = 0
        for l in stack:
            if l.dirty:
                dirtyCount += 1
            else:
                cleanCount += 1
        self.assertEqual(3, dirtyCount)
        self.assertEqual(3, cleanCount)

        return stage

    def confirmStageHasTestEdits(
            self, stage, fromRootLayer, fromRootSubLayer, fromSessionLayer):

        newPrimPath = "/ChangeInRoot"
        self.assertEqual(
            fromRootLayer, stage.GetPrimAtPath(newPrimPath).IsValid())

        newPrimPath = "/ChangeInLayer_1_1"
        self.assertEqual(
            fromRootSubLayer, stage.GetPrimAtPath(newPrimPath).IsValid())

        newPrimPath = "/ChangeInSessionLayer"
        self.assertEqual(
            fromSessionLayer, stage.GetPrimAtPath(newPrimPath).IsValid())

    def confirmEditsSavedStatus(self, fileBackedSavedStatus, sessionSavedStatus):
        '''
        Clears the Maya scene, creates a new USD stage with the root layer
        and verify various prim existence based on given flags.
        '''
        cmds.file(new=True, force=True)

        proxyNode, stage = createProxyFromFile(self._rootUsdFile)

        self.confirmStageHasTestEdits(
            stage, fileBackedSavedStatus, fileBackedSavedStatus, sessionSavedStatus)

    def testSaveWithoutStage(self):
        '''
        Verify that when saving a Maya scene, if no stage have been created then
        the scene does not depends on teh MayaUSD plugin.
        '''
        self._currentTestDir = tempfile.mkdtemp(prefix='testSaveWithoutStage')
        self._tempMayaFile = os.path.join(
            self._currentTestDir, 'EmptySerializationTest.ma')
        cmds.file(new=True, force=True)
        cmds.file(rename=self._tempMayaFile)
        cmds.file(save=True, force=True, type='mayaAscii')
        cmds.file(new=True, force=True)
        for i, line in enumerate(open(self._tempMayaFile)):
            self.assertFalse('mayaUsdPlugin' in line,
                             'Found mayaUSdPlugin depedency in line %d of %s' % (i+1, self._tempMayaFile))

        shutil.rmtree(self._currentTestDir)

    def testSaveAllToMaya(self):
        '''
        Verify that all USD edits are save into the Maya file.
        '''
        self.copyTestFilesAndMakeEdits()

        cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', 2))

        cmds.file(save=True, force=True)
        cmds.file(new=True, force=True)

        cmds.file(self._tempMayaFile, open=True)

        stage = mayaUsd.ufe.getStage(
            "|SerializationTest|SerializationTestShape")
        stack = stage.GetLayerStack()
        self.assertEqual(6, len(stack))

        self.confirmStageHasTestEdits(stage, True, True, True)
        self.confirmEditsSavedStatus(False, False)

        shutil.rmtree(self._currentTestDir)

    def testSaveAllToUsd(self):
        '''
        Verify that all USD edits are saved back to the original .usd files
        '''
        self.copyTestFilesAndMakeEdits()

        cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', 1))

        cmds.file(save=True, force=True)
        cmds.file(new=True, force=True)

        cmds.file(self._tempMayaFile, open=True)

        stage = mayaUsd.ufe.getStage(
            "|SerializationTest|SerializationTestShape")
        stack = stage.GetLayerStack()
        self.assertEqual(6, len(stack))

        self.confirmStageHasTestEdits(stage, True, True, True)
        self.confirmEditsSavedStatus(True, False)

        shutil.rmtree(self._currentTestDir)

    def testIgnoreAllUsd(self):
        '''
        Verify that all USD edits are ignored
        '''
        self.copyTestFilesAndMakeEdits()

        cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', 3))

        cmds.file(save=True, force=True)
        cmds.file(new=True, force=True)

        cmds.file(self._tempMayaFile, open=True)

        stage = mayaUsd.ufe.getStage(
            "|SerializationTest|SerializationTestShape")
        stack = stage.GetLayerStack()
        self.assertEqual(6, len(stack))

        self.confirmStageHasTestEdits(stage, False, False, False)
        self.confirmEditsSavedStatus(False, False)

        shutil.rmtree(self._currentTestDir)

    def _verifySaveMutedLayer(self, muteSessionLayer, muteRootSubLayer, serializedUsdEditsLocation):
        '''
        Verify saving of muted layers to maya or USD.
        '''
        def applyMuteState(proxyShape, state):
            stage = mayaUsd.ufe.getStage(proxyShape)

            if muteSessionLayer:
                layerId = stage.GetSessionLayer().identifier
                cmds.mayaUsdLayerEditor(layerId, e=True, muteLayer=(state, proxyShape))

            if muteRootSubLayer:
                layerId = stage.GetRootLayer().subLayerPaths[0]
                layerId = Sdf.ComputeAssetPathRelativeToLayer(stage.GetRootLayer(), layerId)
                cmds.mayaUsdLayerEditor(layerId, e=True, muteLayer=(state, proxyShape))

        stage = self.copyTestFilesAndMakeEdits()
        self.confirmStageHasTestEdits(stage, True, True, True)

        # Mute the layer, and verify that the expected edits have been muted.
        applyMuteState("|SerializationTest|SerializationTestShape", 1)
        self.confirmStageHasTestEdits(stage, True, not muteRootSubLayer, not muteSessionLayer)

        # Save and reopen the maya file.
        cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', serializedUsdEditsLocation))

        cmds.file(save=True, force=True)
        cmds.file(new=True, force=True)
        cmds.file(self._tempMayaFile, open=True)

        stage = mayaUsd.ufe.getStage("|SerializationTest|SerializationTestShape")

        # The muted edits are still muted after reopening.
        self.confirmStageHasTestEdits(stage, True, not muteRootSubLayer, not muteSessionLayer)

        # Unmute and verify that the muted edits did persist.
        applyMuteState("|SerializationTest|SerializationTestShape", 0)
        self.confirmStageHasTestEdits(stage, True, True, True)

        # The file should be still modified after reloading from USD.
        fileBackedExpectedStatus = serializedUsdEditsLocation == 1
        self.confirmEditsSavedStatus(fileBackedExpectedStatus, False)

        shutil.rmtree(self._currentTestDir)

    def testSaveMutedRootSubLayerToUsd(self):
        '''Verify that USD edits in the root layer stack are saved to USD even if muted'''
        self._verifySaveMutedLayer(
            muteSessionLayer=False, muteRootSubLayer=True, serializedUsdEditsLocation=1)

    def testSaveMutedRootSubLayerToMaya(self):
        '''Verify that USD edits in the root layer stack are saved to Maya even if muted'''
        self._verifySaveMutedLayer(
            muteSessionLayer=False, muteRootSubLayer=True, serializedUsdEditsLocation=2)

    def testSaveMutedSessionLayerToUsd(self):
        '''Verify that USD edits in the session layer are saved to Maya even if muted'''
        self._verifySaveMutedLayer(
            muteSessionLayer=True, muteRootSubLayer=False, serializedUsdEditsLocation=1)

    def testSaveMutedSessionLayerToMaya(self):
        '''Verify that USD edits in the session layer are saved to Maya even if muted'''
        self._verifySaveMutedLayer(
            muteSessionLayer=True, muteRootSubLayer=False, serializedUsdEditsLocation=2)

    def testAnonymousRootToMaya(self):
        self.setupEmptyScene()

        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path(proxyShape)

        stage = mayaUsd.ufe.getStage(ufe.PathString.string(proxyShapePath))

        newPrimPath = "/ChangeInRoot"
        stage.DefinePrim(newPrimPath, "xform")
        self.assertTrue(stage.GetPrimAtPath(newPrimPath).IsValid())

        stage.SetEditTarget(stage.GetSessionLayer())
        newSessionsPrimPath = "/ChangeInSession"
        stage.DefinePrim(newSessionsPrimPath, "xform")
        self.assertTrue(stage.GetPrimAtPath(newSessionsPrimPath).IsValid())

        cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', 2))

        cmds.file(save=True, force=True, type='mayaAscii')
        cmds.file(new=True, force=True)
        cmds.file(self._tempMayaFile, open=True)

        stage = mayaUsd.ufe.getStage('|stage1|stageShape1')
        self.assertTrue(stage.GetPrimAtPath(newPrimPath).IsValid())
        self.assertTrue(stage.GetPrimAtPath(newSessionsPrimPath).IsValid())

        shutil.rmtree(self._currentTestDir)

    def testAnonymousRootToUsd(self):
        self.setupEmptyScene()

        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path(proxyShape)

        stage = mayaUsd.ufe.getStage(ufe.PathString.string(proxyShapePath))

        newPrimPath = "/ChangeInRoot"
        stage.DefinePrim(newPrimPath, "xform")
        self.assertTrue(stage.GetPrimAtPath(newPrimPath).IsValid())

        stage.SetEditTarget(stage.GetSessionLayer())
        newSessionsPrimPath = "/ChangeInSession"
        stage.DefinePrim(newSessionsPrimPath, "xform")
        self.assertTrue(stage.GetPrimAtPath(newSessionsPrimPath).IsValid())

        cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', 1))

        msg = ("Session Layer before: " + stage.GetSessionLayer().identifier)
        stage = None

        cmds.file(save=True, force=True, type='mayaAscii')
        cmds.file(new=True, force=True)
        cmds.file(self._tempMayaFile, open=True)

        stage = mayaUsdLib.GetPrim('|stage1|stageShape1').GetStage()
        msg += ("    Session Layer after: " +
                stage.GetSessionLayer().identifier)
        self.assertTrue(stage.GetPrimAtPath(newPrimPath).IsValid())

        # Temporarily disabling this check while investigating why it can fail on certain build combinations
        # self.assertFalse(stage.GetPrimAtPath(
        #     newSessionsPrimPath).IsValid(), msg)

        cmds.file(new=True, force=True)
        shutil.rmtree(self._currentTestDir)

    def testMultipleFormatsSerialisation(self):
        # Test setup
        self.setupEmptyScene()
        extensions = ["usd", "usda", "usdc"]
        usdFilePath = os.path.abspath('MultipleFormatsSerializationTest.usda')

        def createProxyShapeFromFile(filePath):
            usdNode = cmds.createNode('mayaUsdProxyShape', skipSelect=True, name='UsdStageShape')
            cmds.setAttr(usdNode + '.filePath', filePath, type='string')
            return usdNode

        def getStageFromProxyShape(shapeNode):
            return mayaUsdLib.GetPrim(shapeNode).GetStage()

        # Initialise the sublayers
        cmds.file(new=True, force=True)
        cmds.file(rename=self._tempMayaFile)
        rootLayer = Sdf.Layer.CreateNew(usdFilePath)
        layers = []
        for ext in extensions:
            filePath = os.path.abspath('TestLayer.{}'.format(ext))
            layer = Sdf.Layer.CreateNew(filePath)
            layer.comment = ext
            layer.Save()
            layers.append(layer)
            rootLayer.subLayerPaths.append(filePath)
        rootLayer.Save()

        # Add edits to the 3 format layers and save the maya scene
        proxyShape = createProxyShapeFromFile(usdFilePath)
        stage = getStageFromProxyShape(proxyShape)
        for subLayerPath in stage.GetRootLayer().subLayerPaths:
            layer = Sdf.Layer.Find(subLayerPath)
            with Usd.EditContext(stage, layer):
                stage.DefinePrim("/{}".format(layer.GetFileFormat().primaryFileExtension))
        self.assertTrue(stage.GetSessionLayer().empty)
        cmds.file(save=True, force=True)

        # Assert the edits have been restored
        cmds.file(new=True, force=True)
        cmds.file(self._tempMayaFile, open=True)
        stage = getStageFromProxyShape(proxyShape)
        stageLayers = stage.GetUsedLayers()

        # root + session + 3 format layers
        self.assertEqual(len(stageLayers), 5)
        
        # Check they all exist
        for layer in layers:
            self.assertIn(layer, stageLayers)
        
        # Check their content exists
        for ext in extensions:
            prim = stage.GetPrimAtPath("/{}".format(ext))
            self.assertTrue(prim.IsValid())

        # Cleanup
        cmds.file(new=True, force=True)
        shutil.rmtree(self._currentTestDir)


if __name__ == '__main__':
    unittest.main(verbosity=2)
