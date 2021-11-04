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

class testUsdExportBatchCurves(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        filePath = os.path.join(inputPath, "UsdImportBasisCurvesTest", "batchBezier.usda")
        cmds.file(filePath, force=True, open=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExport(self):
        #Check the imported Usd.
        selectionList = OpenMaya.MSelectionList()
        selectionList.add('NurbsBatchShape')
        dagPath = OpenMaya.MDagPath()
        selectionList.getDagPath( 0, dagPath )
        MFnNurbsCurve = OpenMaya.MFnNurbsCurve(dagPath)
        self.assertEqual(MFnNurbsCurve.numCVs(), 10)

        selectionList.clear()
        selectionList.add('NurbsBatchShape1')
        dagPath = OpenMaya.MDagPath()
        selectionList.getDagPath( 0, dagPath )
        MFnNurbsCurve = OpenMaya.MFnNurbsCurve(dagPath)
        self.assertEqual(MFnNurbsCurve.numCVs(), 4)

        #Export the Maya file and validate a handful of properties.
        usdFile = os.path.abspath('UsdExportBatchBezierTest.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none')

        stage = Usd.Stage.Open(usdFile)
        nc = UsdGeom.BasisCurves.Get(stage, '/NurbsBatch/NurbsBatchShape')
        self.assertEqual(nc.GetWidthsAttr().Get(), Vt.FloatArray([1.0]))
        self.assertEqual(nc.GetWidthsInterpolation(), UsdGeom.Tokens.constant)
        self.assertEqual(nc.GetCurveVertexCountsAttr().Get(), Vt.IntArray([10]))

        nc = UsdGeom.BasisCurves.Get(stage, '/NurbsBatch/NurbsBatchShape1')
        self.assertEqual(nc.GetWidthsAttr().Get(), Vt.FloatArray([1.0]))
        self.assertEqual(nc.GetWidthsInterpolation(), UsdGeom.Tokens.constant)
        self.assertEqual(nc.GetCurveVertexCountsAttr().Get(), Vt.IntArray([4]))


if __name__ == '__main__':
    unittest.main(verbosity=2)
