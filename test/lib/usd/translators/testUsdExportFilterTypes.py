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

class testUsdExportFilterTypes(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        filePath = os.path.join(inputPath, "UsdExportFilterTypesTest", "UsdExportFilterTypesTest.ma")
        cmds.file(filePath, force=True, open=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def doExportImportOneMethod(self, method, constraint=True, place3d=True,
                                nonExist=True, mergeXform=False):
        name = 'UsdExportFilterTypes'
        filter =[]
        if not constraint:
            name += '_noConstraint'
            filter.append('constraint')
        if not place3d:
            name += '_noPlace3d'
            filter.append('place3dTexture')
        if not nonExist:
            name += '_noNonExist'
            filter.append('nonExist')
        if not mergeXform:
            name += '_noMerge'
        if method == 'cmd':
            name += '.usda'
        elif method == 'translator':
            # the translator seems to be only able to export .usd - or at least,
            # I couldn't find an option to do .usda instead...
            name += '.usd'
        else:
            raise ArgumentError("unknown method: {}".format(method))
        usdFile = os.path.abspath(name)
        
        if method == 'cmd':
            exportMethod = cmds.usdExport
            args = ()
            kwargs = {
                'file': usdFile,
                'mergeTransformAndShape': mergeXform,
                'shadingMode': 'none',
            }
            if filter:
                kwargs['filterTypes'] = tuple(filter)
        else:
            exportMethod = cmds.file
            args = (usdFile,)
            kwargs = {
                'type': fixturesUtils.exportTranslatorName(),
                'exportAll': 1,
                'f': 1,
            }
            options = {
                'mergeTransformAndShape': int(mergeXform),
                'shadingMode': 'none',
            }
            if filter:
                options['filterTypes'] = ','.join(filter)
            kwargs['options'] = ';'.join('{}={}'.format(key, options[key])
                                         for key in options.keys())
        print("running: {}(*{!r}, **{!r})".format(exportMethod.__name__, args, kwargs))
        exportMethod(*args, **kwargs)
        return Usd.Stage.Open(usdFile)

    def doExportImportTest(self, validator, constraint=True, place3d=True,
                           mergeXform=False):
        # test with all 4 combinations of nonExist and cmd/translator,
        # since none should affect the result 
        stage = self.doExportImportOneMethod('cmd',
                                             constraint=constraint,
                                             place3d=place3d,
                                             nonExist=True,
                                             mergeXform=mergeXform)
        validator(stage)
        stage = self.doExportImportOneMethod('cmd',
                                             constraint=constraint,
                                             place3d=place3d,
                                             nonExist=False,
                                             mergeXform=mergeXform)
        validator(stage)
        stage = self.doExportImportOneMethod('translator',
                                             constraint=constraint,
                                             place3d=place3d,
                                             nonExist=True,
                                             mergeXform=mergeXform)
        validator(stage)
        stage = self.doExportImportOneMethod('translator',
                                             constraint=constraint,
                                             place3d=place3d,
                                             nonExist=False,
                                             mergeXform=mergeXform)
        validator(stage)

    def assertPrim(self, stage, path, type):
        prim = stage.GetPrimAtPath(path)
        self.assertTrue(prim.IsValid())
        self.assertEqual(prim.GetTypeName(), type)

    def assertNotPrim(self, stage, path):
        self.assertFalse(stage.GetPrimAtPath(path).IsValid())

    def testExportFilterTypes_noFilters_noMerge(self):
        def validator(stage):
            self.assertPrim(stage, '/pCube1', 'Xform')
            self.assertPrim(stage, '/pCube1/pCubeShape1', 'Mesh')
            self.assertPrim(stage, '/pCube1/place3dTexture1', 'Xform')
            self.assertPrim(stage, '/pPyramid1', 'Xform')
            self.assertPrim(stage, '/pPyramid1/pPyramidShape1', 'Mesh')
            self.assertPrim(stage, '/pPyramid1/pPyramid1_parentConstraint1', 'Xform')
        self.doExportImportTest(validator)

    def testExportFilterTypes_noConstraint_noMerge(self):
        def validator(stage):
            self.assertPrim(stage, '/pCube1', 'Xform')
            self.assertPrim(stage, '/pCube1/pCubeShape1', 'Mesh')
            self.assertPrim(stage, '/pCube1/place3dTexture1', 'Xform')
            self.assertPrim(stage, '/pPyramid1', 'Xform')
            self.assertPrim(stage, '/pPyramid1/pPyramidShape1', 'Mesh')
            self.assertNotPrim(stage, '/pPyramid1/pPyramid1_parentConstraint1')
        self.doExportImportTest(validator, constraint=False)

    def testExportFilterTypes_noPlace3d_noMerge(self):
        def validator(stage):
            self.assertPrim(stage, '/pCube1', 'Xform')
            self.assertPrim(stage, '/pCube1/pCubeShape1', 'Mesh')
            self.assertNotPrim(stage, '/pCube1/place3dTexture1')
            self.assertPrim(stage, '/pPyramid1', 'Xform')
            self.assertPrim(stage, '/pPyramid1/pPyramidShape1', 'Mesh')
            self.assertPrim(stage, '/pPyramid1/pPyramid1_parentConstraint1', 'Xform')
        self.doExportImportTest(validator, place3d=False)

    def testExportFilterTypes_noConstraintPlace3d_noMerge(self):
        def validator(stage):
            self.assertPrim(stage, '/pCube1', 'Xform')
            self.assertPrim(stage, '/pCube1/pCubeShape1', 'Mesh')
            self.assertNotPrim(stage, '/pCube1/place3dTexture1')
            self.assertPrim(stage, '/pPyramid1', 'Xform')
            self.assertPrim(stage, '/pPyramid1/pPyramidShape1', 'Mesh')
            self.assertNotPrim(stage, '/pPyramid1/pPyramid1_parentConstraint1')
        self.doExportImportTest(validator, constraint=False, place3d=False)

    def testExportFilterTypes_noFilters_merge(self):
        def validator(stage):
            self.assertPrim(stage, '/pCube1', 'Xform')
            self.assertPrim(stage, '/pCube1/pCubeShape1', 'Mesh')
            self.assertPrim(stage, '/pCube1/place3dTexture1', 'Xform')
            self.assertPrim(stage, '/pPyramid1', 'Xform')
            self.assertPrim(stage, '/pPyramid1/pPyramidShape1', 'Mesh')
            self.assertPrim(stage, '/pPyramid1/pPyramid1_parentConstraint1', 'Xform')
        self.doExportImportTest(validator, mergeXform=True)

    def testExportFilterTypes_noConstraint_merge(self):
        def validator(stage):
            self.assertPrim(stage, '/pCube1', 'Xform')
            self.assertPrim(stage, '/pCube1/pCubeShape1', 'Mesh')
            self.assertPrim(stage, '/pCube1/place3dTexture1', 'Xform')
            self.assertPrim(stage, '/pPyramid1', 'Mesh')
            self.assertNotPrim(stage, '/pPyramid1/pPyramidShape1')
            self.assertNotPrim(stage, '/pPyramid1/pPyramid1_parentConstraint1')
        self.doExportImportTest(validator, constraint=False, mergeXform=True)

    def testExportFilterTypes_noPlace3d_merge(self):
        def validator(stage):
            self.assertPrim(stage, '/pCube1', 'Mesh')
            self.assertNotPrim(stage, '/pCube1/pCubeShape1')
            self.assertNotPrim(stage, '/pCube1/place3dTexture1')
            self.assertPrim(stage, '/pPyramid1', 'Xform')
            self.assertPrim(stage, '/pPyramid1/pPyramidShape1', 'Mesh')
            self.assertPrim(stage, '/pPyramid1/pPyramid1_parentConstraint1', 'Xform')
        self.doExportImportTest(validator, place3d=False, mergeXform=True)

    def testExportFilterTypes_noConstraintPlace3d_merge(self):
        def validator(stage):
            self.assertPrim(stage, '/pCube1', 'Mesh')
            self.assertNotPrim(stage, '/pCube1/pCubeShape1')
            self.assertNotPrim(stage, '/pCube1/place3dTexture1')
            self.assertPrim(stage, '/pPyramid1', 'Mesh')
            self.assertNotPrim(stage, '/pPyramid1/pPyramidShape1')
            self.assertNotPrim(stage, '/pPyramid1/pPyramid1_parentConstraint1')
        self.doExportImportTest(validator, constraint=False, place3d=False, mergeXform=True)


if __name__ == '__main__':
    unittest.main(verbosity=2)
