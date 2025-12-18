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

from pxr import Usd, UsdGeom

import fixturesUtils

class testUsdExportUnitsCamera(unittest.TestCase):
    """Test for modifying the units when exporting cameras."""

    @classmethod
    def setUpClass(cls):
        cls._path = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        """Clear the scene"""
        cmds.file(f=True, new=True)
        cmds.currentUnit(linear='cm')

    def _prepareScene(self):
        cmds.camera(
            focalLength=35,
            horizontalFilmAperture=2,
            horizontalFilmOffset=1,
            verticalFilmAperture=3,
            verticalFilmOffset=4,
            depthOfField=1,
            focusDistance=10,
            nearClipPlane=1,
            farClipPlane=10000,
        )

    def _exportScene(self, unitName):
        usdFile = os.path.abspath(self._testMethodName + '.usda')
        cmds.mayaUSDExport(
            file=usdFile,
            shadingMode='none',
            unit=unitName)
        return Usd.Stage.Open(usdFile)

    def _runTestExportUnits(self, unitName, expectedMetersPerUnit):
        """Test exporting a camera with some units."""
        self._prepareScene()
        stage = self._exportScene(unitName)

        if unitName == 'none':
            self.assertFalse(stage.HasAuthoredMetadata('metersPerUnit'))
            expectedMetersPerUnit = 0.01
        else:
            self.assertTrue(stage.HasAuthoredMetadata('metersPerUnit'))
            actualMetersPerUnit = UsdGeom.GetStageMetersPerUnit(stage)
            self.assertAlmostEqual(actualMetersPerUnit, expectedMetersPerUnit)

        expectedScale = 0.01 / expectedMetersPerUnit

        cameraPrim = stage.GetPrimAtPath('/camera1')
        self.assertTrue(cameraPrim)

        inchToMM = 25.4

        value = cameraPrim.GetAttribute('focalLength').Get()
        self.assertAlmostEqual(value, 35.0 * expectedScale, places=5)

        value = cameraPrim.GetAttribute('horizontalAperture').Get()
        self.assertAlmostEqual(value, 2.0 * expectedScale * inchToMM, places=5)

        value = cameraPrim.GetAttribute('horizontalApertureOffset').Get()
        self.assertAlmostEqual(value, 1.0 * expectedScale * inchToMM, places=5)

        value = cameraPrim.GetAttribute('verticalAperture').Get()
        self.assertAlmostEqual(value, 3.0 * expectedScale * inchToMM, places=5)

        value = cameraPrim.GetAttribute('verticalApertureOffset').Get()
        self.assertAlmostEqual(value, 4.0 * expectedScale * inchToMM, places=5)

        # Maya focus distance and clipping range are not affected by its units preference
        # for some reason, so we compensate for the prefs test in mm mode.
        extraFactor = 1.0
        if unitName == 'mayaPrefs':
            curUnit = cmds.currentUnit(query=True, linear=True)
            if curUnit == 'mm':
                extraFactor = 0.1

        value = cameraPrim.GetAttribute('focusDistance').Get()
        self.assertAlmostEqual(value, 10.0 * expectedScale * extraFactor, places=5)

        value = cameraPrim.GetAttribute('clippingRange').Get()
        self.assertAlmostEqual(value[0], 1.0 * expectedScale * extraFactor, places=3)
        self.assertAlmostEqual(value[1], 10000.0 * expectedScale * extraFactor, delta=expectedScale/1000)

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
