#!/usr/bin/env python

#
# Copyright 2025 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License")
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import fixturesUtils
import mayaUtils
import testUtils
import ufeUtils

from maya import standalone

import ufe

from pxr import Usd, UsdShade

import os
import platform
import subprocess
import sys
import unittest

import MaterialX_1_38_to_1_39

def diff(filePath, expectedResultPath):
    if platform.system() == 'Windows':
        diff = 'fc.exe'
    else:
        diff = '/usr/bin/diff'
    cmd = [diff, filePath, expectedResultPath]

    # This will print any diffs to stdout which is a nice side-effect
    return subprocess.call(cmd) == 0

class MaterialXUpgradeTestCase(unittest.TestCase):
    '''Verify the MaterialX upgrade produces expected results. The source test
    file will exercise every branch of the upgrade code.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testPythonUpgrade(self):
        '''Test MaterialX upgrade using Python script. This script can be used
          at command line level to quickly process files.'''

        # Fetch the test file.
        testFile = testUtils.getTestScene("LookdevXUsd", "mx_updates_to_1_39.usda")
        resultsFile = testUtils.getTestScene("LookdevXUsd", "mx_updates_to_1_39_final.usda")
        outputFile = os.path.join(os.getcwd(),
                                  "mx_updates_to_1_39_upgraded_python.usda")

        # Perform the upgrade.
        stage = Usd.Stage.Open(testFile)
        for prim in [x for x in stage.Traverse() if x.IsA(UsdShade.Material)]:
            MaterialX_1_38_to_1_39.ConvertMaterialTo139(prim)        
        stage.GetRootLayer().Export(outputFile)

        # Compare with expected results:
        self.assertTrue(diff(outputFile, resultsFile))

    def testMayaUpgrade(self):
        '''Test MaterialX upgrade using MayaUSD code. This simulates a user
        opening a stage in MayaUSD and having the upgrade applied using a
        contextual operation. Please note that Python bindings for the
        MaterialHandler itself are not provided by LookdevXUfe.'''

        # Fetch the test file.
        testFile = testUtils.getTestScene("LookdevXUsd", "mx_updates_to_1_39.usda")
        resultsFile = testUtils.getTestScene("LookdevXUsd", "mx_updates_to_1_39_final.usda")
        outputFile = os.path.join(os.getcwd(),
                                  "mx_updates_to_1_39_upgraded_maya.usda")

        # Open the test file in Maya.
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)
        self.assertIsNotNone(shapeNode)
        self.assertIsNotNone(stage)

        layerMaterial = ufeUtils.createUfeSceneItem(shapeNode,
            "/mtl/layer_update_material")
        self.assertIsNotNone(layerMaterial)
        
        contextOps = ufe.ContextOps.contextOps(layerMaterial)
        self.assertIsNotNone(contextOps)
        items = contextOps.getItems([])
        self.assertIn("Upgrade Material", [item.label for item in items])

        cmd = contextOps.doOpCmd(["Upgrade Material",])
        self.assertIsNotNone(cmd)
        cmd.execute()

        # Should not require anymore upgrade:
        items = contextOps.getItems([])
        self.assertNotIn("Upgrade Material", [item.label for item in items])

        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        for matName in ["subsurface_update_material", "switch_update_material", "swizzle_update_material", "atan_update_material", "normalmap_update_material"]:
            matItem = ufeUtils.createUfeSceneItem(shapeNode, f"/mtl/{matName}")
            globalSelection.append(matItem)
        contextOps = ufe.ContextOps.contextOps(matItem)
        items = contextOps.getItems([])
        self.assertIn("Upgrade Material", [item.label for item in items])

        cmd = contextOps.doOpCmd(["Upgrade Material",])
        self.assertIsNotNone(cmd)
        cmd.execute()

        # Should not require anymore upgrade:
        items = contextOps.getItems([])
        self.assertNotIn("Upgrade Material", [item.label for item in items])

        stage.GetRootLayer().Export(outputFile)

        # Compare with expected results:
        self.assertTrue(diff(outputFile, resultsFile))

if __name__ == '__main__':
    unittest.main(verbosity=2)
