#!/usr/bin/env mayapy
#
# Copyright 2024 Autodesk
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
import transformUtils

class testUsdExportUnits(unittest.TestCase):
    """Test for modifying the units when exporting."""

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

    def testExportUnitsNone(self):
        """Test exporting without any units."""
        cmds.polySphere()
        cmds.move(3, 0, 0, relative=True)

        usdFile = os.path.abspath('UsdExportUnits_None.usda')
        cmds.mayaUSDExport(file=usdFile,
                           shadingMode='none',
                           unit='none')

        stage = Usd.Stage.Open(usdFile)
        self.assertFalse(stage.HasAuthoredMetadata('metersPerUnit'))

        spherePrim = stage.GetPrimAtPath('/pSphere1')
        self.assertTrue(spherePrim)

        transformUtils.assertPrimXforms(self, spherePrim, [
            ('xformOp:translate', (3., 0., 0.))])

    def testExportUnitsFollowMayaPrefs(self):
        """Test exporting and following the Maya unit preference."""
        cmds.polySphere()
        cmds.move(0, 0, 3, relative=True)

        usdFile = os.path.abspath('UsdExportUnits_FollowMayaPrefs.usda')
        cmds.mayaUSDExport(file=usdFile,
                           shadingMode='none',
                           unit='mayaPrefs')
        
        stage = Usd.Stage.Open(usdFile)
        self.assertTrue(stage.HasAuthoredMetadata('metersPerUnit'))

        expectedMetersPerUnit = 0.01
        actualMetersPerUnit = UsdGeom.GetStageMetersPerUnit(stage)
        self.assertEqual(actualMetersPerUnit, expectedMetersPerUnit)

        spherePrim = stage.GetPrimAtPath('/pSphere1')
        self.assertTrue(spherePrim)

        transformUtils.assertPrimXforms(self, spherePrim, [
            ('xformOp:translate', (0., 0., 3.))])

    def testExportUnitsFollowDifferentMayaPrefs(self):
        """Test exporting and following the Maya unit preference when they differ from the internal units."""
        cmds.polySphere()
        cmds.move(0, 0, 3, relative=True)

        cmds.currentUnit(linear='mm')

        usdFile = os.path.abspath('UsdExportUnits_FollowMayaPrefs.usda')
        cmds.mayaUSDExport(file=usdFile,
                           shadingMode='none',
                           unit='mayaPrefs')
        
        stage = Usd.Stage.Open(usdFile)
        self.assertTrue(stage.HasAuthoredMetadata('metersPerUnit'))

        expectedMetersPerUnit = 0.001
        actualMetersPerUnit = UsdGeom.GetStageMetersPerUnit(stage)
        self.assertEqual(actualMetersPerUnit, expectedMetersPerUnit)

        spherePrim = stage.GetPrimAtPath('/pSphere1')
        self.assertTrue(spherePrim)

        transformUtils.assertPrimXforms(self, spherePrim, [
            ('xformOp:translate', (0., 0., 30.)),
        ])

    def testExportUnitsNanometers(self):
        """Test exporting and forcing units of nanometers, different from Maya prefs."""
        self._runTestExportUnitsDifferentUnits('nm', 1e-9)

    def testExportUnitsMicrometers(self):
        """Test exporting and forcing units of micrometers, different from Maya prefs."""
        self._runTestExportUnitsDifferentUnits('um', 1e-6)

    def testExportUnitsMillimeters(self):
        """Test exporting and forcing units of millimeters, different from Maya prefs."""
        self._runTestExportUnitsDifferentUnits('mm', 1e-3)

    def testExportUnitsDecimeters(self):
        """Test exporting and forcing units of decimeters, different from Maya prefs."""
        self._runTestExportUnitsDifferentUnits('dm', 1e-1)

    def testExportUnitsMeters(self):
        """Test exporting and forcing units of meters, different from Maya prefs."""
        self._runTestExportUnitsDifferentUnits('m', 1.)

    def testExportUnitsKilometers(self):
        """Test exporting and forcing units of kilometers, different from Maya prefs."""
        self._runTestExportUnitsDifferentUnits('km', 1000.)

    def testExportUnitsInches(self):
        """Test exporting and forcing units of inches, different from Maya prefs."""
        self._runTestExportUnitsDifferentUnits('inch', 0.0254)

    def testExportUnitsFeet(self):
        """Test exporting and forcing units of feet, different from Maya prefs."""
        self._runTestExportUnitsDifferentUnits('foot', 0.3048)

    def testExportUnitsYards(self):
        """Test exporting and forcing units of yards, different from Maya prefs."""
        self._runTestExportUnitsDifferentUnits('yard', 0.9144)

    def testExportUnitsMiles(self):
        """Test exporting and forcing units of miles, different from Maya prefs."""
        self._runTestExportUnitsDifferentUnits('mile', 1609.344)

    def _runTestExportUnitsDifferentUnits(self, unitName, expectedMetersPerUnit):
        """Test exporting and forcing units, different from Maya prefs."""
        cmds.polySphere()
        cmds.move(0, 0, 3, relative=True)

        usdFile = os.path.abspath('UsdExportUnits_DifferentY.usda')
        cmds.mayaUSDExport(file=usdFile,
                           shadingMode='none',
                           unit=unitName)
        
        stage = Usd.Stage.Open(usdFile)
        self.assertTrue(stage.HasAuthoredMetadata('metersPerUnit'))

        actualMetersPerUnit = UsdGeom.GetStageMetersPerUnit(stage)
        self.assertAlmostEqual(actualMetersPerUnit, expectedMetersPerUnit)

        spherePrim = stage.GetPrimAtPath('/pSphere1')
        self.assertTrue(spherePrim)

        expectedScale = 0.01 / expectedMetersPerUnit
        transformUtils.assertPrimXforms(self, spherePrim, [
            ('xformOp:translate', (0., 0., 3. * expectedScale)),
        ])


if __name__ == '__main__':
    unittest.main(verbosity=2)
