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
import shutil
import unittest

from maya import cmds
from AL.usdmaya import ProxyShape

from pxr import Sdf, Usd

class TestLayerManagerSerialisation(unittest.TestCase):
    """Test cases for layer manager serialisation and deserialisation"""

    @classmethod
    def setUpClass(cls):
        # Reset the output directory
        cls._outputPath = os.path.join(os.path.abspath('.'), os.path.splitext(__file__)[0] + "Output")
        if os.path.exists(cls._outputPath):
            shutil.rmtree(cls._outputPath)
        os.mkdir(cls._outputPath)
        cls._mayaFilePath = os.path.join(cls._outputPath, 'LayerManagerSerialisation.ma')
        cls._usdFilePath = os.path.join(cls._outputPath, 'LayerManagerSerialisation.usda')

        # Load the plugin
        cmds.loadPlugin("AL_USDMayaPlugin", quiet=True)

    @classmethod
    def tearDownClass(cls):
        # Unload the plugin
        cmds.unloadPlugin("AL_USDMayaPlugin", force=True)

    app = 'maya'
    def setUp(self):
        self.assertTrue(cmds.pluginInfo("AL_USDMayaPlugin", query=True, loaded=True))
        
        # Clear the maya file
        cmds.file(self._mayaFilePath, force=True, new=True)

        # Ensure sphere geometry exists
        self._sphere = cmds.polySphere(constructionHistory=False, name="sphere")[0]
        cmds.select(self._sphere)

        # Export, new scene, import
        cmds.file(self._usdFilePath, exportSelected=True, force=True, type="AL usdmaya export")
        cmds.file(force=True, new=True)
        self._proxyName = cmds.AL_usdmaya_ProxyShapeImport(file=self._usdFilePath)[0]

        # Ensure proxy exists
        self.assertIsNotNone(self._proxyName)

        # Store stage
        proxy = ProxyShape.getByName(self._proxyName)
        self._stage = proxy.getUsdStage()

    def tearDown(self):
        """New Maya scene, reset class member variables."""

        cmds.file(force=True, new=True)

        self._stage = None
        self._sphere = None
        self._proxyName = None

    def test_editTargetSerialisation(self):
        """ As long as we set an editTarget and it ends updirty, it should be serialised
            when we save Maya scene, and it should be reloadable by SdfLayer::Reload()
        """
        self.assertTrue(self._stage)
        self._stage.SetEditTarget(self._stage.GetRootLayer())
        newPrimPath = "/ChangeInRoot"
        self._stage.DefinePrim(newPrimPath, "xform")
        self._stage.SetEditTarget(self._stage.GetSessionLayer())

        cmds.file(rename=self._mayaFilePath)
        cmds.file(save=True, force=True)
        cmds.file(new=True, force=True)
        cmds.file(self._mayaFilePath, open=True)
        self.assertIsNotNone(self._proxyName)
        ps = ProxyShape.getByName(self._proxyName)
        self.assertTrue(ps)
        stage = ps.getUsdStage()
        self.assertTrue(stage)
        self.assertTrue(stage.GetPrimAtPath(newPrimPath))
        self.assertTrue(stage.GetRootLayer().Reload())
        self.assertFalse(stage.GetPrimAtPath(newPrimPath))

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
        cmds.file(rename=self._mayaFilePath)
        cmds.file(save=True, force=True)
        cmds.file(new=True, force=True)
        cmds.file(self._mayaFilePath, open=True)
        self.assertFalse(cmds.getAttr('%s.sessionLayerName' % self._proxyName))
        os.remove(self._mayaFilePath)

        ps = ProxyShape.getByName(self._proxyName)
        self.assertTrue(ps)
        stage = ps.getUsdStage()
        # Make session layer dirty and save the scene:
        stage.SetEditTarget(stage.GetSessionLayer())
        newPrimPath = "/ChangeInSession"
        stage.DefinePrim(newPrimPath, "xform")

        cmds.file(rename=self._mayaFilePath)
        cmds.file(save=True, force=True)
        cmds.file(new=True, force=True)
        cmds.file(self._mayaFilePath, open=True)
        self.assertTrue(cmds.getAttr('%s.sessionLayerName' % self._proxyName))

        ps = ProxyShape.getByName(self._proxyName)
        self.assertTrue(ps)
        stage = ps.getUsdStage()
        self.assertTrue(stage)
        self.assertTrue(stage.GetPrimAtPath(newPrimPath))
        self.assertTrue(stage.GetSessionLayer().Reload())
        self.assertFalse(stage.GetPrimAtPath(newPrimPath))

    def test_multipleFormatsSerialisation(self):
        """Tests multiple dirty layers with various formats can be saved and restored.

        """

        def _initialiseLayers(usdFilePath):
            cmds.file(new=True, force=True)
            rootLayer = Sdf.Layer.CreateNew(usdFilePath)
            subLayerPaths = rootLayer.subLayerPaths
            layers = []
            for ext in ["usd", "usda", "usdc"]:
                _format = Sdf.FileFormat.FindByExtension(ext)
                filePath = os.path.join(self._outputPath, 'TestLayer.{}'.format(ext))
                layer = Sdf.Layer.CreateNew(filePath)
                layer.comment = ext
                layer.Save()
                layers.append(layer)
                subLayerPaths.append(filePath)

            rootLayer.Save()

        def _buildAndEditAndSaveScene(usdFilePath):
            """Add edits to the 3 format layers and scene the maya scene.

            """
            cmds.file(new=True, force=True)
            cmds.file(rename=self._mayaFilePath)
            proxyName = cmds.AL_usdmaya_ProxyShapeImport(file=usdFilePath)[0]
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

        def _reloadAndAssert(usdFilePath, proxyName):
            """Assert the edits have been restored
            
            """
            cmds.file(new=True, force=True)
            cmds.file(self._mayaFilePath, open=True)

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

        _initialiseLayers(self._usdFilePath)
        proxyName = _buildAndEditAndSaveScene(self._usdFilePath)
        _reloadAndAssert(self._usdFilePath, proxyName)


if __name__ == "__main__":

    tests = [unittest.TestLoader().loadTestsFromTestCase(TestLayerManagerSerialisation),]
    results = [unittest.TextTestRunner(verbosity=2).run(test) for test in tests]
    exitCode = int(not all([result.wasSuccessful() for result in results]))
    cmds.quit(exitCode=(exitCode))
