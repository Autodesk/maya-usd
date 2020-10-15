#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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
import shutil

from pxr import Usd

from maya import cmds
from maya import standalone

import fixturesUtils

class testUsdExportOverImport(unittest.TestCase):

    BEFORE_PRIM_NAME = 'Cube'
    BEFORE_PRIM_PATH = '/CubeModel/Geom/%s' % BEFORE_PRIM_NAME
    AFTER_PRIM_NAME = 'NewCube'
    AFTER_PRIM_PATH = '/CubeModel/Geom/%s' % AFTER_PRIM_NAME

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        srcFile = os.path.join(inputPath, "UsdExportOverImportTest", "CubeModel.usda")

        # Copy source file to working directory, as we'll be overwriting it.
        shutil.copy(srcFile, os.getcwd())

        cls.USD_FILE = os.path.join(os.getcwd(), 'CubeModel.usda')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _ValidateUsdBeforeExport(self):
        usdStage = Usd.Stage.Open(self.USD_FILE)
        self.assertTrue(usdStage)

        cubePrim = usdStage.GetPrimAtPath(self.BEFORE_PRIM_PATH)
        self.assertTrue(cubePrim)

        invalidPrim = usdStage.GetPrimAtPath(self.AFTER_PRIM_PATH)
        self.assertFalse(invalidPrim)

    def _ModifyMayaScene(self):
        cmds.rename(self.BEFORE_PRIM_NAME, self.AFTER_PRIM_NAME)

    def _ValidateUsdAfterExport(self):
        usdStage = Usd.Stage.Open(self.USD_FILE)
        self.assertTrue(usdStage)

        invalidPrim = usdStage.GetPrimAtPath(self.BEFORE_PRIM_PATH)
        self.assertFalse(invalidPrim)

        cubePrim = usdStage.GetPrimAtPath(self.AFTER_PRIM_PATH)
        self.assertTrue(cubePrim)

    def testImportModifyAndExportCubeModel(self):
        """
        Tests that re-exporting over a previously imported USD file works, and
        that changes are reflected in the new version of the file.
        """
        self._ValidateUsdBeforeExport()

        cmds.usdImport(file=self.USD_FILE, shadingMode=[["none", "default"], ])

        self._ModifyMayaScene()

        cmds.usdExport(mergeTransformAndShape=True, file=self.USD_FILE,
            shadingMode='none')

        self._ValidateUsdAfterExport()


if __name__ == '__main__':
    unittest.main(verbosity=2)
