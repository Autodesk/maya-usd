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
from math import radians

from maya import cmds
from maya import standalone

from pxr import Gf

import fixturesUtils

def _GetMayaTransform(transformName):
    '''Retrieve the transformation matrix of a Maya node.'''
    selectionList = om.MSelectionList()
    selectionList.add(transformName)
    node = selectionList.getDependNode(0)
    return om.MFnTransform(node)

def _GetMayaMatrix(transformName):
    '''Retrieve the three XYZ local rotation angles of a Maya node.'''
    mayaTransform = _GetMayaTransform(transformName)
    transformation = mayaTransform.transformation()
    return transformation.asMatrix()

def _GetRotationFromMatrix(matrix):
    return om.MTransformationMatrix(matrix).rotation()

def _GetMayaRotation(transformName):
    return _GetRotationFromMatrix(_GetMayaMatrix(transformName))

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
        cmds.upAxis(axis='z')

    def testImportChangeMayaPrefs(self):
        """Test importing and changing the Maya up-axis preference."""
        usd_file = os.path.join(self._path, "UsdImportUpAxisTests", "UpAxisY.usda")

        cmds.mayaUSDImport(file=usd_file,
                           primPath="/",
                           upAxis=1,
                           axisAndUnitMethod='overwritePrefs')
        
        expectedAxis = 'y'
        actualAxis = cmds.upAxis(query=True, axis=True)
        self.assertEqual(actualAxis, expectedAxis)

    def testImportRotateGroup(self):
        """Test importing and adding a group to hold the rotation."""
        usd_file = os.path.join(self._path, "UsdImportUpAxisTests", "UpAxisY.usda")

        cmds.mayaUSDImport(file=usd_file,
                           primPath="/",
                           upAxis=1,
                           axisAndUnitMethod='addTransform')
        
        expectedAxis = 'z'
        actualAxis = cmds.upAxis(query=True, axis=True)
        self.assertEqual(actualAxis, expectedAxis)

        rootNodes = cmds.ls('RootPrim_converted')
        self.assertEqual(len(rootNodes), 1)

        EPSILON = 1e-3

        expectedRotation = _GetRotationFromMatrix(
            om.MEulerRotation(radians(90.), radians(0.), radians(0.)).asMatrix())
        actualRotation = _GetMayaRotation(rootNodes[0])
        self.assertTrue(actualRotation.isEquivalent(expectedRotation))

        self.assertEqual(cmds.getAttr('%s.OriginalUSDUpAxis' % rootNodes[0]), 'Y')

    def testImportRotateRootNodes(self):
        """Test importing and rotating the root nodes."""
        usd_file = os.path.join(self._path, "UsdImportUpAxisTests", "UpAxisY.usda")

        cmds.mayaUSDImport(file=usd_file,
                           primPath="/",
                           upAxis=1,
                           axisAndUnitMethod='scaleRotate')
        
        expectedAxis = 'z'
        actualAxis = cmds.upAxis(query=True, axis=True)
        self.assertEqual(actualAxis, expectedAxis)

        rootNodes = cmds.ls('RootPrim')
        self.assertEqual(len(rootNodes), 1)

        EPSILON = 1e-3

        expectedRotation = _GetRotationFromMatrix(
            om.MEulerRotation(radians(90.), radians(0.), radians(0.)).asMatrix())
        actualRotation = _GetMayaRotation(rootNodes[0])
        self.assertTrue(actualRotation.isEquivalent(expectedRotation))

        self.assertEqual(cmds.getAttr('%s.OriginalUSDUpAxis' % rootNodes[0]), 'Y')

if __name__ == '__main__':
    unittest.main(verbosity=2)
