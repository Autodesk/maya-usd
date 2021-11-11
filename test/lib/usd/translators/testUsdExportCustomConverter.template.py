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

from pxr import Usd
from pxr import UsdShade

import os
import unittest

from maya import cmds
from maya import standalone

import fixturesUtils

class testUsdExportCustomConverter(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # In case tests are run in parallel, make sure we export to separate
        # directories:
        plugin_path = os.getenv("@PXR_OVERRIDE_PLUGINPATH_NAME@")
        maya_path = [i for i in plugin_path.split(os.pathsep) if i.endswith("Maya")]
        if maya_path:
            suffix = "Maya"
        else:
            suffix = "USD"

        fixturesUtils.setUpClass(__file__, suffix)

        cls.input_path = os.getenv("INPUT_PATH")

        test_dir = os.path.join(cls.input_path,
                                "UsdExportCustomConverterTest")

        cmds.workspace(test_dir, o=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testUsdExportCustomConverter(self):
        """
        Tests a custom exporter for a conversion that exists in an unloaded
        plugin.
        """
        # Load test scene:
        file_path = os.path.join(self.input_path,
                                 "UsdExportCustomConverterTest",
                                 "testScene.ma")
        cmds.file(file_path, force=True, open=True)

        # Export to USD:
        usd_path = os.path.abspath('UsdExportCustomConverterTest.usda')

        # Using the "maya" material conversion, which only exists in the test
        # plugin
        options = ["shadingMode=useRegistry",
                   "convertMaterialsTo=[maya]",
                   "mergeTransformAndShape=1"]

        default_ext_setting = cmds.file(q=True, defaultExtensions=True)
        cmds.file(defaultExtensions=False)
        cmds.file(usd_path, force=True,
                  options=";".join(options),
                  typ="USD Export", pr=True, ea=True)
        cmds.file(defaultExtensions=default_ext_setting)

        # Make sure we have a Maya standardSurface material in the USD file:
        stage = Usd.Stage.Open(usd_path)
        material = stage.GetPrimAtPath(
            "/pCube1/Looks/standardSurface2SG/standardSurface2")
        shader = UsdShade.Shader(material)
        self.assertEqual(shader.GetIdAttr().Get(), "standardSurface")


if __name__ == '__main__':
    unittest.main(verbosity=2)
