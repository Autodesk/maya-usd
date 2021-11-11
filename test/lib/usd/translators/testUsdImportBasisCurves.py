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

class testUsdExportBasisCurves(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        filePath = os.path.join(inputPath, "UsdImportBasisCurvesTest", "basisCurve.usda")
        cmds.file(filePath, force=True, open=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExport(self):
        #Check the imported Usd.
        selectionList = OpenMaya.MSelectionList()
        selectionList.add('Curve')
        dagPath = OpenMaya.MDagPath()
        selectionList.getDagPath( 0, dagPath )
        MFnNurbsCurve = OpenMaya.MFnNurbsCurve(dagPath)
        self.assertEqual(MFnNurbsCurve.numCVs(), 7)

        #Export the Maya file and validate a handful of properties.
        usdFile = os.path.abspath('UsdExportBasisCurveTest.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none')

        stage = Usd.Stage.Open(usdFile)
        nc = UsdGeom.BasisCurves.Get(stage, '/Curve')
        self.assertEqual(nc.GetWidthsAttr().Get(), Vt.FloatArray([1.0]))
        self.assertEqual(nc.GetWidthsInterpolation(), UsdGeom.Tokens.constant)
        self.assertEqual(nc.GetCurveVertexCountsAttr().Get(), Vt.IntArray([7]))



if __name__ == '__main__':
    unittest.main(verbosity=2)
