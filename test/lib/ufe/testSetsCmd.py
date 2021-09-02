#!/usr/bin/env python

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

import fixturesUtils
import mayaUtils
import testUtils
import ufeUtils
import usdUtils

import mayaUsd.ufe

from pxr import Kind, Usd, UsdGeom, Vt

from maya import cmds
from maya import standalone

import ufe

import os
import unittest


class SetsCmdTestCase(unittest.TestCase):
    '''Verify the Maya sets command
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        ''' Called initially to set up the Maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

        # Clear selection to start off
        cmds.select(clear=True)

    def testSetsCmd(self):
        usdaFile = testUtils.getTestScene("setsCmd", "5prims.usda")
        proxyDagPath, sphereStage = mayaUtils.createProxyFromFile(usdaFile)
        usdCube = proxyDagPath + ",/Cube1"
        usdCylinder = proxyDagPath + ",/Cylinder1"
        usdXform = proxyDagPath + ",/Xform1"

        cmds.select(usdCube)
        set1 = cmds.sets()
        cmds.select(usdCylinder)
        set2 = cmds.sets()

        # set contents
        self.assertEqual(usdCube, *cmds.sets( set1, q=True ))

        # select set
        cmds.select( set1 )
        self.assertEqual(usdCube, *cmds.ls( selection=True, ufe=True ))
        
        # set of sets
        setOfSets = cmds.sets( set1, set2, n="setOfSets")
        cmds.select( setOfSets )
        self.assertEqual([usdCube, usdCylinder], sorted(cmds.ls( selection=True, ufe=True)))

        # union of the sets
        self.assertEqual([usdCube, usdCylinder], sorted(cmds.sets( set2, un=set1 )))
        cmds.sets( set2, ii=set1 ) # do the sets have common members

        # query set contents, remove from set, add to set
        self.assertTrue(cmds.sets( usdCube, im=set1 )) # Test if Cube1 is in set1
        self.assertFalse(cmds.sets( usdCube, im=set2 )) # Test if Cube1 is in set2
        cmds.sets( usdCube, rm=set1 ) # Remove Cube1 from set1
        self.assertFalse(cmds.sets( usdCube, im=set1 )) # Test if Cube1 is in set1
        cmds.sets( usdCube, add=set1 ) # Add if Cube1 to set1
        self.assertTrue(cmds.sets( usdCube, im=set1 )) # Test if Cube1 is in set1

        #reparent Cube1 under Xform1
        cmds.parent(usdCube, usdXform, relative=True)
        usdCube = usdXform + "/Cube1"
        self.assertTrue(cmds.sets( usdCube, im=set1 )) # Test if Cube1 is in set1
        cmds.parent(usdCube, proxyDagPath, relative=True)
        usdCube = proxyDagPath + ",/Cube1"
        self.assertTrue(cmds.sets( usdCube, im=set1 )) # Test if Cube1 is in set1

        #reparent the proxy shape
        locatorShape = cmds.createNode("locator")
        locator = "|" + cmds.listRelatives(locatorShape, parent=True)[0]
        cmds.parent("|stage", locator, relative=True)
        usdCube = locator + usdCube
        self.assertTrue(cmds.sets( usdCube, im=set1 )) # Test if Cube1 is in set1

if __name__ == '__main__':
    unittest.main(verbosity=2)
