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
import testUtils
import usdUtils

from maya import cmds
from maya import standalone

import ufe

import unittest

class SceneSegmentTestCase(unittest.TestCase):
    '''Verify the Scene Segment UFE interface, for multiple runtimes.
    
    UFE Feature : ProxyShape, stage nesting
    Maya Feature : ProxyShape
    Action : query scene segments
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
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

        # load the file and get ready to test!
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")
        testFile = testUtils.getTestScene("camera", 'TranslateRotate_vs_xform.usda')
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()

    def testProxyShapeSceneSegmentHandler(self):
        mayaPathSegment = mayaUtils.createUfePathSegment('|stage|stageShape')

        # Test that the camera handlers can find USD cameras in a scene segment
        proxyShapeParentSegment = mayaUtils.createUfePathSegment('|stage')
        camerasParentPathSegment = usdUtils.createUfePathSegment('/cameras')
        
        proxyShapePath = ufe.Path([mayaPathSegment])
        proxyShapeParentPath = ufe.Path([proxyShapeParentSegment])
        camerasParentPath = ufe.Path([mayaPathSegment, camerasParentPathSegment])

        # searching on a gateway item should give all gateway nodes in the child segment.
        # USD doesn't have any gateway nodes, so the result should be empty
        handler = ufe.RunTimeMgr.instance().sceneSegmentHandler(proxyShapePath.runTimeId())
        result = handler.findGatewayItems(proxyShapePath)
        self.assertTrue(result.empty())

        # searching the the parent of a gateway item searches the Maya scene segment
        # for gateway nodes without recursing into USD. should be the proxy shape
        handler = ufe.RunTimeMgr.instance().sceneSegmentHandler(proxyShapeParentPath.runTimeId())
        result = handler.findGatewayItems(proxyShapeParentPath)
        self.assertTrue(result.contains(proxyShapePath))
        self.assertEqual(len(result), 1)

        # searching for the USD parent of both cameras should find no scene segment handler
        handler = ufe.RunTimeMgr.instance().sceneSegmentHandler(camerasParentPath.runTimeId())
        self.assertEqual(handler, None)

if __name__ == '__main__':
    unittest.main(verbosity=2)
