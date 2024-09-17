#!/usr/bin/env python

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

import fixturesUtils
import mayaUtils
from testUtils import assertVectorAlmostEqual, getTestScene

from maya import cmds
from maya import standalone

import ufe

import unittest

class PivotConversionTestCase(unittest.TestCase):
    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        ''' Called initially to set up the maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

    def testUSDPivotConversion(self):
        '''
        Test that USD-style pivot gets converted to Maya-style pivots
        when the pivot is modified.
        '''
        # Open the USD file containing the prim we want to affect.
        testFile = getTestScene("pivot", "cube-usd-pivot.usda")
        testDagPath, stage = mayaUtils.createProxyFromFile(testFile)
        self.assertIsNotNone(stage)

        # Retrieve the UFE item to the prim and the prim.
        cubeUfePathString = testDagPath + ",/Cube1"
        cubeUfePath = ufe.PathString.path(cubeUfePathString)
        cubeUfeItem = ufe.Hierarchy.createItem(cubeUfePath)
        self.assertIsNotNone(cubeUfeItem)
        cubeUsdPrim = stage.GetPrimAtPath("/Cube1")
        self.assertIsNotNone(cubeUsdPrim)

        # Verify the cube overall position.
        def verifyCubePosition():
            cubeUfeT3d = ufe.Transform3d.transform3d(cubeUfeItem)
            cubeUfeMtx = cubeUfeT3d.matrix()
            flatMtx = []
            for row in cubeUfeMtx.matrix:
                flatMtx.extend(row)
            assertVectorAlmostEqual(self, flatMtx, [ 1., 0., 0., 0.,
                                                     0., 1., 0., 0.,
                                                     0., 0., 1., 0.,
                                                     0., 20., 0., 1.])
            
        def verifyCubeUSDPivots(usdPivot, mayaRotPivot, mayaScalePivot):
            namesAndValues = [
                ('xformOp:translate:pivot',                 usdPivot),
                ('xformOp:translate:rotatePivot',           mayaRotPivot),
                ('xformOp:translate:scalePivot',            mayaScalePivot),
            ]
            for name, value in namesAndValues:
                if value is None:
                    continue
                usdAttr = cubeUsdPrim.GetAttribute(name)
                self.assertIsNotNone(usdAttr)
                assertVectorAlmostEqual(self, usdAttr.Get(), value)

        def verifyCubeUFEPivots(mayaRotPivot, mayaScalePivot):
            cubeUfeT3d = ufe.Transform3d.transform3d(cubeUfeItem)
            assertVectorAlmostEqual(self, cubeUfeT3d.rotatePivot().vector,            mayaRotPivot)
            assertVectorAlmostEqual(self, cubeUfeT3d.scalePivot().vector,             mayaScalePivot)

        verifyCubePosition()

        verifyCubeUSDPivots(
            [7., 7., 8.],       # USD translate pivot
            None,               # Maya rotate pivot
            None                # Maya scale pivot
        )

        # Note: when going through UFE, the USD translate pivot is added to the Maya
        #       pivots (translate and scale). So the USD translate pivot that was verified
        #       above to be [7., 7., 8.] is instead added to the other pivot values.
        verifyCubeUFEPivots(
            [7., 7., 8.],     # Maya rotate pivot
            [7., 7., 8.]      # Maya scale pivot
        )

        def modifyPivot():
            cubeUfeT3d = ufe.Transform3d.transform3d(cubeUfeItem)
            cmd = cubeUfeT3d.rotatePivotCmd(2., 2., 2.)
            self.assertIsNotNone(cmd)
            cmd.execute()

        modifyPivot()

        verifyCubePosition()

        # Editing the Maya pivot converted the USD pivot to Maya pivots, so the USD
        # pivot is now zero and its previous value is added to the Maya pivots.
        # Since the command that we execute explivitly set the rotate pivot, the
        # converted value was overwritten with this new value after the conversion.
        verifyCubeUSDPivots(
            [0., 0., 0.],       # USD translate pivot
            [2., 2., 2.],       # Maya rotate pivot
            [7., 7., 8.]        # Maya scale pivot
        )

        verifyCubeUFEPivots(
            [2., 2., 2.],     # Maya rotate pivot
            [7., 7., 8.]      # Maya scale pivot
        )


if __name__ == '__main__':
    unittest.main(verbosity=2)
