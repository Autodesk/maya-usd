#!/usr/bin/env mayapy
#
# Copyright 2021 Autodesk
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

import mayaUsd.lib as mayaUsdLib

from pxr import UsdGeom

from maya import cmds
from maya.api import OpenMaya
from maya import standalone

import fixturesUtils, os

import unittest

class primReaderTest(mayaUsdLib.PrimReader):
    def Read(self, context):
        usdPrim = self._GetArgs().GetUsdPrim()
        sphere = UsdGeom.Sphere(usdPrim)
        cmds.polySphere(r=sphere.GetRadiusAttr().Get(), name=usdPrim.GetName())
        MayaNode = context.GetMayaNode(usdPrim.GetPath().GetParentPath(), True)
        return True

class testPrimReader(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.readOnlySetUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testSimplePrimReader(self):
        mayaUsdLib.PrimReader.Register(primReaderTest, "UsdGeomSphere")

        usdFilePath = os.path.join(self.inputPath, "testPrimReader.usda")
        cmds.usdImport(file=usdFilePath, shadingMode=[['none', 'default'], ])

        # Make sure what was created is an OpenMaya.MFn.kPolySphere
        sel_list = OpenMaya.MSelectionList()
        sel_list.add( 'sphere' )
        dagPath = sel_list.getDagPath(0)
        mesh = cmds.listRelatives(dagPath, c=True, type='mesh')[0]
        inMesh = cmds.listConnections('{}.inMesh'.format(mesh), s=True)
        sel_list = OpenMaya.MSelectionList()
        sel_list.add( inMesh[0] )
        dn = sel_list.getDependNode(0)
        self.assertTrue(dn.apiType() == OpenMaya.MFn.kPolySphere)


if __name__ == '__main__':
    unittest.main(verbosity=2)
