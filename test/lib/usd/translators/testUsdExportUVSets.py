#!/usr/bin/env mayapy
#
# Copyright 2016 Pixar
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

from pxr import Gf
from pxr import Sdf
from pxr import Tf
from pxr import Usd
from pxr import UsdGeom
from pxr import Vt

import mayaUsd.lib as mayaUsdLib

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as OM

import os
import unittest

import fixturesUtils

class testUsdExportUVSets(unittest.TestCase):

    _stage = None

    def _AssertUVPrimvar(self, primvar,
            expectedValues=None, expectedInterpolation=None,
            expectedIndices=None, expectedUnauthoredValuesIndex=None):
        self.assertTrue(primvar)

        if isinstance(expectedIndices, list):
            expectedIndices = Vt.IntArray(expectedIndices)
        if expectedUnauthoredValuesIndex is None:
            expectedUnauthoredValuesIndex = -1

        if not mayaUsdLib.WriteUtil.WriteUVAsFloat2():
            self.assertEqual(primvar.GetTypeName(), Sdf.ValueTypeNames.TexCoord2fArray)
        else: 
            self.assertEqual(primvar.GetTypeName(), Sdf.ValueTypeNames.Float2Array)

        for idx in range(len(primvar.Get())):
            self.assertEqual(primvar.Get()[idx], expectedValues[idx])
        self.assertEqual(primvar.GetIndices(), expectedIndices)
        self.assertEqual(primvar.GetUnauthoredValuesIndex(),
            expectedUnauthoredValuesIndex)
        self.assertEqual(primvar.GetInterpolation(), expectedInterpolation)

    @staticmethod
    def _GetMayaMesh(meshNodePath):
        selectionList = OM.MSelectionList()
        selectionList.add(meshNodePath)
        mObj = selectionList.getDependNode(0)
        return OM.MFnMesh(mObj)

    def setUp(self):
        # Only do this setup once. We don't use the setUpClass method
        # because we want access to self.assertEquals().
        if not testUsdExportUVSets._stage:
            asFloat2 = mayaUsdLib.WriteUtil.WriteUVAsFloat2()
            suffix = ""
            if asFloat2:
                suffix += "Float"

            inputPath = fixturesUtils.setUpClass(__file__, suffix)

            if asFloat2:
                filePath = os.path.join(inputPath, 'UsdExportUVSetsFloatTest', 'UsdExportUVSetsTest_Float.ma')
            else:
                filePath = os.path.join(inputPath, 'UsdExportUVSetsTest', 'UsdExportUVSetsTest.ma')

            cmds.file(filePath, open=True, force=True)

            # Make some live edits to the box with weird UVs for the
            # testExportUvVersusUvIndexFromIterator test.
            cmds.select("box.map[0:299]", r=True)
            cmds.polyEditUV(u=1.0, v=1.0)

            # *** Note ***
            #
            # Although the UV sets on the "SharedFacesCubeShape" are stored in
            # the Maya scene with a minimal number of UV values and UV shells, they
            # seem to be expanded when the file is opened such that we end up with
            # a UV value per face vertex rather than these smaller arrays of UV
            # values with only the indices being per face vertex. As a result, we
            # have to reassign the UVs just before we export.
            meshNodePath = 'SharedFacesCubeShape'
            meshFn = testUsdExportUVSets._GetMayaMesh(meshNodePath)

            uvSetName = 'AllFacesSharedSet'
            (uArray, vArray) = meshFn.getUVs(uvSetName)
            (numUVShells, shellIndices) = meshFn.getUvShellsIds(uvSetName)

            # The size of the UV array and shells might be incorrect,
            # in that they are not what's stored in the Maya scene,
            # and not what we expect to export. We fix this below.

            # Fix up the "all shared" UV set.
            if len(uArray) != 4 or numUVShells != 1:
                meshFn.clearUVs(uvSetName)
                meshFn.setUVs(
                    [0.0, 1.0, 1.0, 0.0],
                    [0.0, 0.0, 1.0, 1.0],
                    uvSetName)
                meshFn.assignUVs([4, 4, 4, 4, 4, 4], [0, 1, 2, 3] * 6, uvSetName)
                (numUVShells, shellIndices) = meshFn.getUvShellsIds(uvSetName)
            self.assertEqual(numUVShells, 1)

            uvSetName = 'PairedFacesSet'
            (uArray, vArray) = meshFn.getUVs(uvSetName)
            (numUVShells, shellIndices) = meshFn.getUvShellsIds(uvSetName)

            # Fix up the "paired" UV set.
            if len(uArray) != 7 or numUVShells != 2:
                meshFn.clearUVs(uvSetName)
                meshFn.setUVs(
                    [0.0, 0.5, 0.5, 0.0, 1.0, 1.0, 0.5],
                    [0.0, 0.0, 0.5, 0.5, 0.5, 1.0, 1.0],
                    uvSetName)
                meshFn.assignUVs([4, 4, 4, 4, 4, 4], [0, 1, 2, 3, 2, 4, 5, 6] * 3, uvSetName)
                (numUVShells, shellIndices) = meshFn.getUvShellsIds(uvSetName)
            self.assertEqual(numUVShells, 2)

        usdFilePath = os.path.abspath('UsdExportUVSetsTest.usda')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            shadingMode='none',
            exportColorSets=False,
            exportDisplayColor=False,
            exportUVs=True)

        testUsdExportUVSets._stage = Usd.Stage.Open(usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStageOpens(self):
        self.assertTrue(self._stage)

    def _GetCubeUsdMesh(self, cubeName):
        cubePrimPath = '/UsdExportUVSetsTest/Geom/CubeMeshes/%s' % cubeName
        cubePrim = self._stage.GetPrimAtPath(cubePrimPath)
        self.assertTrue(cubePrim)

        usdMesh = UsdGeom.Mesh(cubePrim)
        self.assertTrue(usdMesh)

        return usdMesh

    def testExportEmptyDefaultUVSet(self):
        """
        Tests that a cube mesh with an empty default UV set (named 'map1' in
        Maya) gets renamed if requested.
        """
        usdCubeMesh = self._GetCubeUsdMesh('EmptyDefaultUVSetCube')

        self.assertFalse(UsdGeom.PrimvarsAPI(usdCubeMesh).GetPrimvar('map1'))

    def testExportDefaultUVSet(self):
        """
        Tests that a cube mesh with the default values for the default UV set
        (named 'map1' in Maya) gets renamed if requested.
        """
        usdCubeMesh = self._GetCubeUsdMesh('DefaultUVSetCube')

        # These are the default UV values and indices that are exported for a
        # regular Maya polycube. If you just created a new cube and then
        # exported it to USD, these are the values and indices you would see
        # for the default UV set 'map1'.
        expectedValues = [
            Gf.Vec2f(0.375, 0),
            Gf.Vec2f(0.625, 0),
            Gf.Vec2f(0.375, 0.25),
            Gf.Vec2f(0.625, 0.25),
            Gf.Vec2f(0.375, 0.5),
            Gf.Vec2f(0.625, 0.5),
            Gf.Vec2f(0.375, 0.75),
            Gf.Vec2f(0.625, 0.75),
            Gf.Vec2f(0.375, 1),
            Gf.Vec2f(0.625, 1),
            Gf.Vec2f(0.875, 0),
            Gf.Vec2f(0.875, 0.25),
            Gf.Vec2f(0.125, 0),
            Gf.Vec2f(0.125, 0.25)
        ]
        expectedIndices = [
            0, 1, 3, 2,
            2, 3, 5, 4,
            4, 5, 7, 6,
            6, 7, 9, 8,
            1, 10, 11, 3,
            12, 0, 2, 13]

        expectedInterpolation = UsdGeom.Tokens.faceVarying

        primvar = UsdGeom.PrimvarsAPI(usdCubeMesh).GetPrimvar('st')

        self._AssertUVPrimvar(primvar,
            expectedValues=expectedValues,
            expectedInterpolation=expectedInterpolation,
            expectedIndices=expectedIndices)

    def testExportOneMissingFaceUVSet(self):
        """
        Tests that a cube mesh with values for all but one face in the default
        UV set (named 'map1' in Maya) gets renamed if requested.
        """
        usdCubeMesh = self._GetCubeUsdMesh('OneMissingFaceCube')

        expectedValues = [
            Gf.Vec2f(0, 0),
            Gf.Vec2f(0.375, 0),
            Gf.Vec2f(0.625, 0),
            Gf.Vec2f(0.375, 0.25),
            Gf.Vec2f(0.625, 0.25),
            Gf.Vec2f(0.375, 0.5),
            Gf.Vec2f(0.625, 0.5),
            Gf.Vec2f(0.375, 0.75),
            Gf.Vec2f(0.625, 0.75),
            Gf.Vec2f(0.375, 1),
            Gf.Vec2f(0.625, 1),
            Gf.Vec2f(0.875, 0),
            Gf.Vec2f(0.875, 0.25),
            Gf.Vec2f(0.125, 0),
            Gf.Vec2f(0.125, 0.25)
        ]
        expectedIndices = [
            1, 2, 4, 3,
            3, 4, 6, 5,
            0, 0, 0, 0,
            7, 8, 10, 9,
            2, 11, 12, 4,
            13, 1, 3, 14
        ]
        expectedUnauthoredValuesIndex = 0

        expectedInterpolation = UsdGeom.Tokens.faceVarying

        primvar = UsdGeom.PrimvarsAPI(usdCubeMesh).GetPrimvar('st')

        self._AssertUVPrimvar(primvar,
            expectedValues=expectedValues,
            expectedInterpolation=expectedInterpolation,
            expectedIndices=expectedIndices,
            expectedUnauthoredValuesIndex=expectedUnauthoredValuesIndex)

    def testExportOneAssignedFaceUVSet(self):
        """
        Tests that a cube mesh with values for only one face in the default
        UV set (named 'map1' in Maya) gets renamed if requested.
        """
        usdCubeMesh = self._GetCubeUsdMesh('OneAssignedFaceCube')

        expectedValues = [
            Gf.Vec2f(0, 0),
            Gf.Vec2f(0.375, 0.5),
            Gf.Vec2f(0.625, 0.5),
            Gf.Vec2f(0.375, 0.75),
            Gf.Vec2f(0.625, 0.75)
        ]
        expectedIndices = [
            0, 0, 0, 0,
            0, 0, 0, 0,
            1, 2, 4, 3,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0
        ]
        expectedUnauthoredValuesIndex = 0

        expectedInterpolation = UsdGeom.Tokens.faceVarying

        primvar = UsdGeom.PrimvarsAPI(usdCubeMesh).GetPrimvar('st')

        self._AssertUVPrimvar(primvar,
            expectedValues=expectedValues,
            expectedInterpolation=expectedInterpolation,
            expectedIndices=expectedIndices,
            expectedUnauthoredValuesIndex=expectedUnauthoredValuesIndex)

    @staticmethod
    def _GetRenamedPrimvar(mesh, name):
        """
        Uses roundtripping utils to get a renamed primvar.
        """
        for primVar in UsdGeom.PrimvarsAPI(mesh).GetPrimvars():
            if mayaUsdLib.RoundTripUtil.GetPrimVarMayaName(primVar) == name:
                return primVar

    def testExportSharedFacesUVSets(self):
        """
        Tests that UV sets on a cube mesh that use the same UV ranges for
        multiple faces get exported correctly.
        """
        usdCubeMesh = self._GetCubeUsdMesh('SharedFacesCube')

        # All six faces share the same range 0.0-1.0.
        uvSetName = 'AllFacesSharedSet'
        expectedValues = [
            Gf.Vec2f(0.0, 0.0),
            Gf.Vec2f(1.0, 0.0),
            Gf.Vec2f(1.0, 1.0),
            Gf.Vec2f(0.0, 1.0)
        ]
        expectedIndices = [0, 1, 2, 3] * 6
        expectedInterpolation = UsdGeom.Tokens.faceVarying

        primvar = testUsdExportUVSets._GetRenamedPrimvar(usdCubeMesh, uvSetName)
        self._AssertUVPrimvar(primvar,
            expectedValues=expectedValues,
            expectedInterpolation=expectedInterpolation,
            expectedIndices=expectedIndices)

        # The faces alternate between ranges 0.0-0.5 and 0.5-1.0.
        uvSetName = 'PairedFacesSet'
        expectedValues = [
            Gf.Vec2f(0.0, 0.0),
            Gf.Vec2f(0.5, 0.0),
            Gf.Vec2f(0.5, 0.5),
            Gf.Vec2f(0.0, 0.5),
            Gf.Vec2f(1.0, 0.5),
            Gf.Vec2f(1.0, 1.0),
            Gf.Vec2f(0.5, 1.0)
        ]
        expectedIndices = [
            0, 1, 2, 3,
            2, 4, 5, 6] * 3
        expectedInterpolation = UsdGeom.Tokens.faceVarying

        primvar = testUsdExportUVSets._GetRenamedPrimvar(usdCubeMesh, uvSetName)
        self._AssertUVPrimvar(primvar,
            expectedValues=expectedValues,
            expectedInterpolation=expectedInterpolation,
            expectedIndices=expectedIndices)

    def testExportUvVersusUvIndexFromIterator(self):
        """
        Tests that UVs export properly on a mesh where the UV returned by
        MItMeshFaceVertex::getUV is known to be different from that
        returned by MItMeshFaceVertex::getUVIndex and indexed into the UV array.
        (The latter should return the desired result from the outMesh and is the
        method currently used by usdMaya.)
        """
        brokenBoxMesh = UsdGeom.Mesh(self._stage.GetPrimAtPath(
                "/UsdExportUVSetsTest/Geom/BrokenUVs/box"))
        stPrimvar = UsdGeom.PrimvarsAPI(brokenBoxMesh).GetPrimvar("st").ComputeFlattened()

        self.assertEqual(stPrimvar[0], Gf.Vec2f(1.0, 2.0))

    def testExportMultipleUVSetsPreserveNames(self):
        """
        Tests that a cube mesh with multiple UV sets renames them appropriately,
        preserves them if requested, and remaps them as specified
        """
        usdFilePath = os.path.abspath('UsdExportUVSetsTest.usda')
        def reexportUVSets(preserveUVSetNames, remapUVSetsTo):
            cmds.usdExport(mergeTransformAndShape=True,
                file=usdFilePath,
                shadingMode='none',
                exportColorSets=False,
                exportDisplayColor=False,
                exportUVs=True,
                preserveUVSetNames=preserveUVSetNames,
                remapUVSetsTo=remapUVSetsTo)
            testUsdExportUVSets._stage = Usd.Stage.Open(usdFilePath)
            return self._GetCubeUsdMesh('MultipleUVSetsCube')

        # Sets should all be renamed to st1, st2, etc. by default
        usdCubeMesh = self._GetCubeUsdMesh('MultipleUVSetsCube')
        map1 = usdCubeMesh.GetPrimvar('map1')
        st = usdCubeMesh.GetPrimvar('st')
        st1 = usdCubeMesh.GetPrimvar('st1')
        foo1 = usdCubeMesh.GetPrimvar('foo1')
        self.assertFalse(map1)
        self.assertTrue(st)
        self.assertTrue(st1)
        self.assertFalse(foo1)

        # Sets should not be renamed at all when preserveUVSetNames is True
        usdCubeMesh = reexportUVSets(
            preserveUVSetNames=True,
            remapUVSetsTo=[])
        map1 = usdCubeMesh.GetPrimvar('map1')
        st = usdCubeMesh.GetPrimvar('st')
        st1 = usdCubeMesh.GetPrimvar('st1')
        foo1 = usdCubeMesh.GetPrimvar('foo1')
        self.assertTrue(map1)
        self.assertFalse(st)
        self.assertFalse(st1)
        self.assertTrue(foo1)

        # Sets should be renamed only if specified under remapUVSetsTo
        # when preserveUVSetNames is True
        usdCubeMesh = reexportUVSets(
            preserveUVSetNames=True,
            remapUVSetsTo=[('map1', 'test1')])
        map1 = usdCubeMesh.GetPrimvar('map1')
        st = usdCubeMesh.GetPrimvar('st')
        st1 = usdCubeMesh.GetPrimvar('st1')
        foo1 = usdCubeMesh.GetPrimvar('foo1')
        test1 = usdCubeMesh.GetPrimvar('test1')
        self.assertFalse(map1)
        self.assertFalse(st)
        self.assertFalse(st1)
        self.assertTrue(foo1)
        self.assertTrue(test1)

        # Sets should be renamed if specified under remapUVSetsTo
        # but otherwise renamed to st1, st2, etc. by default
        usdCubeMesh = reexportUVSets(
            preserveUVSetNames=False,
            remapUVSetsTo=[('map1', 'test1')])
        map1 = usdCubeMesh.GetPrimvar('map1')
        st = usdCubeMesh.GetPrimvar('st')
        st1 = usdCubeMesh.GetPrimvar('st1')
        foo1 = usdCubeMesh.GetPrimvar('foo1')
        test1 = usdCubeMesh.GetPrimvar('test1')
        self.assertFalse(map1)
        self.assertFalse(st)
        self.assertTrue(st1)
        self.assertFalse(foo1)
        self.assertTrue(test1)

        # Sets should be renamed if specified under remapUVSetsTo
        # but otherwise renamed to st1, st2, etc. by default
        usdCubeMesh = reexportUVSets(
            preserveUVSetNames=False,
            remapUVSetsTo=[('map1', 'test1'), ('foo1', 'test2')])
        map1 = usdCubeMesh.GetPrimvar('map1')
        st = usdCubeMesh.GetPrimvar('st')
        st1 = usdCubeMesh.GetPrimvar('st1')
        foo1 = usdCubeMesh.GetPrimvar('foo1')
        test1 = usdCubeMesh.GetPrimvar('test1')
        test2 = usdCubeMesh.GetPrimvar('test2')
        self.assertFalse(map1)
        self.assertFalse(st)
        self.assertFalse(st1)
        self.assertFalse(foo1)
        self.assertTrue(test1)
        self.assertTrue(test2)

if __name__ == '__main__':
    unittest.main(verbosity=2)
