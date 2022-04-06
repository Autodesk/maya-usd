#!/usr/bin/env mayapy
#
# Copyright 2022 Autodesk
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

import os
import unittest

from maya import cmds
from maya import standalone

import filecmp

import fixturesUtils

class testUsdExportRoot(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()
        
    def testExport_exportRoots_And_parentScope(self):
        cmds.file(new=True, force=True)

        r = cmds.sphere( name='pSphere1', polygon=1, r=10 )

        # Run this command to check exportRoots works:
        usdFile_roots = os.path.abspath('roots.usda')
        cmds.mayaUSDExport(file=usdFile_roots, exportRoots=['|pSphere1'])

        #Then run this command to check parentScope works:
        usdFile_parent = os.path.abspath('parent.usda')
        cmds.mayaUSDExport(file=usdFile_parent, parentScope='/geo_GRP')

        # Then attempt this command with both flags to see the error:
        usdFile_both = os.path.abspath('both.usda')
        cmds.mayaUSDExport(file=usdFile_both, exportRoots=['|pSphere1'], parentScope='/geo_GRP')

        self.assertTrue(filecmp.cmp(usdFile_parent, usdFile_both))

if __name__ == '__main__':
    unittest.main(verbosity=2)
