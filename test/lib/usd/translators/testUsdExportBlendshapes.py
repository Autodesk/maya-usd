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


class TestUsdExportBlendshapes(unittest.TestCase):
    """
    Verify that exporting blend shapes from Maya works.
    """

    @classmethod
    def setUpClass(cls):
        cls.temp_dir = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testBlendShapesExport(self):
        cmds.file(new=True, force=True)
        parent = cmds.group(name="root", empty=True)
        base, _ = cmds.polyCube(name="base")
        cmds.parent(base, parent)
        target, _ = cmds.polyCube(name="blend")
        cmds.parent(target, parent)
        cmds.polyMoveVertex('{}.vtx[0:2]'.format(target), s=(1.0, 1.5, 1.0))

        cmds.blendShape(target, base, automatic=True)

        cmds.select(base, replace=True)
        temp_file = os.path.join(self.temp_dir, 'blendshape.usda')
        cmds.mayaUSDExport(f=temp_file, v=True, sl=True, ebs=True, skl="auto")

        stage = Usd.Stage.Open(temp_file)
        prim = stage.GetPrimAtPath("/root/base/blendShape")
        offsets = prim.GetAttribute("offsets").Get()

        for i, coords in enumerate(offsets):
            coords = list(coords)  # convert from GfVec3
            self.assertEqual(coords, [0, -0.25 if i < 2 else 0.25, 0])

        """
        Sample BlendShape prim:
        
        def BlendShape "blendShape"
        {
            uniform vector3f[] normalOffsets = [(0, 0, 0), (0, 0, 0), (0, 0, 0)]
            uniform vector3f[] offsets = [(0, -0.25, 0), (0, -0.25, 0), (0, 0.25, 0)]
            uniform int[] pointIndices = [0, 1, 2]
        }
        """


if __name__ == '__main__':
    unittest.main(verbosity=2)
