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


import os
import unittest

import fixturesUtils
from maya import cmds
from maya import standalone
from pxr import Usd


class testUsdExportAnimation(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.temp_dir = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportStaticSingleSampleOn(self):
        for state in (True, False):
            # Set up the scene in here to prevent having to maintain a Maya file
            cmds.file(new=True, force=True)
            root = cmds.group(empty=True, name="root")
            cube, _ = cmds.polyCube(name="Cube")
            joint = cmds.joint()
            cmds.parent(cube, joint, root)
            cmds.skinCluster(joint, cube)


            cmds.setKeyframe(joint, v=0, at='translateY', time=1)
            cmds.setKeyframe(root, v=0, at='translateY', time=1)
            cmds.setKeyframe(joint, v=0, at='translateY', time=10)

            # Okay now do the export
            path = os.path.join(self.temp_dir, "staticSingleFrame{}.usda".format("On" if state else "Off"))

            cmds.mayaUSDExport(f=path, exportSkels="auto", frameRange=(1, 10), sss=state)

            stage = Usd.Stage.Open(path)

            # Check to see if the flag is working
            prim = stage.GetPrimAtPath("/root")
            attr = prim.GetAttribute("xformOp:translate")
            num_samples = attr.GetNumTimeSamples()
            self.assertEqual(num_samples, int(not state))