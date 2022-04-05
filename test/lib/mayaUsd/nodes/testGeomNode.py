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
import logging


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

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testImportGeometry(self):
        '''
            Test importing geometry with parent matrix
        '''
        cmds.file(new=True, f=True)
        mayaUtils.loadPlugin('mayaUsdPlugin')

        geomNode = cmds.createNode('mayaUsdGeomNode')

        cmds.setAttr('{}.filePath'.format(geomNode),
                     self.usdTestParentFilePath, type='string')

        # Check imported matrix
        mat = cmds.getAttr('{}.matrix[1]'.format(geomNode))

        self.assertEqual(mat, [1.0, 0.0, 0.0, 0.0,
                               0.0, 2.8136587142944336, 0.0, 0.0,
                               0.0, 0.0, 1.0, 0.0,
                               0.0, -0.4708861620018041, 2.1663852079004746, 1.0])

        # Check that geometry was imported
        shapeNode = cmds.createNode('mesh')
        cmds.connectAttr('{}.geometry[1]'.format(
            geomNode), '{}.inMesh'.format(shapeNode), f=True)

        bbox = cmds.getAttr('{}.boundingBox.boundingBoxSize'.format(shapeNode))

        self.assertEqual(bbox, [(1.7888545989990234, 1.7013016939163208, 2.0)])

    def testRootPrim(self):
        '''
            Test importing only a part of the usd file using the root prim
        '''
        cmds.file(new=True, f=True)
        mayaUtils.loadPlugin('mayaUsdPlugin')

        geomNode = cmds.createNode('mayaUsdGeomNode')

        cmds.setAttr('{}.filePath'.format(geomNode),
                     self.usdTestRootPrimFilePath, type='string')
        cmds.setAttr('{}.rootPrim'.format(geomNode),
                     '/pCylinder1/pCube1/pSphere1', type='string')

        # call get Attr to force compute
        cmds.getAttr('{}.matrix[0]'.format(geomNode))

        # get the array of output attributes to compare the number of imported prims.
        imported = cmds.listAttr('{}.geometry'.format(geomNode), m=True)
        numberOfImports = len(imported)

        # Expecting only a single prim imported since the root was set.
        self.assertEqual(numberOfImports, 1)

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2022, 'Component tags only available in Maya 2022 or greater.')
    def testComponentTags(self):
        '''
            Test that component tags are applied to mesh
        '''
        cmds.file(new=True, f=True)
        mayaUtils.loadPlugin('mayaUsdPlugin')

        geomNode = cmds.createNode('mayaUsdGeomNode')
        cmds.setAttr('{}.filePath'.format(geomNode),
                     self.usdTestComponentTagsFilePath, type='string')

        polyUniteNode = cmds.createNode('polyUnite')

        cmds.connectAttr('{}.matrix'.format(geomNode),
                         '{}.inputMat'.format(polyUniteNode), f=True)
        cmds.connectAttr('{}.geometry'.format(geomNode),
                         '{}.inputPoly'.format(polyUniteNode), f=True)

        # Get component tag history, should return: [{'key': 'comptagfaces', 'node': 'polyUnite1', 'affectCount': 3, 'fullCount': 20, 'modified': False, 'procedural': True, 'editable': False, 'category': 4, 'injectionOrder': 1, 'final': True}]
        componentTags = cmds.geometryAttrInfo(
            '{}.output'.format(polyUniteNode), cth=True)
        self.assertEqual(len(componentTags), 1)
        self.assertEqual(componentTags[0]['key'], 'comptagfaces')
        self.assertEqual(componentTags[0]['node'], 'polyUnite1')


if __name__ == '__main__':
    unittest.main(verbosity=2)
