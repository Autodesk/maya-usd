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
from pxr import Gf


class TestUsdImportXformOps(unittest.TestCase):
    """
    Verify that importing USDs with paired xform/inverted xform pivot ops creates
    the correct transform when imported into Maya.
    """

    @classmethod
    def setUpClass(cls):
        cls.temp_dir = fixturesUtils.setUpClass(__file__)
        cls.scene_path = os.path.join(cls.temp_dir, "UsdImportXformOpsTest", "UsdImportXformOpsTest.usda").replace("\\", "/")

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testImportPairedXformOps(self):
        """
        Tests that importing a USD cube mesh with non-identity scale that has paired
        XformOps on it will result in the correct transform when imported into Maya.
        """
        om.MFileIO.newFile(True)
        cmds.mayaUSDImport(file=self.scene_path)
        selList = om.MSelectionList()
        selList.add("pCube1")
        cubeDagPath = om.MDagPath()
        selList.getDagPath(0, cubeDagPath)
        transform = cubeDagPath.inclusiveMatrix()
        expectedTranslation = Gf.Vec3f(0.0, -1.3681414, 0.0)
        actualTranslation = om.MTransformationMatrix(transform).getTranslation(om.MSpace.kTransform)  # NOTE: (yliangsiew) world space
        actualTranslation = Gf.Vec3f(actualTranslation[0], actualTranslation[1], actualTranslation[2])
        print actualTranslation
        self.assertTrue(Gf.IsClose(expectedTranslation, actualTranslation, 0.001))

if __name__ == '__main__':
    unittest.main(verbosity=2)
