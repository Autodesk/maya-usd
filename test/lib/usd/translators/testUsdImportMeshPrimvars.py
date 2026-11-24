#!/usr/bin/env mayapy
#
# Copyright 2026 Sony Interactive Entertainment
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

"""
Checks that UV and color set primvars land on the right face-vertices when a
mesh is imported, for every interpolation, indexed or not, and on right- and
left-handed meshes.
"""

import os
import unittest

from pxr import Gf, Sdf, Usd, UsdGeom, Vt

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as om

import fixturesUtils

# A quad and a triangle: mixed face sizes.
_COUNTS = [4, 3]
_INDICES = [0, 1, 2, 3, 1, 4, 2]
_POINTS = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0), (2, 0.5, 0)]
_NUM_VERTS = len(_POINTS)


def _uv(k):
    return Gf.Vec2f(0.1 * k, 0.2 * k)


def _rgb(k):
    return Gf.Vec3f(0.1 * k, 0.5, 1.0 - 0.1 * k)


def _rgba(k):
    return Gf.Vec4f(0.1 * k, 0.5, 1.0 - 0.1 * k, 0.05 * k)


def _defineMesh(stage, path, leftHanded):
    mesh = UsdGeom.Mesh.Define(stage, path)
    mesh.CreateFaceVertexCountsAttr(_COUNTS)
    mesh.CreateFaceVertexIndicesAttr(_INDICES)
    mesh.CreatePointsAttr([Gf.Vec3f(*p) for p in _POINTS])
    mesh.CreateSubdivisionSchemeAttr(UsdGeom.Tokens.none)
    if leftHanded:
        mesh.CreateOrientationAttr(UsdGeom.Tokens.leftHanded)

    tokens = UsdGeom.Tokens
    types = Sdf.ValueTypeNames
    primvars = UsdGeom.PrimvarsAPI(mesh)

    def add(name, typeName, interpolation, values, indices=None):
        primvar = primvars.CreatePrimvar(name, typeName, interpolation)
        primvar.Set(values)
        if indices is not None:
            primvar.SetIndices(Vt.IntArray(indices))
        return primvar

    # UV sets: all equal to _uv(vertex id) at every face-vertex.
    add('st', types.TexCoord2fArray, tokens.faceVarying,
        [_uv(v) for v in range(_NUM_VERTS)], _INDICES)
    add('stFlat', types.TexCoord2fArray, tokens.faceVarying, [_uv(v) for v in _INDICES])
    add('stVertex', types.TexCoord2fArray, tokens.vertex, [_uv(v) for v in range(_NUM_VERTS)])

    # Color sets.
    add('uniformRGB', types.Color3fArray, tokens.uniform, [_rgb(f) for f in range(len(_COUNTS))])
    add('vertexRGB', types.Color3fArray, tokens.vertex, [_rgb(v) for v in range(_NUM_VERTS)])
    add('varyingRGB', types.Color3fArray, tokens.varying, [_rgb(v) for v in range(_NUM_VERTS)])
    add('vertexAlpha', types.FloatArray, tokens.vertex, [0.1 * v for v in range(_NUM_VERTS)])
    add('faceVaryingRGBA', types.Color4fArray, tokens.faceVarying, [_rgba(v) for v in _INDICES])
    # Indexed, with value 0 unauthored: face 0 gets _rgb(1), face 1 gets nothing.
    add('unauthoredRGB', types.Color3fArray, tokens.uniform,
        [_rgb(0), _rgb(1)], [1, 0]).SetUnauthoredValuesIndex(0)
    add('displayColor', types.Color3fArray, tokens.constant, [Gf.Vec3f(0.5)])
    return mesh


