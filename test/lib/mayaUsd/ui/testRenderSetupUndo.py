#!/usr/bin/env python

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

import unittest

import maya.cmds as cmds
import mayaUsd_createStageWithNewLayer

from pxr import Gf, Sdf, UsdGeom, UsdRender

import mayaUsd.lib as mayaUsdLib
from mayaUsd.lib import UsdDefaultRenderSettings


def _authorRenderSettings(stage):
    '''Author a minimal render-settings graph on stage.'''
    settings = UsdRender.Settings.Define(stage, Sdf.Path('/Render/Settings'))
    UsdGeom.Camera.Define(stage, Sdf.Path('/Camera'))
    return settings


def _mayaEditCommit(stage, doEdit, undoLabel='Edit'):
    '''Protocol-level simulation of MayaEditCommitter::commit() for testing.

    Exercises the USD undo-capture flow (trackLayerStates + UsdUndoBlock wrapped
    in a Maya undo chunk) without driving the C++ class directly, since
    MayaEditCommitter has no Python bindings.  Tests that use this helper cover
    the undo/redo correctness of the capture protocol; they do not exercise
    MayaEditCommitter-specific behaviour such as chunk-name sanitization or the
    isLocalEditInFlight counter.
    '''
    mayaUsdLib.UsdUndoManager.trackLayerStates(stage.GetEditTarget().GetLayer())
    chunkName = undoLabel.replace(' ', '_')
    cmds.undoInfo(openChunk=True, chunkName=chunkName)
    try:
        with mayaUsdLib.UsdUndoBlock():
            doEdit()
    finally:
        cmds.undoInfo(closeChunk=True)


class TestRenderSetupUndo(unittest.TestCase):
    '''Undo/redo for Render Setup-style USD edits on proxy-shape stages.

    Tests use _mayaEditCommit() to simulate the capture protocol.  Full
    integration coverage of MayaEditCommitter::commit() (chunk-name escaping,
    isLocalEditInFlight gating) requires C++ test infrastructure.
    '''

    @classmethod
    def setUpClass(cls):
        cmds.loadPlugin('mayaUsdPlugin')

    def setUp(self):
        cmds.file(force=True, new=True)
        cmds.select(clear=True)

    def _createProxyStage(self):
        shapeNode = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsdLib.GetPrim(shapeNode).GetStage()
        settings = _authorRenderSettings(stage)
        return stage, settings

    def testAttributeEditUndoRedo(self):
        stage, settings = self._createProxyStage()
        nbCmds = cmds.undoInfo(q=True)

        _mayaEditCommit(stage, lambda: settings.GetResolutionAttr().Set(Gf.Vec2i(1280, 720)))

        self.assertEqual(settings.GetResolutionAttr().Get(), Gf.Vec2i(1280, 720))
        self.assertEqual(cmds.undoInfo(q=True), nbCmds + 1)

        cmds.undo()
        self.assertFalse(settings.GetResolutionAttr().HasAuthoredValue())

        cmds.redo()
        self.assertEqual(settings.GetResolutionAttr().Get(), Gf.Vec2i(1280, 720))

    def testCameraRelationshipUndoRedo(self):
        stage, settings = self._createProxyStage()
        cameraPath = Sdf.Path('/Camera')

        _mayaEditCommit(stage, lambda: settings.GetCameraRel().SetTargets([cameraPath]))
        self.assertEqual(settings.GetCameraRel().GetTargets(), [cameraPath])

        cmds.undo()
        self.assertEqual(settings.GetCameraRel().GetTargets(), [])

        cmds.redo()
        self.assertEqual(settings.GetCameraRel().GetTargets(), [cameraPath])

    @unittest.skipUnless(
        hasattr(UsdDefaultRenderSettings, 'getUsdStage'),
        'Scene Render Settings stage requires MAYA_HAS_USD_SETTINGS_NODES')
    def testSceneRenderSettingsUntrackedEditNotOnMayaUndoQueue(self):
        '''A direct USD edit on the Scene Render Settings stage made without first
        calling trackLayerStates is not captured and does not push a Maya undo entry.

        This tests the untracked path.  When MayaEditCommitter is used and the
        settings stage is included in setStages(), trackStagesEditTargets() installs
        the delegate and the edit IS Maya-undoable.  That path is blocked today
        because UsdSceneSettingsManager does not yet feed the settings stage into
        the Render Setup host-stage list (see Known limitation in README-USD-Undo.md).
        '''
        stage = UsdDefaultRenderSettings.getUsdStage()
        settings = UsdDefaultRenderSettings.getDefaultRenderSettingsPrim()
        self.assertTrue(settings.IsValid())
        renderSettings = UsdRender.Settings(settings)

        nbCmds = cmds.undoInfo(q=True)

        # Edit without tracking — no delegate is installed, so nothing is captured.
        with mayaUsdLib.UsdUndoBlock():
            renderSettings.GetResolutionAttr().Set(Gf.Vec2i(640, 480))

        self.assertEqual(renderSettings.GetResolutionAttr().Get(), Gf.Vec2i(640, 480))
        # No undo entry should have been pushed.
        self.assertEqual(cmds.undoInfo(q=True), nbCmds)

        cmds.undo()
        # Value unchanged because nothing was on the undo queue.
        self.assertEqual(renderSettings.GetResolutionAttr().Get(), Gf.Vec2i(640, 480))
