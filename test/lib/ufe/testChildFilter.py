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
import testUtils

from maya import cmds
from maya import standalone
from maya.internal.ufeSupport import ufeCmdWrapper as ufeCmd

import mayaUsd.ufe

import ufe

import unittest
import collections


class ChildFilterTestCase(unittest.TestCase):
    '''Verify the ChildFilter USD implementation.
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

    def testChildFilter(self):
        # Ensure we have one USD child filter
        rid = ufe.RunTimeMgr.instance().getId('USD')
        usdHierHndlr = ufe.RunTimeMgr.instance().hierarchyHandler(rid)
        cf = usdHierHndlr.childFilter()
        self.assertEqual(2, len(cf))

        # Make sure the USD hierarchy handler has an inactive prims filter
        self.assertEqual('InactivePrims', cf[0].name)

        # Ensure we have the same child filter on the Maya runtime (for the
        # ProxyShapeHierarchyHandler.
        rid = ufe.RunTimeMgr.instance().getId('Maya-DG')
        mayaHierHndlr = ufe.RunTimeMgr.instance().hierarchyHandler(rid)
        mayaCf = mayaHierHndlr.childFilter()
        self.assertEqual(2, len(mayaCf))
        self.assertEqual(cf[0].name, mayaCf[0].name)
        self.assertEqual(cf[0].label, mayaCf[0].label)
        self.assertEqual(cf[0].value, mayaCf[0].value)

    def testFilteredChildren(self):
        mayaUtils.openGroupBallsScene()
        cmds.select(clear=True)

        # Check that we have six balls
        propsPath = ufe.PathString.path('|transform1|proxyShape1,/Ball_set/Props')
        propsItem = ufe.Hierarchy.createItem(propsPath)
        propsHier = ufe.Hierarchy.hierarchy(propsItem)
        self.assertEqual(6, len(propsHier.children()))

        # Deactivate Ball_3 (which will remove it from children)
        ball3PathStr = '|transform1|proxyShape1,/Ball_set/Props/Ball_3'
        ball3Path = ufe.PathString.path(ball3PathStr)
        ball3Item = ufe.Hierarchy.createItem(ball3Path)

        # Set Ball_3 to inactive.
        ball3Prim = mayaUsd.ufe.ufePathToPrim(ball3PathStr)
        ball3Prim.SetActive(False)

        # Props should now have 5 children and ball3 should not be one of them.
        children = propsHier.children()
        self.assertEqual(5, len(children))
        self.assertNotIn(ball3Item, children)

        # Get the child filter for the hierarchy handler corresponding
        # to the runtime of the Ball_3.
        usdHierHndlr = ufe.RunTimeMgr.instance().hierarchyHandler(ball3Item.runTimeId())
        cf = usdHierHndlr.childFilter()

        # Toggle "Inactive Prims" off and get the filtered children
        # Props filtered children should have 5 children and ball3 should not be one of them.
        cf[0].value = False
        children = propsHier.filteredChildren(cf)
        self.assertEqual(5, len(children))
        self.assertNotIn(ball3Item, children)

        # Toggle "Inactive Prims" on and get the filtered children
        # (with inactive prims) and verify ball3 is one of them.
        cf[0].value = True
        children = propsHier.filteredChildren(cf)
        self.assertEqual(6, len(children))
        self.assertIn(ball3Item, children)

    def testFilteredClassPrims(self):
        classPrimFileName = testUtils.getTestScene('classPrims', 'class-prims.usda')
        shapeNode, stage = mayaUtils.createProxyFromFile(classPrimFileName)
        self.assertTrue(stage)
        cmds.select(clear=True)

        # Check that we see the 2 active prims
        topPathStr = shapeNode + ',/top'
        topPath = ufe.PathString.path(topPathStr)
        topItem = ufe.Hierarchy.createItem(topPath)
        topHier = ufe.Hierarchy.hierarchy(topItem)
        self.assertEqual(2, len(topHier.children()))

        # Get the child filter for the hierarchy handler corresponding
        # to the runtime of the top item.
        usdHierarchyHandler = ufe.RunTimeMgr.instance().hierarchyHandler(topItem.runTimeId())
        childrenFilters = usdHierarchyHandler.childFilter()

        # The prims that can be found in the file.
        activeConeItem = ufe.Hierarchy.createItem(ufe.PathString.path(topPathStr + '/active_cone'))
        activeCubeItem = ufe.Hierarchy.createItem(ufe.PathString.path(topPathStr + '/active_cube'))
        inactiveCylinderItem = ufe.Hierarchy.createItem(ufe.PathString.path(topPathStr + '/inactive_cylinder'))
        activeClassItem = ufe.Hierarchy.createItem(ufe.PathString.path(topPathStr + '/active_class'))
        inactiveClassItem = ufe.Hierarchy.createItem(ufe.PathString.path(topPathStr + '/inactive_class'))

        # Declare different filter settings that will be tested.
        FilterSettings = collections.namedtuple('FilterSettings', ['filters', 'expecteditems'])
        filterSettings = [
            FilterSettings(
                filters={ 'InactivePrims': False, 'ClassPrims': False },
                expecteditems=[ activeConeItem, activeCubeItem ]),
            FilterSettings(
                filters={ 'InactivePrims': False, 'ClassPrims': True  },
                expecteditems=[ activeConeItem, activeCubeItem, activeClassItem ]),
            FilterSettings(
                filters={ 'InactivePrims': True,  'ClassPrims': False },
                expecteditems=[ activeConeItem, activeCubeItem, inactiveCylinderItem ]),
            FilterSettings(
                filters={ 'InactivePrims': True,  'ClassPrims': True  },
                expecteditems=[ activeConeItem, activeCubeItem, inactiveCylinderItem, activeClassItem, inactiveClassItem ]),
        ]

        for settings in filterSettings:
            # Set the children settings according to the desired test settings.
            for filter in childrenFilters:
                self.assertIn(filter.name, settings.filters)
                filter.value = settings.filters[filter.name]
            
            # Verify we have the expected count of children.
            children = topHier.filteredChildren(childrenFilters)
            self.assertEqual(len(settings.expecteditems), len(children))
            for item in settings.expecteditems:
                self.assertIn(item, children)

    def testProxyShapeFilteredChildren(self):
        mayaUtils.openGroupBallsScene()
        cmds.select(clear=True)

        # Check that the proxy shape has a single child, Ball_set.
        psPath = ufe.PathString.path('|transform1|proxyShape1')
        psItem = ufe.Hierarchy.createItem(psPath)
        psHier = ufe.Hierarchy.hierarchy(psItem)
        psChildren = psHier.children()
        self.assertEqual(len(psChildren), 1)

        # Verify that the single child is the Ball_set.
        ballSetPathStr = '|transform1|proxyShape1,/Ball_set'
        ballSetPath = ufe.PathString.path(ballSetPathStr)
        ballSetItem = ufe.Hierarchy.createItem(ballSetPath)
        self.assertEqual(ballSetItem, psChildren[0])

        # Inactivate Ball_set.
        ballSetPrim = mayaUsd.ufe.ufePathToPrim(ballSetPathStr)
        ballSetPrim.SetActive(False)

        # Proxy shape should now have no children.
        children = psHier.children()
        self.assertEqual(0, len(children))
        self.assertNotIn(ballSetItem, children)

        # Get the child filter for the hierarcy handler corresponding
        # to the runtime of the ProxyShape.
        # Because the proxy shape hierarchy handler overrides the
        # Maya child filter and returns the same child filter.
        usdHierHndlr = ufe.RunTimeMgr.instance().hierarchyHandler(psItem.runTimeId())
        cf = usdHierHndlr.childFilter()

        # Toggle the "Inactive Prims" on and get the filtered children
        # (with inactive prims) and verify Ball_set is one of them.
        cf[0].value = True
        children = psHier.filteredChildren(cf)
        self.assertEqual(1, len(children))
        self.assertIn(ballSetItem, children)

    def testUnload(self):
        mayaUtils.openTopLayerScene()

        propsPath = ufe.PathString.path('|transform1|proxyShape1,/Room_set/Props')
        propsItem = ufe.Hierarchy.createItem(propsPath)
        propsHier = ufe.Hierarchy.hierarchy(propsItem)

        # Before unloading it should have 35 children (35 balls).
        self.assertTrue(propsHier.hasChildren())
        ball1Children = propsHier.children()
        self.assertEqual(len(ball1Children), 35)

        # Unload props
        contextOps = ufe.ContextOps.contextOps(propsItem)
        cmd = contextOps.doOpCmd(['Unload'])
        ufeCmd.execute(cmd)

        # After unloading, it should still have 35 children because
        # the children method returns unloaded prims by default.
        self.assertTrue(propsHier.hasChildren())
        ball1Children = propsHier.children()
        self.assertEqual(len(ball1Children), 35)

if __name__ == '__main__':
    unittest.main(verbosity=2)
