#!/usr/bin/env python

#
# Copyright 2024 Autodesk
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

from maya import standalone

import ufe
import mayaUsd.ufe
import usdUfe

from pxr import Usd

import unittest
from collections import defaultdict


class SchemasTestCase(unittest.TestCase):
    '''Test known schemas.'''

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def convertToSchemasByPlugins(self, schemas, multiApply):
        schemaByPlugins = defaultdict(dict)
        for info in schemas:
            self.assertIn('pluginName', info)
            self.assertIn('schemaType', info)
            self.assertIn('schemaTypeName', info)
            self.assertIn('isMultiApply', info)
            if multiApply == info['isMultiApply']:
                schemaByPlugins[info['pluginName']][info['schemaTypeName']] = info['schemaType']
        return schemaByPlugins


    def testKnownSchemas(self):
        '''Test the getKnownApplicableSchemas function.'''

        # Test on a bunch of arbitrary attributes
        knownSchemas = usdUfe.getKnownApplicableSchemas()

        singleSchemaByPlugins = self.convertToSchemasByPlugins(knownSchemas, False)

        self.assertIn('usdGeom', singleSchemaByPlugins)
        self.assertIn('GeomModelAPI', singleSchemaByPlugins['usdGeom'])

        self.assertIn('usdShade', singleSchemaByPlugins)
        self.assertIn('MaterialBindingAPI', singleSchemaByPlugins['usdShade'])

        self.assertIn('usdLux', singleSchemaByPlugins)
        self.assertIn('LightAPI', singleSchemaByPlugins['usdLux'])

        multiSchemaByPlugins = self.convertToSchemasByPlugins(knownSchemas, True)
        self.assertIn('usd', multiSchemaByPlugins)
        self.assertIn('CollectionAPI', multiSchemaByPlugins['usd'])


    def testApplySingleApplySchema(self):
        '''Test applying a single-apply schema to a prim.'''
        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim('/Hello')
        self.assertTrue(prim)

        knownSchemas = usdUfe.getKnownApplicableSchemas()
        singleSchemaByPlugins = self.convertToSchemasByPlugins(knownSchemas, False)

        self.assertIn('usdGeom', singleSchemaByPlugins)
        self.assertIn('GeomModelAPI', singleSchemaByPlugins['usdGeom'])

        modelSchemaType = singleSchemaByPlugins['usdGeom']['GeomModelAPI']
        self.assertTrue(modelSchemaType)

        self.assertFalse(prim.HasAPI(modelSchemaType))
        result = usdUfe.applySchemaToPrim(prim, modelSchemaType)
        self.assertTrue(result)
        self.assertTrue(prim.HasAPI(modelSchemaType))

    def testApplyMultiApplySchema(self):
        '''Test applying a multi-apply schema to a prim.'''
        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim('/Hello')
        self.assertTrue(prim)

        knownSchemas = usdUfe.getKnownApplicableSchemas()
        multiSchemaByPlugins = self.convertToSchemasByPlugins(knownSchemas, True)

        self.assertIn('usd', multiSchemaByPlugins)
        self.assertIn('CollectionAPI', multiSchemaByPlugins['usd'])

        collectionSchemaType = multiSchemaByPlugins['usd']['CollectionAPI']
        self.assertTrue(collectionSchemaType)

        instanceName = 'mine'
        self.assertFalse(prim.HasAPI(collectionSchemaType))
        self.assertFalse(prim.HasAPI(collectionSchemaType, instanceName=instanceName))
        result = usdUfe.applyMultiSchemaToPrim(prim, collectionSchemaType, instanceName)
        self.assertTrue(result)
        self.assertTrue(prim.HasAPI(collectionSchemaType))
        self.assertTrue(prim.HasAPI(collectionSchemaType, instanceName=instanceName))

    def testFindSchemaInfo(self):
        '''Test that findSchemasByTypeName works.'''
        unknown = usdUfe.findSchemasByTypeName('unknown')
        self.assertFalse(unknown)

        collInfo = usdUfe.findSchemasByTypeName('CollectionAPI')
        self.assertTrue(collInfo)

        self.assertIn('pluginName', collInfo)
        self.assertIn('schemaType', collInfo)
        self.assertIn('schemaTypeName', collInfo)
        self.assertIn('isMultiApply', collInfo)

        self.assertEqual(collInfo['pluginName'], 'usd')
        self.assertTrue(collInfo['schemaType'])
        self.assertEqual(collInfo['schemaTypeName'], 'CollectionAPI')
        self.assertEqual(collInfo['isMultiApply'], True)


if __name__ == '__main__':
    unittest.main(verbosity=2)
