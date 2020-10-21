#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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

from pxr import Usd, UsdGeom, Vt

import fixturesUtils

class testUsdExportGeomSubset(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        filePath = os.path.join(inputPath,
                                "UsdExportGeomSubsetTest",
                                "UsdExportGeomSubsetTest.ma")
        cmds.file(filePath, force=True, open=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExport(self):
        """
        The test scene has multiple face set connections to materials. Make sure
        the export code has merged the indices correctly.
        """
        usdFile = os.path.abspath('UsdExportGeomSubset.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
                       shadingMode='useRegistry')

        stage = Usd.Stage.Open(usdFile)

        expected = [
            ('/pPlane1/blinn1SG', [0, 3]),
            ('/pPlane1/lambert2SG', [1, 2]),
            ('/pPlane2/blinn1SG', [1, 2]),
            ('/pPlane2/lambert2SG', [0, 3]),
        ]

        for subset_path, indices in expected:
            subset = UsdGeom.Subset(stage.GetPrimAtPath(subset_path))
            self.assertEqual(subset.GetElementTypeAttr().Get(), UsdGeom.Tokens.face)
            self.assertEqual(subset.GetIndicesAttr().Get(), Vt.IntArray(indices))


if __name__ == '__main__':
    unittest.main(verbosity=2)
