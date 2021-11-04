#!/usr/bin/env mayapy
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
import unittest

from maya import cmds
from maya import standalone

import mayaUsd.lib as mayaUsdLib

import fixturesUtils


class testUsdMayaListShadingModesCommand(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testUsdMayaListShadingModesCommand(self):
        """
        Tests the mayaUSDListShadingModes command.
        """

        # Alias the long named command:
        modes = cmds.mayaUSDListShadingModes

        preview = "USD Preview Surface"
        preview_option = ";".join(("shadingMode=useRegistry",
                                   "convertMaterialsTo=[UsdPreviewSurface]"))
        maya_shaders = "Maya Shaders"
        maya_options = ";".join(("shadingMode=useRegistry",
                                 "convertMaterialsTo=[maya]"))
        none = "None"
        none_options = "shadingMode=none"

        # These 2 should always be there:
        exporters = modes(ex=True)
        self.assertTrue("None" in exporters)
        self.assertTrue(preview in exporters)

        # This one is no longer an exported shading mode:
        self.assertFalse("Display Colors" in exporters)

        self.assertEqual(modes(findExportName="UsdPreviewSurface"), preview)
        self.assertTrue(len(modes(exportAnnotation=preview)) > 5)
        self.assertEqual(modes(exportOptions=preview), preview_option)

        self.assertEqual(modes(findExportName="none"), none)
        self.assertTrue(len(modes(exportAnnotation=none)) > 5)
        self.assertEqual(modes(exportOptions=none), none_options)

        # And we should not have this one yet
        self.assertFalse(maya_shaders in exporters)

        # Check importers:
        importers = modes(im=True)
        self.assertTrue("None" in importers)
        self.assertTrue("Display Colors" in importers)
        self.assertTrue(preview in importers)

        self.assertEqual(modes(findImportName="UsdPreviewSurface"), preview)
        self.assertTrue(len(modes(importAnnotation=preview)) > 5)
        self.assertEqual(modes(importOptions=preview), ["useRegistry",
                                                        "UsdPreviewSurface"])

        self.assertEqual(modes(findImportName="none"), none)
        self.assertTrue(len(modes(importAnnotation=none)) > 5)
        self.assertEqual(modes(importOptions=none), ["none",
                                                     "none"])

        # And we should not have this one yet
        self.assertFalse(maya_shaders in importers)

        # Load the test plugin directly to get the "Maya shading" export. This
        # plugin was not discoverable by USD so it was not loaded when asking
        # for the modes the first time. We will test discoverability in the
        # testUsdExportCustomConverter.py test.
        cmds.loadPlugin("usdTestMayaPlugin")

        exporters = modes(ex=True)

        # Should have a new shading mode now:
        self.assertTrue(maya_shaders in exporters)
        self.assertEqual(modes(fen="maya"), maya_shaders)
        self.assertTrue(len(modes(ea=maya_shaders)) > 5)
        self.assertEqual(modes(eo=maya_shaders), maya_options)

        importers = modes(im=True)

        self.assertTrue(maya_shaders in importers)
        self.assertEqual(modes(fin="maya"), maya_shaders)
        self.assertTrue(len(modes(ia=maya_shaders)) > 5)
        self.assertEqual(modes(io=maya_shaders), ["useRegistry", "maya"])

    def testUsdMayaListUseRegistryShadingModesCommand(self):
        """
        Tests the mayaUSDListShadingModes command in useRegistryOnly mode.
        """

        # Alias the long named command:
        modes = cmds.mayaUSDListShadingModes

        preview = "USD Preview Surface"
        preview_option = "UsdPreviewSurface"
        maya_shaders = "Maya Shaders"
        maya_options = "maya"

        # The preview surface should always be there:
        exporters = modes(ex=True, ur=True)
        self.assertTrue(preview in exporters)

        self.assertFalse("None" in exporters)
        self.assertFalse("Display Colors" in exporters)

        self.assertEqual(modes(findExportName="UsdPreviewSurface", ur=True), preview)
        self.assertTrue(len(modes(exportAnnotation=preview, ur=True)) > 5)
        self.assertEqual(modes(exportOptions=preview, ur=True), preview_option)

        # Check importers:
        importers = modes(im=True, ur=True)
        self.assertTrue(preview in importers)

        # These two are not useRegistry based:
        self.assertFalse("None" in importers)
        self.assertFalse("Display Colors" in importers)

        self.assertEqual(modes(findImportName="UsdPreviewSurface", ur=True), preview)
        self.assertTrue(len(modes(importAnnotation=preview, ur=True)) > 5)
        self.assertEqual(modes(importOptions=preview, ur=True), ["useRegistry",
                                                        "UsdPreviewSurface"])

    def testShadingModeRegistry(self):
        """
        Tests the ShadingModeRegistry.
        """

        # Alias the long named command:
        modes = cmds.mayaUSDListShadingModes

        testImport = "Test import"
        testExport = "Test export"

        mayaUsdLib.ShadingModeRegistry.RegisterImportConversion("Registration Test Import", "Test", testImport, "Test Import description.")
        mayaUsdLib.ShadingModeRegistry.RegisterExportConversion("Registration Test Export", "Test", testExport, "Test Export description.")

        importers = modes(im=True)
        self.assertTrue(testImport in importers)

        exporters = modes(ex=True)
        self.assertTrue(testExport in exporters)


if __name__ == '__main__':
    unittest.main(verbosity=2)
