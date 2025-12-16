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

from pxr import Gf, Usd, UsdGeom, UsdSkel, Vt

import fixturesUtils

class testUsdExportUnitsSkeleton(unittest.TestCase):
    """Test for modifying the units when exporting skeletons."""

    @classmethod
    def setUpClass(cls):
        cls._path = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        """Clear the scene"""
        mayaFile = os.path.join(self._path, "UsdExportSkeletonTest",
            "UsdExportSkeletonWithIdentityBindPose.ma")
        cmds.file(mayaFile, force=True, open=True)
        cmds.currentUnit(linear='cm')

    def _exportScene(self, unitName):
        usdFile = os.path.abspath(self._testMethodName + '.usda')
        cmds.mayaUSDExport(
            file=usdFile,
            mergeTransformAndShape=True,
            frameRange=[1, 5],
            exportSkels='auto',
            shadingMode='none',
            unit=unitName)
        return Usd.Stage.Open(usdFile)

    def _runTestExportUnits(self, unitName, expectedMetersPerUnit):
        """Test exporting a skeleton with some units."""
        stage = self._exportScene(unitName)

        if unitName == 'none':
            self.assertFalse(stage.HasAuthoredMetadata('metersPerUnit'))
            expectedMetersPerUnit = 0.01
        else:
            self.assertTrue(stage.HasAuthoredMetadata('metersPerUnit'))
            actualMetersPerUnit = UsdGeom.GetStageMetersPerUnit(stage)
            self.assertAlmostEqual(actualMetersPerUnit, expectedMetersPerUnit)

        expectedScale = 0.01 / expectedMetersPerUnit
        EPSILON = expectedScale * 1e-5

        skeleton = UsdSkel.Skeleton.Get(stage, '/cubeRig/skel/joint1')
        bindTrfs = skeleton.GetBindTransformsAttr().Get()
        self.assertEqual(len(bindTrfs), 1)
        self.assertTrue(
            Gf.IsClose(
                bindTrfs[0],
                Gf.Matrix4d(1.0 * expectedScale),
                EPSILON
            )
        )
        self.assertEqual(skeleton.GetJointsAttr().Get(),
            Vt.TokenArray(['joint1']))
        restTrfs = skeleton.GetRestTransformsAttr().Get()
        self.assertEqual(len(restTrfs), 1)
        self.assertTrue(
            Gf.IsClose(
                restTrfs[0],
                Gf.Matrix4d(
                    (1 * expectedScale, 0, 0, 0),
                    (0, 1 * expectedScale, 0, 0),
                    (0, 0, 1 * expectedScale, 0),
                    (5 * expectedScale, 5 * expectedScale, 0, 1 * expectedScale)
                ),
                EPSILON
            )
        )

        self.assertTrue(skeleton.GetPrim().HasAPI(UsdSkel.BindingAPI))
        skelBindingAPI = UsdSkel.BindingAPI(skeleton)
        self.assertTrue(skelBindingAPI)

        animSourcePrim = skelBindingAPI.GetAnimationSource()
        self.assertEqual(animSourcePrim.GetPath(),
            '/cubeRig/skel/joint1/Animation')
        animSource = UsdSkel.Animation(animSourcePrim)
        self.assertTrue(animSource)

        self.assertEqual(skeleton.GetJointsAttr().Get(),
            Vt.TokenArray(['joint1']))
        self.assertEqual(animSource.GetRotationsAttr().Get(),
            Vt.QuatfArray([Gf.Quatf(1.0, Gf.Vec3f(0.0))]))
        self.assertEqual(animSource.GetScalesAttr().Get(),
            Vt.Vec3hArray([Gf.Vec3h(1.0)]))
        self.assertEqual(
            animSource.GetTranslationsAttr().Get(Usd.TimeCode.Default()),
            Vt.Vec3fArray([Gf.Vec3f(5.0 * expectedScale, 5.0 * expectedScale, 0.0 * expectedScale)]))

        self.assertEqual(
            animSource.GetTranslationsAttr().Get(1.0),
            Vt.Vec3fArray([Gf.Vec3f(0.0 * expectedScale, 0.0 * expectedScale, 0.0 * expectedScale)]))
        self.assertEqual(
            animSource.GetTranslationsAttr().Get(2.0),
            Vt.Vec3fArray([Gf.Vec3f(1.25 * expectedScale, 1.25 * expectedScale, 0.0 * expectedScale)]))
        self.assertEqual(
            animSource.GetTranslationsAttr().Get(3.0),
            Vt.Vec3fArray([Gf.Vec3f(2.5 * expectedScale, 2.5 * expectedScale, 0.0 * expectedScale)]))
        self.assertEqual(
            animSource.GetTranslationsAttr().Get(4.0),
            Vt.Vec3fArray([Gf.Vec3f(3.75 * expectedScale, 3.75 * expectedScale, 0.0 * expectedScale)]))
        self.assertEqual(
            animSource.GetTranslationsAttr().Get(5.0),
            Vt.Vec3fArray([Gf.Vec3f(5.0 * expectedScale, 5.0 * expectedScale, 0.0 * expectedScale)]))

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
