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
import usdUtils

import unittest

import mayaUsd.lib

import maya.cmds as cmds
from maya import standalone

class MayaUsdSchemaCommandTestCase(unittest.TestCase):
    """ Test the MayaUsd Schema command. """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()
        

    def testKnownSingleApplySchemasCommand(self):
        '''Test the -singleApplicationSchemas flag'''

        schemas = cmds.mayaUsdSchema(singleApplicationSchemas=True)
        self.assertIn('LightAPI', schemas)
        self.assertIn('MaterialBindingAPI', schemas)
        self.assertIn('GeomModelAPI', schemas)
        self.assertNotIn('CollectionAPI', schemas)

        schemas = cmds.mayaUsdSchema(sas=True)
        self.assertIn('LightAPI', schemas)
        self.assertIn('MaterialBindingAPI', schemas)
        self.assertIn('GeomModelAPI', schemas)
        self.assertNotIn('CollectionAPI', schemas)


    def testKnownMultiApplySchemasCommand(self):
        '''Test the -multiApplicationSchemas flag'''

        schemas = cmds.mayaUsdSchema(multiApplicationSchemas=True)
        self.assertNotIn('LightAPI', schemas)
        self.assertNotIn('MaterialBindingAPI', schemas)
        self.assertNotIn('GeomModelAPI', schemas)
        self.assertIn('CollectionAPI', schemas)

        schemas = cmds.mayaUsdSchema(mas=True)
        self.assertNotIn('LightAPI', schemas)
        self.assertNotIn('MaterialBindingAPI', schemas)
        self.assertNotIn('GeomModelAPI', schemas)
        self.assertIn('CollectionAPI', schemas)


    def testApplySchemaCommand(self):
        '''Test the -ufe, -schema and -appliedSchemas flags for a single-apply schema'''

        # Create a stage and a prim.
        psUfePathStr, psUfePath, _ = usdUtils.createSimpleStage()
        stage = mayaUsd.lib.GetPrim(psUfePathStr).GetStage()

        primUsdPath ='/Hello'
        primUfePath = psUfePathStr + ',' + primUsdPath
        prim = stage.DefinePrim(primUsdPath)
        self.assertTrue(prim)

        # Verify the prim has no schema when created.
        applied = cmds.mayaUsdSchema(primUfePath=primUfePath, appliedSchemas=True)
        self.assertFalse(applied)

        # Add a schema and verify it has been applied.
        cmds.mayaUsdSchema(primUfePath=primUfePath, schema='MaterialBindingAPI')
        applied = cmds.mayaUsdSchema(primUfePath=primUfePath, appliedSchemas=True)
        self.assertTrue(applied)
        self.assertEqual(len(applied), 1)
        self.assertIn('MaterialBindingAPI', applied)

        # Verify undo removes the schema.
        cmds.undo()
        applied = cmds.mayaUsdSchema(primUfePath=primUfePath, appliedSchemas=True)
        self.assertFalse(applied)

        # Verify redo adds the schema.
        cmds.redo()
        applied = cmds.mayaUsdSchema(primUfePath=primUfePath, appliedSchemas=True)
        self.assertTrue(applied)
        self.assertEqual(len(applied), 1)
        self.assertIn('MaterialBindingAPI', applied)


    @unittest.skipIf(int(cmds.about(apiVersion=True)) < 20230000, 'Command flag parsing of multi-use flags called from Python is buggy in Maya 2022 and earlier.')
    def testApplySchemaToMultiplePrimsCommand(self):
        '''
        Test the -ufe, -schema and -appliedSchemas flags for a single-apply schema
        on multiple prims at once.
        '''

        # Create a stage and a multiple prims.
        psUfePathStr, psUfePath, _ = usdUtils.createSimpleStage()
        stage = mayaUsd.lib.GetPrim(psUfePathStr).GetStage()

        primUfePaths = []
        for primUsdPath in ['/Hello', '/Bye']:
            prim = stage.DefinePrim(primUsdPath)
            self.assertTrue(prim)
            primUfePaths.append(psUfePathStr + ',' + primUsdPath)

        # Verify the prims have no schema when created.
        applied = cmds.mayaUsdSchema(primUfePath=primUfePaths, appliedSchemas=True)
        self.assertFalse(applied)

        # Add a schema to both prims and verify it has been applied.
        cmds.mayaUsdSchema(primUfePath=primUfePaths, schema='MaterialBindingAPI')
        applied = cmds.mayaUsdSchema(primUfePath=primUfePaths, appliedSchemas=True)
        self.assertTrue(applied)
        self.assertEqual(len(applied), 1)
        self.assertIn('MaterialBindingAPI', applied)

        # Verify undo removes the schema.
        cmds.undo()
        applied = cmds.mayaUsdSchema(primUfePath=primUfePaths, appliedSchemas=True)
        self.assertFalse(applied)

        # Verify redo adds the schema.
        cmds.redo()
        applied = cmds.mayaUsdSchema(primUfePath=primUfePaths, appliedSchemas=True)
        self.assertTrue(applied)
        self.assertEqual(len(applied), 1)
        self.assertIn('MaterialBindingAPI', applied)


    def testApplyMultiSchemaCommand(self):
        '''Test the -ufe, -schema, -instanceName and -appliedSchemas flags for a multi-apply schema'''

        # Create a stage and a prim.
        psUfePathStr, psUfePath, _ = usdUtils.createSimpleStage()
        stage = mayaUsd.lib.GetPrim(psUfePathStr).GetStage()

        primUsdPath ='/Hello'
        primUfePath = psUfePathStr + ',' + primUsdPath
        prim = stage.DefinePrim(primUsdPath)
        self.assertTrue(prim)

        # Verify the prim has no schema when created.
        applied = cmds.mayaUsdSchema(primUfePath=primUfePath, appliedSchemas=True)
        self.assertFalse(applied)

        # Incorrectly add a multi-apply schema and verify it has not been applied.
        self.assertRaises(RuntimeError, lambda: cmds.mayaUsdSchema(primUfePath=primUfePath, schema='CollectionAPI'))
        applied = cmds.mayaUsdSchema(primUfePath=primUfePath, appliedSchemas=True)
        self.assertFalse(applied)

        # Add a multi-apply schema and verify it has been applied.
        cmds.mayaUsdSchema(primUfePath=primUfePath, schema='CollectionAPI', instanceName='this')
        applied = cmds.mayaUsdSchema(primUfePath=primUfePath, appliedSchemas=True)
        self.assertTrue(applied)
        self.assertEqual(len(applied), 1)
        self.assertIn('CollectionAPI:this', applied)

        # Verify undo removes the schema.
        cmds.undo()
        applied = cmds.mayaUsdSchema(primUfePath=primUfePath, appliedSchemas=True)
        self.assertFalse(applied)

        # Verify redo adds the schema.
        cmds.redo()
        applied = cmds.mayaUsdSchema(primUfePath=primUfePath, appliedSchemas=True)
        self.assertTrue(applied)
        self.assertEqual(len(applied), 1)
        self.assertIn('CollectionAPI:this', applied)


if __name__ == '__main__':
    unittest.main(verbosity=2)
