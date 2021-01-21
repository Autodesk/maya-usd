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

import os
import unittest

from pxr import Usd, Sdf

from maya import cmds
from maya import standalone

import fixturesUtils

class testUsdImportSessionLayer(unittest.TestCase):

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.readOnlySetUpClass(__file__)

    def testUsdImport(self):
        """
        This test executes import from an anonymous layer as per GitHub Discussion
        https://github.com/Autodesk/maya-usd/discussions/1069
        """

        layer = Sdf.Layer.CreateAnonymous()
        stage = Usd.Stage.Open(layer)
        stage.DefinePrim('/Foo', 'Xform')
        
        cmds.mayaUSDImport(f=layer.identifier, primPath='/')

        expectedMayaNodesSet = set([
            '|Foo'])
        mayaNodesSet = set(cmds.ls('|Foo*', long=True))
        self.assertEqual(expectedMayaNodesSet, mayaNodesSet)


if __name__ == '__main__':
    unittest.main(verbosity=2)
