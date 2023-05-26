#!/usr/bin/env python

#
# Copyright 2023 Autodesk
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

class CenterPivotTestCase(unittest.TestCase):
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


    def checkPos(self, m, p):
        self.assertAlmostEqual(m[ndx(3,0)], p[0])
        self.assertAlmostEqual(m[ndx(3,1)], p[1])
        self.assertAlmostEqual(m[ndx(3,2)], p[2])

    def testCenterPivot(self):
        '''Verify the behavior Transform3d UFE pivot interface.

        UFE Feature : Transform3d
        Maya Feature : center pivot
        Action : Center the pivot.
        Applied On Selection : the Maya command always uses the slection.

        Undo/Redo Test : Maya undoable command only.
        '''
        # Open the USD file containing the prim we want to affect.
        testFile = getTestScene("instancer_pivot", "pivot.usda")
        testDagPath, stage = mayaUtils.createProxyFromFile(testFile)
        self.assertIsNotNone(stage)

        # Retrieve the UFE item to the prim and the prim.
        instancerUfePathString = testDagPath + ",/PointInstancer"
        instancerUfePath = ufe.PathString.path(instancerUfePathString)
        instancerUfeItem = ufe.Hierarchy.createItem(instancerUfePath)
        self.assertIsNotNone(instancerUfeItem)
        instancerUsdPrim = stage.GetPrimAtPath("/PointInstancer")
        self.assertIsNotNone(instancerUsdPrim)

        # Verify the point instancer overall position.
        #
        # Note: it has a 90 degree rotation, so it is not the identity matrix.
        def verifyPointInstancerPosition():
            instancerUfeT3d = ufe.Transform3d.transform3d(instancerUfeItem)
            instancerUfeMtx = instancerUfeT3d.matrix()
            flatMtx = []
            for row in instancerUfeMtx.matrix:
                flatMtx.extend(row)
            assertVectorAlmostEqual(self, flatMtx, [ 0., 1., 0., 0.,
                                                    -1., 0., 0., 0.,
                                                    0., 0., 1., 0.,
                                                    0., 0., 0., 1.])
            
        verifyPointInstancerPosition()

        # Select the prim.
        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(instancerUfeItem)

        cmds.CenterPivot()

        # Verify the USD prim xform
        rptUsdAttr = instancerUsdPrim.GetAttribute('xformOp:translate:rotatePivotTranslate')
        self.assertIsNotNone(rptUsdAttr)
        rpt = rptUsdAttr.Get()
        assertVectorAlmostEqual(self, rpt, [-5., -5., 0.])

        rpAttr = instancerUsdPrim.GetAttribute('xformOp:translate:rotatePivot')
        self.assertIsNotNone(rpAttr)
        rp = rpAttr.Get()
        assertVectorAlmostEqual(self, rp, [0., 5., 0.])

        # Verify the UFE item pivots
        instancerUfeT3d = ufe.Transform3d.transform3d(instancerUfeItem)

        instancerUfeRotPivotPos = instancerUfeT3d.rotatePivot()
        assertVectorAlmostEqual(self, instancerUfeRotPivotPos.vector, [0., 5., 0.])

        instancerUfeRotPivotTranslatePos = instancerUfeT3d.rotatePivotTranslation()
        assertVectorAlmostEqual(self, instancerUfeRotPivotTranslatePos.vector, [-5., -5., 0.])

        # Verify the object did not move.
        verifyPointInstancerPosition()


if __name__ == '__main__':
    unittest.main(verbosity=2)
