#!/usr/bin/env mayapy
#
# Copyright 2025 Autodesk
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

from pxr import UsdGeom
from pxr import Usd
from pxr import Sdf

from maya import cmds
import maya.api.OpenMaya as OpenMaya
from maya import standalone

import fixturesUtils, os

import unittest


class ImportChaserTouchSession(mayaUsdLib.ImportChaser):
    """This chaser modifies the session layer."""

    primInSessionCount = 0

    def PostImport(self, return_predicate: bool, stage: Usd.Stage, dag_paths, sdf_paths, job_args):
        """Reimplementation of a PostImport method from the parent class."""
        try:
            sessionLayer = stage.GetSessionLayer()
            primSpec = sessionLayer.GetPrimAtPath("/session_prim")
            if primSpec:
                ImportChaserTouchSession.primInSessionCount += 1
            else:
                Sdf.PrimSpec(sessionLayer, "session_prim", Sdf.SpecifierOver)
                primSpec = sessionLayer.GetPrimAtPath("/session_prim")
                if not primSpec:
                    raise Exception("Failed to create prim /session_prim in session layer")
        except Exception as e:
            # Exceptions inside chasers are obscured by the parent maya import usd command, so lets log it
            print(e)
            raise
        return True


class testImportChaserMultiImports(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        self.stagePath = os.path.join(self.temp_dir, "importChaserMultiImports.usda")
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

    def testImportChaserMultipleTimes(self):
        """
        Verifies that importing multiple times does not get the same session layer.
        """
        mayaUsdLib.ImportChaser.Register(ImportChaserTouchSession, "info")
        self.assertEqual(ImportChaserTouchSession.primInSessionCount, 0)

        rootPaths = cmds.mayaUSDImport(v=True, f=self.stagePath, chaser=['info'])
        self.assertEqual(ImportChaserTouchSession.primInSessionCount, 0)

        cmds.file(new=True, force=True)

        rootPaths = cmds.mayaUSDImport(v=True, f=self.stagePath, chaser=['info'])
        self.assertEqual(ImportChaserTouchSession.primInSessionCount, 0)


if __name__ == '__main__':
    unittest.main(verbosity=2)
