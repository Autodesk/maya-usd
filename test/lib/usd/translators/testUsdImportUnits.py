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

from pxr import Gf

import fixturesUtils


def _GetMayaTransform(transformName):
    '''Retrieve the Maya SDK transformation API (MFnTransform) of a Maya node.'''
    selectionList = om.MSelectionList()
    selectionList.add(transformName)
    node = selectionList.getDependNode(0)
    return om.MFnTransform(node)

def _GetMayaMatrix(transformName):
    '''Retrieve the transformation matrix (MMatrix) of a Maya node.'''
    mayaTransform = _GetMayaTransform(transformName)
    transformation = mayaTransform.transformation()
    return transformation.asMatrix()

def _GetScalingFromMatrix(matrix):
    '''Extract the scaling from a Maya matrix.'''
    return om.MTransformationMatrix(matrix).scale(om.MSpace.kObject)

def _GetMayaScaling(transformName):
    '''Extract the scaling from a Maya node.'''
    return _GetScalingFromMatrix(_GetMayaMatrix(transformName))


class testUsdImportUpAxis(unittest.TestCase):
    """Test for modifying the up-axis when importing."""

    @classmethod
    def setUpClass(cls):
        cls._path = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        """Clear the scene"""
        cmds.file(f=True, new=True)
        # Make sure up-axis is Y like in teh USD file.
        cmds.upAxis(axis='y')
        # Make sure the units are centimeters.
        cmds.currentUnit(linear='cm')

    @unittest.skip('Preference is changed on idle, so we cannot get the new settings in a test.')
    def testImportChangeMayaPrefs(self):
        """Test importing and changing the Maya unit preference."""
        usd_file = os.path.join(self._path, "UsdImportUnitsTests", "UnitsMillimeters.usda")

        cmds.mayaUSDImport(file=usd_file,
                           primPath="/",
                           unit=1,
                           axisAndUnitMethod='overwritePrefs')

        cmds.sleep

        # Preference should have been changed.
        expectedUnits = 'mm'
        actualUnits = cmds.currentUnit(query=True, linear=True)
        self.assertEqual(actualUnits, expectedUnits)

    def testImportScaleGroup(self):
        """Test importing and adding a group to hold the scaling."""
        usd_file = os.path.join(self._path, "UsdImportUnitsTests", "UnitsMillimeters.usda")

        cmds.mayaUSDImport(file=usd_file,
                           primPath="/",
                           unit=1,
                           axisAndUnitMethod='addTransform')
        
        # Preference should have been left alone.
        expectedUnits = 'cm'
        actualUnits = cmds.currentUnit(query=True, linear=True)
        self.assertEqual(actualUnits, expectedUnits)

        rootNodes = cmds.ls('RootPrim_converted')
        self.assertEqual(len(rootNodes), 1)

        EPSILON = 1e-6

        expectedScaling = [0.1, 0.1, 0.1]
        actualScaling = _GetMayaScaling(rootNodes[0])
        self.assertTrue(Gf.IsClose(actualScaling, expectedScaling, EPSILON))

        self.assertEqual(cmds.getAttr('%s.OriginalUSDMetersPerUnit' % rootNodes[0]), '0.001')

    def testImportScaleRootNodes(self):
        """Test importing and scaling the root nodes."""
        usd_file = os.path.join(self._path, "UsdImportUnitsTests", "UnitsMillimeters.usda")

        cmds.mayaUSDImport(file=usd_file,
                           primPath="/",
                           unit=1,
                           axisAndUnitMethod='scaleRotate')
        
        # Preference should have been left alone.
        expectedUnits = 'cm'
        actualUnits = cmds.currentUnit(query=True, linear=True)
        self.assertEqual(actualUnits, expectedUnits)

        rootNodes = cmds.ls('RootPrim')
        self.assertEqual(len(rootNodes), 1)

        EPSILON = 1e-6

        expectedScaling = [0.1, 0.1, 0.1]
        actualScaling = _GetMayaScaling(rootNodes[0])
        self.assertTrue(Gf.IsClose(actualScaling, expectedScaling, EPSILON))

        self.assertEqual(cmds.getAttr('%s.OriginalUSDMetersPerUnit' % rootNodes[0]), '0.001')


if __name__ == '__main__':
    unittest.main(verbosity=2)
