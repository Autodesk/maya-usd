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

import unittest

import maya.cmds as cmds
import maya.mel as mel
import fixturesUtils

class MayaUsdInteractiveMaterialsScopeNameTestCase(unittest.TestCase):
    """Test interactive commands that set the materials scope name."""

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__, initializeStandalone=False, loadPlugin=False)
        cmds.loadPlugin('mayaUsdPlugin')

    def testDefault(self):
        cmds.file(new=True, force=True)
        # Look for a materialsScopeName of mtl (should pass)
        result1 = mel.eval("$w = `window`; $scrollLayout = `scrollLayout`; proc testTranslator(string $s){ global int $materialsScopeResult; $materialsScopeResult=endsWith($s, \"materialsScopeName=mtl\"); } mayaUsdTranslatorExport($scrollLayout, \"query\", \"\", \"testTranslator\"); deleteUI $w; global int $materialsScopeResult; int $r=$materialsScopeResult;");
        self.assertTrue(result1)
        # Look for a materialsScopeName of shouldFail (should fail)
        result2 = mel.eval("$w = `window`; $scrollLayout = `scrollLayout`; proc testTranslator(string $s){ global int $materialsScopeResult; $materialsScopeResult=endsWith($s, \"materialsScopeName=shouldFail\"); } mayaUsdTranslatorExport($scrollLayout, \"query\", \"\", \"testTranslator\"); deleteUI $w; global int $materialsScopeResult; int $r=$materialsScopeResult;");
        self.assertFalse(result2)

    def testMaterialsScopeNameEnvVariable(self):
        cmds.file(new=True, force=True)
        # Look for a materialsScopeName of test based upon env variable (should pass)
        mel.eval("putenv(\"MAYAUSD_MATERIALS_SCOPE_NAME\", \"test\")")
        result1 = mel.eval("$w = `window`; $scrollLayout = `scrollLayout`; proc testTranslator(string $s){ global int $materialsScopeResult; $materialsScopeResult=endsWith($s, \"materialsScopeName=test\"); } mayaUsdTranslatorExport($scrollLayout, \"query\", \"\", \"testTranslator\"); deleteUI $w; global int $materialsScopeResult; int $r=$materialsScopeResult;");
        self.assertTrue(result1)

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
