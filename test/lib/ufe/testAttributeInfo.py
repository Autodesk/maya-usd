#!/usr/bin/env python

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

import fixturesUtils
import mayaUtils
import ufeUtils
import usdUtils

from maya import standalone

import ufe

import unittest


class AttributesTestCase(unittest.TestCase):
    '''Verify AttributeInfo interface.'''

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

        # Get a UFE scene item for one of the balls in the scene

        ball35Item = ufeUtils.createItem('|transform1|proxyShape1,/Room_set/Props/Ball_35')

        # Find the list of attributes

        self._attrs = ufe.Attributes.attributes(ball35Item)
        self.assertIsNotNone(self._attrs)
        self._attrNames = self._attrs.attributeNames
        self.assertTrue(len(self._attrNames) > 0)

    def testAttributeInfo(self):
        '''Test the AttributeInfo usage.'''

        # Test on a bunch of arbitrary attributes

        for attrName in self._attrNames:
            attr = self._attrs.attribute(attrName)

            # Create the corresponding AttributeInfo from the Attribute

            info = ufe.AttributeInfo(attr)

            self.assertEqual(info.runTimeId, attr.sceneItem().runTimeId())
            self.assertEqual(info.path, attr.sceneItem().path())
            self.assertEqual(info.name, attr.name)

            # Create the corresponding AttributeInfo from the Attribute info

            info = ufe.AttributeInfo(attr.sceneItem().path(), attr.name)

            self.assertEqual(info.runTimeId, attr.sceneItem().runTimeId())
            self.assertEqual(info.path, attr.sceneItem().path())
            self.assertEqual(info.name, attr.name)

            # Create the corresponding Attribute from the AttributeInfo

            attrTmp = info.attribute()

            self.assertEqual(attrTmp.sceneItem().runTimeId(), attr.sceneItem().runTimeId())
            self.assertEqual(attrTmp.sceneItem().path(), attr.sceneItem().path())
            self.assertEqual(attrTmp.name, attr.name)

if __name__ == '__main__':
    unittest.main(verbosity=2)
