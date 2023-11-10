#!/usr/bin/env python

#
# Copyright 2023 Autodesk
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
import mayaUsd

from maya import cmds
from maya import standalone

import unittest

class DuplicateProxyShapeTestCase(unittest.TestCase):
    '''
    Verify the Maya duplicate command when duplicating a proxy shape.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        '''
        Setup the class, which will create an output folder and set
        the current directory to it so we can write files.
        '''
        fixturesUtils.setUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        '''
        Called initially to set up the Maya test environment
        '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

    def testDuplicate(self):
        '''
        Duplicate the Maya transform holding the MayaUSD proxy shape.
        Verify that the new proxy shape has the expected content.
        '''

        # Initial setup: a new stage with a cone prim
        proxyShapePathStr, usdStage = mayaUtils.createProxyAndStage()
        proxyShapeParentPathStr = cmds.listRelatives(proxyShapePathStr, parent=1)[0]
        self.assertTrue(proxyShapeParentPathStr)

        conePrimPathStr = '/cone'
        usdStage.DefinePrim(conePrimPathStr, 'Cone')

        def verifyCone(stage):
            conePrim = stage.GetPrimAtPath(conePrimPathStr)
            self.assertIsNotNone(conePrim)
            self.assertTrue(conePrim)

        verifyCone(usdStage)

        # Duplicate the Maya transform parent that is holding the proxy shape.
        cmds.select(proxyShapePathStr)
        duplicatedNodesPathStr = cmds.duplicate()

        # Extract the new proxy shape parent path from the duplicate command results.
        self.assertEqual(len(duplicatedNodesPathStr), 1)
        newProxyShapeParentPathStr = duplicatedNodesPathStr[0]
        self.assertTrue(proxyShapeParentPathStr)

        # Verify there is a stage below the new proxy parent.
        newProxyShapePathStr = proxyShapePathStr.replace(proxyShapeParentPathStr, newProxyShapeParentPathStr)
        self.assertNotEqual(newProxyShapePathStr, proxyShapePathStr)

        # Retrieve the new USD stage
        newUsdStage = mayaUsd.lib.GetPrim(newProxyShapePathStr).GetStage()
        self.assertTrue(newUsdStage)
        verifyCone(newUsdStage)

        cmds.undo()

        verifyCone(usdStage)

        cmds.redo()
        verifyCone(usdStage)
        verifyCone(newUsdStage)

    def testNoModificationDuplicate(self):
        '''
        Duplicate the Maya transform holding the MayaUSD proxy shape
        and immediately save. It used to crash. The stages need to not
        be modified before saving to trigger the crash.
        
        Verify that it no longer crashes.
        '''

        # Initial setup: a new stage with a cone prim
        proxyShapePathStr, usdStage = mayaUtils.createProxyAndStage()
        proxyShapeParentPathStr = cmds.listRelatives(proxyShapePathStr, parent=1)[0]
        self.assertTrue(proxyShapeParentPathStr)

        # Duplicate the Maya transform parent that is holding the proxy shape.
        cmds.select(proxyShapePathStr)
        duplicatedNodesPathStr = cmds.duplicate()

        # Extract the new proxy shape parent path from the duplicate command results.
        self.assertEqual(len(duplicatedNodesPathStr), 1)
        newProxyShapeParentPathStr = duplicatedNodesPathStr[0]
        self.assertTrue(proxyShapeParentPathStr)

        # Verify there is a stage below the new proxy parent.
        newProxyShapePathStr = proxyShapePathStr.replace(proxyShapeParentPathStr, newProxyShapeParentPathStr)
        self.assertNotEqual(newProxyShapePathStr, proxyShapePathStr)

        # Retrieve the new USD stage
        newUsdStage = mayaUsd.lib.GetPrim(newProxyShapePathStr).GetStage()
        self.assertTrue(newUsdStage)

        cmds.file(rename='DuplicateProxyShapeTest.ma')
        cmds.file(save=True, force=True)


if __name__ == '__main__':
    unittest.main(verbosity=2)
