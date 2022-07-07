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

from maya import cmds
from maya import standalone

import mayaUtils
import mayaUsd
import mayaUsd_createStageWithNewLayer

import unittest

class DisplayLayerTestCase(unittest.TestCase):
    '''
    Test Display Layer Ufe items (non-Maya) operations such as rename/reparent.
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
        cmds.file(new=True, force=True)

    @unittest.skipUnless(mayaUtils.ufeSupportFixLevel() >= 2, "Requires Display Layer Ufe item rename fix.")
    def testDisplayLayerItemRename(self):
        # First create Display Layer and add some prims to it.
        cmds.createDisplayLayer(name='layer1', number=1, empty=True)
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/Cube1', 'Cube')
        stage.DefinePrim('/Sphere1', 'Sphere')

        # Add the two prims to the layer.
        cmds.editDisplayLayerMembers('layer1', '|stage1|stageShape1,/Sphere1', '|stage1|stageShape1,/Cube1', noRecurse=True)

        # Verify they are in layer.
        layerObjs = cmds.editDisplayLayerMembers('layer1', query=True, fn=True)
        self.assertTrue('|stage1|stageShape1,/Cube1' in layerObjs)
        self.assertTrue('|stage1|stageShape1,/Sphere1' in layerObjs)
        self.assertFalse('|stage1|stageShape1,/NewSphere1' in layerObjs)

        # Rename the Sphere and make sure it is still in the layer.
        cmds.select('|stage1|stageShape1,/Sphere1', replace=True)
        cmds.rename('NewSphere1')
        layerObjs = cmds.editDisplayLayerMembers('layer1', query=True, fn=True)
        self.assertTrue('|stage1|stageShape1,/Cube1' in layerObjs)
        self.assertFalse('|stage1|stageShape1,/Sphere1' in layerObjs)
        self.assertTrue('|stage1|stageShape1,/NewSphere1' in layerObjs)

        # Undo the rename.
        cmds.undo()
        layerObjs = cmds.editDisplayLayerMembers('layer1', query=True, fn=True)
        self.assertTrue('|stage1|stageShape1,/Cube1' in layerObjs)
        self.assertTrue('|stage1|stageShape1,/Sphere1' in layerObjs)
        self.assertFalse('|stage1|stageShape1,/NewSphere1' in layerObjs)

        # Redo the rename.
        cmds.redo()
        layerObjs = cmds.editDisplayLayerMembers('layer1', query=True, fn=True)
        self.assertTrue('|stage1|stageShape1,/Cube1' in layerObjs)
        self.assertFalse('|stage1|stageShape1,/Sphere1' in layerObjs)
        self.assertTrue('|stage1|stageShape1,/NewSphere1' in layerObjs)

    def testDisplayLayerItemReparent(self):
        # First create Display Layer and add some prims to it.
        cmds.createDisplayLayer(name='layer1', number=1, empty=True)
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/Xform1', 'Xform')
        stage.DefinePrim('/Cube1', 'Cube')
        stage.DefinePrim('/Sphere1', 'Sphere')

        # Add the two prims to the layer.
        cmds.editDisplayLayerMembers('layer1', '|stage1|stageShape1,/Sphere1', '|stage1|stageShape1,/Cube1', noRecurse=True)

        # Verify they are in layer.
        layerObjs = cmds.editDisplayLayerMembers('layer1', query=True, fn=True)
        self.assertTrue('|stage1|stageShape1,/Cube1' in layerObjs)
        self.assertTrue('|stage1|stageShape1,/Sphere1' in layerObjs)
        self.assertFalse('|stage1|stageShape1,/Xform1' in layerObjs)

        # Reparent the Sphere and make sure it is still in the layer.
        cmds.parent('|stage1|stageShape1,/Sphere1', '|stage1|stageShape1,/Xform1')
        layerObjs = cmds.editDisplayLayerMembers('layer1', query=True, fn=True)
        self.assertTrue('|stage1|stageShape1,/Cube1' in layerObjs)
        self.assertTrue('|stage1|stageShape1,/Xform1/Sphere1' in layerObjs)
        self.assertFalse('|stage1|stageShape1,/Xform1' in layerObjs)

        # Undo the reparent.
        cmds.undo()
        layerObjs = cmds.editDisplayLayerMembers('layer1', query=True, fn=True)
        self.assertTrue('|stage1|stageShape1,/Cube1' in layerObjs)
        self.assertTrue('|stage1|stageShape1,/Sphere1' in layerObjs)
        self.assertFalse('|stage1|stageShape1,/Xform1' in layerObjs)

        # Redo the reparent.
        cmds.redo()
        layerObjs = cmds.editDisplayLayerMembers('layer1', query=True, fn=True)
        self.assertTrue('|stage1|stageShape1,/Cube1' in layerObjs)
        self.assertTrue('|stage1|stageShape1,/Xform1/Sphere1' in layerObjs)
        self.assertFalse('|stage1|stageShape1,/Xform1' in layerObjs)

    def testDisplayLayerItemDelete(self):
        # First create Display Layer and add some prims to it.
        cmds.createDisplayLayer(name='layer1', number=1, empty=True)
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/Cube1', 'Cube')
        stage.DefinePrim('/Sphere1', 'Sphere')

        # Add the two prims to the layer.
        cmds.editDisplayLayerMembers('layer1', '|stage1|stageShape1,/Sphere1', '|stage1|stageShape1,/Cube1', noRecurse=True)

        # Verify they are in layer.
        layerObjs = cmds.editDisplayLayerMembers('layer1', query=True, fn=True)
        self.assertTrue('|stage1|stageShape1,/Cube1' in layerObjs)
        self.assertTrue('|stage1|stageShape1,/Sphere1' in layerObjs)
        self.assertFalse('|stage1|stageShape1,/NewSphere1' in layerObjs)

        # Delete the Sphere and make sure it is removed from the layer.
        cmds.delete('|stage1|stageShape1,/Sphere1')
        layerObjs = cmds.editDisplayLayerMembers('layer1', query=True, fn=True)
        self.assertTrue('|stage1|stageShape1,/Cube1' in layerObjs)
        self.assertFalse('|stage1|stageShape1,/Sphere1' in layerObjs)

        # TODO - test the undo/redo for delete of Ufe item in Layer.
        # This currently doesn't work.
