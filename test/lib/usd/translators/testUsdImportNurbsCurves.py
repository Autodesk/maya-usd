#!/usr/bin/env mayapy
#
# Copyright 2021 Autodesk
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
from maya import OpenMaya

from pxr import Gf
from pxr import Usd
from pxr import UsdGeom
from pxr import Vt

import fixturesUtils

class testUsdImportNurbsCurves(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testImport(self):
        '''Test that importing a curve from USD works.'''
        filePath = os.path.join(self.inputPath, "UsdImportBasisCurvesTest", "nurbsCurve.usda")
        cmds.file(filePath, force=True, open=True)

        selectionList = OpenMaya.MSelectionList()
        selectionList.add('Nurbs')
        dagPath = OpenMaya.MDagPath()
        selectionList.getDagPath( 0, dagPath )
        MFnNurbsCurve = OpenMaya.MFnNurbsCurve(dagPath)
        self.assertEqual(MFnNurbsCurve.numCVs(), 7)

        #Export the Maya file and validate a handful of properties.
        usdFile = os.path.abspath('UsdExportNurbsCurveTest.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none')

        stage = Usd.Stage.Open(usdFile)
        nc = UsdGeom.NurbsCurves.Get(stage, '/Nurbs')
        self.assertEqual(nc.GetWidthsAttr().Get(), Vt.FloatArray([1.0]))
        self.assertEqual(nc.GetRangesAttr().Get(), Vt.Vec2dArray([Gf.Vec2d(0, 2)]))
        self.assertEqual(nc.GetOrderAttr().Get(), Vt.IntArray([4]))
        self.assertEqual(nc.GetWidthsInterpolation(), UsdGeom.Tokens.constant)
        self.assertEqual(nc.GetCurveVertexCountsAttr().Get(), Vt.IntArray([7]))

    def testImportInvalidCurves(self):
        '''Test that importing a file with invalid curve does not crash.'''
        filePath = os.path.join(self.inputPath, "UsdImportBasisCurvesTest", "invalidNurbsCurves.usda")
        cmds.file(filePath, force=True, open=True)
        # The goal of the test is merely that Maya does not crash, but do check one node
        # to make sure the file was imported.
        selectionList = OpenMaya.MSelectionList()
        selectionList.add('nurbs_curve1_0')
        dagPath = OpenMaya.MDagPath()
        selectionList.getDagPath( 0, dagPath )
        MFnNurbsCurve = OpenMaya.MFnNurbsCurve(dagPath)
        self.assertEqual(MFnNurbsCurve.numCVs(), 4)


if __name__ == '__main__':
    unittest.main(verbosity=2)