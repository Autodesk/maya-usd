#!/usr/bin/env python

#
# Copyright 2020 Autodesk
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

import unittest
import os
from maya import cmds


class MayaUsdPythonImportTestCase(unittest.TestCase):
    """
    Verify that the mayaUsd Python module imports correctly.
    """

    def testImportModule(self):
        from mayaUsd import lib as mayaUsdLib

        # Test calling a function that depends on USD. This exercises the
        # script module loader registry function and ensures that loading the
        # mayaUsd library also loaded its dependencies (i.e. the core USD
        # libraries). We test the type name as a string to ensure that we're
        # not causing USD libraries to be loaded any other way.
        stageCache = mayaUsdLib.StageCache.Get(True, True)
        self.assertEqual(type(stageCache).__name__, 'StageCache')

    def testPixarModulesAreSafeForScripting(self):
        """Test that a script node can import the
           Pixar Python modules in a secure scripting
           context"""

        node = cmds.createNode("script")

        # This Pixar import will fail and stop the script unless it was added to the secure import paths.
        cmds.setAttr(node + ".b", "from maya import cmds\nfrom pxr import Sdr\ncmds.createNode('script')\n", type="string")
        cmds.setAttr(node + ".st", 1) # Execute on Open call
        cmds.setAttr(node + ".stp", 1) # Python script

        # Save and re-load the scene
        testDir = os.path.join(os.path.abspath('.'), 'TestMayaUsdPythonImport')
        try:
            os.makedirs(testDir)
        except OSError:
            pass
        tmpMayaFile = os.path.join(testDir, 'testPxrScriptNode.ma')
        cmds.file(rename=tmpMayaFile)
        cmds.file(save=True, type='mayaAscii')
        cmds.file(new=True, force=True)
        cmds.file(tmpMayaFile, open=True, force=True)

        # If the script was safe, we should have a second script node:
        self.assertEqual(cmds.getAttr("script2.st"), 0)
