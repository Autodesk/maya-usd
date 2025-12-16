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
import transformUtils

class testUsdExportUnitsInstances(unittest.TestCase):
    """Test for modifying the units when exporting instancess."""

    @classmethod
    def setUpClass(cls):
        cls._testpath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        """Clear the scene"""
        filePath = os.path.join(self._testpath, "UsdExportInstancesTest", "UsdExportInstancesTest.ma")
        cmds.file(filePath, force=True, open=True)
        cmds.currentUnit(linear='cm')

    def _exportScene(self, unitName):
        usdFile = os.path.abspath(self._testMethodName + '.usda')
        cmds.mayaUSDExport(
            file=usdFile,
            mergeTransformAndShape=True,
            exportInstances=True,
            stripNamespaces=True,
            shadingMode='none',
            unit=unitName)

        return Usd.Stage.Open(usdFile)

    def _runTestExportUnits(self, unitName, expectedMetersPerUnit):
        """Test exporting a instances with some units."""
        stage = self._exportScene(unitName)

        if unitName == 'none':
            self.assertFalse(stage.HasAuthoredMetadata('metersPerUnit'))
            expectedMetersPerUnit = 0.01
        else:
            self.assertTrue(stage.HasAuthoredMetadata('metersPerUnit'))
            actualMetersPerUnit = UsdGeom.GetStageMetersPerUnit(stage)
            self.assertAlmostEqual(actualMetersPerUnit, expectedMetersPerUnit)

        expectedScale = 0.01 / expectedMetersPerUnit

        scope = UsdGeom.Scope.Get(stage, '/MayaExportedInstanceSources')
        self.assertTrue(scope.GetPrim().IsValid())

        expectedMeshPath = [
            ('/MayaExportedInstanceSources/pCube1_pCubeShape1/pCubeShape1', -0.5),
            ('/MayaExportedInstanceSources/pCube1_pCube2_pCubeShape2/pCubeShape2', -0.5),
        ]

        for prototype_path, first_point_x in expectedMeshPath:
            mesh = UsdGeom.Mesh.Get(stage, prototype_path)
            prim = mesh.GetPrim()
            self.assertTrue(prim.IsValid())

            points = mesh.GetPointsAttr().Get()
            self.assertGreater(len(points), 0)
            self.assertAlmostEqual(points[0][0], first_point_x * expectedScale)

        expectedXForm = [
            ('/pCube1', None, (0., 2., -4.)),
            ('/pCube1/pCubeShape1', '/MayaExportedInstanceSources/pCube1_pCubeShape1', None),
            ('/pCube1/pCube2', '/MayaExportedInstanceSources/pCube1_pCube2', (0., -2, 0.)),
            ('/pCube1/pCube3', '/MayaExportedInstanceSources/pCube1_pCube3', (0., -4, 0.)),
            ('/pCube4', None, (0., 2., 4.)),
            ('/pCube4/pCubeShape1', '/MayaExportedInstanceSources/pCube1_pCubeShape1', None),
            ('/pCube4/pCube2', '/MayaExportedInstanceSources/pCube1_pCube2', None),
            ('/pCube4/pCube3', '/MayaExportedInstanceSources/pCube1_pCube3', None),
            ('/MayaExportedInstanceSources/pCube1_pCube2/pCubeShape2',
                    '/MayaExportedInstanceSources/pCube1_pCube2_pCubeShape2', None),
            ('/MayaExportedInstanceSources/pCube1_pCube3/pCubeShape2',
                    '/MayaExportedInstanceSources/pCube1_pCube2_pCubeShape2', None),
            # "ns:" should be stripped in usd result:
            ('/group1', None, None),
            ('/group2', None, None),
            ('/group1/pCube5', '/MayaExportedInstanceSources/group1_pCube5_pCubeShape5', (0., 0., 6.)),
            ('/group2/pCube6', '/MayaExportedInstanceSources/group1_pCube5_pCubeShape5', (0., 0., 8.)),
        ]

        layer = stage.GetLayerStack()[1]

        for instance_path, prototype_path, translation in expectedXForm:
            x = UsdGeom.Xform.Get(stage, instance_path)
            prim = x.GetPrim()
            self.assertTrue(prim.IsValid())
            if prototype_path is not None:
                self.assertTrue(prim.IsInstanceable())
                self.assertTrue(prim.IsInstance())
                instance_prim_spec = layer.GetPrimAtPath(instance_path)

                ref = instance_prim_spec.referenceList.GetAddedOrExplicitItems()[0]
                self.assertEqual(ref.assetPath, "")
                self.assertEqual(ref.primPath,  prototype_path)
            
            if translation is not None:
                transformUtils.assertPrimXforms(self, prim, [
                    (
                        'xformOp:translate',
                        (
                            translation[0] * expectedScale,
                            translation[1] * expectedScale,
                            translation[2] * expectedScale
                        )
                    )
                ])


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
