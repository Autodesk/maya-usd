#!/pxrpythonsubst
#
# Copyright 2021 Apple
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

import fixturesUtils
import maya.OpenMaya as om
from maya import cmds
from maya import standalone
from pxr import Usd


class TestUsdImportUSDZTextures(unittest.TestCase):
    """
    Verify that extracting textures on import from a USDZ file works.
    """

    @classmethod
    def setUpClass(cls):
        cls.temp_dir = fixturesUtils.setUpClass(__file__)
        cls.usdz_path = os.path.join(cls.temp_dir, "UsdImportUSDZTexturesTest", "test_cubes.usdz")

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testUSDZImport(self):
        om.MFileIO.newFile(True)
        write_dir_path = os.path.dirname(self.usdz_path)
        cmds.mayaUSDImport(f=self.usdz_path, importUSDZTextures=True, importUSDZTexturesFilePath=write_dir_path)
        self.assertTrue(os.path.isfile(os.path.join(write_dir_path, "clouds_128_128.png")))
        self.assertTrue(os.path.isfile(os.path.join(write_dir_path, "red_128_128.png")))


if __name__ == '__main__':
    unittest.main(verbosity=2)
