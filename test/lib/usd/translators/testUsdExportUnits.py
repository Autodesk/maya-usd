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

import maya.api.OpenMaya as om
import os
import unittest

from maya import cmds
from maya import standalone

from pxr import Gf, Usd, UsdGeom

import fixturesUtils

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

    def assertPrimXform(self, prim, xforms):
        '''
        Verify that the prim has the given xform in the roder given.
        xforms should be a list of pairs, each containing the xform op name and its value.
        '''
        EPSILON = 1e-3
        xformOpOrder = prim.GetAttribute('xformOpOrder').Get()
        self.assertEqual(len(xformOpOrder), len(xforms))
        for name, value in xforms:
            self.assertEqual(xformOpOrder[0], name)
            attr = prim.GetAttribute(name)
            self.assertIsNotNone(attr)
            self.assertTrue(Gf.IsClose(attr.Get(), value, EPSILON))
            # Chop off the first xofrm op for the next loop.
            xformOpOrder = xformOpOrder[1:]

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

        self.assertPrimXform(spherePrim, [
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

        self.assertPrimXform(spherePrim, [
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

        self.assertPrimXform(spherePrim, [
            ('xformOp:translate', (0., 0., 30.)),
            ('xformOp:scale', (10., 10., 10.))])

    def testExportUnitsDifferentUnits(self):
        """Test exporting and forcing units of kilometers, different from Maya prefs."""
        cmds.polySphere()
        cmds.move(0, 0, 3, relative=True)

        usdFile = os.path.abspath('UsdExportUnits_DifferentY.usda')
        cmds.mayaUSDExport(file=usdFile,
                           shadingMode='none',
                           unit='km')
        
        stage = Usd.Stage.Open(usdFile)
        self.assertTrue(stage.HasAuthoredMetadata('metersPerUnit'))

        expectedMetersPerUnit = 1000.
        actualMetersPerUnit = UsdGeom.GetStageMetersPerUnit(stage)
        self.assertEqual(actualMetersPerUnit, expectedMetersPerUnit)

        spherePrim = stage.GetPrimAtPath('/pSphere1')
        self.assertTrue(spherePrim)

        self.assertPrimXform(spherePrim, [
            ('xformOp:translate', (0., 0., 0.00003)),
            ('xformOp:scale', (0.00001, 0.00001, 0.00001))])


if __name__ == '__main__':
    unittest.main(verbosity=2)
