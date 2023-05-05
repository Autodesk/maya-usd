#!/usr/bin/env python

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

import fixturesUtils
import mayaUtils
import testUtils
import ufeUtils
import usdUtils

from pxr import Gf

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as om

import ufe

from functools import partial
import unittest

import os

class MaterialTestCase(unittest.TestCase):
    '''Verify the Material UFE interface.
    
    UFE Feature : Material
    Action : Query materials on SceneObjects
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

    def _StartTest(self, testName):
        cmds.file(force=True, new=True)
        self._testName = testName
        testFile = testUtils.getTestScene("material", self._testName + ".usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()

    # methods:
    #   - getMaterials
    #   - hasMaterial   (since UFE version 0.5.2 , or '5002')

    def testUsdNoMaterial(self):
        """
        Checks that an object is bound to zero materials.
        """
        self._StartTest('noMaterial')
        mayaPathSegment = mayaUtils.createUfePathSegment('|stage|stageShape')
        
        cubeUsdPathSegment = usdUtils.createUfePathSegment('/cube')
        cubePath = ufe.Path([mayaPathSegment, cubeUsdPathSegment])
        cubeItem = ufe.Hierarchy.createItem(cubePath)

        materialInterface = ufe.Material.material(cubeItem)

        materials = materialInterface.getMaterials()
        self.assertEqual(len(materials), 0)

        if(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') >= '5002'):
            hasAnyMaterial = materialInterface.hasMaterial()
            self.assertFalse(hasAnyMaterial)

    def testUsdSingleMaterial(self):
        """
        Checks that an object is bound to exactly one material.
        """
        self._StartTest('singleMaterial')
        mayaPathSegment = mayaUtils.createUfePathSegment('|stage|stageShape')
        
        cubeUsdPathSegment = usdUtils.createUfePathSegment('/cube')
        cubePath = ufe.Path([mayaPathSegment, cubeUsdPathSegment])
        cubeItem = ufe.Hierarchy.createItem(cubePath)

        materialInterface = ufe.Material.material(cubeItem)

        materials = materialInterface.getMaterials()
        self.assertEqual(len(materials), 1)

        if(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') >= '5002'):
            hasAnyMaterial = materialInterface.hasMaterial()
            self.assertTrue(hasAnyMaterial)

    def testUsdMultipleMaterials(self):
        """
        Checks that an object is bound to multiple materials.
        """
        self._StartTest('multipleMaterials')
        mayaPathSegment = mayaUtils.createUfePathSegment('|stage|stageShape')
        
        cubeUsdPathSegment = usdUtils.createUfePathSegment('/cube')
        cubePath = ufe.Path([mayaPathSegment, cubeUsdPathSegment])
        cubeItem = ufe.Hierarchy.createItem(cubePath)

        materialInterface = ufe.Material.material(cubeItem)

        materials = materialInterface.getMaterials()
        self.assertEqual(len(materials), 2)

        if(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') >= '5002'):
            hasAnyMaterial = materialInterface.hasMaterial()
            self.assertTrue(hasAnyMaterial)

if __name__ == '__main__':
    unittest.main(verbosity=2)
