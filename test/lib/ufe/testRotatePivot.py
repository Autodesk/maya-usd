#!/usr/bin/env python

#
# Copyright 2019 Autodesk
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
import usdUtils

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as om

import ufe

from math import degrees
from math import radians
from math import sin
import os
import unittest


def v3dToMPoint(v):
    return om.MPoint(v.x(), v.y(), v.z())

def assertMPointAlmostEqual(testCase, a, b, places=7):
    testCase.assertAlmostEqual(a.x, b.x)
    testCase.assertAlmostEqual(a.y, b.y)
    testCase.assertAlmostEqual(a.z, b.z)
    testCase.assertAlmostEqual(a.w, b.w)

# Index into MMatrix linearly as a 16-element vector, starting at row 0.
def ndx(i, j):
    return i*4+j

class RotatePivotTestCase(unittest.TestCase):
    '''Verify the Transform3d UFE rotate pivot interface.

    UFE Feature : Transform3d
    Maya Feature : rotate pivot
    Action : Set the rotate pivot.
    Applied On Selection : 
        - No selection - Given node as param
        - Single Selection : Not tested.
        - Multiple Selection [Mixed, Non-Maya] : Not tested.

    Undo/Redo Test : UFE undoable command only.
    Expect Results To Test :
        - Maya Dag object world space position.
        - USD object world space position.
    Edge Cases :
        - None.
    '''

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

        # Open twoSpheres.ma scene in testSamples
        mayaUtils.openTwoSpheresScene()

    def checkPos(self, m, p):
        self.assertAlmostEqual(m[ndx(3,0)], p[0])
        self.assertAlmostEqual(m[ndx(3,1)], p[1])
        self.assertAlmostEqual(m[ndx(3,2)], p[2])

    def testRotatePivot(self):
        # mayaSphere is at (10, 0, 10) in local space, and since it has no
        # parent, in world space as well.
        spherePath = om.MSelectionList().add('mayaSphere').getDagPath(0)
        sphereFn = om.MFnTransform(spherePath)
        rotZ = radians(45)
        rot = om.MEulerRotation(0, 0, rotZ)
        # Pivot around x=0.
        pivot = om.MPoint(-10, 0, 0)
        sphereFn.setRotatePivot(pivot, om.MSpace.kTransform, False)
        sphereFn.setRotation(rot, om.MSpace.kTransform)
        # MFnTransform and MTransformationMatrix always return the individual
        # components of the matrix, including translation, which is unaffected
        # by pivot changes, as expected.  The fully-multiplied result is in the
        # computed MMatrix.
        xyWorldValue = sin(rotZ) * 10
        sphereMatrix = sphereFn.transformation().asMatrix()

        self.checkPos(sphereMatrix, [xyWorldValue, xyWorldValue, 10])

        # Do the same with the USD object, through UFE.
        # USD sphere is at (10, 0, 0) in local space, and since its parents
        # have an identity transform, in world space as well.
        usdSpherePath = ufe.PathString.path(
                '|usdSphereParent|usdSphereParentShape,/sphereXform/sphere')
        usdSphereItem = ufe.Hierarchy.createItem(usdSpherePath)
        t3d = ufe.Transform3d.transform3d(usdSphereItem)

        t3d.rotatePivot(pivot[0], pivot[1], pivot[2])
        usdPivot = t3d.rotatePivot()
        assertMPointAlmostEqual(self, v3dToMPoint(usdPivot), pivot)

        t3d.rotate(degrees(rot[0]), degrees(rot[1]), degrees(rot[2]))
        sphereMatrix = om.MMatrix(t3d.inclusiveMatrix().matrix)
        self.checkPos(sphereMatrix, [xyWorldValue, xyWorldValue, 0])

        t3d.rotatePivot(0, 0, 0)
        usdPivot = t3d.rotatePivot()
        assertMPointAlmostEqual(self, v3dToMPoint(usdPivot), om.MPoint(0, 0, 0))

        sphereMatrix = om.MMatrix(t3d.inclusiveMatrix().matrix)
        self.checkPos(sphereMatrix, [10, 0, 0])

        # Use a UFE undoable command to set the pivot.
        rotatePivotCmd = t3d.rotatePivotCmd()
        rotatePivotCmd.set(pivot[0], pivot[1], pivot[2])

        usdPivot = t3d.rotatePivot()
        assertMPointAlmostEqual(self, v3dToMPoint(usdPivot), pivot)

        sphereMatrix = om.MMatrix(t3d.inclusiveMatrix().matrix)

        self.checkPos(sphereMatrix, [xyWorldValue, xyWorldValue, 0])

        rotatePivotCmd.undo()

        usdPivot = t3d.rotatePivot()
        assertMPointAlmostEqual(self, v3dToMPoint(usdPivot), om.MPoint(0, 0, 0))

        sphereMatrix = om.MMatrix(t3d.inclusiveMatrix().matrix)
        self.checkPos(sphereMatrix, [10, 0, 0])

        # redo() cannot be tested, as it currently is intentionally not
        # implemented, because the Maya move command handles undo by directly
        # calling the translate() method.  This is fragile,
        # implementation-specific, and should be changed.  PPT, 3-Sep-2020.

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2022, 'Requires Maya fixes only available in Maya 2022 or greater.')
    def testRotatePivotCmd(self):
        rotZ = radians(45)
        rot = om.MEulerRotation(0, 0, rotZ)
        # Pivot around x=0.
        pivot = om.MPoint(-10, 0, 0)
        pivotTranslate = om.MPoint(0, 0, 0)
        xyWorldValue = sin(rotZ) * 10
        rotatedPosition = [xyWorldValue, xyWorldValue, 0]

        # USD sphere is at (10, 0, 0) in local space, and since its parents
        # have an identity transform, in world space as well.
        spherePath = ufe.PathString.path(
                '|usdSphereParent|usdSphereParentShape,/sphereXform')
        sphereItem = ufe.Hierarchy.createItem(spherePath)
        ufe.GlobalSelection.get().append(sphereItem)

        # Create a Transform3d interface to read from USD.
        t3d = ufe.Transform3d.transform3d(sphereItem)

        # Start with a non-zero initial rotate pivot.  This is required to test
        # MAYA-105345, otherwise a zero initial rotate pivot produces the
        # correct result through an unintended code path.
        t3d.rotatePivot(2, 0, 0)
        usdPivot = t3d.rotatePivot()
        assertMPointAlmostEqual(self, v3dToMPoint(usdPivot), om.MPoint(2, 0, 0))
        t3d.rotatePivot(0, 0, 0)

        # Start with a non-zero initial rotation. This is required to test
        # MAYA-112175, otherwise a zero initial rotation means rotate pivot
        # translation will be empty and we get the correct result by accident.
        if (mayaUtils.previewReleaseVersion() >= 127):
            cmds.rotate(0, 0, 90)
            print(type(pivot))
            pivot = om.MPoint(0, 10, 0)
            print(type(pivot))
            pivotTranslate = om.MPoint(-10, -10, 0)
            rotatedPosition = [xyWorldValue, -xyWorldValue, 0]

        cmds.move(-10, 0, 0, relative=True, ufeRotatePivot=True)
        usdPivot = t3d.rotatePivot()
        assertMPointAlmostEqual(self, v3dToMPoint(usdPivot), pivot)
        usdRotatePivotTranslation = t3d.rotatePivotTranslation()
        assertMPointAlmostEqual(self, v3dToMPoint(usdRotatePivotTranslation), pivotTranslate)

        cmds.undo()

        usdPivot = t3d.rotatePivot()
        assertMPointAlmostEqual(self, v3dToMPoint(usdPivot), om.MPoint(0, 0, 0))
        usdRotatePivotTranslation = t3d.rotatePivotTranslation()
        assertMPointAlmostEqual(self, v3dToMPoint(usdRotatePivotTranslation), om.MPoint(0, 0, 0))

        cmds.redo()

        usdPivot = t3d.rotatePivot()
        assertMPointAlmostEqual(self, v3dToMPoint(usdPivot), pivot)

        cmds.rotate(degrees(rot[0]), degrees(rot[1]), degrees(rot[2]))
        sphereMatrix = om.MMatrix(t3d.inclusiveMatrix().matrix)
        self.checkPos(sphereMatrix, rotatedPosition)

        if (mayaUtils.previewReleaseVersion() >= 127):
            cmds.undo()
            cmds.move(10, 10, 0, absolute=True)
            sphereMatrix = om.MMatrix(t3d.inclusiveMatrix().matrix)
            self.checkPos(sphereMatrix, [10, 10, 0])
            cmds.move(10, 10, 0, absolute=True, rotatePivotRelative=True)
            sphereMatrix = om.MMatrix(t3d.inclusiveMatrix().matrix)
            self.checkPos(sphereMatrix, [20, 10, 0])


if __name__ == '__main__':
    unittest.main(verbosity=2)
