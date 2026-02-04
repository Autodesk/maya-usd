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
import testUtils
from testUtils import assertVectorAlmostEqual, assertVectorNotAlmostEqual
import usdUtils
from usdUtils import filterUsdStr

import mayaUsd.ufe

from pxr import UsdGeom, Vt, Gf, Sdf, Usd

from maya import cmds
from maya import standalone

import ufe

import os
import unittest

def childrenNames(children):
    return [str(child.path().back()) for child in children]

def matrixToList(m):
    mList = []
    for i in m.matrix:
        mList = mList + i
    return mList


class OpenFileCtx(object):
    def __init__(self, fileName):
        self._fileName = fileName

    def __enter__(self):
        mayaUtils.openTestScene("parentCmd", self._fileName)

    def __exit__(self, type, value, traceback):
        # Close the file.
        cmds.file(force=True, new=True)


class ParentCmdTestCase(unittest.TestCase):
    '''Verify the Maya parent command on a USD scene.'''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        cmds.file(new=True, force=True)

        standalone.uninitialize()

    def setUp(self):
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

        # Load a file that has the same scene in both the Maya Dag
        # hierarchy and the USD hierarchy.
        mayaUtils.openTestScene("parentCmd", "simpleSceneMayaPlusUSD_TRS.ma")

        # Clear selection to start off
        cmds.select(clear=True)

        # Save current 'MAYA_USD_MATRIX_XFORM_OP_NAME' env var, if present.
        self.mayaUsdMatrixXformOpName = os.environ.get(
            'MAYA_USD_MATRIX_XFORM_OP_NAME')

    def tearDown(self):
        # If there was no 'MAYA_USD_MATRIX_XFORM_OP_NAME' environment variable,
        # make sure there is none at the end of the test.
        if self.mayaUsdMatrixXformOpName is None:
            if 'MAYA_USD_MATRIX_XFORM_OP_NAME' in os.environ:
                del os.environ['MAYA_USD_MATRIX_XFORM_OP_NAME']
        else:
            # Restore previous value.
            os.environ['MAYA_USD_MATRIX_XFORM_OP_NAME'] = \
                self.mayaUsdMatrixXformOpName

    def testParentRelative(self):
        # Create scene items for the cube and the cylinder.
        shapeSegment = mayaUtils.createUfePathSegment(
            "|mayaUsdProxy1|mayaUsdProxyShape1")
        cubePath = ufe.Path(
            [shapeSegment, usdUtils.createUfePathSegment("/cubeXform")])
        cubeItem = ufe.Hierarchy.createItem(cubePath)
        cylinderPath = ufe.Path(
            [shapeSegment, usdUtils.createUfePathSegment("/cylinderXform")])
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)

        # get the USD stage
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(shapeSegment)))

        # check GetLayerStack behavior
        self.assertEqual(stage.GetEditTarget().GetLayer(),
                         stage.GetRootLayer())

        # The cube is not a child of the cylinder.
        cylHier = ufe.Hierarchy.hierarchy(cylinderItem)
        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 1)

        # Get the components of the cube's local transform.
        cubeT3d = ufe.Transform3d.transform3d(cubeItem)
        cubeT = cubeT3d.translation()
        cubeR = cubeT3d.rotation()
        cubeS = cubeT3d.scale()

        # Parent cube to cylinder in relative mode, using UFE path strings.
        cmds.parent("|mayaUsdProxy1|mayaUsdProxyShape1,/cubeXform",
                    "|mayaUsdProxy1|mayaUsdProxyShape1,/cylinderXform",
                    relative=True)

        # Confirm that the cube is now a child of the cylinder.
        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 2)
        self.assertIn("cubeXform", childrenNames(cylChildren))

        # Undo: the cube is no longer a child of the cylinder.
        cmds.undo()

        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 1)

        # Redo: confirm that the cube is again a child of the cylinder.
        cmds.redo()

        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 2)
        self.assertIn("cubeXform", childrenNames(cylChildren))

        # Confirm that the cube's local transform has not changed.  Must
        # re-create the item, as its path has changed.
        cubeChildPath = ufe.Path(
            [shapeSegment, usdUtils.createUfePathSegment("/cylinderXform/cubeXform")])
        cubeChildItem = ufe.Hierarchy.createItem(cubeChildPath)
        cubeChildT3d = ufe.Transform3d.transform3d(cubeChildItem)

        self.assertEqual(cubeChildT3d.translation(), cubeT)
        self.assertEqual(cubeChildT3d.rotation(),    cubeR)
        self.assertEqual(cubeChildT3d.scale(),       cubeS)

        # Move the parent
        ufe.GlobalSelection.get().append(cylinderItem)

        cmds.move(0, 10, 0, relative=True)

        # Do the same on the equivalent Maya objects.
        cmds.parent("pCube1", "pCylinder1", relative=True)
        cmds.move(0, 10, 0, "pCylinder1", relative=True)

        # Get its world space transform.
        cubeWorld = cubeChildT3d.inclusiveMatrix()
        cubeWorldList = matrixToList(cubeWorld)

        # Do the same for the equivalent Maya object.
        mayaCubeWorld = cmds.xform(
            "|pCylinder1|pCube1", q=True, ws=True, m=True)

        # Equivalent Maya and USD objects must have the same world transform.
        assertVectorAlmostEqual(self, cubeWorldList, mayaCubeWorld)

        # Undo to bring scene to its original state.
        for i in range(4):
            cmds.undo()

        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 1)

    def testParentAbsolute(self):
        # Create scene items for the cube and the cylinder.
        shapeSegment = mayaUtils.createUfePathSegment(
            "|mayaUsdProxy1|mayaUsdProxyShape1")
        cubePath = ufe.Path(
            [shapeSegment, usdUtils.createUfePathSegment("/cubeXform")])
        cubeItem = ufe.Hierarchy.createItem(cubePath)
        cylinderPath = ufe.Path(
            [shapeSegment, usdUtils.createUfePathSegment("/cylinderXform")])
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)

        # get the USD stage
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(shapeSegment)))

        # check GetLayerStack behavior
        self.assertEqual(stage.GetEditTarget().GetLayer(),
                         stage.GetRootLayer())

        # The cube is not a child of the cylinder.
        cylHier = ufe.Hierarchy.hierarchy(cylinderItem)
        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 1)

        # Get its world space transform.
        cubeT3d = ufe.Transform3d.transform3d(cubeItem)
        cubeWorld = cubeT3d.inclusiveMatrix()
        cubeWorldListPre = matrixToList(cubeWorld)

        # Parent cube to cylinder in absolute mode (default), using UFE
        # path strings.
        cmds.parent("|mayaUsdProxy1|mayaUsdProxyShape1,/cubeXform",
                    "|mayaUsdProxy1|mayaUsdProxyShape1,/cylinderXform")

        # Confirm that the cube is now a child of the cylinder.
        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 2)
        self.assertIn("cubeXform", childrenNames(cylChildren))

        # Undo: the cube is no longer a child of the cylinder.
        cmds.undo()

        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 1)

        # Redo: confirm that the cube is again a child of the cylinder.
        cmds.redo()

        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 2)
        self.assertIn("cubeXform", childrenNames(cylChildren))

        # Confirm that the cube's world transform has not changed.  Must
        # re-create the item, as its path has changed.
        cubeChildPath = ufe.Path(
            [shapeSegment, usdUtils.createUfePathSegment("/cylinderXform/cubeXform")])
        cubeChildItem = ufe.Hierarchy.createItem(cubeChildPath)
        cubeChildT3d = ufe.Transform3d.transform3d(cubeChildItem)

        cubeWorld = cubeChildT3d.inclusiveMatrix()
        assertVectorAlmostEqual(
            self, cubeWorldListPre, matrixToList(cubeWorld), places=6)

        # Cube world y coordinate is currently 0.
        self.assertAlmostEqual(cubeWorld.matrix[3][1], 0)

        # Move only the parent.
        sn = ufe.Selection()
        sn.append(cylinderItem)
        ufe.GlobalSelection.get().replaceWith(sn)

        cmds.move(0, 10, 0, relative=True)

        # Get the world space transform again.  The y component should
        # have incremented by 10.
        cubeWorld = cubeChildT3d.inclusiveMatrix()
        self.assertAlmostEqual(cubeWorld.matrix[3][1], 10)

        # Undo everything.
        for i in range(2):
            cmds.undo()

        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 1)

    def testParentRelativeLayer(self):
        '''
        Test that parenting in sub-layer that are loaded in relative mode works.
        '''
        usdFileName = testUtils.getTestScene('relativeSubLayer', 'root.usda')
        shapeNode, stage = mayaUtils.createProxyFromFile(usdFileName)
        self.assertEqual(len(stage.GetRootLayer().subLayerPaths), 1)
        sublayer = Sdf.Layer.FindRelativeToLayer(stage.GetRootLayer(), stage.GetRootLayer().subLayerPaths[0])
        with Usd.EditContext(stage, sublayer):
            cmds.parent(shapeNode + ',/group/geo', shapeNode + ',/group/other')
        movedCube = stage.GetPrimAtPath('/group/other/geo/cube')
        self.assertTrue(movedCube)

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2023, 'Requires Maya fixes only available in Maya 2023 or greater.')
    def testParentAbsoluteSingleMatrixOp(self):
        """Test parent -absolute on prim with a single matrix op."""

        # Our test strategy is the following: we use the existing scene's cube
        # prim as reference, and set its local matrix onto a new prim that has
        # a single matrix op.  The cube prim is used as a reference.
        #
        # We then parent -absolute the new prim as well as the cube, and assert
        # that neither the new prim or the cube have moved in world space.

        # Create scene items for the cube, the cylinder, and the proxy shape.
        proxyShapePathStr = '|mayaUsdProxy1|mayaUsdProxyShape1'
        proxyShapePath = ufe.PathString.path(proxyShapePathStr)
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        shapeSegment = mayaUtils.createUfePathSegment(proxyShapePathStr)
        cubePath = ufe.Path(
            [shapeSegment, usdUtils.createUfePathSegment("/cubeXform")])
        cubeItem = ufe.Hierarchy.createItem(cubePath)
        cylinderPath = ufe.Path(
            [shapeSegment, usdUtils.createUfePathSegment("/cylinderXform")])
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)

        # get the USD stage
        stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(shapeSegment)))

        # check GetLayerStack behavior
        self.assertEqual(stage.GetEditTarget().GetLayer(),
                         stage.GetRootLayer())

        # Create a capsule prim directly under the proxy shape
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'Capsule'])

        capsulePath = ufe.PathString.path(proxyShapePathStr+',/Capsule1')

        capsulePrim = mayaUsd.ufe.ufePathToPrim(
            ufe.PathString.string(capsulePath))
        capsuleXformable = UsdGeom.Xformable(capsulePrim)

        # The capsule is not a child of the cylinder.
        cylHier = ufe.Hierarchy.hierarchy(cylinderItem)
        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 1)

        # Add a single matrix transform op to the capsule.
        capsulePrim = mayaUsd.ufe.ufePathToPrim(
            ufe.PathString.string(capsulePath))
        capsuleXformable = UsdGeom.Xformable(capsulePrim)
        capsuleXformable.AddTransformOp()

        self.assertEqual(
            capsuleXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray([
                "xformOp:transform"]))

        # Delay creating the Transform3d interface until after we've added our
        # single matrix transform op, so that we get a UsdTransform3dMatrixOp.
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)
        capsuleT3d = ufe.Transform3d.transform3d(capsuleItem)

        # The cube is a direct child of the proxy shape, whose transform is the
        # identity, so the cube's world and local space transforms are identical
        cubeT3d = ufe.Transform3d.transform3d(cubeItem)
        cubeLocal = cubeT3d.matrix()
        cubeWorld = cubeT3d.inclusiveMatrix()
        cubeWorldListPre = matrixToList(cubeWorld)

        # Set the capsule's transform to be the same as the cube's.
        capsuleT3d.setMatrix(cubeLocal)

        self.assertEqual(
            capsuleXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray([
                "xformOp:transform"]))

        capsuleLocal = capsuleT3d.matrix()
        capsuleWorld = capsuleT3d.inclusiveMatrix()
        capsuleWorldListPre = matrixToList(capsuleWorld)

        assertVectorAlmostEqual(
            self, matrixToList(cubeLocal), matrixToList(capsuleLocal))
        assertVectorAlmostEqual(
            self, matrixToList(cubeWorld), matrixToList(capsuleWorld))

        # Parent cube and capsule to cylinder in absolute mode (default), using
        # UFE path strings.
        cmds.parent("|mayaUsdProxy1|mayaUsdProxyShape1,/cubeXform",
                    "|mayaUsdProxy1|mayaUsdProxyShape1,/Capsule1",
                    "|mayaUsdProxy1|mayaUsdProxyShape1,/cylinderXform")

        # Confirm that the cube and capsule are now children of the cylinder.
        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 3)
        self.assertIn("cubeXform", childrenNames(cylChildren))
        self.assertIn("Capsule1", childrenNames(cylChildren))

        # Undo: cylinder no longer has children.
        cmds.undo()

        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 1)

        # Redo: confirm children are back.
        cmds.redo()

        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 3)
        self.assertIn("cubeXform", childrenNames(cylChildren))
        self.assertIn("Capsule1", childrenNames(cylChildren))

        # Confirm that the cube and capsule's world transform has not changed.
        # Must re-create the items, as their path has changed.
        cubeChildPath = ufe.Path(
            [shapeSegment, usdUtils.createUfePathSegment("/cylinderXform/cubeXform")])
        cubeChildItem = ufe.Hierarchy.createItem(cubeChildPath)
        cubeChildT3d = ufe.Transform3d.transform3d(cubeChildItem)
        capsuleChildPath = ufe.Path(
            [shapeSegment, usdUtils.createUfePathSegment("/cylinderXform/Capsule1")])
        capsuleChildItem = ufe.Hierarchy.createItem(capsuleChildPath)
        capsuleChildT3d = ufe.Transform3d.transform3d(capsuleChildItem)

        cubeWorld = cubeChildT3d.inclusiveMatrix()
        capsuleWorld = capsuleChildT3d.inclusiveMatrix()
        assertVectorAlmostEqual(
            self, cubeWorldListPre, matrixToList(cubeWorld), places=6)
        assertVectorAlmostEqual(
            self, capsuleWorldListPre, matrixToList(capsuleWorld), places=6)

        # Undo everything.
        cmds.undo()

        cylChildren = cylHier.children()
        self.assertEqual(len(cylChildren), 1)

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2023, 'Requires Maya fixes only available in Maya 2023 or greater.')
    def testParentAbsoluteFallback(self):
        """Test parent -absolute on prim with a fallback Maya transform stack."""
        # Create a scene with an xform and a capsule.
        import mayaUsd_createStageWithNewLayer

        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePathStr = '|stage1|stageShape1'
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()
        xformPrim = stage.DefinePrim('/Xform1', 'Xform')
        capsulePrim = stage.DefinePrim('/Capsule1', 'Capsule')
        xformXformable = UsdGeom.Xformable(xformPrim)
        capsuleXformable = UsdGeom.Xformable(capsulePrim)

        proxyShapePathSegment = mayaUtils.createUfePathSegment(
            proxyShapePathStr)

        # Translate and rotate the xform and capsule to set up their initial
        # transform ops.
        xformPath = ufe.Path([proxyShapePathSegment,
                              usdUtils.createUfePathSegment('/Xform1')])
        xformItem = ufe.Hierarchy.createItem(xformPath)
        capsulePath = ufe.Path([proxyShapePathSegment,
                                usdUtils.createUfePathSegment('/Capsule1')])
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)

        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(xformItem)

        cmds.move(0, 5, 0, r=True, os=True, wd=True)
        cmds.rotate(0, 90, 0, r=True, os=True, fo=True)

        self.assertEqual(
            xformXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray((
                "xformOp:translate", "xformOp:rotateXYZ")))

        sn.clear()
        sn.append(capsuleItem)

        cmds.move(-10, 0, 8, r=True, os=True, wd=True)
        cmds.rotate(90, 0, 0, r=True, os=True, fo=True)

        # Add an extra rotate transform op.  In so doing, the capsule prim no
        # longer matches the Maya transform stack, so our parent -absolute
        # operation will be forced to append Maya fallback transform stack ops
        # to the capsule.
        capsuleXformable.AddRotateXOp()
        self.assertEqual(
            capsuleXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray((
                "xformOp:translate", "xformOp:rotateXYZ", "xformOp:rotateX")))

        capsuleT3d = ufe.Transform3d.transform3d(capsuleItem)
        capsuleWorld = capsuleT3d.inclusiveMatrix()
        capsuleWorldPre = matrixToList(capsuleWorld)

        # The xform currently has no children.
        xformHier = ufe.Hierarchy.hierarchy(xformItem)
        xformChildren = xformHier.children()
        self.assertEqual(len(xformChildren), 0)

        # Parent the capsule to the xform.
        cmds.parent(ufe.PathString.string(capsulePath),
                    ufe.PathString.string(xformPath))

        def checkParentDone():
            # The xform now has the capsule as its child.
            xformChildren = xformHier.children()
            self.assertEqual(len(xformChildren), 1)
            self.assertIn('Capsule1', childrenNames(xformChildren))

            # Confirm that the capsule has not moved in world space.  Must
            # re-create the prim and its scene item, as its path has changed.
            capsulePath = ufe.Path(
                [proxyShapePathSegment,
                 usdUtils.createUfePathSegment('/Xform1/Capsule1')])
            capsulePrim = mayaUsd.ufe.ufePathToPrim(
                ufe.PathString.string(capsulePath))
            capsuleXformable = UsdGeom.Xformable(capsulePrim)
            capsuleItem = ufe.Hierarchy.createItem(capsulePath)
            capsuleT3d = ufe.Transform3d.transform3d(capsuleItem)
            capsuleWorld = capsuleT3d.inclusiveMatrix()
            assertVectorAlmostEqual(
                self, capsuleWorldPre, matrixToList(capsuleWorld))

            # The capsule's transform ops now have Maya fallback stack ops.  A
            # scale fallback op is added, even though it's uniform unit.
            self.assertEqual(
                capsuleXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray((
                    "xformOp:translate", "xformOp:rotateXYZ", "xformOp:rotateX",
                    "xformOp:translate:maya_fallback",
                    "xformOp:rotateXYZ:maya_fallback",
                    "xformOp:scale:maya_fallback")))

        checkParentDone()

        # Undo: the xform no longer has a child, the capsule is still where it
        # has always been, and the fallback transform ops are gone.
        cmds.undo()

        xformChildren = xformHier.children()
        self.assertEqual(len(xformChildren), 0)

        capsulePath = ufe.Path(
            [proxyShapePathSegment, usdUtils.createUfePathSegment('/Capsule1')])
        capsulePrim = mayaUsd.ufe.ufePathToPrim(
            ufe.PathString.string(capsulePath))
        capsuleXformable = UsdGeom.Xformable(capsulePrim)
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)
        capsuleT3d = ufe.Transform3d.transform3d(capsuleItem)
        capsuleWorld = capsuleT3d.inclusiveMatrix()
        assertVectorAlmostEqual(
            self, capsuleWorldPre, matrixToList(capsuleWorld))
        self.assertEqual(
            capsuleXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray((
                "xformOp:translate", "xformOp:rotateXYZ", "xformOp:rotateX")))

        # Redo: capsule still hasn't moved, Maya fallback ops are back.
        cmds.redo()

        checkParentDone()

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2023, 'Requires Maya fixes only available in Maya 2023 or greater.')
    def testParentAbsoluteMultiMatrixOp(self):
        """Test parent -absolute on prim with a transform stack with multiple matrix ops."""

        cmds.file(new=True, force=True)

        # Create a scene with an xform and a capsule.
        import mayaUsd_createStageWithNewLayer

        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePathStr = '|stage1|stageShape1'
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()
        xformPrim = stage.DefinePrim('/Xform1', 'Xform')
        capsulePrim = stage.DefinePrim('/Capsule1', 'Capsule')
        xformXformable = UsdGeom.Xformable(xformPrim)
        capsuleXformable = UsdGeom.Xformable(capsulePrim)

        proxyShapePathSegment = mayaUtils.createUfePathSegment(
            proxyShapePathStr)

        # Translate and rotate the xform.
        xformPath = ufe.Path([proxyShapePathSegment,
                              usdUtils.createUfePathSegment('/Xform1')])
        xformItem = ufe.Hierarchy.createItem(xformPath)

        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(xformItem)

        cmds.move(0, -5, 0, r=True, os=True, wd=True)
        cmds.rotate(0, -90, 0, r=True, os=True, fo=True)

        self.assertEqual(
            xformXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray((
                "xformOp:translate", "xformOp:rotateXYZ")))

        sn.clear()

        capsulePath = ufe.Path([proxyShapePathSegment,
                                usdUtils.createUfePathSegment('/Capsule1')])
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)

        # Add 3 matrix transform ops to the capsule.
        # matrix A: tx  5, ry 90
        # matrix B: ty 10, rz 90
        # matrix C: tz 15, rx 90
        op = capsuleXformable.AddTransformOp(opSuffix='A')
        matrixValA = Gf.Matrix4d(0, 0, -1, 0, 0, 1, 0,
                                 0, 1, 0, 0, 0, 5, 0, 0, 1)
        op.Set(matrixValA)
        op = capsuleXformable.AddTransformOp(opSuffix='B')
        matrixValB = Gf.Matrix4d(0, 1, 0, 0, -1, 0, 0,
                                 0, 0, 0, 1, 0, 0, 10, 0, 1)
        op.Set(matrixValB)
        op = capsuleXformable.AddTransformOp(opSuffix='C')
        matrixValC = Gf.Matrix4d(
            1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 15, 1)
        op.Set(matrixValC)

        matrixOps = [
            "xformOp:transform:A", "xformOp:transform:B", "xformOp:transform:C"]
        matrixValues = {
            "xformOp:transform:A": matrixValA,
            "xformOp:transform:B": matrixValB,
            "xformOp:transform:C": matrixValC}

        self.assertEqual(capsuleXformable.GetXformOpOrderAttr().Get(),
                         Vt.TokenArray(matrixOps))

        # Capture the current world space transform of the capsule.
        capsuleT3d = ufe.Transform3d.transform3d(capsuleItem)
        capsuleWorldPre = matrixToList(capsuleT3d.inclusiveMatrix())

        # We will run the parenting test 3 times, targeting each matrix op in
        # turn.
        for matrixOp in matrixOps:
            os.environ['MAYA_USD_MATRIX_XFORM_OP_NAME'] = matrixOp

            # The xform currently has no children.
            xformHier = ufe.Hierarchy.hierarchy(xformItem)
            xformChildren = xformHier.children()
            self.assertEqual(len(xformChildren), 0)

            # Parent the capsule to the xform.
            cmds.parent(ufe.PathString.string(capsulePath),
                        ufe.PathString.string(xformPath))

            def checkParentDone():
                # The xform now has the capsule as its child.
                xformChildren = xformHier.children()
                self.assertEqual(len(xformChildren), 1)
                self.assertIn('Capsule1', childrenNames(xformChildren))

                # Confirm that the capsule has not moved in world space.  Must
                # re-create the prim and its scene item after path change.
                capsulePath = ufe.Path(
                    [proxyShapePathSegment,
                     usdUtils.createUfePathSegment('/Xform1/Capsule1')])
                capsulePrim = mayaUsd.ufe.ufePathToPrim(
                    ufe.PathString.string(capsulePath))
                capsuleXformable = UsdGeom.Xformable(capsulePrim)
                capsuleItem = ufe.Hierarchy.createItem(capsulePath)
                capsuleT3d = ufe.Transform3d.transform3d(capsuleItem)
                capsuleWorld = capsuleT3d.inclusiveMatrix()
                assertVectorAlmostEqual(
                    self, capsuleWorldPre, matrixToList(capsuleWorld))

                # No change in the capsule's transform ops.
                self.assertEqual(
                    capsuleXformable.GetXformOpOrderAttr().Get(),
                    Vt.TokenArray(matrixOps))

                # Matrix ops that were not targeted did not change.
                for checkMatrixOp in matrixOps:
                    if checkMatrixOp == matrixOp:
                        continue
                    op = UsdGeom.XformOp(
                        capsulePrim.GetAttribute(checkMatrixOp))
                    self.assertEqual(
                        op.GetOpTransform(mayaUsd.ufe.getTime(
                            ufe.PathString.string(capsulePath))),
                        matrixValues[checkMatrixOp])

            checkParentDone()

            # Undo: the xform no longer has a child, the capsule is still where
            # it has always been.
            cmds.undo()

            xformChildren = xformHier.children()
            self.assertEqual(len(xformChildren), 0)

            capsulePath = ufe.Path(
                [proxyShapePathSegment, usdUtils.createUfePathSegment('/Capsule1')])
            capsulePrim = mayaUsd.ufe.ufePathToPrim(
                ufe.PathString.string(capsulePath))
            capsuleXformable = UsdGeom.Xformable(capsulePrim)
            capsuleItem = ufe.Hierarchy.createItem(capsulePath)
            capsuleT3d = ufe.Transform3d.transform3d(capsuleItem)
            capsuleWorld = capsuleT3d.inclusiveMatrix()
            assertVectorAlmostEqual(
                self, capsuleWorldPre, matrixToList(capsuleWorld))
            self.assertEqual(
                capsuleXformable.GetXformOpOrderAttr().Get(),
                Vt.TokenArray(matrixOps))

            # Redo: capsule still hasn't moved.
            cmds.redo()

            checkParentDone()

            # Go back to initial conditions for next iteration of loop.
            cmds.undo()

    def testParentAbsoluteXformCommonAPI(self):
        """Test parent -absolute on prim with a USD common transform APIfallback Maya transform stack."""
        # Create a scene with an xform and a capsule.
        import mayaUsd_createStageWithNewLayer

        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePathStr = '|stage1|stageShape1'
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()
        xformPrim = stage.DefinePrim('/Xform1', 'Xform')
        capsulePrim = stage.DefinePrim('/Capsule1', 'Capsule')
        xformXformable = UsdGeom.Xformable(xformPrim)
        capsuleXformable = UsdGeom.Xformable(capsulePrim)

        proxyShapePathSegment = mayaUtils.createUfePathSegment(
            proxyShapePathStr)

        xformPath = ufe.Path([proxyShapePathSegment,
                              usdUtils.createUfePathSegment('/Xform1')])
        xformItem = ufe.Hierarchy.createItem(xformPath)
        capsulePath = ufe.Path([proxyShapePathSegment,
                                usdUtils.createUfePathSegment('/Capsule1')])
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)

        # Give the capsule a common API single pivot.  This will match the
        # common API, but not the Maya API.
        capsuleXformable.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "pivot")
        capsuleXformable.AddTranslateOp(
            UsdGeom.XformOp.PrecisionFloat, "pivot", True)

        self.assertEqual(
            capsuleXformable.GetXformOpOrderAttr().Get(),
            Vt.TokenArray(("xformOp:translate:pivot",
                           "!invert!xformOp:translate:pivot")))
        self.assertTrue(UsdGeom.XformCommonAPI(capsuleXformable))

        # Now set the transform on both objects.
        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(xformItem)

        cmds.move(5, 10, 15, r=True, os=True, wd=True)
        cmds.rotate(30, -30, 45, r=True, os=True, fo=True)

        sn.clear()
        sn.append(capsuleItem)

        cmds.move(-5, -10, -15, r=True, os=True, wd=True)
        cmds.rotate(-30, 60, -45, r=True, os=True, fo=True)

        capsuleT3d = ufe.Transform3d.transform3d(capsuleItem)
        capsuleWorld = capsuleT3d.inclusiveMatrix()
        capsuleWorldPre = matrixToList(capsuleWorld)

        # The xform currently has no children.
        xformHier = ufe.Hierarchy.hierarchy(xformItem)
        xformChildren = xformHier.children()
        self.assertEqual(len(xformChildren), 0)

        # Parent the capsule to the xform.
        cmds.parent(ufe.PathString.string(capsulePath),
                    ufe.PathString.string(xformPath))

        def checkParentDone():
            # The xform now has the capsule as its child.
            xformChildren = xformHier.children()
            self.assertEqual(len(xformChildren), 1)
            self.assertIn('Capsule1', childrenNames(xformChildren))

            # Confirm that the capsule has not moved in world space.  Must
            # re-create the prim and its scene item, as its path has changed.
            capsulePath = ufe.Path(
                [proxyShapePathSegment,
                 usdUtils.createUfePathSegment('/Xform1/Capsule1')])
            capsulePrim = mayaUsd.ufe.ufePathToPrim(
                ufe.PathString.string(capsulePath))
            capsuleXformable = UsdGeom.Xformable(capsulePrim)
            capsuleItem = ufe.Hierarchy.createItem(capsulePath)
            capsuleT3d = ufe.Transform3d.transform3d(capsuleItem)
            capsuleWorld = capsuleT3d.inclusiveMatrix()
            assertVectorAlmostEqual(
                self, capsuleWorldPre, matrixToList(capsuleWorld), places=6)
            # Using setMatrixCmd() on a common transform API prim will set
            # translate, rotate, and scale.
            self.assertEqual(
                capsuleXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray((
                    "xformOp:translate", "xformOp:translate:pivot",
                    "xformOp:rotateXYZ", "xformOp:scale",
                    "!invert!xformOp:translate:pivot")))

        checkParentDone()

        # Undo: the xform no longer has a child, the capsule is still where it
        # has always been.
        cmds.undo()

        xformChildren = xformHier.children()
        self.assertEqual(len(xformChildren), 0)

        capsulePath = ufe.Path(
            [proxyShapePathSegment, usdUtils.createUfePathSegment('/Capsule1')])
        capsulePrim = mayaUsd.ufe.ufePathToPrim(
            ufe.PathString.string(capsulePath))
        capsuleXformable = UsdGeom.Xformable(capsulePrim)
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)
        capsuleT3d = ufe.Transform3d.transform3d(capsuleItem)
        capsuleWorld = capsuleT3d.inclusiveMatrix()
        assertVectorAlmostEqual(
            self, capsuleWorldPre, matrixToList(capsuleWorld))

        # Redo: capsule still hasn't moved.
        cmds.redo()

        checkParentDone()

    @unittest.skipUnless(mayaUtils.ufeSupportFixLevel() >= 8, 'Requires parent command fix in Maya.')
    def testParentToSelection(self):
        '''
        Test that the parent command with a single argument will parent to the selection.
        '''
        # Create scene items for the cube and the cylinder.
        shapeSegment = mayaUtils.createUfePathSegment(
            "|mayaUsdProxy1|mayaUsdProxyShape1")
        
        cylinderPath = ufe.Path(
            [shapeSegment, usdUtils.createUfePathSegment("/cylinderXform")])
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)

        def verifyInitialSetup():
            '''Verify that the cube is not a child of the cylinder.'''
            cylHier = ufe.Hierarchy.hierarchy(cylinderItem)
            cylChildren = cylHier.children()
            self.assertEqual(len(cylChildren), 1)
            self.assertNotIn("cubeXform", childrenNames(cylChildren))

        verifyInitialSetup()

        # Parent cube to cylinder by passing the cube but not the cylinder
        # while the cylinder is selected.
        cmds.select("|mayaUsdProxy1|mayaUsdProxyShape1,/cylinderXform")
        cmds.parent("|mayaUsdProxy1|mayaUsdProxyShape1,/cubeXform")

        def verifyCubeUnderCylinder():
            '''Verify that the cube is under the cylinder.'''
            cylHier = ufe.Hierarchy.hierarchy(cylinderItem)
            cylChildren = cylHier.children()
            self.assertEqual(len(cylChildren), 2)
            self.assertIn("cubeXform", childrenNames(cylChildren))

        # Confirm that the cube is now a child of the cylinder.
        verifyCubeUnderCylinder()

        # Undo: the cube is no longer a child of the cylinder.
        cmds.undo()
        verifyInitialSetup()

        # Redo: the cube is again a child of the cylinder.
        cmds.redo()
        verifyCubeUnderCylinder()

    def testParentToProxyShape(self):

        # Load a file with a USD hierarchy at least 2-levels deep.
        with OpenFileCtx("simpleHierarchy.ma"):

            # Create scene items for the proxy shape and the sphere.
            shapeSegment = mayaUtils.createUfePathSegment(
                "|mayaUsdProxy1|mayaUsdProxyShape1")
            shapePath = ufe.Path([shapeSegment])
            shapeItem = ufe.Hierarchy.createItem(shapePath)

            spherePath = ufe.Path(
                [shapeSegment, usdUtils.createUfePathSegment("/pCylinder1/pCube1/pSphere1")])
            sphereItem = ufe.Hierarchy.createItem(spherePath)

            # get the USD stage
            stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(shapeSegment)))

            # check GetLayerStack behavior
            self.assertEqual(stage.GetEditTarget().GetLayer(),
                             stage.GetRootLayer())

            # The sphere is not a child of the proxy shape.
            shapeHier = ufe.Hierarchy.hierarchy(shapeItem)
            shapeChildren = shapeHier.children()
            self.assertNotIn("pSphere1", childrenNames(shapeChildren))

            # Get its world space transform.
            sphereT3d = ufe.Transform3d.transform3d(sphereItem)
            sphereWorld = sphereT3d.inclusiveMatrix()
            sphereWorldListPre = matrixToList(sphereWorld)

            # Parent sphere to proxy shape in absolute mode (default), using UFE
            # path strings.Expect the exception happens
            cmds.parent("|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1/pCube1/pSphere1",
                        "|mayaUsdProxy1|mayaUsdProxyShape1")

            # Confirm that the sphere is now a child of the proxy shape.
            shapeChildren = shapeHier.children()
            self.assertIn("pSphere1", childrenNames(shapeChildren))

            # Undo: the sphere is no longer a child of the proxy shape.
            cmds.undo()

            shapeChildren = shapeHier.children()
            self.assertNotIn("pSphere1", childrenNames(shapeChildren))

            # Redo: confirm that the sphere is again a child of the proxy shape.
            cmds.redo()

            shapeChildren = shapeHier.children()
            self.assertIn("pSphere1", childrenNames(shapeChildren))

            # Confirm that the sphere's world transform has not changed.  Must
            # re-create the item, as its path has changed.
            sphereChildPath = ufe.Path(
                [shapeSegment, usdUtils.createUfePathSegment("/pSphere1")])
            sphereChildItem = ufe.Hierarchy.createItem(sphereChildPath)
            sphereChildT3d = ufe.Transform3d.transform3d(sphereChildItem)

            sphereWorld = sphereChildT3d.inclusiveMatrix()
            assertVectorAlmostEqual(
                self, sphereWorldListPre, matrixToList(sphereWorld), places=6)

            # Undo.
            cmds.undo()

            shapeChildren = shapeHier.children()
            self.assertNotIn("pSphere1", childrenNames(shapeChildren))

    def testIllegalChild(self):
        '''Parenting an object to a descendant must report an error.'''

        with OpenFileCtx("simpleHierarchy.ma"):
            with self.assertRaises(RuntimeError):
                cmds.parent(
                    "|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1",
                    "|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1/pCube1")

    def testAlreadyChild(self):
        '''Parenting an object to its current parent is a no-op.'''

        with OpenFileCtx("simpleHierarchy.ma"):
            shapeSegment = mayaUtils.createUfePathSegment(
                "|mayaUsdProxy1|mayaUsdProxyShape1")
            spherePath = ufe.Path(
                [shapeSegment,
                 usdUtils.createUfePathSegment("/pCylinder1/pCube1/pSphere1")])
            sphereItem = ufe.Hierarchy.createItem(spherePath)
            cylinderShapePath = ufe.Path(
                [shapeSegment,
                 usdUtils.createUfePathSegment("/pCylinder1/pCylinderShape1")])
            cylinderShapeItem = ufe.Hierarchy.createItem(cylinderShapePath)
            parentPath = ufe.Path(
                [shapeSegment, usdUtils.createUfePathSegment("/pCylinder1")])
            parentItem = ufe.Hierarchy.createItem(parentPath)

            parent = ufe.Hierarchy.hierarchy(parentItem)
            childrenPre = parent.children()

            # get the USD stage
            stage = mayaUsd.ufe.getStage(ufe.PathString.string(ufe.Path(shapeSegment)))

            # check GetLayerStack behavior
            self.assertEqual(stage.GetEditTarget().GetLayer(),
                             stage.GetRootLayer())

            # The sphere is not a child of the cylinder
            self.assertNotIn("pSphere1", childrenNames(childrenPre))

            # The cylinder shape is a child of the cylinder
            self.assertIn("pCylinderShape1", childrenNames(childrenPre))

            # Parent the sphere and the cylinder shape to the cylinder.  This
            # is a no-op for the cylinder shape, as it's already a child of the
            # cylinder, and the sphere is parented to the cylinder.
            cmds.parent(ufe.PathString.string(spherePath),
                        ufe.PathString.string(cylinderShapePath),
                        ufe.PathString.string(parentPath))

            children = parent.children()
            self.assertEqual(len(childrenPre)+1, len(children))
            self.assertIn("pSphere1", childrenNames(children))
            self.assertIn("pCylinderShape1", childrenNames(children))

            # Undo / redo
            cmds.undo()

            children = parent.children()
            self.assertEqual(len(childrenPre), len(children))
            self.assertNotIn("pSphere1", childrenNames(children))
            self.assertIn("pCylinderShape1", childrenNames(children))

            cmds.redo()

            children = parent.children()
            self.assertEqual(len(childrenPre)+1, len(children))
            self.assertIn("pSphere1", childrenNames(children))
            self.assertIn("pCylinderShape1", childrenNames(children))

    def testIllegalInstance(self):
        '''Parenting an object to an instance must report an error.'''

        with OpenFileCtx("simpleHierarchy.ma"):
            with self.assertRaises(RuntimeError):
                cmds.parent(
                    "|mayaUsdProxy1|mayaUsdProxyShape1,/hierarchy_instance2",
                    "|mayaUsdProxy1|mayaUsdProxyShape1,/hierarchy_instance1")

    def testUnparentUSD(self):
        '''Unparent USD node.'''

        with OpenFileCtx("simpleHierarchy.ma"):
            # Unparent a USD node
            cubePathStr = '|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1/pCube1'
            cubePath = ufe.PathString.path(cubePathStr)
            cylinderItem = ufe.Hierarchy.createItem(ufe.PathString.path(
                '|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1'))
            proxyShapeItem = ufe.Hierarchy.createItem(
                ufe.PathString.path('|mayaUsdProxy1|mayaUsdProxyShape1'))
            proxyShape = ufe.Hierarchy.hierarchy(proxyShapeItem)
            cylinder = ufe.Hierarchy.hierarchy(cylinderItem)

            def checkUnparent(done):
                proxyShapeChildren = proxyShape.children()
                cylinderChildren = cylinder.children()
                self.assertEqual(
                    'pCube1' in childrenNames(proxyShapeChildren), done)
                self.assertEqual(
                    'pCube1' in childrenNames(cylinderChildren), not done)

            checkUnparent(done=False)

            cmds.parent(cubePathStr, world=True)
            checkUnparent(done=True)

            cmds.undo()
            checkUnparent(done=False)

            cmds.redo()
            checkUnparent(done=True)

    def testUnparentMultiStage(self):
        '''Unparent USD nodes in more than one stage.'''

        with OpenFileCtx("simpleHierarchy.ma"):
            # An early version of this test imported the same file into the
            # opened file.  Layers are then shared between the stages, because
            # they come from the same USD file, causing changes done below one
            # proxy shape to be seen in the other.  Import from another file.
            filePath = testUtils.getTestScene(
                "parentCmd", "simpleSceneUSD_TRS.ma")
            cmds.file(filePath, i=True)

            # Unparent a USD node in each stage.
            stage1ItemPath = '|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1/pCube1'
            stage2ItemPath = '|simpleSceneUSD_TRS_mayaUsdProxy1|simpleSceneUSD_TRS_mayaUsdProxyShape1,/cylinderXform/pCylinder1'

            stage1ParentItem = ufe.Hierarchy.createItem(ufe.PathString.path(
                '|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1'))
            stage2ParentItem = ufe.Hierarchy.createItem(
                ufe.PathString.path('|simpleSceneUSD_TRS_mayaUsdProxy1|simpleSceneUSD_TRS_mayaUsdProxyShape1,/cylinderXform'))
            stage1ProxyShapeItem = ufe.Hierarchy.createItem(ufe.PathString.path(
                '|mayaUsdProxy1|mayaUsdProxyShape1'))
            stage2ProxyShapeItem = ufe.Hierarchy.createItem(ufe.PathString.path(
                '|simpleSceneUSD_TRS_mayaUsdProxy1|simpleSceneUSD_TRS_mayaUsdProxyShape1'))
            stage1ParentHierarchy = ufe.Hierarchy.hierarchy(stage1ParentItem)
            stage2ParentHierarchy = ufe.Hierarchy.hierarchy(stage2ParentItem)
            stage1ProxyShapeHierarchy = ufe.Hierarchy.hierarchy(stage1ProxyShapeItem)
            stage2ProxyShapeHierarchy = ufe.Hierarchy.hierarchy(stage2ProxyShapeItem)

            def checkUnparent(done):
                stage1ProxyShapeChildren = stage1ProxyShapeHierarchy.children()
                stage2ProxyShapeChildren = stage2ProxyShapeHierarchy.children()
                stage1ParentChildren = stage1ParentHierarchy.children()
                stage2ParentChildren = stage2ParentHierarchy.children()
                self.assertEqual(
                    'pCube1' in childrenNames(stage1ProxyShapeChildren), done)
                self.assertEqual(
                    'pCube1' in childrenNames(stage1ParentChildren), not done)
                self.assertEqual(
                    'pCylinder1' in childrenNames(stage2ProxyShapeChildren), done)
                self.assertEqual(
                    'pCylinder1' in childrenNames(stage2ParentChildren), not done)

            checkUnparent(done=False)

            cmds.parent(stage1ItemPath, stage2ItemPath, w=True)
            checkUnparent(done=True)

            cmds.undo()
            checkUnparent(done=False)

            cmds.redo()
            checkUnparent(done=True)

    def testParentingToGPrim(self):
        '''Parenting an object to UsdGeomGprim object is not allowed'''

        # open tree scene
        mayaUtils.openTreeScene()

        with self.assertRaises(RuntimeError):
            cmds.parent("|Tree_usd|Tree_usdShape,/TreeBase/trunk",
                        "|Tree_usd|Tree_usdShape,/TreeBase/leavesXform/leaves")

    def testParentRestrictionDefaultPrim(self):
        '''
        Verify that a prim that is the default prim prevent
        parenting that prim when not targeting the root layer.
        '''
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('defaultPrimInSub', 'root.usda')
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)

        capsulePathStr = '|stage|stageShape,/Capsule1'

        layer = Sdf.Layer.FindRelativeToLayer(stage.GetRootLayer(), stage.GetRootLayer().subLayerPaths[0])
        self.assertIsNotNone(layer)
        stage.SetEditTarget(layer)
        self.assertEqual(stage.GetEditTarget().GetLayer(), layer)

        x1 = stage.DefinePrim('/Xform1', 'Xform')
        self.assertIsNotNone(x1)
        x1PathStr = '|stage|stageShape,/Xform1'
        
        with self.assertRaises(RuntimeError):
            cmds.parent(capsulePathStr, x1PathStr)

    def testParentToStrongerLayer(self):
        '''
        Verify that parenting a prim to a prim defined in a lower layer
        is permitted.
        '''
        cmds.file(new=True, force=True)

        # Create an empty stage with a sub-layer
        import mayaUsd_createStageWithNewLayer
        proxyShapePathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()
        subLayer = usdUtils.addNewLayerToStage(stage)

        # Create a xform in the sub-layer and a capsule in the root layer.
        with Usd.EditContext(stage, subLayer):
            subXFormName = '/SubXForm'
            subXFormPrim = stage.DefinePrim(subXFormName, 'Xform')
            self.assertTrue(subXFormPrim)

        rootCapsuleName = '/RootCapsule'
        rootCapsulePrim = stage.DefinePrim(rootCapsuleName, 'Capsule')
        self.assertTrue(rootCapsulePrim)

        subXFormUFEPath = proxyShapePathStr + "," + subXFormName
        rootCapsuleUFEPath = proxyShapePathStr + "," + rootCapsuleName
        
        cmds.parent(rootCapsuleUFEPath, subXFormUFEPath)

        newRootCapsuleUSDPath = subXFormName + rootCapsuleName
        self.assertTrue(stage.GetPrimAtPath(newRootCapsuleUSDPath))

    def testParentMultiLayers(self):
        '''
        Verify that parenting a prim defined in multiple layers work.
        '''
        cmds.file(new=True, force=True)

        # Create an empty stage with a sub-layer
        import mayaUsd_createStageWithNewLayer
        proxyShapePathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()

        # Create an xform and a capsule in the root layer.
        xformName = "/xf"
        xformUFEPathStr = proxyShapePathStr + "," + xformName
        xformPrim = stage.DefinePrim(xformName, 'Xform')
        self.assertTrue(xformPrim)

        rootCapsuleName = '/RootCapsule'
        rootCapsuleUFEPath = proxyShapePathStr + "," + rootCapsuleName
        rootCapsulePrim = stage.DefinePrim(rootCapsuleName, 'Capsule')
        self.assertTrue(rootCapsulePrim)
        
        # Move the capsule while targeting the session layer.
        sessionLayer = stage.GetSessionLayer()
        with Usd.EditContext(stage, sessionLayer):
            capsulePath = ufe.PathString.path(rootCapsuleUFEPath)
            capsuleItem = ufe.Hierarchy.createItem(capsulePath)

            sn = ufe.GlobalSelection.get()
            sn.clear()
            sn.append(capsuleItem)

            cmds.move(0, -5, 0, r=True, os=True, wd=True)

        with Usd.EditContext(stage, sessionLayer):
            cmds.parent(rootCapsuleUFEPath, xformUFEPathStr)

        newRootCapsuleUSDPath = xformName + rootCapsuleName
        self.assertTrue(stage.GetPrimAtPath(newRootCapsuleUSDPath))
        self.assertIsNotNone(sessionLayer.GetPrimAtPath(newRootCapsuleUSDPath))

        rootLayer = stage.GetRootLayer()
        self.assertIsNone(rootLayer.GetPrimAtPath(newRootCapsuleUSDPath))

    def testParentMultiLayersKeepSessionData(self):
        '''
        Verify that parenting a prim defined in multiple layers work
        while keeping session data in the session layer.
        '''
        cmds.file(new=True, force=True)

        # Create an empty stage with a sub-layer
        import mayaUsd_createStageWithNewLayer
        proxyShapePathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()

        # Create an xform and a capsule in the root layer.
        xformName = "/xf"
        xformUFEPathStr = proxyShapePathStr + "," + xformName
        xformPrim = stage.DefinePrim(xformName, 'Xform')
        self.assertTrue(xformPrim)

        rootCapsuleUSDPath = '/RootCapsule'
        rootCapsuleUFEPath = proxyShapePathStr + "," + rootCapsuleUSDPath
        rootCapsulePrim = stage.DefinePrim(rootCapsuleUSDPath, 'Capsule')
        self.assertTrue(rootCapsulePrim)
        
        # Move the capsule while targeting the session layer.
        sessionLayer = stage.GetSessionLayer()
        with Usd.EditContext(stage, sessionLayer):
            capsulePath = ufe.PathString.path(rootCapsuleUFEPath)
            capsuleItem = ufe.Hierarchy.createItem(capsulePath)

            sn = ufe.GlobalSelection.get()
            sn.clear()
            sn.append(capsuleItem)

            cmds.move(0, -5, 0, r=True, os=True, wd=True)

        def verifySessionData(capsulePath):
            capsuleSessionSpec = sessionLayer.GetPrimAtPath(capsulePath)
            self.assertIsNotNone(capsuleSessionSpec)
            attrsSpecs = capsuleSessionSpec.attributes
            self.assertIn('xformOp:translate', attrsSpecs)
            self.assertIn('xformOpOrder', attrsSpecs)

        verifySessionData(rootCapsuleUSDPath)

        self.assertIsNone(sessionLayer.GetPrimAtPath(xformName))

        with Usd.EditContext(stage, stage.GetRootLayer()):
            cmds.parent(rootCapsuleUFEPath, xformUFEPathStr)

        newRootCapsuleUSDPath = xformName + rootCapsuleUSDPath
        self.assertTrue(stage.GetPrimAtPath(newRootCapsuleUSDPath))

        self.assertIsNotNone(sessionLayer.GetPrimAtPath(xformName))

        # This is the key bit: the session data should still be there.
        verifySessionData(newRootCapsuleUSDPath)

        # The root layer data shoudl also still be there.
        rootLayer = stage.GetRootLayer()
        self.assertIsNotNone(rootLayer.GetPrimAtPath(newRootCapsuleUSDPath))
        self.assertEqual(rootLayer.GetPrimAtPath(newRootCapsuleUSDPath).typeName, "Capsule")


    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2023, 'Requires Maya fixes only available in Maya 2023 or greater.')
    def testParentShader(self):
        '''Shaders can only have NodeGraphs and Materials as parent.'''
        
        # Create a new scene with an empty stage.
        cmds.file(new=True, force=True)
        import mayaUsd_createStageWithNewLayer
        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePathStr = '|stage1|stageShape1'
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()

        # Create a simple hierarchy.
        scopePrim = stage.DefinePrim('/mtl', 'Scope')
        scopePathStr = proxyShapePathStr + ",/mtl"
        self.assertIsNotNone(scopePrim)
        materialPrim = stage.DefinePrim('/mtl/Material1', 'Material')
        materialPathStr = scopePathStr + "/Material1"
        self.assertIsNotNone(materialPrim)
        nodeGraphPrim = stage.DefinePrim('/mtl/Material1/NodeGraph1', 'NodeGraph')
        nodeGraphPathStr = materialPathStr + "/NodeGraph1"
        self.assertIsNotNone(nodeGraphPrim)
        shaderPrim = stage.DefinePrim('/mtl/Material1/NodeGraph1/Shader1', 'Shader')
        self.assertIsNotNone(shaderPrim)

        # Get UFE hierarchy objects.
        materialItem = ufe.Hierarchy.createItem(ufe.PathString.path(materialPathStr))
        materialHierarchy = ufe.Hierarchy.hierarchy(materialItem)
        nodeGraphItem = ufe.Hierarchy.createItem(ufe.PathString.path(nodeGraphPathStr))
        nodeGraphHierarchy = ufe.Hierarchy.hierarchy(nodeGraphItem)

        # Parenting a Shader to a Material is allowed.
        cmds.parent(nodeGraphPathStr + "/Shader1", materialPathStr)
        self.assertEqual('Shader1' in childrenNames(materialHierarchy.children()), True)
        self.assertEqual('Shader1' in childrenNames(nodeGraphHierarchy.children()), False)
        
        # Parenting a Shader to a NodeGraph is allowed.
        cmds.parent(materialPathStr + "/Shader1", nodeGraphPathStr)
        self.assertEqual('Shader1' in childrenNames(materialHierarchy.children()), False)
        self.assertEqual('Shader1' in childrenNames(nodeGraphHierarchy.children()), True)

        # Parenting a Shader to a Scope is not allowed.
        with self.assertRaises(RuntimeError):
            cmds.parent(nodeGraphPathStr + "/Shader1", scopePathStr)

        # Parenting a Shader to a ProxyShape is not allowed.
        with self.assertRaises(RuntimeError):
            cmds.parent(nodeGraphPathStr + "/Shader1", world=True)

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2023, 'Requires Maya fixes only available in Maya 2023 or greater.')
    def testParentNodeGraph(self):
        '''NodeGraphs can only have a NodeGraphs and Materials as parent.'''
        
        # Create a new scene with an empty stage.
        cmds.file(new=True, force=True)
        import mayaUsd_createStageWithNewLayer
        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePathStr = '|stage1|stageShape1'
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()

        # Create a simple hierarchy.
        scopePrim = stage.DefinePrim('/mtl', 'Scope')
        scopePathStr = proxyShapePathStr + ",/mtl"
        self.assertIsNotNone(scopePrim)
        materialPrim = stage.DefinePrim('/mtl/Material1', 'Material')
        materialPathStr = scopePathStr + "/Material1"
        self.assertIsNotNone(materialPrim)
        nodeGraphPrim = stage.DefinePrim('/mtl/Material1/NodeGraph1', 'NodeGraph')
        nodeGraphPathStr = materialPathStr + "/NodeGraph1"
        self.assertIsNotNone(nodeGraphPrim)
        nodeGraphPrim2 = stage.DefinePrim('/mtl/Material1/NodeGraph1/NodeGraph2', 'NodeGraph')
        self.assertIsNotNone(nodeGraphPrim2)

        # Get UFE hierarchy objects.
        materialItem = ufe.Hierarchy.createItem(ufe.PathString.path(materialPathStr))
        materialHierarchy = ufe.Hierarchy.hierarchy(materialItem)
        nodeGraphItem = ufe.Hierarchy.createItem(ufe.PathString.path(nodeGraphPathStr))
        nodeGraphHierarchy = ufe.Hierarchy.hierarchy(nodeGraphItem)

        # Parenting a NodeGraph to a Material is allowed.
        cmds.parent(nodeGraphPathStr + "/NodeGraph2", materialPathStr)
        self.assertEqual('NodeGraph2' in childrenNames(materialHierarchy.children()), True)
        self.assertEqual('NodeGraph2' in childrenNames(nodeGraphHierarchy.children()), False)
        
        # Parenting a NodeGraph to a NodeGraph is allowed.
        cmds.parent(materialPathStr + "/NodeGraph2", nodeGraphPathStr)
        self.assertEqual('NodeGraph2' in childrenNames(materialHierarchy.children()), False)
        self.assertEqual('NodeGraph2' in childrenNames(nodeGraphHierarchy.children()), True)

        # Parenting a NodeGraph to a Scope is not allowed.
        with self.assertRaises(RuntimeError):
            cmds.parent(nodeGraphPathStr + "/NodeGraph2", scopePathStr)

        # Parenting a NodeGraph to a ProxyShape is not allowed.
        with self.assertRaises(RuntimeError):
            cmds.parent(nodeGraphPathStr + "/NodeGraph2", world=True)

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2023, 'Requires Maya fixes only available in Maya 2023 or greater.')
    def testParentMaterial(self):
        '''Materials cannot have Shaders, NodeGraphs or Materials as parent.'''
        
        # Create a new scene with an empty stage.
        cmds.file(new=True, force=True)
        import mayaUsd_createStageWithNewLayer
        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePathStr = '|stage1|stageShape1'
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()

        # Create a simple hierarchy.
        scopePrim = stage.DefinePrim('/mtl', 'Scope')
        scopePathStr = proxyShapePathStr + ",/mtl"
        self.assertIsNotNone(scopePrim)
        materialPrim = stage.DefinePrim('/mtl/Material1', 'Material')
        materialPathStr = scopePathStr + "/Material1"
        self.assertIsNotNone(materialPrim)
        nodeGraphPrim = stage.DefinePrim('/mtl/Material1/NodeGraph1', 'NodeGraph')
        nodeGraphPathStr = materialPathStr + "/NodeGraph1"
        self.assertIsNotNone(nodeGraphPrim)
        shaderPrim = stage.DefinePrim('/mtl/Material1/NodeGraph1/Shader1', 'Shader')
        shaderPathStr = nodeGraphPathStr + "/Shader1"
        self.assertIsNotNone(shaderPrim)
        materialPrim2 = stage.DefinePrim('/mtl/Material2', 'Material')
        self.assertIsNotNone(materialPrim2)

        # Get UFE hierarchy objects.
        scopeItem = ufe.Hierarchy.createItem(ufe.PathString.path(scopePathStr))
        scopeHierarchy = ufe.Hierarchy.hierarchy(scopeItem)
        proxyShapeItem = ufe.Hierarchy.createItem(ufe.PathString.path(proxyShapePathStr))
        proxyShapeHierarchy = ufe.Hierarchy.hierarchy(proxyShapeItem)

        # Parenting a Material to a ProxyShape is allowed.
        cmds.parent(scopePathStr + "/Material2", world=True)
        self.assertEqual('Material2' in childrenNames(proxyShapeHierarchy.children()), True)
        self.assertEqual('Material2' in childrenNames(scopeHierarchy.children()), False)

        # Parenting a Material to a Scope is allowed.
        cmds.parent(proxyShapePathStr + ",/Material2", scopePathStr)
        self.assertEqual('Material2' in childrenNames(proxyShapeHierarchy.children()), False)
        self.assertEqual('Material2' in childrenNames(scopeHierarchy.children()), True)

        # Parenting a Material to a Material is not allowed.
        with self.assertRaises(RuntimeError):
            cmds.parent(scopePathStr + "/Material2", materialPathStr)
        
        # Parenting a Material to a NodeGraph is not allowed.
        with self.assertRaises(RuntimeError):
            cmds.parent(scopePathStr + "/Material2", nodeGraphPathStr)

        # Parenting a Material to a Shader is not allowed.
        with self.assertRaises(RuntimeError):
            cmds.parent(scopePathStr + "/Material2", shaderPathStr)

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2023, 'Requires Maya fixes only available in Maya 2023 or greater.')
    def testParentHierarchy(self):
        '''Parenting a node and a descendant.'''

        # MAYA-112957: when parenting a node and its descendant, with the node
        # selected first, the descendant path becomes stale as soon as its
        # ancestor gets reparented.  The Maya parent command must deal with
        # this.  A similar test is done for grouping in testGroupCmd.py.

        cmds.file(new=True, force=True)
        import mayaUsd_createStageWithNewLayer

        # Create the following hierarchy:
        #
        # ps
        #  |_ A
        #      |_ B
        #          |_ C
        #  |_ D
        #      |_ E
        #          |_ F
        #
        #  |_ G
        #
        # We will select A, B, C, E, F and G, in order, and parent.  This
        # must parent A, B, C, E, and F to G.

        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/A', 'Xform')
        stage.DefinePrim('/A/B', 'Xform')
        stage.DefinePrim('/A/B/C', 'Xform')
        stage.DefinePrim('/D', 'Xform')
        stage.DefinePrim('/D/E', 'Xform')
        stage.DefinePrim('/D/E/F', 'Xform')
        stage.DefinePrim('/G', 'Xform')

        psPath = ufe.PathString.path(psPathStr)
        psPathSegment = psPath.segments[0]
        ps = ufe.Hierarchy.createItem(psPath)
        psHier = ufe.Hierarchy.hierarchy(ps)
        dPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/D')])
        d = ufe.Hierarchy.createItem(dPath)
        dHier = ufe.Hierarchy.hierarchy(d)
        gPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/G')])
        g = ufe.Hierarchy.createItem(gPath)
        gHier = ufe.Hierarchy.hierarchy(g)

        def hierarchyBefore():
            aPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/A')])
            a = ufe.Hierarchy.createItem(aPath)
            bPath = aPath + ufe.PathComponent('B')
            b = ufe.Hierarchy.createItem(bPath)
            cPath = bPath + ufe.PathComponent('C')
            c = ufe.Hierarchy.createItem(cPath)
            ePath = dPath + ufe.PathComponent('E')
            e = ufe.Hierarchy.createItem(ePath)
            fPath = ePath + ufe.PathComponent('F')
            f = ufe.Hierarchy.createItem(fPath)
            return [a, b, c, e, f]

        def hierarchyAfter():
            return [ufe.Hierarchy.createItem(gPath + ufe.PathComponent(pc)) for pc in ['A', 'B', 'C', 'E', 'F']]

        def checkBefore(a, b, c, e, f):
            psChildren = psHier.children()
            aHier = ufe.Hierarchy.hierarchy(a)
            bHier = ufe.Hierarchy.hierarchy(b)
            cHier = ufe.Hierarchy.hierarchy(c)
            eHier = ufe.Hierarchy.hierarchy(e)
            fHier = ufe.Hierarchy.hierarchy(f)

            self.assertIn(a, psChildren)
            self.assertIn(d, psChildren)
            self.assertIn(g, psChildren)
            self.assertIn(b, aHier.children())
            self.assertIn(c, bHier.children())
            self.assertIn(e, dHier.children())
            self.assertIn(f, eHier.children())
            self.assertFalse(gHier.hasChildren())

        def checkAfter(a, b, c, e, f):
            psChildren = psHier.children()
            self.assertNotIn(a, psChildren)
            self.assertIn(d, psChildren)
            self.assertIn(g, psChildren)

            gChildren = gHier.children()

            for child in [a, b, c, e, f]:
                hier = ufe.Hierarchy.hierarchy(child)
                self.assertFalse(hier.hasChildren())
                self.assertIn(child, gChildren)

        children = hierarchyBefore()
        checkBefore(*children)

        sn = ufe.GlobalSelection.get()
        sn.clear()
        for child in children:
            sn.append(child)
        sn.append(g)

        cmds.parent()

        children = hierarchyAfter()
        checkAfter(*children)

        cmds.undo()

        children = hierarchyBefore()
        checkBefore(*children)

        cmds.redo()

        children = hierarchyAfter()
        checkAfter(*children)

    def testEditRouter(self):
        '''Test edit router functionality.'''

        cmds.file(new=True, force=True)
        import mayaUsd_createStageWithNewLayer

        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()

        # Create the following layer hierarchy:
        #
        # anonymousLayer1
        #  |_ bSubLayer
        #  |_ aSubLayer
        #
        # Sublayer B is thus higher-priority than A.
        rootLayerId = stage.GetRootLayer().identifier
        aSubLayerId = cmds.mayaUsdLayerEditor(rootLayerId, edit=True, addAnonymous="aSubLayer")[0]
        bSubLayerId = cmds.mayaUsdLayerEditor(rootLayerId, edit=True, addAnonymous="bSubLayer")[0]

        # Create the following hierarchy in lower-priority layer A.
        #
        # ps
        #  |_ A
        #      |_ B
        #  |_ C
        #
        cmds.mayaUsdEditTarget(psPathStr, edit=True, editTarget=aSubLayerId)
        stage.DefinePrim('/A', 'Xform')
        stage.DefinePrim('/A/B', 'Xform')
        stage.DefinePrim('/C', 'Xform')

        def firstSubLayer(context, routingData):
            # Write edits to the highest-priority child layer of the root.

            # Here, prim is the parent prim.
            prim = context.get('prim')
            self.assertIsNot(prim, None)
            self.assertFalse(len(prim.GetStage().GetRootLayer().subLayerPaths)==0)
            layerId = prim.GetStage().GetRootLayer().subLayerPaths[0]
            layer = Sdf.Layer.Find(layerId)
            routingData['layer'] = layerId

        # Register our edit router which directs the parent edit to
        # higher-priority layer B, which is not the edit target.
        mayaUsd.lib.registerEditRouter('parent', firstSubLayer)
        try:
            # Check that layer B is empty.
            bSubLayer = Sdf.Layer.Find(bSubLayerId)
            self.assertEqual(filterUsdStr(bSubLayer.ExportToString()), '')

            # We select B and C, in order, and parent.  This parents B to C.
            sn = ufe.GlobalSelection.get()
            sn.clear()
            b = ufe.Hierarchy.createItem(ufe.PathString.path(psPathStr+',/A/B'))
            c = ufe.Hierarchy.createItem(ufe.PathString.path(psPathStr+',/C'))
            sn.append(b)
            sn.append(c)

            a = ufe.Hierarchy.createItem(ufe.PathString.path(psPathStr+',/A'))
            self.assertEqual(ufe.Hierarchy.hierarchy(b).parent(), a)

            cmds.parent()

            # Check that prim B is now a child of prim C.  Re-create its scene
            # item, as its path has changed.
            b = ufe.Hierarchy.createItem(ufe.PathString.path(psPathStr+',/C/B'))
            self.assertEqual(ufe.Hierarchy.hierarchy(b).parent(), c)

            # Check that layer B now has the parent overs.
            self.assertEqual(filterUsdStr(bSubLayer.ExportToString()),
                            filterUsdStr('over "C"\n{\n    def Xform "B"\n    {\n    }\n}'))
        finally:
            # Restore default edit router.
            mayaUsd.lib.restoreDefaultEditRouter('parent')

    @unittest.skipUnless(mayaUtils.ufeSupportFixLevel() >= 7, 'Require parent command fix from Maya')
    def testParentAbsoluteUnderScope(self):
        """Test parent -absolute to move prim under scope."""

        cmds.file(new=True, force=True)

        # Create a scene with an xform, a scope under the xform and a capsule.
        import mayaUsd_createStageWithNewLayer

        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePathStr = '|stage1|stageShape1'
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()

        xformPrim = stage.DefinePrim('/Xform1', 'Xform')
        self.assertIsNotNone(xformPrim)
        scopePrim = stage.DefinePrim('/Xform1/Scope1', 'Scope')
        self.assertIsNotNone(scopePrim)
        capsulePrim = stage.DefinePrim('/Capsule1', 'Capsule')
        self.assertIsNotNone(capsulePrim)

        proxyShapePathSegment = mayaUtils.createUfePathSegment(
            proxyShapePathStr)

        # Translate and rotate the xform.
        xformPath = ufe.Path([proxyShapePathSegment,
                              usdUtils.createUfePathSegment('/Xform1')])
        xformItem = ufe.Hierarchy.createItem(xformPath)

        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(xformItem)

        cmds.move(0, -5, 0, r=True, os=True, wd=True)
        cmds.rotate(0, -90, 0, r=True, os=True, fo=True)

        xformXformable = UsdGeom.Xformable(xformPrim)
        self.assertEqual(
            xformXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray((
                "xformOp:translate", "xformOp:rotateXYZ")))

        sn.clear()

        # Translate and rotate the capsule.
        capsulePath = ufe.Path([proxyShapePathSegment,
                              usdUtils.createUfePathSegment('/Capsule1')])
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)
        self.assertIsNotNone(capsuleItem)

        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(capsuleItem)

        cmds.move(0, 15, 0, r=True, os=True, wd=True)
        cmds.rotate(0, 60, 0, r=True, os=True, fo=True)

        sn.clear()

        # Capture the capsule world space transform to verify later.
        capsuleT3d = ufe.Transform3d.transform3d(capsuleItem)
        capsuleWorld = capsuleT3d.inclusiveMatrix()
        capsuleWorldPre = matrixToList(capsuleWorld)

        # Scope path and item.
        scopePath = ufe.Path([proxyShapePathSegment,
                              usdUtils.createUfePathSegment('/Xform1/Scope1')])
        scopeItem = ufe.Hierarchy.createItem(scopePath)
        self.assertIsNotNone(scopeItem)

        def checkParentDone():
            # The scope has the capsule as its child.
            scopeHier = ufe.Hierarchy.hierarchy(scopeItem)
            self.assertIsNotNone(scopeHier)
            scopeChildren = scopeHier.children()
            self.assertEqual(len(scopeChildren), 1)
            self.assertIn('Capsule1', childrenNames(scopeChildren))

        def checkParentNotDone():
            # The scope has no child.
            scopeHier = ufe.Hierarchy.hierarchy(scopeItem)
            self.assertIsNotNone(scopeHier)
            scopeChildren = scopeHier.children()
            self.assertEqual(len(scopeChildren), 0)

        def checkCapsuleMatrix(capsulePath):
            # Confirm that the capsule has not moved in world space.  Must
            # re-create the scene item after path change.
            capsulePath = ufe.Path(
                [proxyShapePathSegment,
                    usdUtils.createUfePathSegment(capsulePath)])
            capsuleItem = ufe.Hierarchy.createItem(capsulePath)
            self.assertIsNotNone(capsuleItem)
            capsuleT3d = ufe.Transform3d.transform3d(capsuleItem)
            self.assertIsNotNone(capsuleT3d)
            capsuleWorld = capsuleT3d.inclusiveMatrix()
            assertVectorAlmostEqual(
                self, capsuleWorldPre, matrixToList(capsuleWorld))

        # The scope currently has no children, capsule has not moved.
        checkParentNotDone()
        checkCapsuleMatrix('/Capsule1')

        # Parent the capsule to the scope.
        cmds.parent(ufe.PathString.string(capsulePath),
                    ufe.PathString.string(scopePath))

        checkParentDone()
        checkCapsuleMatrix('/Xform1/Scope1/Capsule1')

        # Undo: the scope no longer has a child, the capsule is still where
        # it has always been.
        cmds.undo()

        checkParentNotDone()
        checkCapsuleMatrix('/Capsule1')

        # Redo: capsule still hasn't moved.
        cmds.redo()

        checkParentDone()
        checkCapsuleMatrix('/Xform1/Scope1/Capsule1')

    @unittest.skipUnless(mayaUtils.ufeSupportFixLevel() >= 7, 'Require parent command fix from Maya')
    def testParentAbsoluteScope(self):
        """
        Test parent -absolute to move a scope with a prim under it under an xform.
        Since the scope is not transformable, the prim will move.
        """

        cmds.file(new=True, force=True)

        # Create a scene with an xform, a scope and a capsule under the scope.
        import mayaUsd_createStageWithNewLayer

        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePathStr = '|stage1|stageShape1'
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()

        xformPrim = stage.DefinePrim('/Xform1', 'Xform')
        self.assertIsNotNone(xformPrim)
        scopePrim = stage.DefinePrim('/Scope1', 'Scope')
        self.assertIsNotNone(scopePrim)
        capsulePrim = stage.DefinePrim('/Scope1/Capsule1', 'Capsule')
        self.assertIsNotNone(capsulePrim)

        proxyShapePathSegment = mayaUtils.createUfePathSegment(
            proxyShapePathStr)

        # Translate and rotate the xform.
        xformPath = ufe.Path([proxyShapePathSegment,
                              usdUtils.createUfePathSegment('/Xform1')])
        xformItem = ufe.Hierarchy.createItem(xformPath)

        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(xformItem)

        cmds.move(0, -5, 0, r=True, os=True, wd=True)

        xformXformable = UsdGeom.Xformable(xformPrim)
        self.assertEqual(
            xformXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray((
                "xformOp:translate", )))

        sn.clear()

        # Translate and rotate the capsule.
        capsulePath = ufe.Path([proxyShapePathSegment,
                              usdUtils.createUfePathSegment('/Scope1/Capsule1')])
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)
        self.assertIsNotNone(capsuleItem)

        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(capsuleItem)

        cmds.move(0, 15, 0, r=True, os=True, wd=True)

        sn.clear()

        # Capture the capsule world space transform to verify later.
        capsuleT3d = ufe.Transform3d.transform3d(capsuleItem)
        capsuleWorld = capsuleT3d.inclusiveMatrix()
        capsuleWorldPre = matrixToList(capsuleWorld)

        # Scope path and item.
        scopePath = ufe.Path([proxyShapePathSegment,
                              usdUtils.createUfePathSegment('/Scope1')])
        scopeItem = ufe.Hierarchy.createItem(scopePath)
        self.assertIsNotNone(scopeItem)

        def checkParentDone():
            # The xform has the scope as its child.
            xformHier = ufe.Hierarchy.hierarchy(xformItem)
            self.assertIsNotNone(xformHier)
            xformChildren = xformHier.children()
            self.assertEqual(len(xformChildren), 1)
            self.assertIn('Scope1', childrenNames(xformChildren))

        def checkParentNotDone():
            # The xform has no child.
            xformHier = ufe.Hierarchy.hierarchy(xformItem)
            self.assertIsNotNone(xformHier)
            xformChildren = xformHier.children()
            self.assertEqual(len(xformChildren), 0)

        def checkCapsuleMatrix(capsulePath, xpos):
            # Confirm that the capsule has moved in world space.  Must
            # re-create the scene item after path change.
            capsulePath = ufe.Path(
                [proxyShapePathSegment,
                    usdUtils.createUfePathSegment(capsulePath)])
            capsuleItem = ufe.Hierarchy.createItem(capsulePath)
            self.assertIsNotNone(capsuleItem)
            capsuleT3d = ufe.Transform3d.transform3d(capsuleItem)
            self.assertIsNotNone(capsuleT3d)
            capsuleWorld = capsuleT3d.inclusiveMatrix()
            self.assertEqual(xpos, matrixToList(capsuleWorld)[13])
            assertVectorAlmostEqual(
                self, capsuleWorldPre[0:13], matrixToList(capsuleWorld)[0:13])
            assertVectorAlmostEqual(
                self, capsuleWorldPre[14:16], matrixToList(capsuleWorld)[14:16])

        # The xform currently has no children, capsule has not moved.
        checkParentNotDone()
        checkCapsuleMatrix('/Scope1/Capsule1', 15.0)

        # Parent the scope to the xform, the capsule moved due to relative mode
        cmds.parent(ufe.PathString.string(scopePath),
                    ufe.PathString.string(xformPath))

        checkParentDone()
        checkCapsuleMatrix('/Xform1/Scope1/Capsule1', 10.0)

        # Undo: the xform no longer has a child, the capsule moved back due to relative mode.
        cmds.undo()

        checkParentNotDone()
        checkCapsuleMatrix('/Scope1/Capsule1', 15.0)

        # Redo: the capsule moved due to relative mode.
        cmds.redo()

        checkParentDone()
        checkCapsuleMatrix('/Xform1/Scope1/Capsule1', 10.0)

    @unittest.skipUnless(mayaUtils.ufeSupportFixLevel() >= 7, 'Require parent command fix from Maya')
    def testParentRelativeScope(self):
        """
        Test parent -relative to move a scope with a prim under it under an xform.
        Since the command is relative, the prim will move.
        """

        cmds.file(new=True, force=True)

        # Create a scene with an xform, a scope and a capsule under the scope.
        import mayaUsd_createStageWithNewLayer

        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePathStr = '|stage1|stageShape1'
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()

        xformPrim = stage.DefinePrim('/Xform1', 'Xform')
        self.assertIsNotNone(xformPrim)
        scopePrim = stage.DefinePrim('/Scope1', 'Scope')
        self.assertIsNotNone(scopePrim)
        capsulePrim = stage.DefinePrim('/Scope1/Capsule1', 'Capsule')
        self.assertIsNotNone(capsulePrim)

        proxyShapePathSegment = mayaUtils.createUfePathSegment(
            proxyShapePathStr)

        # Translate and rotate the xform.
        xformPath = ufe.Path([proxyShapePathSegment,
                              usdUtils.createUfePathSegment('/Xform1')])
        xformItem = ufe.Hierarchy.createItem(xformPath)

        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(xformItem)

        cmds.move(0, -5, 0, r=True, os=True, wd=True)

        xformXformable = UsdGeom.Xformable(xformPrim)
        self.assertEqual(
            xformXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray((
                "xformOp:translate", )))

        sn.clear()

        # Translate and rotate the capsule.
        capsulePath = ufe.Path([proxyShapePathSegment,
                              usdUtils.createUfePathSegment('/Scope1/Capsule1')])
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)
        self.assertIsNotNone(capsuleItem)

        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(capsuleItem)

        cmds.move(0, 15, 0, r=True, os=True, wd=True)

        sn.clear()

        # Capture the capsule world space transform to verify later.
        capsuleT3d = ufe.Transform3d.transform3d(capsuleItem)
        capsuleWorld = capsuleT3d.inclusiveMatrix()
        capsuleWorldPre = matrixToList(capsuleWorld)

        # Scope path and item.
        scopePath = ufe.Path([proxyShapePathSegment,
                              usdUtils.createUfePathSegment('/Scope1')])
        scopeItem = ufe.Hierarchy.createItem(scopePath)
        self.assertIsNotNone(scopeItem)

        def checkParentDone():
            # The xform has the scope as its child.
            xformHier = ufe.Hierarchy.hierarchy(xformItem)
            self.assertIsNotNone(xformHier)
            xformChildren = xformHier.children()
            self.assertEqual(len(xformChildren), 1)
            self.assertIn('Scope1', childrenNames(xformChildren))

        def checkParentNotDone():
            # The xform has no child.
            xformHier = ufe.Hierarchy.hierarchy(xformItem)
            self.assertIsNotNone(xformHier)
            xformChildren = xformHier.children()
            self.assertEqual(len(xformChildren), 0)

        def checkCapsuleMatrix(capsulePath, xpos):
            # Confirm that the capsule has moved in world space.  Must
            # re-create the scene item after path change.
            capsulePath = ufe.Path(
                [proxyShapePathSegment,
                    usdUtils.createUfePathSegment(capsulePath)])
            capsuleItem = ufe.Hierarchy.createItem(capsulePath)
            self.assertIsNotNone(capsuleItem)
            capsuleT3d = ufe.Transform3d.transform3d(capsuleItem)
            self.assertIsNotNone(capsuleT3d)
            capsuleWorld = capsuleT3d.inclusiveMatrix()
            self.assertEqual(xpos, matrixToList(capsuleWorld)[13])
            assertVectorAlmostEqual(
                self, capsuleWorldPre[0:13], matrixToList(capsuleWorld)[0:13])
            assertVectorAlmostEqual(
                self, capsuleWorldPre[14:16], matrixToList(capsuleWorld)[14:16])

        # The xform currently has no children, capsule has not moved.
        checkParentNotDone()
        checkCapsuleMatrix('/Scope1/Capsule1', 15.0)

        # Parent the scope to the xform, the capsule moved due to relative mode
        cmds.parent(ufe.PathString.string(scopePath),
                    ufe.PathString.string(xformPath), relative=True)

        checkParentDone()
        checkCapsuleMatrix('/Xform1/Scope1/Capsule1', 10.0)

        # Undo: the xform no longer has a child, the capsule moved back due to relative mode.
        cmds.undo()

        checkParentNotDone()
        checkCapsuleMatrix('/Scope1/Capsule1', 15.0)

        # Redo: the capsule moved due to relative mode.
        cmds.redo()

        checkParentDone()
        checkCapsuleMatrix('/Xform1/Scope1/Capsule1', 10.0)

if __name__ == '__main__':
    unittest.main(verbosity=2)
