#!/pxrpythonsubst
#
# Copyright 2020 Apple
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

import fixturesUtils
import maya.cmds as cmds
import maya.standalone as mayastandalone
import os
import unittest
from pxr import Usd


class TestUsdExportBindTransform(unittest.TestCase):
    MAYAUSD_PLUGIN_NAME = 'mayaUsdPlugin'

    @classmethod
    def setUpClass(cls):
        cls.temp_dir = fixturesUtils.setUpClass(__file__)

    def setUp(self):
        scene_path = os.path.join(
            os.path.dirname(os.path.abspath(__file__)),
            "UsdExportBindTransformsTest", "bindTransformsExport.ma"
        )
        cmds.file(scene_path, open=True, force=True)

    @classmethod
    def tearDownClass(cls):
        mayastandalone.uninitialize()

    def testBindTransformExport(self):
        temp_export_file = os.path.join(self.temp_dir, 'test_output.usda')
        cmds.select('a', r=True)
        cmds.mayaUSDExport(v=True, sl=True, f=temp_export_file, skl='auto', skn='auto',
                           defaultMeshScheme='catmullClark')

        stage = Usd.Stage.Open(temp_export_file)
        prim = stage.GetPrimAtPath('/a/b/c')
        bindTransforms = prim.GetAttribute('bindTransforms').Get()
        self.assertNotEqual(bindTransforms, [[1, 0, 0, 0],
                                             [0, 1, 0, 0],
                                             [0, 0, 1, 0],
                                             [0, 0, 0, 1]])


if __name__ == '__main__':
    unittest.main()
