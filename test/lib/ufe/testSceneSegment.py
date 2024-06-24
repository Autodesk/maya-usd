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
import ufeUtils

from maya import cmds
from maya import standalone

import ufe

import os
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
        proxyShapePath = ufe.PathString.path('|stage|stageShape')
        proxyShapeParentPath = ufe.PathString.path('|stage')
        camerasParentPath = ufe.PathString.path('|stage|stageShape,/cameras')

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

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test for UFE v4 or later')
    def testFilteredFindGatewayItems(self):
        proxyShapePath = ufe.PathString.path('|stage|stageShape')
        proxyShapeParentPath = ufe.PathString.path('|stage')

        # Searching on a gateway item should give all gateway nodes in
        # the child segment. USD doesn't have any gateway nodes, so the
        # result should be empty. When using the filtered version of
        # `findGatewayItems()`, the result should still be empty.
        # Filtering can never increase the cardinality of the result.
        handler = ufe.RunTimeMgr.instance().sceneSegmentHandler(proxyShapePath.runTimeId())
        
        result = handler.findGatewayItems(proxyShapePath)
        self.assertTrue(result.empty())

        usdRunTimeId = ufe.RunTimeMgr.instance().getId('USD')
        result = handler.findGatewayItems(proxyShapePath, usdRunTimeId)
        self.assertTrue(result.empty())

        otherRunTimeId = 6174
        result = handler.findGatewayItems(proxyShapePath, otherRunTimeId)
        self.assertTrue(result.empty())

        # Searching from the parent of a gateway item searches the Maya
        # scene segment for gateway nodes without recursing into USD.
        # If no filter is specified or if the USD runtime ID is used as
        # a filter, this should return the proxy shape. If a different
        # runtime ID is used as a filter, the result should be empty.
        handler = ufe.RunTimeMgr.instance().sceneSegmentHandler(proxyShapeParentPath.runTimeId())
        
        result = handler.findGatewayItems(proxyShapeParentPath)
        self.assertTrue(result.contains(proxyShapePath))
        self.assertEqual(len(result), 1)

        result = handler.findGatewayItems(proxyShapeParentPath, usdRunTimeId)
        self.assertTrue(result.contains(proxyShapePath))
        self.assertTrue(len(result), 1)

        result = handler.findGatewayItems(proxyShapeParentPath, otherRunTimeId)
        self.assertTrue(result.empty())

    def testRootSceneSegmentRootPath(self):
        proxyShapePath = ufe.PathString.path('|stage|stageShape')
        handler = ufe.RunTimeMgr.instance().sceneSegmentHandler(proxyShapePath.runTimeId())
        rootPath = handler.rootSceneSegmentRootPath()

        self.assertEqual(str(rootPath), "|world", "The scene root path is not correct")

if __name__ == '__main__':
    unittest.main(verbosity=2)
