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

# Sample translated from C++ from 
#   test/lib/usd/plugin/infoImportChaser.cpp and 
#   test/lib/usd/translators/testUsdImportChaser.py

import mayaUsd.lib as mayaUsdLib

from pxr import Gf
from pxr import Sdf
from pxr import Tf
from pxr import Vt
from pxr import UsdGeom
from pxr import Usd

from maya import cmds
import maya.api.OpenMaya as OpenMaya
from maya import standalone

import fixturesUtils, os

import unittest

class importChaserTest(mayaUsdLib.ImportChaser):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.undoRecord = OpenMaya.MDGModifier()
        self.editsRecord = []

    def PostImport(self, returnPredicate, stage, dagPaths, sdfPaths, jobArgs):
        print("importChaserTest.PostImport called")
        sdfPathsStr = "SdfPaths imported: "
        for sdfPath in sdfPaths:
            sdfPathsStr += str(sdfPath) + "\n"

        stageTraverseStr = "Stage traversal: "
        for prim in stage.TraverseAll():
            stageTraverseStr += prim.GetName() + "\n"

        customLayerData = stage.GetRootLayer().customLayerData
        customLayerDataStr = "Custom layer data: "

        for data in customLayerData:
            customLayerDataStr += data + customLayerData[data] + "\n"

        print("Info from import:\n" + sdfPathsStr + stageTraverseStr + customLayerDataStr)

        # NOTE: (yliangsiew) Just for the sake of having something that we can actually run
        # unit tests against, we'll add a custom attribute to the root DAG paths imported
        # so that we can verify that the import chaser is working, since we can't easily
        # parse Maya Script Editor output.
        for curDagPath in dagPaths:
            defaultStr = OpenMaya.MFnStringData().create(customLayerDataStr)
            strAttr = OpenMaya.MFnTypedAttribute().create("customData", "customData", OpenMaya.MFnData.kString, defaultStr)
            fnDagNode = OpenMaya.MFnDagNode(curDagPath)
            fnDagNode.addAttribute(strAttr)

            self.editsRecord.append([fnDagNode,strAttr])

        return True

    def Redo(self):
        self.undoRecord.undoIt(); # NOTE: (yliangsiew) Undo the undo to re-do.
        return True

    def Undo(self):
        for edit in self.editsRecord:
            # TODO: (yliangsiew) This seems like a bit of a code smell...why would this crash
            # otherwise? But for now, this guards an undo-redo chain crash where the MObject is no
            # longer valid between invocations. Need to look at this further.
            if not edit[0].isValid() or not edit[1].isValid():
                continue
            self.undoRecord.removeAttribute(edit[0], edit[1])
        self.undoRecord.doIt()
        return True


class testImportChaser(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        self.stagePath = os.path.join(os.environ.get('MAYA_APP_DIR'), "importChaser.usda")
        stage = Usd.Stage.CreateNew(self.stagePath)
        UsdGeom.Xform.Define(stage, '/a')
        UsdGeom.Sphere.Define(stage, '/a/sphere')
        xformPrim = stage.GetPrimAtPath('/a')

        spherePrim = stage.GetPrimAtPath('/a/sphere')
        radiusAttr = spherePrim.GetAttribute('radius')
        radiusAttr.Set(2)
        extentAttr = spherePrim.GetAttribute('extent')
        extentAttr.Set(extentAttr.Get() * 2)
        stage.GetRootLayer().defaultPrim = 'a'
        stage.GetRootLayer().customLayerData = {"customKeyA" : "customValueA",
                                                 "customKeyB" : "customValueB"}
        stage.GetRootLayer().Save()

    def testSimpleImportChaser(self):
        mayaUsdLib.ImportChaser.Register(importChaserTest, "info")
        rootPaths = cmds.mayaUSDImport(v=True, f=self.stagePath, chaser=['info'])
        self.assertEqual(len(rootPaths), 1)
        sl = OpenMaya.MSelectionList()
        sl.add(rootPaths[0])
        root = sl.getDependNode(0)
        fnNode = OpenMaya.MFnDependencyNode(root)
        self.assertTrue(fnNode.hasAttribute("customData"))
        plgCustomData = fnNode.findPlug("customData", True)
        customDataStr = plgCustomData.asString()
        self.assertEqual(customDataStr, "Custom layer data: customKeyAcustomValueA\ncustomKeyBcustomValueB\n")


if __name__ == '__main__':
    unittest.main(verbosity=2)
