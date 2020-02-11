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

import maya.cmds as cmds

from ufeTestUtils import usdUtils, mayaUtils
import ufe
import mayaUsd.ufe

import unittest

class RenameTestCase(unittest.TestCase):
    '''Test renaming a UFE scene item and its ancestors.

    Renaming should not affect UFE lookup, including renaming the proxy shape.
    '''

    pluginsLoaded = False
    
    @classmethod
    def setUpClass(cls):
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()
    
    @classmethod
    def tearDownClass(cls):
        cmds.file(new=True, force=True)

    def setUp(self):
        ''' Called initially to set up the Maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)
        
        # Open top_layer.ma scene in test-samples
        mayaUtils.openTopLayerScene()
        
        # Clear selection to start off
        cmds.select(clear=True)

    def testRenameProxyShape(self):
        '''Rename proxy shape, UFE lookup should succeed.'''

        mayaSegment = mayaUtils.createUfePathSegment(
            "|world|transform1|proxyShape1")
        usdSegment = usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")

        ball35Path = ufe.Path([mayaSegment, usdSegment])
        ball35PathStr = ','.join(
            [str(segment) for segment in ball35Path.segments])

        # Because of difference in Python binding systems (pybind11 for UFE,
        # Boost Python for mayaUsd and USD), need to pass in strings to
        # mayaUsd functions.  Multi-segment UFE paths need to have
        # comma-separated segments.
        def assertStageAndPrimAccess(
                proxyShapeSegment, primUfePathStr, primSegment):
            proxyShapePathStr = str(proxyShapeSegment)

            stage     = mayaUsd.ufe.getStage(proxyShapePathStr)
            prim      = mayaUsd.ufe.ufePathToPrim(primUfePathStr)
            stagePath = mayaUsd.ufe.stagePath(stage)

            self.assertIsNotNone(stage)
            self.assertEqual(stagePath, proxyShapePathStr)
            self.assertTrue(prim.IsValid())
            self.assertEqual(str(prim.GetPath()), str(primSegment))

        assertStageAndPrimAccess(mayaSegment, ball35PathStr, usdSegment)

        # Rename the proxy shape node itself.  Stage and prim access should
        # still be valid, with the new path.
        mayaSegment = mayaUtils.createUfePathSegment(
            "|world|transform1|potato")
        cmds.rename('|transform1|proxyShape1', 'potato')
        self.assertEqual(len(cmds.ls('potato')), 1)

        ball35Path = ufe.Path([mayaSegment, usdSegment])
        ball35PathStr = ','.join(
            [str(segment) for segment in ball35Path.segments])

        assertStageAndPrimAccess(mayaSegment, ball35PathStr, usdSegment)

    def testRename(self):
        '''Rename USD node.'''

        # Select a USD object.
        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment(
                "|world|transform1|proxyShape1"),
             usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)
        ball35Hierarchy = ufe.Hierarchy.hierarchy(ball35Item)
        propsItem = ball35Hierarchy.parent()

        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)
        propsChildrenPre = propsHierarchy.children()

        ufe.GlobalSelection.get().append(ball35Item)

        newName = 'Ball_35_Renamed'
        cmds.rename(newName)

        # The renamed item is in the selection.
        snIter = iter(ufe.GlobalSelection.get())
        ball35RenItem = next(snIter)
        ball35RenName = str(ball35RenItem.path().back())

        self.assertEqual(ball35RenName, newName)

        # MAYA-92350: should not need to re-bind hierarchy interface objects
        # with their item.
        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)
        propsChildren = propsHierarchy.children()

        self.assertEqual(len(propsChildren), len(propsChildrenPre))
        self.assertIn(ball35RenItem, propsChildren)

        cmds.undo()

        def childrenNames(children):
            return [str(child.path().back()) for child in children]

        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)
        propsChildren = propsHierarchy.children()
        propsChildrenNames = childrenNames(propsChildren)

        self.assertNotIn(ball35RenName, propsChildrenNames)
        self.assertIn('Ball_35', propsChildrenNames)
        self.assertEqual(len(propsChildren), len(propsChildrenPre))

        cmds.redo()

        propsHierarchy = ufe.Hierarchy.hierarchy(propsItem)
        propsChildren = propsHierarchy.children()
        propsChildrenNames = childrenNames(propsChildren)

        self.assertIn(ball35RenName, propsChildrenNames)
        self.assertNotIn('Ball_35', propsChildrenNames)
        self.assertEqual(len(propsChildren), len(propsChildrenPre))

