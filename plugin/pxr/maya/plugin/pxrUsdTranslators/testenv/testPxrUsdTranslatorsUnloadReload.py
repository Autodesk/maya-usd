#!/pxrpythonsubst
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

import os
import unittest

from maya import cmds
from maya import standalone


class testPxrUsdTranslators(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testUnloadReload(self):
        self.assertEqual(cmds.loadPlugin('pxrUsd'), ['pxrUsd'])

        # Exporting a file with a mesh should trigger pxrUsdTranslators to load.
        file1 = os.path.abspath('test1.usda')
        cmds.polyCube()
        cmds.usdExport(file=file1)
        self.assertTrue(
                cmds.pluginInfo('pxrUsdTranslators', q=True, loaded=True))

        # Force unload the plugin.
        cmds.unloadPlugin('pxrUsdTranslators')
        self.assertFalse(
                cmds.pluginInfo('pxrUsdTranslators', q=True, loaded=True))

        # Re-exporting the file should trigger the plugin to load again.
        # Hopefully it doesn't crash.
        file2 = os.path.abspath('test2.usda')
        cmds.usdExport(file=file2)
        self.assertTrue(
                cmds.pluginInfo('pxrUsdTranslators', q=True, loaded=True))

if __name__ == '__main__':
    unittest.main(verbosity=2)
