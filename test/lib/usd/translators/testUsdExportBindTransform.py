#!/usr/bin/env mayapy
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
from pxr import Usd, Gf


class TestUsdExportBindTransform(unittest.TestCase):
    MAYAUSD_PLUGIN_NAME = 'mayaUsdPlugin'

    @classmethod
    def setUpClass(cls):
        cls.input_dir = fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')

    def setUp(self):
        scene_path = os.path.join(self.input_dir, "UsdExportBindTransformsTest", "bindTransformsExport.ma")
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
        bindTransform = bindTransforms[3]  # NOTE: (yliangsiew) The 4th joint is the rt_joint that we're interested in.
        self.assertNotEqual(bindTransform, Gf.Matrix4d(1, 0, 0, 0,
                                                       0, 1, 0, 0,
                                                       0, 0, 1, 0,
                                                       0, 0, 0, 1))

        self.assertEqual(bindTransform, Gf.Matrix4d(-0.002290224357008621, -0.0056947280722027946, 0.9999807051930374, -5.421010862427522e-20,
                                                    0.01567493647243695, 0.9998602653154097, 0.005729941968379504, 0,
                                                    -0.9998745177471474, 0.015687757121603033, -0.0022006397844859223, 0,
                                                    -4.625417221923633, 16.11339899672265, 5.401075137997587, 1.0000000000000002) )

if __name__ == '__main__':
    unittest.main()
