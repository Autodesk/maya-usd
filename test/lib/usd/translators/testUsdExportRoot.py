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

from maya import cmds
from maya import standalone

from pxr import Usd
from pxr import UsdGeom
from pxr import Vt
from pxr import Gf

import fixturesUtils

class testUsdExportRoot(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        filePath = os.path.join(inputPath, "UsdExportRootTest", "UsdExportRootTest.ma")
        cmds.file(filePath, force=True, open=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()
        
    def doExportImportOneMethod(self, method, shouldError=False, root=None, selection=None):
        name = 'UsdExportRoot_root-{root}_sel-{sel}'.format(root=root, sel=selection)
        if method == 'cmd':
            name += '.usda'
        elif method == 'translator':
            # the translator seems to be only able to export .usd - or at least,
            # I couldn't find an option to do .usda instead...
            name += '.usd'
        usdFile = os.path.abspath(name)
        
        if method == 'cmd':
            exportMethod = cmds.usdExport
            args = ()
            kwargs = {
                'file': usdFile,
                'mergeTransformAndShape': True,
                'shadingMode': 'none',
            }
            if root:
                kwargs['root'] = root
            if selection:
                kwargs['selection'] = 1
                cmds.select(selection)
        else:
            exportMethod = cmds.file
            args = (usdFile,)
            kwargs = {
                'type': 'USD Export',
                'f': 1,
            }
            if root:
                kwargs['options'] = 'root={}'.format(root)
            if selection:
                kwargs['exportSelected'] = 1
            else:
                kwargs['exportAll'] = 1

        if shouldError:
            self.assertRaises(RuntimeError, exportMethod, *args, **kwargs)
            return
        exportMethod(*args, **kwargs)
        return Usd.Stage.Open(usdFile)
        
    def doExportImportTest(self, validator, shouldError=False, root=None, selection=None):
        stage = self.doExportImportOneMethod('cmd', shouldError=shouldError,
                                             root=root, selection=selection)
        validator(stage)
        stage = self.doExportImportOneMethod('translator', shouldError=shouldError,
                                             root=root, selection=selection)
        validator(stage)

    def assertPrim(self, stage, path, type):
        prim = stage.GetPrimAtPath(path)
        self.assertTrue(prim.IsValid())
        self.assertEqual(prim.GetTypeName(), type)

    def assertNotPrim(self, stage, path):
        self.assertFalse(stage.GetPrimAtPath(path).IsValid())

    def testNonOverlappingSelectionRoot(self):
        # test that the command errors if we give it a root + selection that
        # have no intersection
        def validator(stage):
            pass
        self.doExportImportTest(validator, shouldError=True, root='Mid', selection='OtherTop')

    def testExportRoot_rootTop_selNone(self):
        def validator(stage):
            self.assertPrim(stage, '/Top/Mid/Cube', 'Mesh')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertPrim(stage, '/Top/OtherMid', 'Xform')
            self.assertPrim(stage, '/Top/Mid/OtherLowest', 'Xform')
        self.doExportImportTest(validator, root='Top')

    def testExportRoot_rootTop_selTop(self):
        def validator(stage):
            self.assertPrim(stage, '/Top/Mid/Cube', 'Mesh')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertPrim(stage, '/Top/OtherMid', 'Xform')
            self.assertPrim(stage, '/Top/Mid/OtherLowest', 'Xform')
        self.doExportImportTest(validator, root='Top', selection='Top')

    def testExportRoot_rootTop_selMid(self):
        def validator(stage):
            self.assertPrim(stage, '/Top/Mid/Cube', 'Mesh')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/Top/OtherMid')
            self.assertPrim(stage, '/Top/Mid/OtherLowest', 'Xform')
        self.doExportImportTest(validator, root='Top', selection='Mid')

    def testExportRoot_rootTop_selCube(self):
        def validator(stage):
            self.assertPrim(stage, '/Top/Mid/Cube', 'Mesh')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/Top/OtherMid')
            self.assertNotPrim(stage, '/Top/Mid/OtherLowest')
        self.doExportImportTest(validator, root='Top', selection='Cube')

    def testExportRoot_rootMid_selNone(self):
        def validator(stage):
            self.assertPrim(stage, '/Mid/Cube', 'Mesh')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertPrim(stage, '/Mid/OtherLowest', 'Xform')
        self.doExportImportTest(validator, root='Mid')

    def testExportRoot_rootMid_selTop(self):
        def validator(stage):
            self.assertPrim(stage, '/Mid/Cube', 'Mesh')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertPrim(stage, '/Mid/OtherLowest', 'Xform')
        self.doExportImportTest(validator, root='Mid', selection='Top')

    def testExportRoot_rootMid_selMid(self):
        def validator(stage):
            self.assertPrim(stage, '/Mid/Cube', 'Mesh')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertPrim(stage, '/Mid/OtherLowest', 'Xform')
        self.doExportImportTest(validator, root='Mid', selection='Mid')

    def testExportRoot_rootMid_selCube(self):
        def validator(stage):
            self.assertPrim(stage, '/Mid/Cube', 'Mesh')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertNotPrim(stage, '/Mid/OtherLowest')
        self.doExportImportTest(validator, root='Mid', selection='Cube')

    def testExportRoot_rootCube_selNone(self):
        def validator(stage):
            self.assertPrim(stage, '/Cube', 'Mesh')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/Mid')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertNotPrim(stage, '/OtherLowest')
        self.doExportImportTest(validator, root='Cube')

    def testExportRoot_rootCube_selTop(self):
        def validator(stage):
            self.assertPrim(stage, '/Cube', 'Mesh')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/Mid')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertNotPrim(stage, '/OtherLowest')
        self.doExportImportTest(validator, root='Cube', selection='Top')

    def testExportRoot_rootCube_selMid(self):
        def validator(stage):
            self.assertPrim(stage, '/Cube', 'Mesh')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/Mid')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertNotPrim(stage, '/OtherLowest')
        self.doExportImportTest(validator, root='Cube', selection='Mid')

    def testExportRoot_rootCube_selCube(self):
        def validator(stage):
            self.assertPrim(stage, '/Cube', 'Mesh')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/Mid')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertNotPrim(stage, '/OtherLowest')
        self.doExportImportTest(validator, root='Cube', selection='Cube')


if __name__ == '__main__':
    unittest.main(verbosity=2)
