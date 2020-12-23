#!/usr/bin/env mayapy
#
# Copyright 2018 Pixar
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

from pxr import Gf
from pxr import Usd
from pxr import UsdGeom

from maya import cmds
from maya import standalone

import fixturesUtils

class testUsdExportLocator(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        filePath = os.path.join(inputPath, "UsdExportLocatorTest", "SingleLocator.ma")
        cmds.file(filePath, force=True, open=True)

        cls._singleLocatorUsdFilePathMerged = os.path.abspath(
            'SingleLocator_MERGED.usda')
        cmds.usdExport(file=cls._singleLocatorUsdFilePathMerged,
            mergeTransformAndShape=True, shadingMode="none")

        cls._singleLocatorUsdFilePathUnmerged = os.path.abspath(
            'SingleLocator_UNMERGED.usda')
        cmds.usdExport(file=cls._singleLocatorUsdFilePathUnmerged,
            mergeTransformAndShape=False, shadingMode="none")

        cls._singleLocatorUsdFilePathMergedRanged = os.path.abspath(
            'SingleLocator_MERGED_RANGED.usda')
        cmds.usdExport(file=cls._singleLocatorUsdFilePathMergedRanged,
            mergeTransformAndShape=True, shadingMode="none",
            frameRange=(1, 1))

        cls._singleLocatorUsdFilePathUnmergedRanged = os.path.abspath(
            'SingleLocator_UNMERGED_RANGED.usda')
        cmds.usdExport(file=cls._singleLocatorUsdFilePathUnmergedRanged,
            mergeTransformAndShape=False, shadingMode="none",
            frameRange=(1, 1))

        filePath = os.path.join(inputPath, "UsdExportLocatorTest", "CubeUnderLocator.ma")
        cmds.file(filePath, force=True, open=True)

        cls._locatorParentUsdFilePathMerged = os.path.abspath(
            'CubeUnderLocator_MERGED.usda')
        cmds.usdExport(file=cls._locatorParentUsdFilePathMerged,
            mergeTransformAndShape=True, shadingMode="none")

        cls._locatorParentUsdFilePathUnmerged = os.path.abspath(
            'CubeUnderLocator_UNMERGED.usda')
        cmds.usdExport(file=cls._locatorParentUsdFilePathUnmerged,
            mergeTransformAndShape=False, shadingMode="none")

        cls._locatorParentUsdFilePathMergedRanged = os.path.abspath(
            'CubeUnderLocator_MERGED_RANGED.usda')
        cmds.usdExport(file=cls._locatorParentUsdFilePathMergedRanged,
            mergeTransformAndShape=True, shadingMode="none",
            frameRange=(1, 1))

        cls._locatorParentUsdFilePathUnmergedRanged = os.path.abspath(
            'CubeUnderLocator_UNMERGED_RANGED.usda')
        cmds.usdExport(file=cls._locatorParentUsdFilePathUnmergedRanged,
            mergeTransformAndShape=False, shadingMode="none",
            frameRange=(1, 1))

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _ValidateXformOps(self, prim, expectedTranslation=None):
        xformable = UsdGeom.Xformable(prim)
        self.assertTrue(xformable)

        xformOps = xformable.GetOrderedXformOps()

        if expectedTranslation is None:
            self.assertEqual(len(xformOps), 0)
            return xformable

        self.assertEqual(len(xformOps), 1)

        translateOp = xformOps[0]

        self.assertEqual(translateOp.GetOpName(), 'xformOp:translate')
        self.assertEqual(translateOp.GetOpType(), UsdGeom.XformOp.TypeTranslate)

        self.assertTrue(Gf.IsClose(translateOp.Get(), expectedTranslation, 1e-6))

        return xformable

    def _ValidateXformPrim(self, stage, xformPrimPath,
            expectedTranslation=None):
        xformPrim = stage.GetPrimAtPath(xformPrimPath)
        self.assertTrue(xformPrim)

        xformSchema = UsdGeom.Xform(xformPrim)
        self.assertTrue(xformSchema)

        self._ValidateXformOps(xformPrim, expectedTranslation)

        return xformSchema

    def _ValidateMeshPrim(self, stage, meshPrimPath, expectedTranslation=None):
        meshPrim = stage.GetPrimAtPath(meshPrimPath)
        self.assertTrue(meshPrim)

        meshSchema = UsdGeom.Mesh(meshPrim)
        self.assertTrue(meshSchema)

        self._ValidateXformOps(meshPrim, expectedTranslation)

        return meshSchema

    def testExportSingleLocatorMerged(self):
        """
        Tests that exporting a single Maya locator works correctly when
        exporting with merge transforms and shapes enabled.
        """
        stage = Usd.Stage.Open(self._singleLocatorUsdFilePathMerged)
        self.assertTrue(stage)

        expectedTranslation = Gf.Vec3d(1.0, 2.0, 3.0)
        self._ValidateXformPrim(stage, '/SingleLocator', expectedTranslation)

    def testExportSingleLocatorUnmerged(self):
        """
        Tests that exporting a single Maya locator works correctly when
        exporting with merge transforms and shapes disabled.
        """
        stage = Usd.Stage.Open(self._singleLocatorUsdFilePathUnmerged)
        self.assertTrue(stage)

        expectedTranslation = Gf.Vec3d(1.0, 2.0, 3.0)
        self._ValidateXformPrim(stage, '/SingleLocator', expectedTranslation)

        self._ValidateXformPrim(stage, '/SingleLocator/SingleLocatorShape')

    def testExportSingleLocatorMergedRanged(self):
        """
        Tests that exporting a single Maya locator works correctly when
        exporting with merge transforms and shapes enabled and with a (trivial)
        frame range specified.
        """
        stage = Usd.Stage.Open(self._singleLocatorUsdFilePathMergedRanged)
        self.assertTrue(stage)

        expectedTranslation = Gf.Vec3d(1.0, 2.0, 3.0)
        self._ValidateXformPrim(stage, '/SingleLocator', expectedTranslation)

    def testExportSingleLocatorUnmergedRanged(self):
        """
        Tests that exporting a single Maya locator works correctly when
        exporting with merge transforms and shapes disabled and with a (trivial)
        frame range specified.
        """
        stage = Usd.Stage.Open(self._singleLocatorUsdFilePathUnmergedRanged)
        self.assertTrue(stage)

        expectedTranslation = Gf.Vec3d(1.0, 2.0, 3.0)
        self._ValidateXformPrim(stage, '/SingleLocator', expectedTranslation)

        self._ValidateXformPrim(stage, '/SingleLocator/SingleLocatorShape')

    def testExportLocatorParentMerged(self):
        """
        Tests that exporting a cube parented underneath a Maya locator works
        correctly when exporting with merge transforms and shapes enabled.
        """
        stage = Usd.Stage.Open(self._locatorParentUsdFilePathMerged)
        self.assertTrue(stage)

        expectedTranslation = Gf.Vec3d(1.0, 2.0, 3.0)
        self._ValidateXformPrim(stage, '/LocatorParent', expectedTranslation)

        # The locator does *not* get node collapsed because it has a cube
        # underneath it, so we should have a blank Xform for the shape.
        self._ValidateXformPrim(stage, '/LocatorParent/LocatorParentShape')

        expectedTranslation = Gf.Vec3d(-2.0, -3.0, -4.0)
        self._ValidateMeshPrim(stage, '/LocatorParent/CubeChild',
            expectedTranslation)

    def testExportLocatorParentUnmerged(self):
        """
        Tests that exporting a cube parented underneath a Maya locator works
        correctly when exporting with merge transforms and shapes disabled.
        """
        stage = Usd.Stage.Open(self._locatorParentUsdFilePathUnmerged)
        self.assertTrue(stage)

        expectedTranslation = Gf.Vec3d(1.0, 2.0, 3.0)
        self._ValidateXformPrim(stage, '/LocatorParent', expectedTranslation)

        # The locator does *not* get node collapsed because it has a cube
        # underneath it, so we should have a blank Xform for the shape.
        self._ValidateXformPrim(stage, '/LocatorParent/LocatorParentShape')

        expectedTranslation = Gf.Vec3d(-2.0, -3.0, -4.0)
        self._ValidateXformPrim(stage, '/LocatorParent/CubeChild',
            expectedTranslation)

        self._ValidateMeshPrim(stage, '/LocatorParent/CubeChild/CubeChildShape')

    def testExportLocatorParentMergedRanged(self):
        """
        Tests that exporting a cube parented underneath a Maya locator works
        correctly when exporting with merge transforms and shapes enabled and
        with a (trivial) frame range specified.
        """
        stage = Usd.Stage.Open(self._locatorParentUsdFilePathMergedRanged)
        self.assertTrue(stage)

        expectedTranslation = Gf.Vec3d(1.0, 2.0, 3.0)
        self._ValidateXformPrim(stage, '/LocatorParent', expectedTranslation)

        # The locator does *not* get node collapsed because it has a cube
        # underneath it, so we should have a blank Xform for the shape.
        self._ValidateXformPrim(stage, '/LocatorParent/LocatorParentShape')

        expectedTranslation = Gf.Vec3d(-2.0, -3.0, -4.0)
        self._ValidateMeshPrim(stage, '/LocatorParent/CubeChild',
            expectedTranslation)

    def testExportLocatorParentUnmergedRanged(self):
        """
        Tests that exporting a cube parented underneath a Maya locator works
        correctly when exporting with merge transforms and shapes disabled and
        with a (trivial) frame range specified.
        """
        stage = Usd.Stage.Open(self._locatorParentUsdFilePathUnmergedRanged)
        self.assertTrue(stage)

        expectedTranslation = Gf.Vec3d(1.0, 2.0, 3.0)
        self._ValidateXformPrim(stage, '/LocatorParent', expectedTranslation)

        # The locator does *not* get node collapsed because it has a cube
        # underneath it, so we should have a blank Xform for the shape.
        self._ValidateXformPrim(stage, '/LocatorParent/LocatorParentShape')

        expectedTranslation = Gf.Vec3d(-2.0, -3.0, -4.0)
        self._ValidateXformPrim(stage, '/LocatorParent/CubeChild',
            expectedTranslation)

        self._ValidateMeshPrim(stage, '/LocatorParent/CubeChild/CubeChildShape')


if __name__ == '__main__':
    unittest.main(verbosity=2)
