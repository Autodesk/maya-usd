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

import unittest

from maya import cmds
from maya import standalone

from pxr import Plug, Sdf, Usd

import fixturesUtils


class testPrimTranslatorPlugins(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def _assertMayaPluginLoaded(self, pluginName, expected):
        self.assertEqual(
            bool(cmds.pluginInfo(pluginName, q=True, loaded=True)), expected,
            "pluginInfo('%s', loaded=True)" % pluginName)

    def _preloadMayaUsdTranslators(self):
        plugin = Plug.Registry().GetPluginWithName('mayaUsd_Translators')
        self.assertIsNotNone(plugin, 'mayaUsd_Translators')
        plugin.Load()
        self.assertTrue(plugin.isLoaded)

    def testMultiplePrimReaderPlugin(self):
        """Test if a prim reader plugin is loaded even when there is already a reader for this type."""

        # Ensure the plugin providing the default mayaUsd translatorsis loaded,
        # so that 'UsdGeomXform' already has a reader registered.
        self._preloadMayaUsdTranslators()

        srcStage = Usd.Stage.CreateInMemory()
        prim = srcStage.DefinePrim("/Test", "Xform")
        srcStage.SetDefaultPrim(prim)

        # 'UsdGeomXform' already has a reader registered. dummyXformReaderPlugin declares a reader
        # for it in its plugInfo.json and must still be discovered and loaded.

        self._assertMayaPluginLoaded('dummyXformReaderPlugin', False)
        cmds.mayaUSDImport(f=srcStage.GetRootLayer().identifier)
        self._assertMayaPluginLoaded('dummyXformReaderPlugin', True)

        # After an explicit unload the plugin is not discovered and loaded again.
        cmds.unloadPlugin('dummyXformReaderPlugin')
        self._assertMayaPluginLoaded('dummyXformReaderPlugin', False)

        cmds.mayaUSDImport(f=srcStage.GetRootLayer().identifier)
        self._assertMayaPluginLoaded('dummyXformReaderPlugin', False)

    def testMultiplePrimWriterPlugin(self):
        """Test if a prim writer plugin is loaded even when there is already a writer for this type."""

        # Ensure the plugin providing the default mayaUsd translatorsis loaded,
        # so that 'locator' already has a writer registered.
        self._preloadMayaUsdTranslators()

        dstLayer = Sdf.Layer.CreateAnonymous()
        locator = cmds.spaceLocator()

        # 'locator' already has a writer registered. dummyLocatorWriterPlugin declares a writer
        # for it in its plugInfo.json and must still be discovered and loaded.

        self._assertMayaPluginLoaded('dummyLocatorWriterPlugin', False)
        cmds.mayaUSDExport(locator, file=dstLayer.identifier)
        self._assertMayaPluginLoaded('dummyLocatorWriterPlugin', True)

        # After an explicit unload the plugin is not discovered and loaded again.
        cmds.unloadPlugin('dummyLocatorWriterPlugin')
        self._assertMayaPluginLoaded('dummyLocatorWriterPlugin', False)

        cmds.mayaUSDExport(locator, file=dstLayer.identifier)
        self._assertMayaPluginLoaded('dummyLocatorWriterPlugin', False)


if __name__ == '__main__':
    unittest.main(verbosity=2)
