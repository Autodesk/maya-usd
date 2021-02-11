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

from pxr import Usd

from maya import cmds
from maya import standalone

import ufe

import os
import unittest


class SceneItemTestCase(unittest.TestCase):
    '''Verify the SceneItem UFE interface.
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
        ''' Called initially to set up the maya test environment '''
        self.assertTrue(self.pluginsLoaded)

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

    def testNodeTypes(self):
        '''Engine method to run scene item test.'''

        # Get a UFE scene item for one of the balls in the scene.
        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment("|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)

        # Ball35 is an Xform.
        ball35NodeType = ball35Item.nodeType()
        self.assertEqual(ball35NodeType, "Xform")

        # This node type should be the first item on the ancestor node type list
        # and it should have other items.
        ball35AncestorNodeTypes = ball35Item.ancestorNodeTypes()
        if Usd.GetVersion() > (0, 20, 2):
            self.assertEqual(ball35NodeType, ball35AncestorNodeTypes[0])
        else:
            self.assertEqual("UsdGeomXform", ball35AncestorNodeTypes[0])
            
        self.assertTrue(len(ball35AncestorNodeTypes) > 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
