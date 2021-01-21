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

import unittest

import mayaUtils, ufeUtils, usdUtils
import mayaUsd
import ufe

import maya.cmds as cmds


class PythonWrappersTestCase(unittest.TestCase):
    '''Test the maya-usd ufe python wrappers.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    def setUp(self):
        ''' Called initially to set up the Maya test environment '''
        self.assertTrue(self.pluginsLoaded)

    def testWrappers(self):

        ''' Verify the python wappers.'''

        # Create empty stage and add a prim.
        import mayaUsd_createStageWithNewLayer
        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.Path(mayaUtils.createUfePathSegment('|stage1|stageShape1'))

        # Test maya-usd getStage() wrapper.
        mayaUsdStage = mayaUsd.ufe.getStage(str(proxyShapePath))
        self.assertIsNotNone(mayaUsdStage)

        # Verify that this the stage returned from maya-usd wrapper
        # is same as one from Pixar wrapper.
        shapeNode = cmds.ls(sl=True, l=True)[0]
        pixarStage = mayaUsd.lib.GetPrim(shapeNode).GetStage()
        self.assertIsNotNone(pixarStage)
        self.assertIs(mayaUsdStage, pixarStage)

        # Test maya-usd stagePath() wrapper.
        stagePath = mayaUsd.ufe.stagePath(mayaUsdStage)
        self.assertIsNotNone(stagePath)

        # It should also be the same as the shape node string
        # (minus the extra |world at front).
        self.assertEqual(shapeNode, stagePath.replace('|world', ''))

        # Test the maya-usd ufePathToPrim() wrapper.
        mayaUsdStage.DefinePrim("/Capsule1", "Capsule")
        if ufeUtils.ufeFeatureSetVersion() >= 2:
            capsulePrim = mayaUsd.ufe.ufePathToPrim('|stage1|stageShape1,/Capsule1')
        else:
            capsulePrim = mayaUsd.ufe.ufePathToPrim('|world|stage1|stageShape1,/Capsule1')
        self.assertIsNotNone(capsulePrim)

        if ufeUtils.ufeFeatureSetVersion() >= 2:
            # Test the maya-usd getPrimFromRawItem() wrapper.
            capsulePath = proxyShapePath + usdUtils.createUfePathSegment('/Capsule1')
            capsuleItem = ufe.Hierarchy.createItem(capsulePath)
            rawItem = capsuleItem.getRawAddress()
            capsulePrim2 = mayaUsd.ufe.getPrimFromRawItem(rawItem)
            self.assertIsNotNone(capsulePrim2)
            self.assertEqual(capsulePrim, capsulePrim2)

            # Test the maya-usd getNodeTypeFromRawItem() wrapper.
            nodeType = mayaUsd.ufe.getNodeTypeFromRawItem(rawItem)
            self.assertIsNotNone(nodeType)

            # Test the maya-usd getNodeNameFromRawItem() wrapper.
            nodeName = mayaUsd.ufe.getNodeNameFromRawItem(rawItem)
            self.assertIsNotNone(nodeName)

        # Test the maya-usd runtime id wrappers.
        mayaRtid = mayaUsd.ufe.getMayaRunTimeId()
        usdRtid = mayaUsd.ufe.getUsdRunTimeId()
        self.assertTrue(mayaRtid > 0)
        self.assertTrue(usdRtid > 0)
        self.assertNotEqual(mayaRtid, usdRtid)
