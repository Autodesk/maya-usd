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

from pxr import UsdGeom

import mayaUsd.lib as mayaUsdLib

from maya import cmds
from maya import standalone

import os
import unittest

import fixturesUtils

class testUsdImportMesh(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.readOnlySetUpClass(__file__)

        usdFile = os.path.join(inputPath, "UsdImportMeshTest", "Mesh.usda")
        cmds.usdImport(file=usdFile, shadingMode=[['none', 'default'], ])

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def verifyPolyMeshCommonAttributes(self, mesh):
        self.assertTrue(cmds.objExists(mesh))

        schema = mayaUsdLib.Adaptor(mesh).GetSchema(UsdGeom.Mesh)
        subdivisionScheme = schema.GetAttribute(
                UsdGeom.Tokens.subdivisionScheme).Get()
        self.assertEqual(subdivisionScheme, UsdGeom.Tokens.none)

        faceVaryingLinearInterpolation = schema.GetAttribute(
                UsdGeom.Tokens.faceVaryingLinearInterpolation).Get()
        self.assertIsNone(faceVaryingLinearInterpolation) # not authored

        interpolateBoundary = schema.GetAttribute(
                UsdGeom.Tokens.interpolateBoundary).Get()
        self.assertEqual(interpolateBoundary, UsdGeom.Tokens.none)

        self.assertTrue(
                cmds.attributeQuery("USD_EmitNormals", node=mesh, exists=True))

    def testImportPoly(self):
        self.verifyPolyMeshCommonAttributes('PolyMeshShape')

    def testImportIndexedNormals(self):
        self.verifyPolyMeshCommonAttributes('IndexedNormalsMeshShape')

    def testImportLeftHandedPoly(self):
        self.verifyPolyMeshCommonAttributes('LeftHandedPolyMeshShape')

    def verifySubdivCommonAttributes(self, mesh):
        self.assertTrue(cmds.objExists(mesh))

        schema = mayaUsdLib.Adaptor(mesh).GetSchema(UsdGeom.Mesh)
        subdivisionScheme = \
                schema.GetAttribute(UsdGeom.Tokens.subdivisionScheme).Get()
        self.assertIsNone(subdivisionScheme)

        faceVaryingLinearInterpolation = schema.GetAttribute(
                UsdGeom.Tokens.faceVaryingLinearInterpolation).Get()
        self.assertEqual(faceVaryingLinearInterpolation, UsdGeom.Tokens.all)

        interpolateBoundary = schema.GetAttribute(
                UsdGeom.Tokens.interpolateBoundary).Get()
        self.assertEqual(interpolateBoundary, UsdGeom.Tokens.edgeOnly)

        self.assertFalse(
                cmds.attributeQuery("USD_EmitNormals", node=mesh, exists=True))

    def testImportSubdiv(self):
        self.verifySubdivCommonAttributes('SubdivMeshShape')

    def testImportLeftHandedSubdiv(self):
        self.verifySubdivCommonAttributes('LeftHandedSubdivMeshShape')

if __name__ == '__main__':
    unittest.main(verbosity=2)
