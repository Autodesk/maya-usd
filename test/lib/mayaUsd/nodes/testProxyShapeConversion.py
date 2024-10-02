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

import mayaUsdStageConversion

import maya.api.OpenMaya as om
from maya import cmds
from maya import standalone

from pxr import Gf

import os
import unittest
from math import radians

import fixturesUtils
import transformUtils
from mayaUtils import createProxyFromFile


class testProxyShapeConversion(unittest.TestCase):
    """
    Test for modifying the stage or Maya prefs to match
    the up-axis and units when opening a USD stage.
    """

    @classmethod
    def setUpClass(cls):
        cls._path = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        """Clear the scene and setup the Maya preferences."""
        cmds.file(f=True, new=True)
        # Make sure up-axis is Z.
        cmds.upAxis(axis='z')
        # Make sure the units are centimeters.
        cmds.currentUnit(linear='cm')

    def testCreateStageChangeUpAxisMayaPrefs(self):
        """Test creating a stage and changing the Maya up-axis preference."""
        usd_file = os.path.join(self._path, "ProxyShapeConversionTest", "UpAxisY.usda")
        proxyShapeNode, stage = createProxyFromFile(usd_file)

        convertUpAxis = True
        convertUnit = False
        conversionMethod = 'overwritePrefs'
        mayaUsdStageConversion.convertUpAxisAndUnit(proxyShapeNode, convertUpAxis, convertUnit, conversionMethod)
        
        expectedAxis = 'y'
        actualAxis = cmds.upAxis(query=True, axis=True)
        self.assertEqual(actualAxis, expectedAxis)

    def testCreateStageRotateUpAxis(self):
        """Test creating a stage and rotate the proxy shape."""
        usd_file = os.path.join(self._path, "ProxyShapeConversionTest", "UpAxisY.usda")
        proxyShapeNode, stage = createProxyFromFile(usd_file)

        convertUpAxis = True
        convertUnit = False
        conversionMethod = 'rotateScale'
        mayaUsdStageConversion.convertUpAxisAndUnit(proxyShapeNode, convertUpAxis, convertUnit, conversionMethod)

        expectedAxis = 'z'
        actualAxis = cmds.upAxis(query=True, axis=True)
        self.assertEqual(actualAxis, expectedAxis)

        rootNodes = cmds.ls('stage')
        self.assertEqual(len(rootNodes), 1)

        expectedRotation = transformUtils.getMayaMatrixRotation(
            om.MEulerRotation(radians(90.), radians(0.), radians(0.)).asMatrix())
        actualRotation = transformUtils.getMayaNodeRotation(rootNodes[0])
        self.assertTrue(actualRotation.isEquivalent(expectedRotation))

    def testCreateStageUpAxisNothingToDo(self):
        """Test creating a stage and having the up-axis and units already match."""
        # Make sure all preferences will match.
        cmds.upAxis(axis='y')
        cmds.currentUnit(linear='mm')

        usd_file = os.path.join(self._path, "ProxyShapeConversionTest", "UpAxisY.usda")
        proxyShapeNode, stage = createProxyFromFile(usd_file)

        convertUpAxis = True
        convertUnit = True
        conversionMethod = 'rotateScale'
        mayaUsdStageConversion.convertUpAxisAndUnit(proxyShapeNode, convertUpAxis, convertUnit, conversionMethod)

        # Verify Maya prefs were not changed.
        expectedAxis = 'y'
        actualAxis = cmds.upAxis(query=True, axis=True)
        self.assertEqual(actualAxis, expectedAxis)

        rootNodes = cmds.ls('stage')
        self.assertEqual(len(rootNodes), 1)

        # Verify no rotation was applied.
        expectedRotation = transformUtils.getMayaMatrixRotation(
            om.MEulerRotation(radians(0.), radians(0.), radians(0.)).asMatrix())
        actualRotation = transformUtils.getMayaNodeRotation(rootNodes[0])
        self.assertTrue(actualRotation.isEquivalent(expectedRotation))

    def testCreateStageUpAxisDoNotModify(self):
        """Test creating a stage and asking to leave everything alone."""
        usd_file = os.path.join(self._path, "ProxyShapeConversionTest", "UpAxisY.usda")
        proxyShapeNode, stage = createProxyFromFile(usd_file)

        # Make sure we ask not to convert up-axis and units
        convertUpAxis = False
        convertUnit = False
        conversionMethod = 'rotateScale'
        mayaUsdStageConversion.convertUpAxisAndUnit(proxyShapeNode, convertUpAxis, convertUnit, conversionMethod)

        # Verify Maya prefs were not changed.
        expectedAxis = 'z'
        actualAxis = cmds.upAxis(query=True, axis=True)
        self.assertEqual(actualAxis, expectedAxis)

        expectedUnits = 'cm'
        actualUnits = cmds.currentUnit(query=True, linear=True)
        self.assertEqual(actualUnits, expectedUnits)

        rootNodes = cmds.ls('stage')
        self.assertEqual(len(rootNodes), 1)

        # Verify no rotation was applied.
        expectedRotation = transformUtils.getMayaMatrixRotation(
            om.MEulerRotation(radians(0.), radians(0.), radians(0.)).asMatrix())
        actualRotation = transformUtils.getMayaNodeRotation(rootNodes[0])
        self.assertTrue(actualRotation.isEquivalent(expectedRotation))

    def testCreateStageChangeUnitsMayaPrefs(self):
        """Test creating a stage and changing the Maya unit preference."""
        usd_file = os.path.join(self._path, "ProxyShapeConversionTest", "UnitsMillimeters.usda")
        proxyShapeNode, stage = createProxyFromFile(usd_file)

        convertUpAxis = False
        convertUnit = True
        conversionMethod = 'overwritePrefs'
        mayaUsdStageConversion.convertUpAxisAndUnit(proxyShapeNode, convertUpAxis, convertUnit, conversionMethod)

        # Preference should have been changed.
        expectedUnits = 'mm'
        actualUnits = cmds.currentUnit(query=True, linear=True)
        self.assertEqual(actualUnits, expectedUnits)

    def testCreateStageScale(self):
        """Test creating a stage and scaling the proxy shape node."""
        usd_file = os.path.join(self._path, "ProxyShapeConversionTest", "UnitsMillimeters.usda")
        proxyShapeNode, stage = createProxyFromFile(usd_file)

        convertUpAxis = False
        convertUnit = True
        conversionMethod = 'rotateScale'
        mayaUsdStageConversion.convertUpAxisAndUnit(proxyShapeNode, convertUpAxis, convertUnit, conversionMethod)

        # Preference should have been left alone.
        expectedUnits = 'cm'
        actualUnits = cmds.currentUnit(query=True, linear=True)
        self.assertEqual(actualUnits, expectedUnits)

        rootNodes = cmds.ls('stage')
        self.assertEqual(len(rootNodes), 1)

        EPSILON = 1e-6
        expectedScaling = [0.1, 0.1, 0.1]
        actualScaling = transformUtils.getMayaNodeScaling(rootNodes[0])
        self.assertTrue(Gf.IsClose(actualScaling, expectedScaling, EPSILON))

    def testCreateStageUnitsNothingToDo(self):
        """Test creating a stage and having the up-axis and units already match."""
        # Make sure all preferences will match.
        cmds.upAxis(axis='y')
        cmds.currentUnit(linear='cm')

        usd_file = os.path.join(self._path, "ProxyShapeConversionTest", "UnitsCentimeters.usda")
        proxyShapeNode, stage = createProxyFromFile(usd_file)

        convertUpAxis = True
        convertUnit = True
        conversionMethod = 'rotateScale'
        mayaUsdStageConversion.convertUpAxisAndUnit(proxyShapeNode, convertUpAxis, convertUnit, conversionMethod)

        # Verify Maya prefs were not changed.
        expectedAxis = 'y'
        actualAxis = cmds.upAxis(query=True, axis=True)
        self.assertEqual(actualAxis, expectedAxis)

        expectedUnits = 'cm'
        actualUnits = cmds.currentUnit(query=True, linear=True)
        self.assertEqual(actualUnits, expectedUnits)

        rootNodes = cmds.ls('stage')
        self.assertEqual(len(rootNodes), 1)

        # Verify no scaling was applied.
        EPSILON = 1e-6
        expectedScaling = [1.0, 1.0, 1.0]
        actualScaling = transformUtils.getMayaNodeScaling(rootNodes[0])
        self.assertTrue(Gf.IsClose(actualScaling, expectedScaling, EPSILON))

    def testCreateStageUnitsDoNotModify(self):
        """Test creating a stage and asking to leave everything alone."""
        usd_file = os.path.join(self._path, "ProxyShapeConversionTest", "UnitsMillimeters.usda")
        proxyShapeNode, stage = createProxyFromFile(usd_file)

        # Make sure we ask not to convert up-axis and units
        convertUpAxis = False
        convertUnit = False
        conversionMethod = 'rotateScale'
        mayaUsdStageConversion.convertUpAxisAndUnit(proxyShapeNode, convertUpAxis, convertUnit, conversionMethod)

        # Verify Maya prefs were not changed.
        expectedAxis = 'z'
        actualAxis = cmds.upAxis(query=True, axis=True)
        self.assertEqual(actualAxis, expectedAxis)

        expectedUnits = 'cm'
        actualUnits = cmds.currentUnit(query=True, linear=True)
        self.assertEqual(actualUnits, expectedUnits)

        rootNodes = cmds.ls('stage')
        self.assertEqual(len(rootNodes), 1)

        # Verify no scaling was applied.
        EPSILON = 1e-6
        expectedScaling = [1.0, 1.0, 1.0]
        actualScaling = transformUtils.getMayaNodeScaling(rootNodes[0])
        self.assertTrue(Gf.IsClose(actualScaling, expectedScaling, EPSILON))


if __name__ == '__main__':
    unittest.main(verbosity=2)
