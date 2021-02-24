#!/usr/bin/env python

#
# Copyright 2020 Autodesk
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

import mayaUsd.ufe

from pxr import UsdGeom
from pxr import Vt

from maya import cmds
from maya import standalone

import ufe

import unittest


class Transform3dChainOfResponsibilityTestCase(unittest.TestCase):
    '''Verify the Transform3d chain of responsibility for USD.'''

    pluginsLoaded = False
    
    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testXformCommonAPI(self):
        '''The Maya transform API is preferred to the USD common API.'''

        cmds.file(new=True, force=True)

        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        proxyShapePath = ufe.PathString.path(proxyShape)
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'Sphere'])

        spherePath = ufe.PathString.path('%s,/Sphere1' % proxyShape)
        sphereItem = ufe.Hierarchy.createItem(spherePath)
        sphereT3d = ufe.Transform3d.transform3d(sphereItem)

        spherePrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(spherePath))
        sphereXformable = UsdGeom.Xformable(spherePrim)

        # Add a RotXYZ transform op to the new prim.  This is
        # compatible with the common API (and with the Maya stack as well).
        sphereT3d.rotate(30, 0, 0)
        self.assertEqual(sphereT3d.rotation(), ufe.Vector3d(30, 0, 0))

        self.assertEqual(sphereXformable.GetXformOpOrderAttr().Get(), 
            Vt.TokenArray(1, ('xformOp:rotateXYZ')))
        self.assertTrue(UsdGeom.XformCommonAPI(sphereXformable))

        # Add a scale transform op to prim.  This is compatible with
        # both the common API and the Maya transform stack.
        sphereT3d.scale(1, 1, 2)
        self.assertEqual(sphereT3d.scale(), ufe.Vector3d(1, 1, 2))

        self.assertEqual(sphereXformable.GetXformOpOrderAttr().Get(), 
            Vt.TokenArray(('xformOp:rotateXYZ', 'xformOp:scale')))
        self.assertTrue(UsdGeom.XformCommonAPI(sphereXformable))

        # Move pivots in world space, using the selection.  Because the Maya
        # transform stack handler precedes the common API handler in the
        # Transform3dHandler chain of responsibility, it will handle the pivot
        # move, and will add pivot compensations, which is no longer compatible
        # with the common API"
        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(sphereItem)
        cmds.move(0, -2.104143, 3.139701, r=True, urp=True, usp=True)

        self.assertEqual(
            sphereXformable.GetXformOpOrderAttr().Get(), 
            Vt.TokenArray(("xformOp:translate:rotatePivotTranslate",
                           "xformOp:translate:rotatePivot", "xformOp:rotateXYZ",
                           "!invert!xformOp:translate:rotatePivot",
                           "xformOp:translate:scalePivotTranslate",
                           "xformOp:translate:scalePivot",
                           "xformOp:scale",
                           "!invert!xformOp:translate:scalePivot")))
        self.assertFalse(UsdGeom.XformCommonAPI(sphereXformable))

        # Now create a prim, and give it a common API single pivot.  This will
        # match the common API, but not the Maya API.  Setting a rotation, then
        # trying to move the pivot, will NOT create rotate pivot translation
        # compensation, because that is unsupported by the common API.
        proxyShapeContextOps.doOp(['Add New Prim', 'Cube'])
        cubePath = ufe.PathString.path('%s,/Cube1' % proxyShape)
        cubeItem = ufe.Hierarchy.createItem(cubePath)
        cubePrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(cubePath))
        cubeXformable = UsdGeom.Xformable(cubePrim)

        cubeXformable.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "pivot")
        cubeXformable.AddTranslateOp(
            UsdGeom.XformOp.PrecisionFloat, "pivot", True)

        self.assertEqual(
            cubeXformable.GetXformOpOrderAttr().Get(), 
            Vt.TokenArray(("xformOp:translate:pivot",
                           "!invert!xformOp:translate:pivot")))
        self.assertTrue(UsdGeom.XformCommonAPI(cubeXformable))

        # Once we've set up the transform stack to be common API-compatible,
        # create the Transform3d interface.
        cubeT3d = ufe.Transform3d.transform3d(cubeItem)

        # Rotate the cube.
        cubeT3d.rotate(30, 0, 0)
        self.assertEqual(cubeT3d.rotation(), ufe.Vector3d(30, 0, 0))

        self.assertEqual(
            cubeXformable.GetXformOpOrderAttr().Get(), 
            Vt.TokenArray(("xformOp:translate:pivot", "xformOp:rotateXYZ",
                           "!invert!xformOp:translate:pivot")))
        self.assertTrue(UsdGeom.XformCommonAPI(cubeXformable))

        # Move the pivot.  Because the common API is handling the request, no
        # pivot compensation transform ops will be created, and we remain
        # common API compatible.
        sn.clear()
        sn.append(cubeItem)
        self.assertEqual(cubeT3d.rotatePivot(), ufe.Vector3d(0, 0, 0))
        cmds.move(0, -2.104143, 3.139701, r=True, urp=True, usp=True)
        self.assertNotEqual(cubeT3d.rotatePivot(), ufe.Vector3d(0, 0, 0))

        self.assertEqual(
            cubeXformable.GetXformOpOrderAttr().Get(), 
            Vt.TokenArray(("xformOp:translate:pivot", "xformOp:rotateXYZ",
                           "!invert!xformOp:translate:pivot")))
        self.assertTrue(UsdGeom.XformCommonAPI(cubeXformable))


if __name__ == '__main__':
    unittest.main(verbosity=2)
