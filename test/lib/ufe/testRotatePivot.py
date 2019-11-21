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

from ufeTestUtils import usdUtils, mayaUtils

import ufe

import maya.api.OpenMaya as om
import maya.cmds as cmds

from math import radians, degrees, sin
import os

import unittest

def v3dToMPoint(v):
    return om.MPoint(v.x(), v.y(), v.z())

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
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    def setUp(self):
        ''' Called initially to set up the maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

        # Create a simple USD scene.  Ideally this would be done
        # programmatically, but as of 28-Jun-2018 attempts to do so have
        # resulted in a file that crashes Maya on pickWalk of the scene.
        # Use a saved scene read from file instead.
        filePath = os.path.join(os.path.dirname(os.path.realpath(__file__)), "test-samples", "twoSpheres", "twoSpheres.ma" )
        cmds.file(filePath, force=True, open=True)

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
        # MMatrix is indexed linearly as a 16-element vector, starting at row 0.
        def ndx(i, j):
            return i*4+j
        def checkPos(m, p):
            self.assertAlmostEqual(m[ndx(3,0)], p[0])
            self.assertAlmostEqual(m[ndx(3,1)], p[1])
            self.assertAlmostEqual(m[ndx(3,2)], p[2])

        checkPos(sphereMatrix, [xyWorldValue, xyWorldValue, 10])

        # Do the same with the USD object, through UFE.
        # USD sphere is at (10, 0, 0) in local space, and since its parents
        # have an identity transform, in world space as well.
        usdSpherePath = ufe.Path([
            mayaUtils.createUfePathSegment(
                '|world|usdSphereParent|usdSphereParentShape'),
            usdUtils.createUfePathSegment('/sphereXform/sphere')])
        usdSphereItem = ufe.Hierarchy.createItem(usdSpherePath)
        t3d = ufe.Transform3d.transform3d(usdSphereItem)

        t3d.rotatePivotTranslate(pivot[0], pivot[1], pivot[2])
        usdPivot = t3d.rotatePivot()
        self.assertEqual(v3dToMPoint(usdPivot), pivot)

        t3d.rotate(degrees(rot[0]), degrees(rot[1]), degrees(rot[2]))
        sphereMatrix = om.MMatrix(t3d.inclusiveMatrix().matrix)
        checkPos(sphereMatrix, [xyWorldValue, xyWorldValue, 0])

        # Use a UFE undoable command to set the pivot.
        t3d.rotatePivotTranslate(0, 0, 0)
        usdPivot = t3d.rotatePivot()
        self.assertEqual(v3dToMPoint(usdPivot), om.MPoint(0, 0, 0))

        sphereMatrix = om.MMatrix(t3d.inclusiveMatrix().matrix)
        checkPos(sphereMatrix, [10, 0, 0])

        rotatePivotCmd = t3d.rotatePivotTranslateCmd()
        rotatePivotCmd.translate(pivot[0], pivot[1], pivot[2])

        usdPivot = t3d.rotatePivot()
        self.assertEqual(v3dToMPoint(usdPivot), pivot)

        sphereMatrix = om.MMatrix(t3d.inclusiveMatrix().matrix)

        checkPos(sphereMatrix, [xyWorldValue, xyWorldValue, 0])

        rotatePivotCmd.undo()

        usdPivot = t3d.rotatePivot()
        self.assertEqual(v3dToMPoint(usdPivot), om.MPoint(0, 0, 0))

        sphereMatrix = om.MMatrix(t3d.inclusiveMatrix().matrix)
        checkPos(sphereMatrix, [10, 0, 0])

        # redo() cannot be tested, as it is not implemented.
