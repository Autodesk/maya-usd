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

from pxr import UsdGeom, UsdShade, Sdf, Usd, Gf, Vt

import os
import unittest

class LookdevXUfeDebugHandlerTestCase(unittest.TestCase):
    '''Verify the USD implementation of LookdevXUfe DebugHandler.
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

    def test_DebugHandler(self):
        self.loadUsdFile('test_component.usda')

        mtlPathStr = "|stage|stageShape,/mtl/standard_surface1"

        stdSurfaceSceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(mtlPathStr+"/standard_surface1"))
        self.assertTrue(stdSurfaceSceneItem)

        # Use the debug handler to get the data model string representation for the scene item.
        debugHandler = lxufe.DebugHandler.get(stdSurfaceSceneItem.runTimeId())
        sceneItemDump = debugHandler.exportToString(stdSurfaceSceneItem)
        
        if Usd.GetVersion() < (0, 25, 8):
            # Look for the #sdf header to ensure it's a valid file, and look for signs
            # of the xform primitive.
            self.assertTrue("#sdf" in sceneItemDump)
        else:
            # Look for the #usda header to ensure it's a valid file, and look for signs
            # of the xform primitive.
            self.assertTrue("#usda" in sceneItemDump)

        self.assertTrue('def Shader "standard_surface1"' in sceneItemDump)

        nodeDefHandler = ufe.RunTimeMgr.instance().nodeDefHandler(ufe.RunTimeMgr.instance().getId('USD'))
        self.assertTrue(nodeDefHandler)

        nodeDef = nodeDefHandler.definition("UsdPreviewSurface")
        self.assertTrue(nodeDef)

        # TODO: Unimplemented.
        self.assertTrue(debugHandler.hasViewportSupport(nodeDef))

        # TODO: Unimplemented.
        self.assertFalse(debugHandler.getImplementation(nodeDef))

        # TODO: Unimplemented.
        self.assertFalse(debugHandler.isLibraryItem(stdSurfaceSceneItem))

        # TODO: Unimplemented.
        self.assertTrue(debugHandler.isEditable(stdSurfaceSceneItem))


if __name__ == '__main__':
    unittest.main(verbosity=2)
