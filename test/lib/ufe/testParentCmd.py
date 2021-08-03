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
from testUtils import assertVectorAlmostEqual
import usdUtils

import mayaUsd.ufe

from pxr import UsdGeom, Vt, Gf

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
        stage = mayaUsd.ufe.getStage(str(shapeSegment))

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
        stage = mayaUsd.ufe.getStage(str(shapeSegment))

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

    @unittest.skipUnless(mayaUtils.previewReleaseVersion() >= 123, 'Requires Maya fixes only available in Maya Preview Release 123 or later.')
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
        stage = mayaUsd.ufe.getStage(str(shapeSegment))

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

    @unittest.skipUnless(mayaUtils.previewReleaseVersion() >= 123, 'Requires Maya fixes only available in Maya Preview Release 123 or later.')
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

    @unittest.skipUnless(mayaUtils.previewReleaseVersion() >= 123, 'Requires Maya fixes only available in Maya Preview Release 123 or later.')
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
            stage = mayaUsd.ufe.getStage(str(shapeSegment))

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
            stage = mayaUsd.ufe.getStage(str(shapeSegment))

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

            # Unparent a USD node in each stage.  Unparenting Lambert node is
            # nonsensical, but demonstrates the functionality.
            cubePathStr1 = '|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1/pCube1'
            lambertPathStr2 = '|simpleSceneUSD_TRS_mayaUsdProxy1|simpleSceneUSD_TRS_mayaUsdProxyShape1,/initialShadingGroup/initialShadingGroup_lambert'

            cylinderItem1 = ufe.Hierarchy.createItem(ufe.PathString.path(
                '|mayaUsdProxy1|mayaUsdProxyShape1,/pCylinder1'))
            shadingGroupItem2 = ufe.Hierarchy.createItem(
                ufe.PathString.path('|simpleSceneUSD_TRS_mayaUsdProxy1|simpleSceneUSD_TRS_mayaUsdProxyShape1,/initialShadingGroup'))
            proxyShapeItem1 = ufe.Hierarchy.createItem(ufe.PathString.path(
                '|mayaUsdProxy1|mayaUsdProxyShape1'))
            proxyShapeItem2 = ufe.Hierarchy.createItem(ufe.PathString.path(
                '|simpleSceneUSD_TRS_mayaUsdProxy1|simpleSceneUSD_TRS_mayaUsdProxyShape1'))
            cylinder1 = ufe.Hierarchy.hierarchy(cylinderItem1)
            shadingGroup2 = ufe.Hierarchy.hierarchy(shadingGroupItem2)
            proxyShape1 = ufe.Hierarchy.hierarchy(proxyShapeItem1)
            proxyShape2 = ufe.Hierarchy.hierarchy(proxyShapeItem2)

            def checkUnparent(done):
                proxyShape1Children = proxyShape1.children()
                proxyShape2Children = proxyShape2.children()
                cylinder1Children = cylinder1.children()
                shadingGroup2Children = shadingGroup2.children()
                self.assertEqual(
                    'pCube1' in childrenNames(proxyShape1Children), done)
                self.assertEqual(
                    'pCube1' in childrenNames(cylinder1Children), not done)
                self.assertEqual(
                    'initialShadingGroup_lambert' in childrenNames(proxyShape2Children), done)
                self.assertEqual(
                    'initialShadingGroup_lambert' in childrenNames(shadingGroup2Children), not done)

            checkUnparent(done=False)

            # Use relative parenting, else trying to keep absolute world
            # position of Lambert node fails (of course).
            cmds.parent(cubePathStr1, lambertPathStr2, w=True, r=True)
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


if __name__ == '__main__':
    unittest.main(verbosity=2)
