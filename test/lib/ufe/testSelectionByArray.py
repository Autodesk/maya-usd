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

import fixturesUtils
import mayaUtils
import usdUtils

from maya import cmds
from maya import standalone
from maya import mel

import ufe

import unittest


class SelectByArrayTestCase(unittest.TestCase):
    '''Verify UFE selection on a USD scene by passing argument in arrays.'''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()
    
    @classmethod
    def tearDownClass(cls):
        cmds.file(new=True, force=True)

        standalone.uninitialize()

    def setUp(self):
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

        # Load a file that has the same scene in both the Maya Dag
        # hierarchy and the USD hierarchy.
        mayaUtils.openTestScene("parentCmd", "simpleSceneMayaPlusUSD_TRS.ma")

        shapeSegment = mayaUtils.createUfePathSegment(
            "|mayaUsdProxy1|mayaUsdProxyShape1")
        
        def makeUsdPath(name):
            return ufe.Path([shapeSegment, usdUtils.createUfePathSegment(name)])

        ufeNames = ["/cubeXform", "/cylinderXform", "/sphereXform"]
        self.usdPaths = [makeUsdPath(name) for name in ufeNames]
        self.mayaPaths = ["pCube1", "pCylinder1", "pSphere1"]

        # Clear selection to start off
        cmds.select(clear=True)

    #@unittest.skipUnless(mayaUtils.mayaMajorVersion >= 2023, 'testSelectByArray only works with fixes available in Maya 2023.')
    def testSelectByArray(self):
        """
        Select multiple items by passing them in an array.
        Perform the test using Python and using mel.
        """
        def melSelectCmd(paths):
            cmd = '''string $obj[];'''
            for i, path in enumerate(paths):
                cmd = cmd + '$obj[%s] = "%s";' % (i, path)
            cmd = cmd + 'select $obj;'
            return cmd

        def melSelect(paths):
            mel.eval(melSelectCmd(paths))

        # Test using mel commands.

        cmds.select(clear=True)
        melSelect(self.mayaPaths)

        sn = ufe.GlobalSelection.get()
        self.assertEqual(len(sn), 3)

        print(self.usdPaths)
        cmds.select(clear=True)
        melSelect(self.usdPaths)

        sn = ufe.GlobalSelection.get()
        self.assertEqual(len(sn), 3)

        cmds.select(clear=True)
        melSelect(self.mayaPaths + self.usdPaths)

        sn = ufe.GlobalSelection.get()
        self.assertEqual(len(sn), 6)

        # Test using Python commands.

        cmds.select(clear=True)
        cmds.select(self.mayaPaths)

        sn = ufe.GlobalSelection.get()
        self.assertEqual(len(sn), 3)

        print(self.usdPaths)
        cmds.select(clear=True)
        cmds.select(self.usdPaths)

        sn = ufe.GlobalSelection.get()
        self.assertEqual(len(sn), 3)

        cmds.select(clear=True)
        cmds.select(self.mayaPaths + self.usdPaths)

        sn = ufe.GlobalSelection.get()
        self.assertEqual(len(sn), 6)


if __name__ == '__main__':
    unittest.main(verbosity=2)
