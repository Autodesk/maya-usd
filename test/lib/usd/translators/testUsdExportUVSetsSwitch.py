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

from pxr import Gf
from pxr import Sdf
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

class testUsdExportUVSetsSwitch(unittest.TestCase):

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportUVSetsWithShadingSwitch(self):
        """
        Test that exporting a shader with multiple file inputs with multiple
        UV sets does not crash.
        """
        input_dir = fixturesUtils.setUpClass(__file__)
        maya_file = os.path.join(input_dir, "UsdExportUVSetsSwitchTest", "UsdExportUVSwitchTest.ma")

        temp_dir = os.path.abspath('.')
        usd_file_path = os.path.join(temp_dir, "testUsdExportUVSetsSwitch.usda")

        cmds.file(maya_file, force=True, open=True)

        cmds.mayaUSDExport(
            file=usd_file_path,
            exportUVs=1,
            exportSkels="none",
            exportSkin="none",
            exportComponentTags=1,
            defaultMeshScheme="catmullClark",
            defaultUSDFormat="usda",
            rootPrimType="scope",
            defaultPrim="None",
            exportMaterials=1,
            shadingMode="useRegistry",
            convertMaterialsTo=["UsdPreviewSurface"],
            exportAssignedMaterials=1,
            exportRelativeTextures="automatic",
            exportInstances=1,
            exportVisibility=1,
            mergeTransformAndShape=1,
            includeEmptyTransforms=0)

        stage = Usd.Stage.Open(usd_file_path)
        self.assertTrue(stage)


if __name__ == '__main__':
    unittest.main(verbosity=2)
