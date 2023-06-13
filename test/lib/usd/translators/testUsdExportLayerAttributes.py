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
        fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')

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
            temp_file = os.path.join(self.temp_dir, 'test_fps_{}.usda'.format(name))
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
                               ["works", 1, "bool"],
                               ["fail", False, "bool"],
                               # These should fail but continue
                               ["a", "0", "unknown"],
                               ["b", "b", "int"],
                               ["c", "c", "float"],
                               ["d", "d", "double"],
                               ["e", "e", "bool"]
                           ])

        stage = Usd.Stage.Open(temp_file)
        data = stage.GetRootLayer().customLayerData

        self.assertEqual(data['foo'], 'spam')
        self.assertEqual(data['age'], 10)
        self.assertAlmostEqual(data['floaty'], 0.1)
        self.assertAlmostEqual(data['pi'], math.pi)
        self.assertTrue(data['works'])
        self.assertFalse(data['fail'])

        invalid = ('a', 'b', 'c', 'd', 'e')
        for i in invalid:
            self.assertNotIn(i, data)

    def test_metersPerUnit(self):
        units = {
            "meters": 1.0,
            "default": 0
        }

        cmds.file(new=True, force=True)
        transform, shape = cmds.polyCube()

        cmds.setKeyframe(transform, value=0, attribute="translateX", time=1)
        cmds.setKeyframe(transform, value=10, attribute="translateX", time=2)

        for name, metersPerUnit in units.items():
            is_default = not metersPerUnit

            temp_file = os.path.join(self.temp_dir, 'test_metersPerUnit_{}.usda'.format(name))
            cmds.mayaUSDExport(f=temp_file, frameRange=(1, 2), metersPerUnit=metersPerUnit)

            stage = Usd.Stage.Open(temp_file)
            prim = stage.GetPrimAtPath("/{}".format(transform))

            self.assertTrue(prim, "Could not find cube in export")

            points = prim.GetAttribute("points").Get()

            # Only need to check the first of the first vertices
            px = abs(points[0][0])
            self.assertAlmostEqual(px, 0.5 if is_default else 0.005, msg="Mesh points weren't scaled properly")

            # Only need to check tx on the translate
            translate_attr = prim.GetAttribute("xformOp:translate")
            self.assertAlmostEqual(translate_attr.Get(1)[0], 0)
            self.assertAlmostEqual(translate_attr.Get(2)[0], 10 if is_default else 0.1,
                                   msg="Translate values weren't scaled properly")


if __name__ == '__main__':
    unittest.main(verbosity=2)
