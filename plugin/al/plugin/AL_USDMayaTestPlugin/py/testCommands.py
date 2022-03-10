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

from pxr import Sdf, Usd, UsdGeom


class TestImportCommand(unittest.TestCase):
    """Test cases for AL_USDMaya importer command"""

    app = "maya"

    def setUp(self):
        """Export some sphere geometry as .usda, and import into a new Maya scene."""

        cmds.file(force=True, new=True)
        cmds.loadPlugin("AL_USDMayaPlugin", quiet=True)
        self.assertTrue(cmds.pluginInfo("AL_USDMayaPlugin", query=True, loaded=True))

    def tearDown(self):
        """Unload plugin, new Maya scene, reset class member variables."""

        cmds.file(force=True, new=True)
        cmds.unloadPlugin("AL_USDMayaPlugin", force=True)

    def test_varyingDisplayColorConstantOpacity(self):
        """ Varying displayOpacity primvar authored alongside constant displayOpacity
        """

        pathin = "../test_data/cube_displayColor.usda"
        _, pathout = tempfile.mkstemp(suffix=".usda")

        cmds.AL_usdmaya_ImportCommand(file=pathin)
        cmds.AL_usdmaya_ExportCommand(file=pathout)

        primpath = "/cube"

        stagein = Usd.Stage.Open(pathin)
        gprimin = UsdGeom.Mesh(stagein.GetPrimAtPath(primpath))
        primvarin = gprimin.GetDisplayColorPrimvar()
        valuein = primvarin.GetAttr().Get()

        stageout = Usd.Stage.Open(pathout)
        gprimout = UsdGeom.Mesh(stageout.GetPrimAtPath(primpath))
        primvarout = gprimout.GetDisplayColorPrimvar()
        valueout = primvarout.GetAttr().Get()

        self.assertEqual(
            primvarin.GetDeclarationInfo(), primvarout.GetDeclarationInfo()
        )
        self.assertEqual(valuein, valueout)

        os.remove(pathout)


if __name__ == "__main__":

    tests = [
        unittest.TestLoader().loadTestsFromTestCase(TestImportCommand),
    ]
    results = [unittest.TextTestRunner(verbosity=2).run(test) for test in tests]
    exitCode = int(not all([result.wasSuccessful() for result in results]))
    # Note: cmds.quit() does not return exit code as expected, we use Python builtin function instead
    os._exit(exitCode)