class testUsdImportMeshPrimvars(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

        usdFile = os.path.abspath('UsdImportMeshPrimvars.usda')
        stage = Usd.Stage.CreateNew(usdFile)
        _defineMesh(stage, '/RightHanded', leftHanded=False)
        _defineMesh(stage, '/LeftHanded', leftHanded=True)
        # Explicitly turns color set display off.
        displayColorsOff = _defineMesh(stage, '/DisplayColorsOff', leftHanded=False)
        UsdGeom.PrimvarsAPI(displayColorsOff).CreatePrimvar(
            'displayColors', Sdf.ValueTypeNames.Bool, UsdGeom.Tokens.constant).Set(False)
        stage.GetRootLayer().Save()

        cmds.file(new=True, force=True)
        cmds.usdImport(file=usdFile, shadingMode=[['none', 'default']])

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _forEachMesh(self, check):
        """Calls check(meshFn, face, local, vertex) for every face-vertex of
        both meshes."""
        for shape in ('RightHandedShape', 'LeftHandedShape'):
            sel = om.MSelectionList()
            sel.add(shape)
            meshFn = om.MFnMesh(sel.getDagPath(0))
            for face in range(meshFn.numPolygons):
                for local, vertex in enumerate(meshFn.getPolygonVertices(face)):
                    with self.subTest(shape=shape, face=face, local=local):
                        check(meshFn, face, local, vertex)

    def _color(self, meshFn, face, local, colorSet):
        """The color of a face-vertex, or None if it has none."""
        try:
            index = meshFn.getColorIndex(face, local, colorSet)
        except RuntimeError:
            return None
        return meshFn.getColors(colorSet)[index] if index >= 0 else None

    def assertVecAlmostEqual(self, actual, expected):
        self.assertEqual(len(actual), len(expected))
        for a, e in zip(actual, expected):
            self.assertAlmostEqual(a, e, places=5)

    def testUVSets(self):
        def check(meshFn, face, local, vertex):
            for uvSet in ('st', 'stFlat', 'stVertex'):
                self.assertVecAlmostEqual(meshFn.getPolygonUV(face, local, uvSet), _uv(vertex))

        self._forEachMesh(check)

    def testColorSets(self):
        def check(meshFn, face, local, vertex):
            c = self._color(meshFn, face, local, 'uniformRGB')
            self.assertVecAlmostEqual((c.r, c.g, c.b), _rgb(face))

            c = self._color(meshFn, face, local, 'vertexRGB')
            self.assertVecAlmostEqual((c.r, c.g, c.b), _rgb(vertex))

            c = self._color(meshFn, face, local, 'varyingRGB')
            self.assertVecAlmostEqual((c.r, c.g, c.b), _rgb(vertex))

            c = self._color(meshFn, face, local, 'vertexAlpha')
            self.assertAlmostEqual(c.a, 0.1 * vertex, places=5)

            c = self._color(meshFn, face, local, 'faceVaryingRGBA')
            self.assertVecAlmostEqual((c.r, c.g, c.b, c.a), _rgba(vertex))

            c = self._color(meshFn, face, local, 'unauthoredRGB')
            if face == 0:
                self.assertVecAlmostEqual((c.r, c.g, c.b), _rgb(1))
            else:
                self.assertIsNone(c)

        self._forEachMesh(check)

    def testDisplayColorIsCurrent(self):
        for shape in ('RightHandedShape', 'LeftHandedShape'):
            # No history: the color sets were written on a mesh data copy.
            self.assertFalse(cmds.listConnections(shape + '.inMesh', s=True, d=False))
            self.assertEqual(
                cmds.polyColorSet(shape, q=True, currentColorSet=True), ['displayColor'])
            self.assertTrue(cmds.getAttr(shape + '.displayColors'))

    def testAuthoredDisplayColorsIsKept(self):
        shape = 'DisplayColorsOffShape'
        self.assertEqual(
            cmds.polyColorSet(shape, q=True, currentColorSet=True), ['displayColor'])
        self.assertFalse(cmds.getAttr(shape + '.displayColors'))


if __name__ == '__main__':
    unittest.main(verbosity=2)
