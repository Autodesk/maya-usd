#!/usr/bin/env python

#
# Copyright 2023 Autodesk
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

import fixturesUtils
import mayaUtils
import testUtils

from maya import cmds
from maya import standalone

import unittest


class testMaterialCommands(unittest.TestCase):
    '''
    Verify the correctness of the data returned by the MaterialCommands. Currently the
    returned values are largely hard-coded, so this test will become more complex in the 
    future once we dynamically query the renderers for their materials.
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

    def setUp(self):
        ''' Called initially to set up the maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

    def _StartTest(self, testName=None):
        cmds.file(force=True, new=True)
        if (testName):
            self._testName = testName
            testFile = testUtils.getTestScene("material", self._testName + ".usda")
            mayaUtils.createProxyFromFile(testFile)

    def testMayaUsdGetNewMaterials(self):
        """
        Checks that the list of new materials for different renderers matches the expected values.
        Arnold is excluded here as it cannot be assumed to be installed in the testing environment.
        """

        self._StartTest()

        expectedMaterials = ['USD/USD Preview Surface|UsdPreviewSurface',
                             'MaterialX/Standard Surface|ND_standard_surface_surfaceshader',
                             'MaterialX/glTF PBR|ND_gltf_pbr_surfaceshader',
                             'MaterialX/USD Preview Surface|ND_UsdPreviewSurface_surfaceshader'
                             ]

        materials = cmds.mayaUsdGetMaterialsFromRenderers()
        self.assertTrue(set(materials).issuperset(set(expectedMaterials)))


    def testMayaUsdGetExistingMaterials(self):
        """
        Checks that the list of materials found in the stage matches the expected values.
        """
        self._StartTest('multipleMaterials')
        
        expectedMaterials = ['/mtl/UsdPreviewSurface1', '/mtl/UsdPreviewSurface2']
        
        materialsInStage = cmds.mayaUsdGetMaterialsInStage("|stage|stageShape,/cube")
        self.assertEqual(materialsInStage, expectedMaterials)


if __name__ == '__main__':
    unittest.main(globals())
