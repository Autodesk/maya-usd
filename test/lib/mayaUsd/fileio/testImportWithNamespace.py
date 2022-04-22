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

from pxr import Usd, UsdGeom

import fixturesUtils

import unittest

class testReadWriteUtils(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testImportWithNamespace(self):
        '''Importing to a Maya namespace must place imported nodes in the namespace.'''

        # Create a USD scene.
        fileName = 'importWithNamespace.usda'
        stage = Usd.Stage.CreateNew(fileName)
        xformPrim = UsdGeom.Xform.Define(stage, '/xform')
        stage.GetRootLayer().Save()

        # Import it using the Maya file command.
        namespaceName = "testNamespace"
        cmds.file(fileName, i=True, namespace=namespaceName)

        # Check that the namespace was created, and that the 'xform' Dag
        # node is in it.
        self.assertTrue(cmds.namespace(exists=namespaceName))
        self.assertEqual(len(cmds.ls(namespaceName+':xform')), 1)

if __name__ == '__main__':
    unittest.main(verbosity=2)
