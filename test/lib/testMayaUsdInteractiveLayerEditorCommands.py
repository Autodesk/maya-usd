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

import os
import platform
import unittest

import testUtils, mayaUtils

import mayaUsd.lib as mayaUsdLib
import mayaUsd.ufe as mayaUsdUfe

import ufe
from maya import cmds
import maya.mel as mel
import fixturesUtils

class MayaUsdInteractiveLayerEditorCommandsTestCase(unittest.TestCase):
    """Test interactive commands that need the UI of the layereditor."""

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__, initializeStandalone=False, loadPlugin=False)
        cls._baselineDir = os.path.join(inputPath,'VP2RenderDelegatePrimPathTest', 'baseline')
        cls._testDir = os.path.abspath('.')
        cmds.loadPlugin('mayaUsdPlugin')

    def samefile(self, path1, path2):
        if platform.system() == 'Windows':
            return os.path.normcase(os.path.normpath(path1)) == os.path.normcase(os.path.normpath(path2))
        else:
            return os.path.samefile(path1, path2)

    def testCreateStageFromFileWithInvalidUsd(self):
        # We cannot directly call the 'mayaUsdCreateStageFromFile'
        # as it opens a file dialog to choose the scene. So instead
        # we can call what it does once the file is choose.
        # Note: on ballFilePath we replace \ with / to stop the \ as
        #       being interpreted.
        mel.eval('mayaUsdLayerEditorWindow mayaUsdLayerEditor')

        ballFilePath = os.path.normpath(testUtils.getTestScene('ballset', 'StandaloneScene', 'invalid_layer.usda')).replace('\\', '/')
        mel.eval('source \"mayaUsd_createStageFromFile.mel\"')
        shapeNode = mel.eval('mayaUsd_createStageFromFilePath(\"'+ballFilePath+'\")')
        mayaSel = cmds.ls(sl=True)
        self.assertEqual(1, len(mayaSel))
        nt = cmds.nodeType(shapeNode)
        self.assertEqual('mayaUsdProxyShape', nt)

        # Verify that the shape node has the correct file path.
        filePathAttr = cmds.getAttr(shapeNode+'.filePath')
        self.assertTrue(self.samefile(filePathAttr, ballFilePath))

        # Verify that the shape node is connected to time.
        self.assertTrue(cmds.isConnected('time1.outTime', shapeNode+'.time'))

    def test_mayaUsdLayerEditorWindowCommand(self):
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsdLib.GetPrim(proxyShape).GetStage()

        # Create a ContextOps interface for the proxy shape.
        proxyPathSegment = mayaUtils.createUfePathSegment(proxyShape)
        proxyShapePath = ufe.Path([proxyPathSegment])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        # Set the optionVar to show the layer editor.
        cmds.optionVar(intValue=('MayaUSDLayerEditor_AutoHideSessionLayer', 0))

        def makeSureLayerEditorIsReady():
            cmds.refresh(force=True)
            import time
            time.sleep(0.1)

        # Verify that the Layer Editor window can be opened with stage.
        cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', proxyShape=proxyShape)
        makeSureLayerEditorIsReady()

        # Make sure the Layer Editor is showing the ProxyShape we asked.
        leProxyShape = cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', q=True, proxyShape=True)
        self.assertEqual(proxyShape, leProxyShape)

        def selectLayerAndVerify(layerId):
            """Helper function to select the input layer and then verify the selection."""
            cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', edit=True, setSelectedLayers=layerId)
            self.assertEqual(1, cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, selectionLength=True))

            selLayers = cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, getSelectedLayers=True)
            self.assertEqual(1, len(selLayers))
            self.assertEqual(layerId, selLayers[0])

        # Select the root layer.
        self.assertEqual(0, cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, selectionLength=True))
        rootLayerId = stage.GetRootLayer().identifier
        selectLayerAndVerify(rootLayerId)

        # Verify properties of this empty anonymous root layer.
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, isInvalidLayer=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, isSessionLayer=True))
        self.assertTrue(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, isAnonymousLayer=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, isLayerDirty=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, isSubLayer=True))
        self.assertTrue(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerNeedsSaving=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerAppearsMuted=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsMuted=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsReadOnly=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerAppearsLocked=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsLocked=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerAppearsSystemLocked=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsSystemLocked=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerHasSubLayers=True))

        # Select the session layer.
        sessionLayerId = stage.GetSessionLayer().identifier
        selectLayerAndVerify(sessionLayerId)

        # Verify properties of the session layer.
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, isInvalidLayer=True))
        self.assertTrue(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, isSessionLayer=True))
        self.assertTrue(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, isAnonymousLayer=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, isLayerDirty=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, isSubLayer=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerNeedsSaving=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerAppearsMuted=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsMuted=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsReadOnly=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerAppearsLocked=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsLocked=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerAppearsSystemLocked=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsSystemLocked=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerHasSubLayers=True))

        # Add a new sublayer to the root layer.
        selectLayerAndVerify(rootLayerId)
        cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', edit=True, addAnonymousSublayer=True)
        makeSureLayerEditorIsReady()
        newSubLayerId = stage.GetRootLayer().subLayerPaths[0]
        selectLayerAndVerify(newSubLayerId)

        # Verify properties of this empty anonymous sub layer.
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, isInvalidLayer=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, isSessionLayer=True))
        # Layer is anonymous, but not dirty.
        self.assertTrue(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, isAnonymousLayer=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, isLayerDirty=True))
        self.assertTrue(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, isSubLayer=True))
        self.assertTrue(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerNeedsSaving=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerAppearsMuted=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsMuted=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsReadOnly=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerAppearsLocked=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsLocked=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerAppearsSystemLocked=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsSystemLocked=True))
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerHasSubLayers=True))

        # Mute/unmute the layer and test it.
        cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', edit=True, muteLayer=True)
        self.assertTrue(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsMuted=True))
        cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', edit=True, muteLayer=True)
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsMuted=True))

        # Lock/unlock the layer and test it.
        cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', edit=True, lockLayer=True)
        self.assertTrue(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsLocked=True))
        cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', edit=True, lockLayer=True)
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsLocked=True))

        # Lock the root layer and all sublayers and test that.
        selectLayerAndVerify(rootLayerId)
        cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', edit=True, lockLayerAndSubLayers=True)
        self.assertTrue(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsLocked=True))
        selectLayerAndVerify(newSubLayerId)
        self.assertTrue(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsLocked=True))

        # Unlock the root layer and all sublayers and test that.
        selectLayerAndVerify(rootLayerId)
        cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', edit=True, lockLayerAndSubLayers=True)
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsLocked=True))
        selectLayerAndVerify(newSubLayerId)
        self.assertFalse(cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', query=True, layerIsLocked=True))

        # Set the edit target to the new sublayer and verify that.
        cmds.mayaUsdEditTarget(proxyShape, edit=True, editTarget=newSubLayerId)
        self.assertEqual(newSubLayerId, stage.GetEditTarget().GetLayer().identifier)

        # Add a prim to the targetted layer.
        # Note: the prim is auto-selected.
        proxyShapeContextOps.doOp(['Add New Prim', 'Capsule'])
        capsulePath = ufe.GlobalSelection.get().front().path()

        # Clear the Maya selection (to deselect Capsule) and the run the command to select prims with spec.
        cmds.select(clear=True)
        self.assertFalse(ufe.GlobalSelection.get().contains(capsulePath))
        selectLayerAndVerify(newSubLayerId)
        try:
            cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', edit=True, selectPrimsWithSpec=True)
        except:
            self.fail("Failed to select prims with spec.")
        self.assertTrue(ufe.GlobalSelection.get().contains(capsulePath))

        # Clear the layer which in turn will delete the capsule.
        cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', edit=True, clearLayer=True)
        self.assertFalse(ufe.GlobalSelection.get().contains(capsulePath))
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)
        self.assertIsNone(capsuleItem)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())