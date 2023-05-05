#!/usr/bin/env python

#
# Copyright 2023 Autodesk
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

import mayaUtils
import ufeUtils
import usdUtils
import testUtils

from maya import cmds

from pxr import UsdShade, Sdf

import os
import ufe
import unittest

import maya.mel as mel

class ClipboardHandlerTestCase(unittest.TestCase):
    '''Test clipboard operations.'''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    def setUp(self):
        self.assertTrue(self.pluginsLoaded)

        cmds.file(new=True, force=True)

    def testCutCopyPasteCmd(self):
        '''Test the Clipboard cut, copy and paste cmds.'''
        # The cut cmd copies the selected nodes to the clipboard and then deletes them.
        # To test the cut cmd, we must also test the paste cmd.
        testFile = testUtils.getTestScene('MaterialX', 'BatchOpsTestScene.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        ngParentItem = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG')
        self.assertIsNotNone(ngParentItem)
        
        # Copy a compound and a connected node.
        ngCompoundItem = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/MayaNG_ss3SG')
        self.assertIsNotNone(ngCompoundItem)

        ngItem = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/ss3')
        self.assertIsNotNone(ngItem)

        clipboardHandler = ufe.RunTimeMgr.instance().clipboardHandler(ngItem.runTimeId())
        self.assertIsNotNone(clipboardHandler)

        # Create a selection.
        sel = ufe.Selection()
        sel.append(ngItem)
        sel.append(ngCompoundItem)

        # Execute the clipboard cut cmd.
        cmd = clipboardHandler.cutCmd(sel)
        cmd.execute()

        # Check we cut (aka delete and copy to clipboard) the selected items.
        self.assertIsNone(ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/MayaNG_ss3SG'))
        self.assertIsNone(ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/ss3'))

        # The cut cmd undo brings back the cut items.
        cmd.undo()
        self.assertIsNotNone(ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/MayaNG_ss3SG'))
        self.assertIsNotNone(ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/ss3'))

        cmd.redo()
        self.assertIsNone(ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/MayaNG_ss3SG'))
        self.assertIsNone(ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/ss3'))

        # Execute the clipboard paste cmd in the current stage.
        mel.eval('source \"mayaUsd_createStageFromFile.mel\"')
        cmd = clipboardHandler.pasteCmd(ngParentItem)
        cmd.execute()

        # Check we paste back the items which were deleted and then copied to the clipboard.
        ngCompoundItem = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/MayaNG_ss3SG')
        ngItem = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/ss3')
        self.assertIsNotNone(ngCompoundItem)
        self.assertIsNotNone(ngItem)
        self.assertEqual(len(cmd.targetItems()), 2)

        # Check we preserve the same number of children for the pasted compound.
        usdCompoundHier = ufe.Hierarchy.hierarchy(ngCompoundItem)
        self.assertEqual(len(usdCompoundHier.children()), 3)

        # Check we preserve the same number of connections for the pasted node.
        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ngItem.runTimeId())
        connections = connectionHandler.sourceConnections(ngItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)
                
        # The paste cmd undo acts as a cut cmd execute.
        cmd.undo()
        self.assertIsNone(ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/MayaNG_ss3SG'))
        self.assertIsNone(ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/ss3'))

        # The paste cmd redo pastes back the deleted nodes.
        cmd.undo()
        self.assertIsNotNone(ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/MayaNG_ss3SG'))
        self.assertIsNotNone(ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/ss3'))
        
if __name__ == '__main__':
    unittest.main(verbosity=2)
