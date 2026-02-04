#!/usr/bin/env mayapy
#
# Copyright 2018 Pixar
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
import base64

from maya import cmds
from maya import standalone

from pxr import Usd,UsdShade,Gf

import fixturesUtils

TEST_NORMAL_MAP = """iVBORw0KGgoAAAANSUhEUgAAAAgAAAAICAYAAADED76LAAAAAXNSR0IArs4c6QAAADhlWElmTU0A
KgAAAAgAAYdpAAQAAAABAAAAGgAAAAAAAqACAAQAAAABAAAACKADAAQAAAABAAAACAAAAAC2TFlo
AAAAHGlET1QAAAACAAAAAAAAAAQAAAAoAAAABAAAAAQAAABOcQ0LgQAAABpJREFUKBViKCoq+o+M
/6MBBmRJEBsdUK4AAAAA//9KAo7WAAAAGUlEQVRj+I8GioqK/iNjBjR5FEmQQsoVAAAlMMohT07s
XwAAAABJRU5ErkJggg=="""


def build_scene():
    cmds.file(f=1, new=1)
    plane = cmds.polyPlane()
    print(plane)
    shader = cmds.shadingNode('usdPreviewSurface', asShader=True, name='usdPreviewSurface1')
    cmds.select(cl=1)
    sg = cmds.sets(renderable=True, noSurfaceShader=True, name='usdPreviewSurface1SG')
    cmds.connectAttr('{}.outColor'.format(shader) ,'{}.surfaceShader'.format(sg))

    cmds.createNode('place2dTexture', name="place2dTexture1")
    file_node = cmds.createNode('file', name='file1')

    cmds.connectAttr("place2dTexture1.c", "file1.c")
    cmds.connectAttr("place2dTexture1.tf", "file1.tf")
    cmds.connectAttr("place2dTexture1.rf", "file1.rf")
    cmds.connectAttr("place2dTexture1.mu", "file1.mu")
    cmds.connectAttr("place2dTexture1.mv", "file1.mv")
    cmds.connectAttr("place2dTexture1.s", "file1.s")
    cmds.connectAttr("place2dTexture1.wu", "file1.wu")
    cmds.connectAttr("place2dTexture1.wv", "file1.wv")
    cmds.connectAttr("place2dTexture1.re", "file1.re")
    cmds.connectAttr("place2dTexture1.of", "file1.of")
    cmds.connectAttr("place2dTexture1.r", "file1.ro")
    cmds.connectAttr("place2dTexture1.n", "file1.n")
    cmds.connectAttr("place2dTexture1.vt1", "file1.vt1")
    cmds.connectAttr("place2dTexture1.vt2", "file1.vt2")
    cmds.connectAttr("place2dTexture1.vt3", "file1.vt3")
    cmds.connectAttr("place2dTexture1.vc1", "file1.vc1")
    cmds.connectAttr("place2dTexture1.o", "file1.uv")
    cmds.connectAttr("place2dTexture1.ofs", "file1.fs")
    cmds.connectAttr("file1.oc", "usdPreviewSurface1.nrm")

    cmds.sets('pPlane1', e=1, fe=sg)
    return file_node

def build_test_scene(out_dir):
    """Build test scene with reference"""
    fnode = build_scene()
    out_file = os.path.join(out_dir, 'test_scene.mb')
    out_tex = os.path.join(out_dir, 'test_tex.png')
    with open(out_tex, 'wb') as out:
        out.write(base64.b64decode(TEST_NORMAL_MAP))
    cmds.setAttr(fnode+'.ftn', out_tex, type='string')
    
    cmds.file(rename=out_file)
    cmds.file(f=1, save=1, type='mayaBinary')

    cmds.file(f=1, new=1)
    cmds.file(out_file, reference=True, namespace='ref')



class testExport8BitNormalMap(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def test_export(self):
        """Check to see if inputs:scale is set to [2, 2, 2, 1] and inputs:bias is [-1, -1, -1, 0]"""
        build_test_scene(self.temp_dir)

        out_file = os.path.abspath('normalMapTest.usdc')
        cmds.mayaUSDExport(file=out_file, convertMaterialsTo=['UsdPreviewSurface'],legacyMaterialScope=False, defaultPrim="None")
        
        self.assertTrue(os.path.isfile(out_file))

        stage = Usd.Stage.Open(out_file)
        shader = UsdShade.Shader.Get(stage, '/Looks/ref_usdPreviewSurface1SG/ref_file1')
        
        self.assertEqual(Gf.Vec4f(-1, -1, -1, 0), shader.GetInput('bias').Get())
        self.assertEqual(Gf.Vec4f(2, 2, 2, 1), shader.GetInput('scale').Get())
        

if __name__ == '__main__':
    unittest.main(verbosity=2)
