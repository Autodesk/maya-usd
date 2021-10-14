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
import maya.api.OpenMaya as OpenMaya
from maya import standalone

import fixturesUtils, os

import unittest


class primWriterTest(mayaUsdLib.PrimWriter):
    InitCalled = False
    WriteCalled = False
    PostExportCalled = False

    def __init__(self, *args, **kwargs):
        super(primWriterTest, self).__init__(*args, **kwargs)
        if not self.GetDagPath().isValid():
            print("Invalid DagPath '{}'\n".format(self.GetDagPath()))
            return
        primSchema = UsdGeom.Sphere.Define(self.GetUsdStage(), self.GetUsdPath())
        if (primSchema == None ):
            print("Could not define primSchema at path '{}'\n".format(self.GetUsdPath().GetText()))
            return
        usdPrim = primSchema.GetPrim()
        if (usdPrim == None):
            print("Could not get UsdPrim for primSchema at path '{}'\n".format(primSchema.GetPath().GetText()))
            return
        self._SetUsdPrim(usdPrim)
        primWriterTest.InitCalled = True

    def Write(self, usdTime):
        depNodeFn = OpenMaya.MFnDependencyNode(self.GetMayaObject())
        plg = depNodeFn.findPlug('inMesh', True)
        dn = OpenMaya.MFnDependencyNode(plg.source().node())
        plg = dn.findPlug('radius', False)
        radius = plg.asFloat()

        usdPrim = self.GetUsdPrim()
        radiusAttr = usdPrim.GetAttribute('radius')
        radiusAttr.Set(radius)
        primWriterTest.WriteCalled = True

    def PostExport(self):
        primWriterTest.PostExportCalled = True

class testReadWriteUtils(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testSimplePrimWriter(self):
        mayaUsdLib.PrimWriter.Register(primWriterTest, "mesh")

        cmds.polySphere(r = 3.5, name='apple')

        usdFilePath = os.path.join(os.environ.get('MAYA_APP_DIR'),'testPrimWriterExport.usda')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            shadingMode='none')

        self.assertTrue(primWriterTest.InitCalled)
        self.assertTrue(primWriterTest.WriteCalled)
        self.assertTrue(primWriterTest.PostExportCalled)

if __name__ == '__main__':
    unittest.main(verbosity=2)
