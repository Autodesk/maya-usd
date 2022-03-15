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

from maya import cmds
from maya import standalone

import fixturesUtils

import os
import unittest
import tempfile

import usdUtils
import mayaUtils
import ufeUtils


class testGeomNode(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        # Usd file with geometry affected by parent transformation.
        cls.usdTestParentFilePath = os.path.join(inputPath, 'GeomNodeTest',
                                                 'GeomNodeTestParentTransform.usda')

        # Usd file with hierarchy of geometry to test importing specific root.
        cls.usdTestRootPrimFilePath = os.path.join(inputPath, 'GeomNodeTest',
                                                  'GeomNodeTestRootPrim.usda')

        # Usd file with geometry containing component tags.
        cls.usdTestComponentTagsFilePath = os.path.join(inputPath, 'GeomNodeTest',
                                                        'GeomNodeTestComponentTags.usda')

        # Maya scenes with a usdGeomNode connected to a polyunite and then a compare node.
        # The scenes also contains the geometries imported directly.
        cls.mayaSceneFilePath = os.path.join(inputPath, 'GeomNodeTest',
                                             'GeomNodeTestParentTransform.ma')

        cls.mayaRootPrimSceneFilePath = os.path.join(inputPath, 'GeomNodeTest',
                                                    'GeomNodeTestRootPrim.ma')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testCompareGeometry(self):
        '''
            Open test scene that compares directly imported geometry to geometry
            imported using usdGeomNode
        '''
        cmds.file(self.mayaSceneFilePath, open=True, force=True)
        cmds.setAttr('mayaUsdGeomNode1.filePath',
                     self.usdTestParentFilePath, type='string')
        compareValue = cmds.getAttr('compare1.outValue')

        # The compare node compares the output geometry of the usdGeomNode to the geometry
        # directly imported in the scene. The value should be 1 to show that the geometries
        # are the same.
        self.assertEqual(compareValue, 1)

    def testRootPrim(self):
        '''
            Open test scene that compares imported geometry with geometry already in scene
        '''
        cmds.file(self.mayaRootPrimSceneFilePath, open=True, force=True)
        cmds.setAttr('mayaUsdGeomNode1.filePath',
                     self.usdTestRootPrimFilePath, type='string')
        cmds.setAttr('mayaUsdGeomNode1.rootPrim',
                     '/pCylinder1/pCube1/pSphere1', type='string')
        compareValue = cmds.getAttr('compare1.outValue')

        # The compare node compares the output geometry of the usdGeomNode to the geometry
        # in the scene. The value should be 1 to show that the geometries are the same and
        # the usdGeomNode only extracted the geometry at the given root (sphere)
        self.assertEqual(compareValue, 1)

    def testComponentTags(self):
        '''
            Test that component tags are applied to mesh
        '''
        cmds.file(self.mayaSceneFilePath, open=True, force=True)
        cmds.setAttr('mayaUsdGeomNode1.filePath',
                     self.usdTestComponentTagsFilePath, type='string')

        # Get component tag history, should return: [{'key': 'comptagfaces', 'node': 'polyUnite1', 'affectCount': 3, 'fullCount': 20, 'modified': False, 'procedural': True, 'editable': False, 'category': 4, 'injectionOrder': 1, 'final': True}]
        componentTags = cmds.geometryAttrInfo('polyUnite1.output', cth=True)

        self.assertEqual(len(componentTags), 1)
        self.assertEqual(componentTags[0]['key'], 'comptagfaces')
        self.assertEqual(componentTags[0]['node'], 'polyUnite1')


if __name__ == '__main__':
    unittest.main(verbosity=2)
