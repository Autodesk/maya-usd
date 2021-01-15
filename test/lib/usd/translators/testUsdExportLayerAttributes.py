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

import os
import unittest
import math

import fixturesUtils
from maya import cmds
from maya import standalone
from pxr import Usd


class testMayaUsdExportLayerAttributes(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.temp_dir = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def test_fps(self):
        fps_map = {
            "ntsc": 30,
            "game": 15,
            "film": 24,
            "pal": 25,
            "show": 48,
            "palf": 50,
            "ntscf": 60

        }

        for name, fps in fps_map.items():
            cmds.currentUnit(time=name)
            temp_file = os.path.join(self.temp_dir, 'test_{}.usda'.format(name))
            cmds.mayaUSDExport(f=temp_file, frameRange=(1, 5))

            stage = Usd.Stage.Open(temp_file)
            self.assertEqual(stage.GetTimeCodesPerSecond(), fps)
            self.assertEqual(stage.GetFramesPerSecond(), fps)

    def test_customLayerData(self):
        temp_file = os.path.join(self.temp_dir, 'test_customLayerData.usda')
        cmds.mayaUSDExport(f=temp_file,
                           customLayerData=[
                               # These should be successful
                               ["foo", "spam", "string"],
                               ["age", 10, "int"],
                               ["floaty", 0.1, "float"],
                               ["pi", math.pi, "double"],
                               # These should fail but continue
                               ["a", "0", "unknown"],
                               ["b", "b", "int"],
                               ["c", "c", "float"],
                               ["d", "d", "double"]
                           ])

        stage = Usd.Stage.Open(temp_file)
        data = stage.GetRootLayer().customLayerData

        self.assertEqual(data['foo'], 'spam')
        self.assertEqual(data['age'], 10)
        self.assertAlmostEqual(data['floaty'], 0.1)
        self.assertAlmostEqual(data['pi'], math.pi)

        invalid = ('a', 'b', 'c', 'd')
        for i in invalid:
            self.assertNotIn(i, data)


if __name__ == '__main__':
    unittest.main(verbosity=2)
