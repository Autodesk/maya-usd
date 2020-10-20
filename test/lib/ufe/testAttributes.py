#!/usr/bin/env python

#
# Copyright 2019 Autodesk
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

import usdUtils, mayaUtils
import ufe
from pxr import UsdGeom

import unittest

class AttributesTestCase(unittest.TestCase):
    '''Verify the Attributes UFE interface, for multiple runtimes.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    def setUp(self):
        ''' Called initially to set up the maya test environment '''
        self.assertTrue(self.pluginsLoaded)

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

    def testAttributes(self):
        '''Engine method to run attributes test.'''

        # Get a UFE scene item for one of the balls in the scene.
        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment("|world|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)

        # Then create the attributes interface for that item.
        ball35Attrs = ufe.Attributes.attributes(ball35Item)
        self.assertIsNotNone(ball35Attrs)

        # Test that we get the same scene item back.
        self.assertEqual(ball35Item, ball35Attrs.sceneItem())

        # Verify that ball35 contains the visibility attribute.
        self.assertTrue(ball35Attrs.hasAttribute(UsdGeom.Tokens.visibility))

        # Verify the attribute type of 'visibility' which we know is a enum token.
        self.assertEqual(ball35Attrs.attributeType(UsdGeom.Tokens.visibility), ufe.Attribute.kEnumString)

        # Get all the attribute names for this item.
        ball35AttrNames = ball35Attrs.attributeNames

        # Visibility should be in this list.
        self.assertIn(UsdGeom.Tokens.visibility, ball35AttrNames)
