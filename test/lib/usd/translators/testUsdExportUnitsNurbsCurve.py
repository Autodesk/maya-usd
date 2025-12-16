#!/usr/bin/env mayapy
#
# Copyright 2025 Autodesk
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

from pxr import Usd, UsdGeom, Vt, Gf

import fixturesUtils

class testUsdExportUnitsNurbsCurve(unittest.TestCase):
    """Test for modifying the units when exporting nurbscurves."""

    @classmethod
    def setUpClass(cls):
        cls._path = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        """Clear the scene"""
        filePath = os.path.join(self._path, "UsdExportNurbsCurveTest", "UsdExportNurbsCurveTest.ma")
        cmds.file(filePath, force=True, open=True)
        cmds.currentUnit(linear='cm')

    def _prepareScene(self, expectedWidths):
        cmds.select("curveShape1")
        cmds.addAttr(longName="widths", dt="floatArray")
        cmds.setAttr("curveShape1.widths", expectedWidths ,type="floatArray")
        self.assertEqual(cmds.getAttr("curveShape1.widths"), expectedWidths)

    def _exportScene(self, unitName):
        usdFile = os.path.abspath(self._testMethodName + '.usda')
        cmds.mayaUSDExport(
            file=usdFile,
            mergeTransformAndShape=True,
            shadingMode='none',
            unit=unitName)
        return Usd.Stage.Open(usdFile)

    def _runTestExportUnits(self, unitName, expectedMetersPerUnit):
        """Test exporting a nurbscurve with some units."""
        expectedWidths = [1., 2., 3., 4., 5., 6., 7., 8.]

        self._prepareScene(expectedWidths)
        stage = self._exportScene(unitName)

        if unitName == 'none':
            self.assertFalse(stage.HasAuthoredMetadata('metersPerUnit'))
            expectedMetersPerUnit = 0.01
        else:
            self.assertTrue(stage.HasAuthoredMetadata('metersPerUnit'))
            actualMetersPerUnit = UsdGeom.GetStageMetersPerUnit(stage)
            self.assertAlmostEqual(actualMetersPerUnit, expectedMetersPerUnit)

        expectedScale = 0.01 / expectedMetersPerUnit
        expectedDelta = expectedScale / 10000.

        nc = UsdGeom.NurbsCurves.Get(stage, '/curve1')

        expectedPoints = [
            (2.1462670734937892, 3.79699800534065, 0,),
            (0.34560012320021272, 7.2176440187272055, 0),
            (-3.2557337773869248, 14.058936045500237, 0),
            (19.029339405045754, -11.122707012705446, 0),
            (10.772403951593377, -25.203911542506702, 0),
            (-16.553127753074023, 10.987919687634868, 0),
            (-16.859245770764005, 10.380012881071295, 0),
            (-17.012304779609007, 10.076059477789517, 0),
        ]

        for i, pt in enumerate(nc.GetPointsAttr().Get()):
            self.assertAlmostEqual(pt[0], expectedPoints[i][0] * expectedScale, delta=expectedDelta)
            self.assertAlmostEqual(pt[1], expectedPoints[i][1] * expectedScale, delta=expectedDelta)
            self.assertAlmostEqual(pt[2], expectedPoints[i][2] * expectedScale, delta=expectedDelta)

        for i, width in enumerate(nc.GetWidthsAttr().Get()):
            expectedWidth = expectedWidths[i] * expectedScale
            self.assertAlmostEqual(width, expectedWidth, delta=expectedDelta)

        self.assertEqual(nc.GetWidthsInterpolation(), UsdGeom.Tokens.vertex)
        self.assertEqual(nc.GetKnotsAttr().Get(), Vt.DoubleArray([0, 0, 0, 0, 1, 2, 3, 4, 5, 5, 5, 5]))
        self.assertEqual(nc.GetRangesAttr().Get(), Vt.Vec2dArray([Gf.Vec2d(0, 5)]))
        self.assertEqual(nc.GetOrderAttr().Get(), Vt.IntArray([4]))
        self.assertEqual(nc.GetCurveVertexCountsAttr().Get(), Vt.IntArray([8]))

    def testExportUnitsMayaPrefs(self):
        """Test exporting and following the Maya unit preference when they differ from the internal units."""
        self._runTestExportUnits('mayaPrefs', 0.01)

    def testExportUnitsFollowDifferentMayaPrefs(self):
        """Test exporting and following the Maya unit preference when they differ from the internal units."""
        cmds.currentUnit(linear='mm')
        self._runTestExportUnits('mayaPrefs', 0.001)

    def testExportUnitsNone(self):
        """Test exporting without any units."""
        self._runTestExportUnits('none', 0.01)

    def testExportUnitsNanometers(self):
        """Test exporting and forcing units of nanometers, different from Maya prefs."""
        self._runTestExportUnits('nm', 1e-9)

    def testExportUnitsMicrometers(self):
        """Test exporting and forcing units of micrometers, different from Maya prefs."""
        self._runTestExportUnits('um', 1e-6)

    def testExportUnitsMillimeters(self):
        """Test exporting and forcing units of millimeters, different from Maya prefs."""
        self._runTestExportUnits('mm', 1e-3)

    def testExportUnitsDecimeters(self):
        """Test exporting and forcing units of decimeters, different from Maya prefs."""
        self._runTestExportUnits('dm', 1e-1)

    def testExportUnitsMeters(self):
        """Test exporting and forcing units of meters, different from Maya prefs."""
        self._runTestExportUnits('m', 1.)

    def testExportUnitsKilometers(self):
        """Test exporting and forcing units of kilometers, different from Maya prefs."""
        self._runTestExportUnits('km', 1000.)

    def testExportUnitsInches(self):
        """Test exporting and forcing units of inches, different from Maya prefs."""
        self._runTestExportUnits('inch', 0.0254)

    def testExportUnitsFeet(self):
        """Test exporting and forcing units of feet, different from Maya prefs."""
        self._runTestExportUnits('foot', 0.3048)

    def testExportUnitsYards(self):
        """Test exporting and forcing units of yards, different from Maya prefs."""
        self._runTestExportUnits('yard', 0.9144)

    def testExportUnitsMiles(self):
        """Test exporting and forcing units of miles, different from Maya prefs."""
        self._runTestExportUnits('mile', 1609.344)


if __name__ == '__main__':
    unittest.main(verbosity=2)
