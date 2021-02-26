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
import maya.OpenMaya as om
from maya import cmds
from maya import standalone
from pxr import Usd
from pxr import UsdSkel


class TestUsdExportBlendshapes(unittest.TestCase):
    """
    Verify that exporting blend shapes from Maya works.
    """

    @classmethod
    def setUpClass(cls):
        cls.temp_dir = fixturesUtils.setUpClass(__file__)
        cls.scene_path = os.path.join(cls.temp_dir, "UsdExportBlendShapesTest", "blendShapesExport.ma").replace("\\", "/")

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testBlendShapesExport(self):
        # NOTE: (yliangsiew) Basic blendshape export test.
        om.MFileIO.newFile(True)
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

        # NOTE: (yliangsiew) Test simple inbetween setup.
        om.MFileIO.open(self.scene_path, None, True)
        cmds.select("basic_cube_2_inbetweens_no_anim|base", r=True)
        cmds.mayaUSDExport(f=temp_file, v=True, sl=True, ebs=True, skl="auto", skn="auto")
        stage = Usd.Stage.Open(temp_file)
        prim = stage.GetPrimAtPath("/basic_cube_2_inbetweens_no_anim/base/pCube2")
        blendShape = UsdSkel.BlendShape(prim)
        inbetweens = blendShape.GetInbetweens()
        self.assertEqual(len(inbetweens), 2)  # NOTE: (yliangsiew) This particular setup has two additional inbetweens.

        # NOTE: (yliangsiew) Test simple multiple targets setup.
        om.MFileIO.open(self.scene_path, None, True)
        cmds.select("basic_cube_4_blendshapes_no_anim|base", r=True)
        cmds.mayaUSDExport(f=temp_file, v=True, sl=True, ebs=True, skl="auto", skn="auto")
        stage = Usd.Stage.Open(temp_file)
        prim = stage.GetPrimAtPath("/basic_cube_4_blendshapes_no_anim/base")
        blendShapes = prim.GetChildren()
        for bs in blendShapes:
            self.assertEqual(bs.GetTypeName(), 'BlendShape')

        # NOTE: (yliangsiew) Test exporting empty blendshape targets.
        om.MFileIO.open(self.scene_path, None, True)
        cmds.select("cube_empty_blendshape_targets|base", r=True)
        cmds.mayaUSDExport(f=temp_file, v=True, sl=True, ebs=True, skl="auto", skn="auto", ignoreWarnings=True)
        stage = Usd.Stage.Open(temp_file)
        prim = stage.GetPrimAtPath("/cube_empty_blendshape_targets/base")
        blendShapes = prim.GetChildren()
        for bs in blendShapes:
            self.assertEqual(bs.GetTypeName(), 'BlendShape')
            skelBS = UsdSkel.BlendShape(bs)
            normalsAttr = skelBS.GetNormalOffsetsAttr()
            offsetsAttr = skelBS.GetOffsetsAttr()
            normals = normalsAttr.Get()
            offsets = offsetsAttr.Get()
            self.assertEqual(len(normals), 0)
            self.assertEqual(len(offsets), 0)

if __name__ == '__main__':
    unittest.main(verbosity=2)
