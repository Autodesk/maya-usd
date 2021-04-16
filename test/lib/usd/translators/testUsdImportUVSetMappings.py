#!/usr/bin/env mayapy
#
# Copyright 2020 Pixar
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

import os
import unittest

import fixturesUtils


class testUsdImportUVSetMappings(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        input_path = fixturesUtils.setUpClass(__file__)

        cls.test_dir = os.path.join(input_path,
                                    "UsdImportUVSetMappingsTest")

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testImportUVSetMappings(self):
        '''
        Tests that importing complex UV set mappings work:
        '''
        usd_path = os.path.join(self.test_dir, "UsdImportUVSetMappings.usda")
        options = ["shadingMode=[[useRegistry,UsdPreviewSurface]]",
                   "primPath=/"]
        cmds.file(usd_path, i=True, type="USD Import",
                  ignoreVersion=True, ra=True, mergeNamespacesOnClash=False,
                  namespace="Test", pr=True, importTimeRange="combine",
                  options=";".join(options))

        # With merging working, we expect exactly these shading groups (since
        # two of them were made unmergeable)
        expected_sg = set(['initialParticleSE',
                           'initialShadingGroup',
                           'blinn1SG',
                           'blinn2SG',
                           'blinn3SG',
                           'blinn3SG_uvSet1',
                           'blinn4SG',
                           'blinn4SG_uvSet2'])

        self.assertEqual(set(cmds.ls(type="shadingEngine")), expected_sg)

        expected_links = [
            ("file1", ['pPlane4Shape.uvSet[0].uvSetName',
                       'pPlane3Shape.uvSet[0].uvSetName',
                       'pPlane1Shape.uvSet[0].uvSetName',
                       'pPlane2Shape.uvSet[0].uvSetName']),
            ("file2", ['pPlane4Shape.uvSet[1].uvSetName',
                       'pPlane3Shape.uvSet[1].uvSetName',
                       'pPlane1Shape.uvSet[1].uvSetName',
                       'pPlane2Shape.uvSet[1].uvSetName']),
            ("file3", ['pPlane4Shape.uvSet[2].uvSetName',
                       'pPlane3Shape.uvSet[2].uvSetName',
                       'pPlane1Shape.uvSet[2].uvSetName',
                       'pPlane2Shape.uvSet[2].uvSetName']),
            ("file4", ['pPlane4Shape.uvSet[0].uvSetName',
                       'pPlane2Shape.uvSet[0].uvSetName']),
            ("file5", ['pPlane2Shape.uvSet[1].uvSetName',]),
            ("file6", ['pPlane2Shape.uvSet[2].uvSetName',]),
            ("file7", ['pPlane4Shape.uvSet[1].uvSetName',]),
            ("file8", ['pPlane4Shape.uvSet[2].uvSetName',]),
        ]
        for file_name, links in expected_links:
            links = set(links)
            self.assertEqual(set(cmds.uvLink(texture=file_name)), links)


if __name__ == '__main__':
    unittest.main(verbosity=2)
