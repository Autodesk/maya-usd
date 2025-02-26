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
        self.loadUsdFile('RelativeReferencedUdimTextures.usda')

        uvTexturePathStr = "|stage|stageShape,/Def1/pPlane1/mtl/UsdPreviewSurface1/UsdUVTexture1"

        uvTextureItem = ufe.Hierarchy.createItem(ufe.PathString.path(uvTexturePathStr))
        self.assertTrue(uvTextureItem)

        # Exercise the File Handler
        fileHandler = lxufe.FileHandler.get(uvTextureItem.runTimeId())

        # Make sure the updated code for getResolvedPath is returning the correct value.
        uvTextureAttrs = ufe.Attributes.attributes(uvTextureItem)
        fileAttr = uvTextureAttrs.attribute("inputs:file")

        originalFilePath = fileAttr.get()

        resolvedFilePath = fileHandler.getResolvedPath(fileAttr)
        folder = os.path.split(resolvedFilePath)[0]
        self.assertTrue(os.path.exists(folder))
        self.assertTrue(os.path.isdir(folder))
        zeroUdimFile = resolvedFilePath.replace("<UDIM>", "1001")
        self.assertTrue(os.path.exists(zeroUdimFile))
        self.assertTrue(os.path.isfile(zeroUdimFile))

        cmd1 = fileHandler.convertPathToAbsoluteCmd(fileAttr)
        self.assertTrue(cmd1)
        cmd1.execute()

        absoluteFilePath = fileAttr.get()
        self.assertEqual(resolvedFilePath, absoluteFilePath)

        cmd2 = fileHandler.convertPathToRelativeCmd(fileAttr)
        self.assertTrue(cmd2)
        cmd2.execute()

        relativeFilePath = fileAttr.get()
        # Original path is relative to asset. New relative path is relative to edit layer.
        self.assertNotEqual(originalFilePath, relativeFilePath)
        # But still resolves to the same path.
        self.assertEqual(resolvedFilePath, fileHandler.getResolvedPath(fileAttr))

        # Undo back to absolute:
        cmd2.undo()
        self.assertEqual(absoluteFilePath, fileAttr.get())

        # Undo back to original relative path:
        cmd1.undo()
        self.assertEqual(originalFilePath, fileAttr.get())


if __name__ == '__main__':
    unittest.main(verbosity=2)
