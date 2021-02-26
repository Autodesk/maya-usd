#!/usr/bin/env mayapy
#
# Copyright 2018 Pixar
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

import mayaUsd.lib as mayaUsdLib

from maya import cmds
from maya import standalone

import fixturesUtils

import unittest


class testBlockSceneModificationContext(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _AssertSceneIsModified(self, modified):
        isSceneModified = cmds.file(query=True, modified=True)
        self.assertEqual(isSceneModified, modified)

    def setUp(self):
        cmds.file(new=True, force=True)
        self._AssertSceneIsModified(False)

    def testPreserveSceneModified(self):
        """
        Tests that making scene modifications using a
        UsdMayaBlockSceneModificationContext on a scene that has already been
        modified correctly maintains the modification status after the context
        exits.
        """

        # Create a cube to dirty the scene.
        cmds.polyCube()
        self._AssertSceneIsModified(True)

        with mayaUsdLib.BlockSceneModificationContext():
            # Create a cube inside the context manager.
            cmds.polyCube()

        # The scene should still be modified.
        self._AssertSceneIsModified(True)

    def testPreserveSceneNotModified(self):
        """
        Tests that making scene modifications using a
        UsdMayaBlockSceneModificationContext on a scene that has not been
        modified correctly maintains the modification status after the context
        exits.
        """

        with mayaUsdLib.BlockSceneModificationContext():
            # Create a cube inside the context manager.
            cmds.polyCube()

        # The scene should NOT be modified.
        self._AssertSceneIsModified(False)
    

if __name__ == '__main__':
    unittest.main(verbosity=2)
