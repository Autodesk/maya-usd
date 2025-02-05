#!/usr/bin/env python

#
# Copyright 2025 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License")
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http:#www.apache.org/licenses/LICENSE-2.0
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
from ufe.extensions import lookdevXUfe as lxufe
import mayaUsd.ufe

import os
import unittest

def verifyConnections(testHarness, connections, numRegularConnections, numComponentConnections, numConverterConnections):
    numRegular = 0
    numComponent = 0
    numConverter = 0
    for connection in connections:
        if connection.src.component or connection.dst.component:
            numComponent += 1
        elif connection.src.attribute().type == connection.dst.attribute().type:
            numRegular += 1
        else:
            numConverter += 1

    testHarness.assertEqual(numRegular, numRegularConnections)
    testHarness.assertEqual(numComponent, numComponentConnections)
    testHarness.assertEqual(numConverter, numConverterConnections)

class ComponentConnectionsTestCase(unittest.TestCase):
    '''Verify the USD implementation of LookdevXUfe connections.
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

    @staticmethod
    def loadUsdFile(fileName):
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('lookdevXUsd', fileName)
        mayaUtils.createProxyFromFile(testFile)

    def test_isConnectionHidden(self):
        self.loadUsdFile("test_component.usda")

        mtlPathStr = "|stage|stageShape,/mtl/standard_surface1"

        stdSurfaceSceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(mtlPathStr+"/standard_surface1"))
        self.assertTrue(stdSurfaceSceneItem)
        combineSceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(mtlPathStr+"/combine1"))
        self.assertTrue(combineSceneItem)
        separateSceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(mtlPathStr+"/separate1"))
        self.assertTrue(separateSceneItem)
        addSceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(mtlPathStr+"/add1"))
        self.assertTrue(addSceneItem)

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(stdSurfaceSceneItem.runTimeId())

        stdSurfaceSrcConnections = connectionHandler.sourceConnections(stdSurfaceSceneItem)
        for stdSurfaceSrcConnection in stdSurfaceSrcConnections.allConnections():
            self.assertTrue(lxufe.UfeUtils.isConnectionHidden(stdSurfaceSrcConnection))

        combineSrcConnections = connectionHandler.sourceConnections(combineSceneItem)
        for combineSrcConnection in combineSrcConnections.allConnections():
            self.assertTrue(lxufe.UfeUtils.isConnectionHidden(combineSrcConnection))

        separateSrcConnections = connectionHandler.sourceConnections(separateSceneItem)
        for separateSrcConnection in separateSrcConnections.allConnections():
            self.assertTrue(lxufe.UfeUtils.isConnectionHidden(separateSrcConnection))

        addSrcConnections = connectionHandler.sourceConnections(addSceneItem)
        for addSrcConnection in addSrcConnections.allConnections():
            self.assertFalse(lxufe.UfeUtils.isConnectionHidden(addSrcConnection))

    def test_GetAllExtendedConnections(self):
        self.loadUsdFile("extendedConnections.usda")

        mtlPathStr = "|stage|stageShape,/mtl/standard_surface1"

        # Get the standard_surface1 node and scene item.
        stdSurf1Item = ufe.Hierarchy.createItem(ufe.PathString.path(mtlPathStr+"/standard_surface1"))
        self.assertTrue(stdSurf1Item)

        # Get the add1 node and scene item.
        add1Item = ufe.Hierarchy.createItem(ufe.PathString.path(mtlPathStr+"/add1"))
        self.assertTrue(add1Item)

        # Get the add2 node and scene item.
        add2Item = ufe.Hierarchy.createItem(ufe.PathString.path(mtlPathStr+"/add2"))
        self.assertTrue(add2Item)

        # Verify the getAllExtendedConnections function with the standard_surface1 node.
        allStdSurfExtendedConnections = lxufe.UfeUtils.getAllExtendedConnections(stdSurf1Item)
        verifyConnections(self, allStdSurfExtendedConnections, 2, 2, 2)

        stdSurfRegularConnections = lxufe.UfeUtils.getAllExtendedConnections(
            stdSurf1Item, lxufe.UfeUtils.kExtendedRegularConnections)
        verifyConnections(self, stdSurfRegularConnections, 2, 0, 0)

        stdSurfSrcRegularConnections = lxufe.UfeUtils.getSourceExtendedConnections(
            stdSurf1Item, lxufe.UfeUtils.kExtendedRegularConnections)
        verifyConnections(self, stdSurfSrcRegularConnections, 1, 0, 0)

        stdSurfComponentConnections = lxufe.UfeUtils.getAllExtendedConnections(
            stdSurf1Item, lxufe.UfeUtils.kExtendedComponentConnections)
        verifyConnections(self, stdSurfComponentConnections, 0, 2, 0)

        stdSurfSrcComponentConnections = lxufe.UfeUtils.getSourceExtendedConnections(
            stdSurf1Item, lxufe.UfeUtils.kExtendedComponentConnections)
        verifyConnections(self, stdSurfSrcComponentConnections, 0, 2, 0)

        stdSurfConverterConnections = lxufe.UfeUtils.getAllExtendedConnections(
            stdSurf1Item, lxufe.UfeUtils.kExtendedConverterConnections)
        verifyConnections(self, stdSurfConverterConnections, 0, 0, 2)

        stdSurfSrcConverterConnections = lxufe.UfeUtils.getSourceExtendedConnections(
            stdSurf1Item, lxufe.UfeUtils.kExtendedConverterConnections)
        verifyConnections(self, stdSurfSrcConverterConnections, 0, 0, 2)

        stdSurfNonRegularConnections = lxufe.UfeUtils.getAllExtendedConnections(
            stdSurf1Item,
            lxufe.UfeUtils.kExtendedComponentConnections | lxufe.UfeUtils.kExtendedConverterConnections)
        verifyConnections(self, stdSurfNonRegularConnections, 0, 2, 2)

        stdSurfSrcNonRegularConnections = lxufe.UfeUtils.getSourceExtendedConnections(
            stdSurf1Item,
            lxufe.UfeUtils.kExtendedComponentConnections | lxufe.UfeUtils.kExtendedConverterConnections)
        verifyConnections(self, stdSurfSrcNonRegularConnections, 0, 2, 2)

        stdSurfRegularOrComponentConnections = lxufe.UfeUtils.getAllExtendedConnections(
            stdSurf1Item,
            lxufe.UfeUtils.kExtendedRegularConnections | lxufe.UfeUtils.kExtendedComponentConnections)
        verifyConnections(self, stdSurfRegularOrComponentConnections, 2, 2, 0)

        stdSurfSrcRegularOrComponentConnections = lxufe.UfeUtils.getSourceExtendedConnections(
            stdSurf1Item,
            lxufe.UfeUtils.kExtendedRegularConnections | lxufe.UfeUtils.kExtendedComponentConnections)
        verifyConnections(self, stdSurfSrcRegularOrComponentConnections, 1, 2, 0)

        stdSurfRegularOrConverterConnections = lxufe.UfeUtils.getAllExtendedConnections(
            stdSurf1Item,
            lxufe.UfeUtils.kExtendedRegularConnections | lxufe.UfeUtils.kExtendedConverterConnections)
        verifyConnections(self, stdSurfRegularOrConverterConnections, 2, 0, 2)

        stdSurfSrcRegularOrConverterConnections = lxufe.UfeUtils.getSourceExtendedConnections(
            stdSurf1Item,
            lxufe.UfeUtils.kExtendedRegularConnections | lxufe.UfeUtils.kExtendedConverterConnections)
        verifyConnections(self, stdSurfSrcRegularOrConverterConnections, 1, 0, 2)

        # Verify the getAllExtendedConnections function with the add1 node.
        allAdd1ExtendedConnections = lxufe.UfeUtils.getAllExtendedConnections(add1Item)
        verifyConnections(self, allAdd1ExtendedConnections, 0, 6, 0)

        add1SrcExtendedConnections = lxufe.UfeUtils.getSourceExtendedConnections(add1Item)
        verifyConnections(self, add1SrcExtendedConnections, 0, 3, 0)

        # Verify the getAllExtendedConnections function with the add2 node.
        allAdd2ExtendedConnections = lxufe.UfeUtils.getAllExtendedConnections(add2Item)
        verifyConnections(self, allAdd2ExtendedConnections, 0, 0, 4)

        add2SrcExtendedConnections = lxufe.UfeUtils.getSourceExtendedConnections(add2Item)
        verifyConnections(self, add2SrcExtendedConnections, 0, 0, 2)

    def test_IsInternalConnection(self):
        self.loadUsdFile("isInternalConnection.usda")

        mtlPathStr = "|stage|stageShape,/mtl/surface1"

        # Get compound1.
        # Get the add1 node and scene item.
        compound1SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(mtlPathStr+"/compound1"))
        self.assertTrue(compound1SceneItem)

        connections = lxufe.UfeUtils.getAllExtendedConnections(compound1SceneItem)
        self.assertEqual(len(connections), 12)

        numInternalConnections = 0
        for connection in connections:
            if lxufe.UfeUtils.isInternalConnection(connection):
                numInternalConnections += 1

        self.assertEqual(numInternalConnections, 6)

if __name__ == '__main__':
    unittest.main(verbosity=2)
