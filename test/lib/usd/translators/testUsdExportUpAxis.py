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

class testUsdExportUpAxis(unittest.TestCase):
    """Test for modifying the up-axis when exporting."""

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

    def testExportUpAxisNone(self):
        """Test importing and adding a group to hold the rotation."""
        cmds.polySphere()
        cmds.move(3, 0, 0, relative=True)

        usdFile = os.path.abspath('UsdExportUpAxis_None.usda')
        cmds.mayaUSDExport(file=usdFile,
                           shadingMode='none',
                           upAxis='none')

        stage = Usd.Stage.Open(usdFile)
        self.assertFalse(stage.HasAuthoredMetadata('upAxis'))

        spherePrim = UsdGeom.Mesh.Get(stage, '/pSphere1')
        self.assertTrue(spherePrim)

    def testExportUpAxisFollowMayaPrefs(self):
        """Test exporting and following the Maya up-axis preference."""
        cmds.polySphere()
        cmds.move(0, 0, 3, relative=True)

        usdFile = os.path.abspath('UsdExportUpAxis_FollowMayaPrefs.usda')
        cmds.mayaUSDExport(file=usdFile,
                           shadingMode='none',
                           upAxis='mayaPrefs')
        
        stage = Usd.Stage.Open(usdFile)
        self.assertTrue(stage.HasAuthoredMetadata('upAxis'))
        expectedAxis = 'Z'
        actualAxis = UsdGeom.GetStageUpAxis(stage)
        self.assertEqual(actualAxis, expectedAxis)

        spherePrim = stage.GetPrimAtPath('/pSphere1')
        self.assertTrue(spherePrim)

        transformUtils.assertPrimXforms(self, spherePrim, [
            ('xformOp:translate', (0., 0., 3.))])

    def testExportUpAxisDifferentY(self):
        """Test exporting and forcing a up-axis Y different from Maya prefs."""
        cmds.polySphere()
        cmds.move(0, 0, 3, relative=True)

        usdFile = os.path.abspath('UsdExportUpAxis_DifferentY.usda')
        cmds.mayaUSDExport(file=usdFile,
                           shadingMode='none',
                           upAxis='y')
        
        stage = Usd.Stage.Open(usdFile)
        self.assertTrue(stage.HasAuthoredMetadata('upAxis'))
        expectedAxis = 'Y'
        actualAxis = UsdGeom.GetStageUpAxis(stage)
        self.assertEqual(actualAxis, expectedAxis)

        spherePrim = stage.GetPrimAtPath('/pSphere1')
        self.assertTrue(spherePrim)

        transformUtils.assertPrimXforms(self, spherePrim, [
            ('xformOp:translate', (0., 0., 3.)),
            ('xformOp:translate:pivot', (0., 3., -3.)),
            ('!invert!xformOp:translate:pivot', None),
        ])


if __name__ == '__main__':
    unittest.main(verbosity=2)
