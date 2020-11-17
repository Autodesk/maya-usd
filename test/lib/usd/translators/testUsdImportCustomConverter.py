#!/pxrpythonsubst
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

import fixturesUtils

class testUsdImportCustomConverter(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # In case tests are run in parallel, make sure we export to separate
        # directories:
        plugin_path = os.getenv("PXR_PLUGINPATH_NAME")
        maya_path = [i for i in plugin_path.split(os.pathsep) if i.endswith("Maya")]
        if maya_path:
            suffix = "Maya"
        else:
            suffix = "USD"

        # Debugging why plugin load fails on OS/X:
        usd_path = [i for i in plugin_path.split(os.pathsep) if i.endswith("USD")]
        if usd_path:
            dso_path = usd_path[0][:-3]
            print("Contents of dso_path: %s" % dso_path)
            for fn in os.listdir(dso_path):
                print("\t%s" % fn)

        cls.input_path = fixturesUtils.setUpClass(__file__, suffix)

        cls.test_dir = os.path.join(cls.input_path,
                                    "UsdImportCustomConverterTest")

        cls.usd_path = os.path.join(cls.test_dir,
                                    'UsdImportCustomConverterTest.usda')

        cmds.workspace(cls.test_dir, o=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        """Clear the scene"""
        cmds.file(f=True, new=True)
        self.default_materials = cmds.ls(materials=True)

    def checkMaterials(self, expected):
        """Check that all the cubes are linked to the right material"""
        for cube_name, material_type in expected:
            sgs = cmds.listConnections(cube_name, type="shadingEngine")
            shaders = cmds.listConnections(sgs[0] + ".surfaceShader")
            if material_type:
                node_type = cmds.nodeType(shaders[0])
                self.assertEqual(node_type, material_type)
            else:
                self.assertTrue(shaders[0] in self.default_materials)

    def testImportCommand(self):
        """
        Tests a custom exporter for a conversion that exists in an unloaded
        plugin using the mayaUSDImport command.
        """
        modes = [["useRegistry", "maya"],
                 ["useRegistry", "UsdPreviewSurface"],
                 ["displayColor", "default"]]
        cmds.mayaUSDImport(file=self.usd_path, shadingMode=modes,
                           preferredMaterial="none", primPath="/")

        expected = [["pCube1Shape", "lambert"],
                    ["pCube2Shape", "standardSurface"],
                    ["pCube3Shape", "usdPreviewSurface"]]
        self.checkMaterials(expected)

    def testNoImport(self):
        """
        Do not import any materials.
        """

        modes = [["none", "default"], ]
        cmds.mayaUSDImport(file=self.usd_path, shadingMode=modes, primPath="/")

        expected = [["pCube1Shape", None],
                    ["pCube2Shape", None],
                    ["pCube3Shape", None]]

        self.checkMaterials(expected)

    def testNoDisplayColors(self):
        """
        Do not get default down to display colors:
        """
        modes = [["useRegistry", "maya"],
                 ["useRegistry", "UsdPreviewSurface"]]
        cmds.mayaUSDImport(file=self.usd_path, shadingMode=modes,
                           preferredMaterial="none", primPath="/")

        expected = [["pCube1Shape", None],
                    ["pCube2Shape", "standardSurface"],
                    ["pCube3Shape", "usdPreviewSurface"]]
        self.checkMaterials(expected)

    def testFileCommand(self):
        """
        Tests a custom exporter for a conversion that exists in an unloaded
        plugin using the file command.
        """

        modes = ["[useRegistry,maya]",
                 "[useRegistry,UsdPreviewSurface]",
                 "[displayColor, default]"]
        modes = "[" + ",".join(modes) + "]"
        import_options = ("shadingMode=%s" % modes,
                          "preferredMaterial=phong",
                          "primPath=/")
        cmds.file(self.usd_path, i=True, type="USD Import", ignoreVersion=True,
                  ra=True, mergeNamespacesOnClash=False, namespace="Test",
                  pr=True, importTimeRange="combine",
                  options=";".join(import_options))

        expected = [["pCube1Shape", "phong"],
                    ["pCube2Shape", "standardSurface"],
                    ["pCube3Shape", "phong"]]
        self.checkMaterials(expected)

    def testAsMayaDoesItNoConversion(self):
        """
        Tests using the full importer list to make sure we get the expected
        result.
        """

        nice_names = cmds.mayaUSDListShadingModes(im=True)
        modes = []
        for nice_name in nice_names:
            shading_options = cmds.mayaUSDListShadingModes(io=nice_name)
            modes.append("[%s,%s]" % tuple(shading_options))
        modes = "[" + ",".join(modes) + "]"
        import_options = ("shadingMode=%s" % modes,
                          "primPath=/")
        cmds.file(self.usd_path, i=True, type="USD Import", ignoreVersion=True,
                  ra=True, mergeNamespacesOnClash=False, namespace="Test",
                  pr=True, importTimeRange="combine",
                  options=";".join(import_options))

        expected = [["pCube1Shape", "lambert"],
                    ["pCube2Shape", "standardSurface"],
                    ["pCube3Shape", "usdPreviewSurface"]]
        self.checkMaterials(expected)

    def testAsMayaDoesItWithConversion(self):
        """
        Tests using the full importer list to make sure we get the expected
        result.
        """

        nice_names = cmds.mayaUSDListShadingModes(im=True)
        modes = []
        for nice_name in nice_names:
            shading_options = cmds.mayaUSDListShadingModes(io=nice_name)
            modes.append("[%s,%s]" % tuple(shading_options))
        modes = "[" + ",".join(modes) + "]"
        import_options = ("shadingMode=%s" % modes,
                          "preferredMaterial=phong",
                          "primPath=/")
        cmds.file(self.usd_path, i=True, type="USD Import", ignoreVersion=True,
                  ra=True, mergeNamespacesOnClash=False, namespace="Test",
                  pr=True, importTimeRange="combine",
                  options=";".join(import_options))

        expected = [["pCube1Shape", "phong"],
                    ["pCube2Shape", "standardSurface"],
                    ["pCube3Shape", "phong"]]
        self.checkMaterials(expected)


if __name__ == '__main__':
    unittest.main(verbosity=2)
