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

from maya import cmds
from maya import standalone

import maya.internal.common.utils.geometry as geo_utils
import maya.internal.common.utils.componenttag as ctag_utils

import maya.internal.common.utils.geometry as geo_utils
import maya.internal.common.utils.test as test_utils
import maya.internal.common.utils.componenttag as ctag_utils

import fixturesUtils, os

import unittest

class testComponentTags(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testComponentTags(self):
        """
        Test that component tags can round-trip through USD.
        """
        geo = cmds.polySphere(sx=6,sy=6)[0]
        geo_name = geo
        geo = geo_utils.extendToShape(geo)
        ctag_utils.createTag(geo, 'top', ['f[18:23]', 'f[30:35]'])
        ctag_utils.createTag(geo, 'bottom', ['f[0:5]', 'f[24:29]'])

        usdFilePath = os.path.join(os.environ.get('MAYA_APP_DIR'),'testComponentTags.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True,
            file=usdFilePath,
            shadingMode='none')

        cmds.file(new=True, force=True)
        rootPaths = cmds.mayaUSDImport(v=True, f=usdFilePath)
        self.assertEqual(len(rootPaths), 1)

        indices = cmds.getAttr('{0}.componentTags'.format(geo_name), multiIndices=True) or []
        self.assertEqual(len(indices), 2)

    def testNoComponentTags(self):
        """
        Test that component tags are not present when the USD export option is off.
        """
        geo = cmds.polySphere(sx=6,sy=6)[0]
        geo_name = geo
        geo = geo_utils.extendToShape(geo)
        ctag_utils.createTag(geo, 'top', ['f[18:23]', 'f[30:35]'])
        ctag_utils.createTag(geo, 'bottom', ['f[0:5]', 'f[24:29]'])

        usdFilePath = os.path.join(os.environ.get('MAYA_APP_DIR'),'testComponentTags.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True,
            file=usdFilePath,
            shadingMode='none',
            exportComponentTags=False)

        cmds.file(new=True, force=True)
        rootPaths = cmds.mayaUSDImport(v=True, f=usdFilePath)
        self.assertEqual(len(rootPaths), 1)

        indices = cmds.getAttr('{0}.componentTags'.format(geo_name), multiIndices=True)
        self.assertIsNone(indices)


if __name__ == '__main__':
    unittest.main(verbosity=2)
