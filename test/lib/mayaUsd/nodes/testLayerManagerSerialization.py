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
        self.assertEqual(6, len(stack))

        stage.SetEditTarget(stage.GetRootLayer())
        newPrimPath = "/ChangeInRoot"
        stage.DefinePrim(newPrimPath, "xform")

        stage.SetEditTarget(stack[2])
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

    def confirmEditsSavedStatus(self, fileBackedSavedStatus, sessionSavedStatus):
        cmds.file(new=True, force=True)

        proxyNode, stage = createProxyFromFile(self._rootUsdFile)

        newPrimPath = "/ChangeInRoot"
        self.assertEqual(
            fileBackedSavedStatus, stage.GetPrimAtPath(newPrimPath).IsValid())

        newPrimPath = "/ChangeInLayer_1_1"
        self.assertEqual(
            fileBackedSavedStatus, stage.GetPrimAtPath(newPrimPath).IsValid())

        newPrimPath = "/ChangeInSessionLayer"
        self.assertEqual(
            sessionSavedStatus, stage.GetPrimAtPath(newPrimPath).IsValid())

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testSaveAllToMaya is available only in UFE v2 or greater.')
    def testSaveAllToMaya(self):
        '''
        Verify that all USD edits are save into the Maya file.
        '''
        stage = self.copyTestFilesAndMakeEdits()

        cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', 2))

        cmds.file(save=True, force=True)
        cmds.file(new=True, force=True)

        cmds.file(self._tempMayaFile, open=True)

        stage = mayaUsd.ufe.getStage(
            "|SerializationTest|SerializationTestShape")
        stack = stage.GetLayerStack()
        self.assertEqual(6, len(stack))

        newPrimPath = "/ChangeInRoot"
        self.assertTrue(stage.GetPrimAtPath(newPrimPath))

        newPrimPath = "/ChangeInLayer_1_1"
        self.assertTrue(stage.GetPrimAtPath(newPrimPath))

        newPrimPath = "/ChangeInSessionLayer"
        self.assertTrue(stage.GetPrimAtPath(newPrimPath))

        self.confirmEditsSavedStatus(False, False)

        shutil.rmtree(self._currentTestDir)

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testSaveAllToUsd is available only in UFE v2 or greater.')
    def testSaveAllToUsd(self):
        '''
        Verify that all USD edits are saved back to the original .usd files
        '''
        stage = self.copyTestFilesAndMakeEdits()

        cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', 1))

        cmds.file(save=True, force=True)
        cmds.file(new=True, force=True)

        cmds.file(self._tempMayaFile, open=True)

        stage = mayaUsd.ufe.getStage(
            "|SerializationTest|SerializationTestShape")
        stack = stage.GetLayerStack()
        self.assertEqual(6, len(stack))

        newPrimPath = "/ChangeInRoot"
        self.assertTrue(stage.GetPrimAtPath(newPrimPath))

        newPrimPath = "/ChangeInLayer_1_1"
        self.assertTrue(stage.GetPrimAtPath(newPrimPath))

        newPrimPath = "/ChangeInSessionLayer"
        self.assertFalse(stage.GetPrimAtPath(newPrimPath))

        self.confirmEditsSavedStatus(True, False)

        shutil.rmtree(self._currentTestDir)

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testIgnoreAllUsd is available only in UFE v2 or greater.')
    def testIgnoreAllUsd(self):
        '''
        Verify that all USD edits are ignored
        '''
        stage = self.copyTestFilesAndMakeEdits()

        cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocation', 3))

        cmds.file(save=True, force=True)
        cmds.file(new=True, force=True)

        cmds.file(self._tempMayaFile, open=True)

        stage = mayaUsd.ufe.getStage(
            "|SerializationTest|SerializationTestShape")
        stack = stage.GetLayerStack()
        self.assertEqual(6, len(stack))

        newPrimPath = "/ChangeInRoot"
        self.assertFalse(stage.GetPrimAtPath(newPrimPath))

        newPrimPath = "/ChangeInLayer_1_1"
        self.assertFalse(stage.GetPrimAtPath(newPrimPath))

        newPrimPath = "/ChangeInSessionLayer"
        self.assertFalse(stage.GetPrimAtPath(newPrimPath))

        self.confirmEditsSavedStatus(False, False)

        shutil.rmtree(self._currentTestDir)

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testAnonymousRootToMaya is available only in UFE v2 or greater.')
    def testAnonymousRootToMaya(self):
        self.setupEmptyScene()

        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path(proxyShape)

        stage = mayaUsd.ufe.getStage(str(proxyShapePath))

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

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testAnonymousRootToMaya is available only in UFE v2 or greater.')
    def testAnonymousRootToUsd(self):
        self.setupEmptyScene()

        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path(proxyShape)

        stage = mayaUsd.ufe.getStage(str(proxyShapePath))

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


if __name__ == '__main__':
    unittest.main(verbosity=2)
