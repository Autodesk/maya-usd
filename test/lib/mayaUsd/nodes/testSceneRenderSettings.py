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

from maya import cmds
from maya import standalone

import mayaUsd.lib as mayaUsdLib
from mayaUsd.lib import UsdDefaultRenderSettings

from pxr import Sdf, Usd, UsdGeom, UsdRender

import fixturesUtils

import os
import shutil
import tempfile
import unittest


class testSceneRenderSettings(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    # ------------------------------------------------------------------
    # Node existence and singleton
    # ------------------------------------------------------------------

    def testNodeExistsOnStartup(self):
        '''The singleton node should exist after plugin load.'''
        path = UsdDefaultRenderSettings.find()
        self.assertTrue(len(path) > 0, "UsdDefaultRenderSettings node not found")
        self.assertTrue(cmds.objExists(path))

    def testSingleton(self):
        '''find called twice should return the same node; only one instance exists.'''
        path1 = UsdDefaultRenderSettings.find()
        path2 = UsdDefaultRenderSettings.find()
        self.assertEqual(path1, path2)

        nodes = cmds.ls(type='UsdDefaultSettings')
        self.assertEqual(len(nodes), 1,
                         "Expected exactly one UsdDefaultRenderSettings node, "
                         "found %d" % len(nodes))

    # ------------------------------------------------------------------
    # Default stage structure
    # ------------------------------------------------------------------

    def testDefaultStageStructure(self):
        '''The default stage should have /Render scope and /Render/SceneRenderSettings prim.'''
        stage = UsdDefaultRenderSettings.getUsdStage()
        self.assertIsNotNone(stage)

        renderPrim = stage.GetPrimAtPath('/Render')
        self.assertTrue(renderPrim.IsValid(), "/Render prim not found")
        self.assertTrue(renderPrim.IsA(UsdGeom.Scope))

        settingsPrim = stage.GetPrimAtPath('/Render/SceneRenderSettings')
        self.assertTrue(settingsPrim.IsValid(),
                        "/Render/SceneRenderSettings prim not found")
        self.assertTrue(settingsPrim.IsA(UsdRender.Settings))

    def testRenderSettingsPrimPathMetadata(self):
        '''Stage metadata should point to the default render settings prim.'''
        stage = UsdDefaultRenderSettings.getUsdStage()
        metadata = stage.GetMetadata('renderSettingsPrimPath')
        self.assertEqual(metadata, '/Render/SceneRenderSettings')

    # ------------------------------------------------------------------
    # Node properties
    # ------------------------------------------------------------------

    def testNodeIsLocked(self):
        '''The singleton node should be locked.'''
        nodeName = UsdDefaultRenderSettings.find()
        self.assertTrue(cmds.lockNode(nodeName, query=True, lock=True)[0],
                        "Node should be locked")

    def testNodeIsDG(self):
        '''The singleton should be a DG node with no parent transform.'''
        nodeName = UsdDefaultRenderSettings.find()
        parents = cmds.listRelatives(nodeName, parent=True, fullPath=True)
        self.assertIsNone(parents,
                          "DG node should have no parent transform")

    # ------------------------------------------------------------------
    # Stage access
    # ------------------------------------------------------------------

    def testGetUsdStageConsistency(self):
        '''getUsdStage should return the same stage on repeated calls.'''
        stage1 = UsdDefaultRenderSettings.getUsdStage()
        stage2 = UsdDefaultRenderSettings.getUsdStage()
        self.assertIsNotNone(stage1)
        # Same stage object, not just same identifier.
        self.assertEqual(stage1, stage2)

    def testGetDefaultRenderSettingsPrim(self):
        '''getDefaultRenderSettingsPrim should return the /Render/SceneRenderSettings prim.'''
        prim = UsdDefaultRenderSettings.getDefaultRenderSettingsPrim()
        self.assertTrue(prim.IsValid())
        self.assertEqual(prim.GetPath().pathString,
                         '/Render/SceneRenderSettings')
        self.assertTrue(prim.IsA(UsdRender.Settings))

    # ------------------------------------------------------------------
    # Node recreation on file new
    # ------------------------------------------------------------------

    def testNodeRecreatedAfterFileNew(self):
        '''The singleton should be recreated (new MObject) after file new.'''
        pathBefore = UsdDefaultRenderSettings.find()
        self.assertTrue(len(pathBefore) > 0)
        uuidBefore = cmds.ls(pathBefore, uuid=True)[0]

        cmds.file(new=True, force=True)

        pathAfter = UsdDefaultRenderSettings.find()
        self.assertTrue(len(pathAfter) > 0,
                        "Node should be recreated after file new")
        uuidAfter = cmds.ls(pathAfter, uuid=True)[0]
        self.assertNotEqual(uuidBefore, uuidAfter,
                            "Singleton must be a new MObject after File > New, "
                            "not the same node carried over")

        # The new stage should have the default structure.
        stage = UsdDefaultRenderSettings.getUsdStage()
        self.assertTrue(
            stage.GetPrimAtPath('/Render/SceneRenderSettings').IsValid())

    # ------------------------------------------------------------------
    # Serialization round-trip
    # ------------------------------------------------------------------

    def testSerializationRoundTrip(self):
        '''Stage content should survive a save/open cycle.'''
        stage = UsdDefaultRenderSettings.getUsdStage()

        # Author a prim directly on the stage.
        UsdGeom.Xform.Define(stage, '/Render/TestContent')

        # Save.
        tmpDir = tempfile.mkdtemp(prefix='testSceneRenderSettings_')
        try:
            tmpFile = os.path.join(tmpDir, 'testScene.ma')
            cmds.file(rename=tmpFile)
            cmds.file(save=True, type='mayaAscii')

            # Re-open.
            cmds.file(tmpFile, open=True, force=True)

            # Verify.
            stage2 = UsdDefaultRenderSettings.getUsdStage()
            self.assertIsNotNone(stage2)

            self.assertTrue(
                stage2.GetPrimAtPath('/Render/SceneRenderSettings').IsValid(),
                "Default render settings prim should survive serialization")
            self.assertTrue(
                stage2.GetPrimAtPath('/Render/TestContent').IsValid(),
                "Authored content should survive serialization")
        finally:
            cmds.file(new=True, force=True)
            shutil.rmtree(tmpDir, ignore_errors=True)

    # ------------------------------------------------------------------
    # Referencing a scene that contains the singleton
    # ------------------------------------------------------------------

    def testReferencedSceneDoesNotBreakLocalSingleton(self):
        '''Referencing a Maya file that contains a UsdDefaultRenderSettings node
        must not replace or break the current scene's singleton.'''
        refDir = tempfile.mkdtemp(prefix='testSceneRenderSettings_')
        try:
            # Author a marker prim so we can distinguish local vs referenced stage.
            localStage = UsdDefaultRenderSettings.getUsdStage()
            UsdGeom.Xform.Define(localStage, '/Render/LocalMarker')

            localPath = UsdDefaultRenderSettings.find()
            self.assertTrue(len(localPath) > 0)

            # Save the current scene so we can reference it later.
            refFile = os.path.join(refDir, 'referenced.ma')
            cmds.file(rename=refFile)
            cmds.file(save=True, type='mayaAscii')

            # Start a fresh scene (creates a new local singleton).
            cmds.file(new=True, force=True)

            localPathNew = UsdDefaultRenderSettings.find()
            self.assertTrue(len(localPathNew) > 0)

            localStageNew = UsdDefaultRenderSettings.getUsdStage()
            self.assertIsNotNone(localStageNew)

            # The fresh scene should NOT have the marker from the saved file.
            self.assertFalse(
                localStageNew.GetPrimAtPath('/Render/LocalMarker').IsValid(),
                "Fresh scene should not have marker prim before referencing")

            # Reference the saved scene.
            cmds.file(refFile, reference=True, namespace='ref')

            # The local singleton should still be the non-referenced node.
            pathAfterRef = UsdDefaultRenderSettings.find()
            self.assertTrue(len(pathAfterRef) > 0,
                            "Local singleton should still exist after referencing")
            self.assertFalse(
                cmds.referenceQuery(pathAfterRef, isNodeReferenced=True),
                "find() must return the local singleton, not the referenced one")

            # The referenced file must bring its own UsdDefaultRenderSettings
            # node into the scene; verify total count is exactly 2 (one local
            # + one referenced) so a regression where the reference fails to
            # carry the node still trips this test.
            allNodes = cmds.ls(type='UsdDefaultSettings', long=True)
            self.assertEqual(len(allNodes), 2,
                             "Referencing should bring in one settings node from "
                             "the file (local + referenced), got %d" % len(allNodes))
            localNodes = [n for n in allNodes
                          if not cmds.referenceQuery(n, isNodeReferenced=True)]
            self.assertEqual(len(localNodes), 1,
                             "Expected exactly one local UsdDefaultRenderSettings, "
                             "found %d" % len(localNodes))

            # The local stage must still have the default render settings.
            stageAfterRef = UsdDefaultRenderSettings.getUsdStage()
            self.assertIsNotNone(stageAfterRef)
            self.assertTrue(
                stageAfterRef.GetPrimAtPath('/Render/SceneRenderSettings').IsValid(),
                "Local render settings prim should not be broken by referencing")

            # The local stage must NOT have the marker from the referenced file.
            self.assertFalse(
                stageAfterRef.GetPrimAtPath('/Render/LocalMarker').IsValid(),
                "Referenced file's stage data should not leak into local singleton")
        finally:
            cmds.file(new=True, force=True)
            shutil.rmtree(refDir, ignore_errors=True)

    # ------------------------------------------------------------------
    # Regression: save without ever materializing the stage
    # ------------------------------------------------------------------

    def testSaveWithoutMaterializingStage(self):
        '''A scene saved without ever materializing the stage must still
        restore populator content on reopen. Regression for the explicit
        force-the-populator branch in UsdSettingsNode::serializeToAttributes:
        without it, empty default-string plugs would be saved and reopen
        would yield an empty stage missing /Render/SceneRenderSettings.

        Important: this test must NOT call find()/getUsdStage() before save,
        otherwise it short-circuits the very path it is exercising.
        '''
        tmpDir = tempfile.mkdtemp(prefix='testSceneRenderSettings_')
        try:
            tmpFile = os.path.join(tmpDir, 'savedWithoutMaterialize.ma')
            cmds.file(rename=tmpFile)
            cmds.file(save=True, type='mayaAscii')
            cmds.file(new=True, force=True)
            cmds.file(tmpFile, open=True, force=True)

            stage = UsdDefaultRenderSettings.getUsdStage()
            self.assertIsNotNone(stage)
            self.assertTrue(
                stage.GetPrimAtPath('/Render/SceneRenderSettings').IsValid(),
                "Populator content should be present after open even when "
                "the stage was never materialized before save")
        finally:
            cmds.file(new=True, force=True)
            shutil.rmtree(tmpDir, ignore_errors=True)

    # ------------------------------------------------------------------
    # Session-layer round-trip
    # ------------------------------------------------------------------

    def testSessionLayerRoundTrip(self):
        '''Edits authored on the session layer must also survive save/open;
        serializeToAttributes writes both root and session layers but
        testSerializationRoundTrip only exercises the root path.'''
        stage = UsdDefaultRenderSettings.getUsdStage()
        with Usd.EditContext(stage, stage.GetSessionLayer()):
            UsdGeom.Xform.Define(stage, '/Render/SessionMarker')
        # Sanity: confirm the prim is present before save.
        self.assertTrue(
            stage.GetPrimAtPath('/Render/SessionMarker').IsValid(),
            "Session-layer prim should exist before save")

        tmpDir = tempfile.mkdtemp(prefix='testSceneRenderSettings_')
        try:
            tmpFile = os.path.join(tmpDir, 'sessionRoundTrip.ma')
            cmds.file(rename=tmpFile)
            cmds.file(save=True, type='mayaAscii')
            cmds.file(new=True, force=True)
            cmds.file(tmpFile, open=True, force=True)

            stage2 = UsdDefaultRenderSettings.getUsdStage()
            self.assertIsNotNone(stage2)
            self.assertTrue(
                stage2.GetPrimAtPath('/Render/SessionMarker').IsValid(),
                "Session-layer prim should survive serialization")
        finally:
            cmds.file(new=True, force=True)
            shutil.rmtree(tmpDir, ignore_errors=True)

    # ------------------------------------------------------------------
    # Active settings UFE path
    # ------------------------------------------------------------------

    def testActiveSettingsPathDefault(self):
        '''Default activeSettingsPath references the singleton's default prim.'''
        nodeName = UsdDefaultRenderSettings.find()
        activePath = UsdDefaultRenderSettings.getActiveRenderSettingsPath()
        self.assertTrue(len(activePath) > 0)
        self.assertIn(nodeName, activePath)
        self.assertIn('/Render/SceneRenderSettings', activePath)

        self.assertEqual(
            cmds.getAttr(nodeName + '.activeSettingsPath'), activePath)

    def testActiveSettingsPathSetter(self):
        '''Writing through the helper updates every read path.'''
        newPath = '|SomeOtherStage,/Foo/Bar'
        self.assertTrue(
            UsdDefaultRenderSettings.setActiveRenderSettingsPath(newPath))

        self.assertEqual(
            UsdDefaultRenderSettings.getActiveRenderSettingsPath(), newPath)
        nodeName = UsdDefaultRenderSettings.find()
        self.assertEqual(
            cmds.getAttr(nodeName + '.activeSettingsPath'), newPath)

    def testActiveSettingsPathRoundTrip(self):
        '''A custom activeSettingsPath survives a save/open cycle.'''
        customPath = '|RefStage,/Custom/RenderSettings'
        self.assertTrue(
            UsdDefaultRenderSettings.setActiveRenderSettingsPath(customPath))

        tmpDir = tempfile.mkdtemp(prefix='testSceneRenderSettings_')
        try:
            tmpFile = os.path.join(tmpDir, 'activeSettingsPathRoundTrip.ma')
            cmds.file(rename=tmpFile)
            cmds.file(save=True, type='mayaAscii')
            cmds.file(new=True, force=True)
            cmds.file(tmpFile, open=True, force=True)

            self.assertEqual(
                UsdDefaultRenderSettings.getActiveRenderSettingsPath(),
                customPath)
        finally:
            cmds.file(new=True, force=True)
            shutil.rmtree(tmpDir, ignore_errors=True)

    def testActiveSettingsPathResetOnFileNew(self):
        '''File > New restores the populator-authored default value.'''
        defaultPath = UsdDefaultRenderSettings.getActiveRenderSettingsPath()
        self.assertTrue(len(defaultPath) > 0)

        customPath = '|Whatever,/Some/Override'
        self.assertTrue(
            UsdDefaultRenderSettings.setActiveRenderSettingsPath(customPath))
        self.assertEqual(
            UsdDefaultRenderSettings.getActiveRenderSettingsPath(), customPath)

        cmds.file(new=True, force=True)

        self.assertEqual(
            UsdDefaultRenderSettings.getActiveRenderSettingsPath(),
            defaultPath)

    # ------------------------------------------------------------------
    # Locked-node invariant
    # ------------------------------------------------------------------

    def testLockedNodeProtected(self):
        '''The locked singleton must not be deletable or renamable through
        cmds; its DG name is the manager's primary lookup key, so any drift
        would silently break find()/getStage() dispatch.'''
        nodeName = UsdDefaultRenderSettings.find()
        uuidBefore = cmds.ls(nodeName, uuid=True)[0]

        # Tolerate either an exception (lock raises) or a silent no-op,
        # but the node must survive the attempt unchanged.
        try:
            cmds.delete(nodeName)
        except RuntimeError:
            pass
        self.assertTrue(cmds.objExists(nodeName),
                        "Locked node must survive a delete attempt")
        self.assertEqual(uuidBefore, cmds.ls(nodeName, uuid=True)[0],
                         "Locked node must remain the same MObject after delete")

        try:
            cmds.rename(nodeName, 'someOtherName')
        except RuntimeError:
            pass
        self.assertTrue(cmds.objExists(nodeName),
                        "Locked node must keep its name after a rename attempt")
        self.assertEqual(uuidBefore, cmds.ls(nodeName, uuid=True)[0],
                         "Locked node must remain the same MObject after rename")


    # ------------------------------------------------------------------
    # External camera attribute (adskUsd:externalCamera)
    # ------------------------------------------------------------------

    EXTERNAL_CAMERA_ATTR = 'adskUsd:externalCamera'
    DEFAULT_EXTERNAL_CAMERA = '|persp'

    def _createUsdProxyWithCamera(self, cameraPrimPath='/cameras/cam1'):
        '''Create a Maya proxy shape backed by an in-memory stage that
        contains a UsdGeomCamera at cameraPrimPath. Returns
        (proxyShapeNode, stage, cameraUfePath) where cameraUfePath is the
        user-facing UFE form '|proxyParent|proxyShape,/prim'.'''
        cmds.createNode('mayaUsdProxyShape', name='extCamProxyShape')
        shapeNode = cmds.ls(sl=True, long=True)[0]
        cmds.select(clear=True)
        cmds.connectAttr('time1.outTime', '{}.time'.format(shapeNode))
        stage = mayaUsdLib.GetPrim(shapeNode).GetStage()
        UsdGeom.Camera.Define(stage, cameraPrimPath)
        cameraUfePath = '{}{}'.format(shapeNode, cameraPrimPath)
        return shapeNode, stage, cameraUfePath

    def testExternalCameraDefaultValue(self):
        '''The singleton's default render-settings prim is pre-populated with
        adskUsd:externalCamera = "|persp" and has no schema camera
        relationship targets.'''
        prim = UsdDefaultRenderSettings.getDefaultRenderSettingsPrim()
        self.assertTrue(prim.IsValid())

        attr = prim.GetAttribute(self.EXTERNAL_CAMERA_ATTR)
        self.assertTrue(attr.IsValid(),
                        '%s should be authored on the default prim'
                        % self.EXTERNAL_CAMERA_ATTR)
        self.assertEqual(attr.Get(), self.DEFAULT_EXTERNAL_CAMERA)

        cameraRel = UsdRender.Settings(prim).GetCameraRel()
        self.assertEqual(cameraRel.GetTargets(), [])

    def testExternalCameraAttrName(self):
        '''The Python binding exposes the canonical attribute name.'''
        self.assertEqual(UsdDefaultRenderSettings.externalCameraAttrName(),
                         self.EXTERNAL_CAMERA_ATTR)

    def testSetCameraSameStage(self):
        '''Setting a camera on the same stage authors the schema relationship
        and removes any pre-existing external-camera attribute.'''
        stage = UsdDefaultRenderSettings.getUsdStage()
        cameraPrim = UsdGeom.Camera.Define(stage, '/Render/Cam1').GetPrim()

        prim = UsdDefaultRenderSettings.getDefaultRenderSettingsPrim()
        self.assertTrue(prim.HasAttribute(self.EXTERNAL_CAMERA_ATTR))

        self.assertTrue(
            UsdDefaultRenderSettings.setCamera(prim, cameraPrim.GetPath().pathString))

        self.assertFalse(
            prim.HasAttribute(self.EXTERNAL_CAMERA_ATTR),
            'External camera attribute should be removed when a same-stage '
            'camera is set')
        self.assertEqual(
            UsdRender.Settings(prim).GetCameraRel().GetTargets(),
            [Sdf.Path('/Render/Cam1')])

    def testSetCameraMayaNative(self):
        '''Setting a Maya native camera UFE path authors the external
        attribute and clears the schema relationship targets.'''
        stage = UsdDefaultRenderSettings.getUsdStage()
        cameraPrim = UsdGeom.Camera.Define(stage, '/Render/Cam1').GetPrim()
        prim = UsdDefaultRenderSettings.getDefaultRenderSettingsPrim()

        # Seed the relationship so the assertion below confirms it is cleared
        # by the subsequent setCamera call to a Maya-native path.
        UsdDefaultRenderSettings.setCamera(prim, cameraPrim.GetPath().pathString)
        self.assertEqual(
            UsdRender.Settings(prim).GetCameraRel().GetTargets(),
            [Sdf.Path('/Render/Cam1')])

        self.assertTrue(
            UsdDefaultRenderSettings.setCamera(prim, '|persp|perspShape'))

        self.assertEqual(
            UsdRender.Settings(prim).GetCameraRel().GetTargets(), [])
        attr = prim.GetAttribute(self.EXTERNAL_CAMERA_ATTR)
        self.assertTrue(attr.IsValid())
        self.assertEqual(attr.Get(), '|persp|perspShape')

    def testSetCameraDifferentStage(self):
        '''A UsdGeomCamera on a different stage routes through the external
        attribute, not the schema relationship.'''
        proxyShape, _proxyStage, cameraUfePath = self._createUsdProxyWithCamera()

        prim = UsdDefaultRenderSettings.getDefaultRenderSettingsPrim()
        self.assertTrue(UsdDefaultRenderSettings.setCamera(prim, cameraUfePath))

        self.assertEqual(
            UsdRender.Settings(prim).GetCameraRel().GetTargets(), [])
        attr = prim.GetAttribute(self.EXTERNAL_CAMERA_ATTR)
        self.assertTrue(attr.IsValid())
        self.assertEqual(attr.Get(), cameraUfePath)

    def testSetCameraInvalidPrim(self):
        '''Invalid prim or non-RenderSettings prim returns False.'''
        invalidPrim = Usd.Prim()
        self.assertFalse(
            UsdDefaultRenderSettings.setCamera(invalidPrim, '|persp|perspShape'))

        stage = UsdDefaultRenderSettings.getUsdStage()
        nonSettings = UsdGeom.Xform.Define(stage, '/Render/NotSettings').GetPrim()
        self.assertFalse(
            UsdDefaultRenderSettings.setCamera(nonSettings, '|persp|perspShape'))

    def testSetCameraRoundTrip(self):
        '''An external camera value survives a save/open cycle.'''
        prim = UsdDefaultRenderSettings.getDefaultRenderSettingsPrim()
        customCameraPath = '|some|custom|cameraShape'
        self.assertTrue(
            UsdDefaultRenderSettings.setCamera(prim, customCameraPath))

        tmpDir = tempfile.mkdtemp(prefix='testSceneRenderSettings_')
        try:
            tmpFile = os.path.join(tmpDir, 'externalCameraRoundTrip.ma')
            cmds.file(rename=tmpFile)
            cmds.file(save=True, type='mayaAscii')
            cmds.file(new=True, force=True)
            cmds.file(tmpFile, open=True, force=True)

            primAfter = UsdDefaultRenderSettings.getDefaultRenderSettingsPrim()
            attr = primAfter.GetAttribute(self.EXTERNAL_CAMERA_ATTR)
            self.assertTrue(attr.IsValid())
            self.assertEqual(attr.Get(), customCameraPath)
        finally:
            cmds.file(new=True, force=True)
            shutil.rmtree(tmpDir, ignore_errors=True)

    def testExternalCameraDefaultRestoredOnFileNew(self):
        '''File > New restores the populator-authored default value, even
        after a prior override on the singleton's prim.'''
        prim = UsdDefaultRenderSettings.getDefaultRenderSettingsPrim()
        self.assertTrue(
            UsdDefaultRenderSettings.setCamera(prim, '|some|other|cameraShape'))
        self.assertEqual(
            prim.GetAttribute(self.EXTERNAL_CAMERA_ATTR).Get(),
            '|some|other|cameraShape')

        cmds.file(new=True, force=True)

        primAfter = UsdDefaultRenderSettings.getDefaultRenderSettingsPrim()
        self.assertEqual(
            primAfter.GetAttribute(self.EXTERNAL_CAMERA_ATTR).Get(),
            self.DEFAULT_EXTERNAL_CAMERA)


if __name__ == '__main__':
    unittest.main(verbosity=2)
