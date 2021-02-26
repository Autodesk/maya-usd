#!/usr/bin/env python

#
# Copyright 2020 Autodesk
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
import tempfile
import unittest

from maya import cmds
from AL.usdmaya import ProxyShape

from pxr import Sdf, Usd

class TestLayerManagerSerialisation(unittest.TestCase):
    """Test cases for layer manager serialisation and deserialisation"""

    app = 'maya'
    def setUp(self):
        """Export some sphere geometry as .usda, and import into a new Maya scene."""

        cmds.file(force=True, new=True)
        cmds.loadPlugin("AL_USDMayaPlugin", quiet=True)
        self.assertTrue(cmds.pluginInfo("AL_USDMayaPlugin", query=True, loaded=True))

        _tmpfile = tempfile.NamedTemporaryFile(delete=False, suffix=".usda")
        _tmpfile.close()

        self._usdaFile = _tmpfile.name
        # Ensure sphere geometry exists
        self._sphere = cmds.polySphere(constructionHistory=False, name="sphere")[0]
        cmds.select(self._sphere)

        # Export, new scene, import
        cmds.file(self._usdaFile, exportSelected=True, force=True, type="AL usdmaya export")
        cmds.file(force=True, new=True)
        self._proxyName = cmds.AL_usdmaya_ProxyShapeImport(file=self._usdaFile)[0]

        # Ensure proxy exists
        self.assertIsNotNone(self._proxyName)

        # Store stage
        proxy = ProxyShape.getByName(self._proxyName)
        self._stage = proxy.getUsdStage()

    def tearDown(self):
        """Unload plugin, new Maya scene, reset class member variables."""

        cmds.file(force=True, new=True)
        cmds.unloadPlugin("AL_USDMayaPlugin", force=True)

        self._stage = None
        self._sphere = None
        self._proxyName = None
        if os.path.isfile(self._usdaFile):
            os.remove(self._usdaFile)

    def test_editTargetSerialisation(self):
        """ As long as we set an editTarget and it ends updirty, it should be serialised
            when we save Maya scene, and it should be reloadable by SdfLayer::Reload()
        """
        self.assertTrue(self._stage)
        self._stage.SetEditTarget(self._stage.GetRootLayer())
        newPrimPath = "/ChangeInRoot"
        self._stage.DefinePrim(newPrimPath, "xform")
        self._stage.SetEditTarget(self._stage.GetSessionLayer())

        _tmpMayafile = tempfile.NamedTemporaryFile(delete=True, suffix=".ma")
        _tmpMayafile.close()

        cmds.file(rename=_tmpMayafile.name)
        cmds.file(save=True, force=True)
        cmds.file(new=True, force=True)
        cmds.file(_tmpMayafile.name, open=True)
        self.assertIsNotNone(self._proxyName)
        ps = ProxyShape.getByName(self._proxyName)
        self.assertTrue(ps)
        stage = ps.getUsdStage()
        self.assertTrue(stage)
        self.assertTrue(stage.GetPrimAtPath(newPrimPath))
        self.assertTrue(stage.GetRootLayer().Reload())
        self.assertFalse(stage.GetPrimAtPath(newPrimPath))

        os.remove(_tmpMayafile.name)

    def test_sessionLayerSerialisation(self):
        """ A clean session layer should not be serialised on Maya scene save, nor we get
            any complain form usdMaya on Maya scene reopen.
            A dirty session layer should be serialised correctly on Maya scene save, and
            we should get it deserialised on Maya scene reopen. We should also be able to
            reload on session layer to clear it.
        """
        self.assertTrue(self._stage)
        self._stage.SetEditTarget(self._stage.GetSessionLayer())
        # Save the scene with clean session layer:
        _tmpMayafile = tempfile.NamedTemporaryFile(delete=True, suffix=".ma")
        _tmpMayafile.close()

        cmds.file(rename=_tmpMayafile.name)
        cmds.file(save=True, force=True)
        cmds.file(new=True, force=True)
        cmds.file(_tmpMayafile.name, open=True)
        self.assertFalse(cmds.getAttr('%s.sessionLayerName' % self._proxyName))
        os.remove(_tmpMayafile.name)

        ps = ProxyShape.getByName(self._proxyName)
        self.assertTrue(ps)
        stage = ps.getUsdStage()
        # Make session layer dirty and save the scene:
        stage.SetEditTarget(stage.GetSessionLayer())
        newPrimPath = "/ChangeInSession"
        stage.DefinePrim(newPrimPath, "xform")
        _tmpMayafile = tempfile.NamedTemporaryFile(delete=True, suffix=".ma")
        _tmpMayafile.close()

        cmds.file(rename=_tmpMayafile.name)
        cmds.file(save=True, force=True)
        cmds.file(new=True, force=True)
        cmds.file(_tmpMayafile.name, open=True)
        self.assertTrue(cmds.getAttr('%s.sessionLayerName' % self._proxyName))

        ps = ProxyShape.getByName(self._proxyName)
        self.assertTrue(ps)
        stage = ps.getUsdStage()
        self.assertTrue(stage)
        self.assertTrue(stage.GetPrimAtPath(newPrimPath))
        self.assertTrue(stage.GetSessionLayer().Reload())
        self.assertFalse(stage.GetPrimAtPath(newPrimPath))

        os.remove(_tmpMayafile.name)


    def test_multipleFormatsSerialisation(self):
        """Tests multiple dirty layers with various formats can be saved and restored.

        """
        _, rootLayerPath = tempfile.mkstemp(suffix=".usda")
        _, tmpMayafilePath = tempfile.mkstemp(suffix=".ma")

        def _initialiseLayers(rootLayerPath):
            cmds.file(new=True, force=True)
            rootLayer = Sdf.Layer.CreateNew(rootLayerPath)
            subLayerPaths = rootLayer.subLayerPaths
            layers = []
            for ext in ["usd", "usda", "usdc"]:
                _format = Sdf.FileFormat.FindByExtension(ext)
                _, filePath = tempfile.mkstemp(suffix=".{}".format(ext))
                layer = Sdf.Layer.CreateNew(filePath)
                layer.comment = ext
                layer.Save()
                layers.append(layer)
                subLayerPaths.append(filePath)

            rootLayer.Save()

        def _buildAndEditAndSaveScene(rootLayerPath):
            """Add edits to the 3 format layers and scene the maya scene.

            """
            cmds.file(new=True, force=True)
            cmds.file(rename=tmpMayafilePath)
            proxyName = cmds.AL_usdmaya_ProxyShapeImport(file=rootLayerPath)[0]
            ps = ProxyShape.getByName(proxyName)
            self.assertTrue(ps)
            stage = ps.getUsdStage()
            self.assertTrue(stage)

            rootLayer = stage.GetRootLayer()
            for subLayerPath in rootLayer.subLayerPaths:
                layer = Sdf.Layer.Find(subLayerPath)
                with Usd.EditContext(stage, layer):
                    stage.DefinePrim("/{}".format(layer.GetFileFormat().primaryFileExtension))
            self.assertTrue(stage.GetSessionLayer().empty)

            cmds.file(save=True, force=True)
            return proxyName

        def _reloadAndAssert(rootLayerPath, proxyName):
            """Assert the edits have been restored
            
            """
            cmds.file(new=True, force=True)
            cmds.file(tmpMayafilePath, open=True)

            ps = ProxyShape.getByName(proxyName)
            self.assertTrue(ps)
            stage = ps.getUsdStage()
            self.assertTrue(stage)
            # root + session + 3 format layers
            self.assertEqual(len(stage.GetUsedLayers()), 5)

            for ext in ["usd", "usda", "usdc"]:
                prim = stage.GetPrimAtPath("/{}".format(ext))
                self.assertTrue(prim.IsValid())

            stage.Reload()

        _initialiseLayers(rootLayerPath)
        proxyName = _buildAndEditAndSaveScene(rootLayerPath)
        _reloadAndAssert(rootLayerPath, proxyName)


if __name__ == "__main__":

    tests = [unittest.TestLoader().loadTestsFromTestCase(TestLayerManagerSerialisation),]
    results = [unittest.TextTestRunner(verbosity=2).run(test) for test in tests]
    exitCode = int(not all([result.wasSuccessful() for result in results]))
    cmds.quit(exitCode=(exitCode))
