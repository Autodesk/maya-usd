#!/usr/bin/env mayapy
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

from pxr import Usd
from pxr import UsdShade

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as OM

import mayaUsd.lib as mayaUsdLib

import os
import unittest

import fixturesUtils


class testUsdExportUVSetMappings(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        mayaFile = os.path.join(inputPath, 'UsdExportUVSetMappingsTest',
            'UsdExportUVSetMappingsTest.ma')
        cmds.file(mayaFile, force=True, open=True)

        # Export to USD.
        cls._usdFilePath = os.path.abspath('UsdExportUVSetMappingsTest.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=cls._usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo='UsdPreviewSurface',
            materialsScopeName='Materials')

        cls._stage = Usd.Stage.Open(cls._usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStageOpens(self):
        '''
        Tests that the USD stage was opened successfully.
        '''
        self.assertTrue(self._stage)

    def testExportUVSetMappings(self):
        '''
        Tests that exporting multiple Maya planes with varying UV mappings
        setups results in USD data with material specializations:
        '''
        expected = [
            ("/pPlane1", "/blinn1SG", "st", "st", "st"),
            ("/pPlane2", "/blinn1SG", "st", "st", "st"),
            ("/pPlane3", "/blinn1SG", "st", "st", "st"),
            ("/pPlane4", "/blinn1SG", "st", "st", "st"),
            ("/pPlane5", "/blinn1SG_st_st1_st2", "st", "st1", "st2"),
            ("/pPlane6", "/blinn1SG_st1_st2_st", "st1", "st2", "st"),
            ("/pPlane7", "/blinn1SG_st2_st_st1", "st2", "st", "st1"),
        ]

        for mesh_name, mat_name, f1_name, f2_name, f3_name in expected:
            plane_prim = self._stage.GetPrimAtPath(mesh_name)
            binding_api = UsdShade.MaterialBindingAPI(plane_prim)
            mat = binding_api.ComputeBoundMaterial()[0]
            self.assertEqual(mat.GetPath(), mat_name)

            self.assertEqual(mat.GetInput("file1:varname").GetAttr().Get(), f1_name)
            self.assertEqual(mat.GetInput("file2:varname").GetAttr().Get(), f2_name)
            self.assertEqual(mat.GetInput("file3:varname").GetAttr().Get(), f3_name)

        # Initial code had a bug where a material with no UV mappings would
        # specialize itself. Make sure it stays fixed:
        plane_prim = self._stage.GetPrimAtPath("/pPlane8")
        binding_api = UsdShade.MaterialBindingAPI(plane_prim)
        mat = binding_api.ComputeBoundMaterial()[0]
        self.assertEqual(mat.GetPath(), "/pPlane8/Materials/blinn2SG")
        self.assertFalse(mat.GetPrim().HasAuthoredSpecializes())

        # Gather some original information:
        expected_uvs = []
        for i in range(1, 9):
            xform_name = "|pPlane%i" % i
            selectionList = OM.MSelectionList()
            selectionList.add(xform_name)
            dagPath = selectionList.getDagPath(0)
            dagPath = dagPath.extendToShape()
            mayaMesh = OM.MFnMesh(dagPath.node())
            expected_uvs.append(("pPlane%iShape" % i, mayaMesh.getUVSetNames()))

        expected_sg = set(cmds.ls(type="shadingEngine"))

        expected_links = []
        for file_name in cmds.ls(type="file"):
            links = []
            for link in cmds.uvLink(texture=file_name):
                # The name of the geometry does not survive roundtripping, but
                # we know the pattern: pPlaneShapeX -> pPlaneXShape
                plugPath = link.split(".")
                selectionList = OM.MSelectionList()
                selectionList.add(plugPath[0])
                mayaMesh = OM.MFnMesh(selectionList.getDependNode(0))
                meshName = mayaMesh.name()
                plugPath[0] = "pPlane" + meshName[-1] + "Shape"
                links.append(".".join(plugPath))
            expected_links.append((file_name, set(links)))

        # Test roundtripping:
        cmds.file(newFile=True, force=True)

        # Import back:
        options = ["shadingMode=[[useRegistry,UsdPreviewSurface]]",
                   "primPath=/"]
        cmds.file(self._usdFilePath, i=True, type="USD Import",
                  ignoreVersion=True, ra=True, mergeNamespacesOnClash=False,
                  namespace="Test", pr=True, importTimeRange="combine",
                  options=";".join(options))

        # Names should have been restored:
        for mesh_name, mesh_uvs in expected_uvs:
            selectionList = OM.MSelectionList()
            selectionList.add(mesh_name)
            mayaMesh = OM.MFnMesh(selectionList.getDependNode(0))
            self.assertEqual(mayaMesh.getUVSetNames(), mesh_uvs)

        # Same list of shading engines:
        self.assertEqual(set(cmds.ls(type="shadingEngine")), expected_sg)

        # All links correctly restored:
        for file_name, links in expected_links:
            self.assertEqual(set(cmds.uvLink(texture=file_name)), links)

if __name__ == '__main__':
    unittest.main(verbosity=2)
