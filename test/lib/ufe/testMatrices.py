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

from math import cos
from math import radians
from math import sin
import os
import unittest


# AX = |  1    0    0    0 |
#      |  0    cx   sx   0 |
#      |  0   -sx   cx   0 |
#      |  0    0    0    1 |
# sx = sin(rax), cx = cos(rax)
def rotXMatrix(rax):
    sx = sin(rax)
    cx = cos(rax)
    return [[1, 0, 0, 0], [0, cx, sx, 0], [0, -sx, cx, 0], [0, 0, 0, 1]]

# T  = |  1    0    0    0 |
#      |  0    1    0    0 |
#      |  0    0    1    0 |
#      |  tx   ty   tz   1 |
def xlateMatrix(t):
    return [[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [t[0], t[1], t[2], 1]]

identityMatrix = xlateMatrix([0, 0, 0])

class Transform3dMatricesTestCase(unittest.TestCase):
    '''Verify the Transform3d matrix interfaces.'''

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

        # Open usdCylinder.ma scene in testSamples
        mayaUtils.openCylinderScene()

    def assertMatrixAlmostEqual(self, ma, mb):
        for ra, rb in zip(ma, mb):
            for a, b in zip(ra, rb):
                self.assertAlmostEqual(a, b)

    def testMatrices(self):
        '''Test inclusive and exclusive matrices.'''

        # Scene hierarchy:
        #
        #   mayaUsdTransform 
        #   |_ shape
        #      |_ pCylinder1 (USD mesh)
        #
        # transform has a translation of 10 units along the Y axis,
        # and a rotation of 30 degrees around X.  pCylinder1 has a further
        # 30 degree rotation around X (60 degrees total).

        # Create a non-identity transform for the Maya segment of the UFE path:
        # translate the transform of the proxy shape up (Y axis) by 10
        # units, rotate around X 30 degrees.
        cmds.setAttr('mayaUsdTransform.translateY', 10)
        cmds.setAttr('mayaUsdTransform.rotateX', 30)

        # Next, rotate the USD object by another 30 degrees around X.  To
        # do so, select it first.
        mayaPathSegment = mayaUtils.createUfePathSegment('|mayaUsdTransform|shape')
        usdPathSegment = usdUtils.createUfePathSegment('/pCylinder1')
        cylinderPath = ufe.Path([mayaPathSegment, usdPathSegment])
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)

        ufe.GlobalSelection.get().append(cylinderItem)

        cmds.rotate(30, r=True, os=True, rotateX=True)

        # Get a Transform3d interface to the cylinder.
        t3d = ufe.Transform3d.transform3d(cylinderItem)

        # Get the inclusive and exclusive matrices for the cylinder, and
        # for its (last) segment.
        cylSegExclMat = t3d.segmentExclusiveMatrix()
        cylSegInclMat = t3d.segmentInclusiveMatrix()
        cylExclMat = t3d.exclusiveMatrix()
        cylInclMat = t3d.inclusiveMatrix()

        # Since the cylinder has no USD parent, its segment exclusive
        # matrix is the identity, and its segment inclusive matrix is just
        # its 30 degree rotation around X.
        self.assertMatrixAlmostEqual(cylSegExclMat.matrix, identityMatrix)
        self.assertMatrixAlmostEqual(cylSegInclMat.matrix, rotXMatrix(radians(30)))

        # Get a Transform3d interface for the proxy shape's transform, and
        # get its transforms.
        xformPath = ufe.Path(mayaUtils.createUfePathSegment('|mayaUsdTransform'))
        xformItem = ufe.Hierarchy.createItem(xformPath)
        t3d = ufe.Transform3d.transform3d(xformItem)

        xformSegExclMat = t3d.segmentExclusiveMatrix()
        xformSegInclMat = t3d.segmentInclusiveMatrix()
        xformExclMat = t3d.exclusiveMatrix()
        xformInclMat = t3d.inclusiveMatrix()

        # Since the transform has no Maya parent, its segment exclusive
        # matrix is the identity, and its segment inclusive matrix is 
        # its 30 degree rotation around X, and its 10 unit Y translation.
        sx30 = sin(radians(30))
        cx30 = cos(radians(30))
        self.assertMatrixAlmostEqual(xformSegExclMat.matrix, identityMatrix)
        self.assertMatrixAlmostEqual(
            xformSegInclMat.matrix, [[1, 0, 0, 0], [0, cx30, sx30, 0],
                                     [0, -sx30, cx30, 0], [0, 10, 0, 1]])

        # Since the transform is the cylinder's parent, the cylinder's
        # exclusive matrix must be (by definition) the transform's
        # inclusive matrix.
        self.assertMatrixAlmostEqual(xformInclMat.matrix, cylExclMat.matrix)

        # The cylinder's inclusive matrix is the full 60 degree rotation
        # around X, and 10 unit Y translation.
        sx60 = sin(radians(60))
        cx60 = cos(radians(60))
        self.assertMatrixAlmostEqual(
            cylInclMat.matrix, [[1, 0, 0, 0], [0, cx60, sx60, 0],
                                [0, -sx60, cx60, 0], [0, 10, 0, 1]])


if __name__ == '__main__':
    unittest.main(verbosity=2)
