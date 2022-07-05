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

class DisplayLayerSaveNonMaya(unittest.TestCase):
    '''
    Test saving/restoring a Display Layer that has non-Maya items.
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

    def testDisplayLayerSaveAndRestore(self):
        # First create Display Layer and find out if it has the Ufe member save/restore support.
        cmds.createDisplayLayer(name='layer1', number=1, empty=True)
        if 'ufeMembers' not in cmds.listAttr('layer1'):
            self.skipTest('Maya DisplayLayer does not support saving/restoring Ufe (non-Maya) members.')

        # Create some objects to add to layer.
        cmds.CreatePolygonSphere()
        cmds.CreatePolygonCube()

        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/Sphere1', 'Sphere')
        stage.DefinePrim('/Cube1', 'Cube')

        # Add the two sphere (Maya and non-Maya) to the layer.
        cmds.editDisplayLayerMembers('layer1', '|pSphere1', '|stage1|stageShape1,/Sphere1', noRecurse=True)

        # Verify they are in layer.
        layerObjs = cmds.editDisplayLayerMembers('layer1', query=True, fn=True)
        self.assertTrue('|pSphere1' in layerObjs)
        self.assertTrue('|stage1|stageShape1,/Sphere1' in layerObjs)

        # Save the file with ufe display layer members.
        tempMayaFile = 'displayLayerRestore.ma'
        cmds.file(rename=tempMayaFile)
        cmds.file(save=True, force=True, type='mayaAscii')

        # Clear and reopen the file.
        cmds.file(new=True, force=True)
        cmds.file(tempMayaFile, open=True)

        # Verify the two objects (Maya and non-Maya are in layer).
        layerObjs = cmds.editDisplayLayerMembers('layer1', query=True, fn=True)
        self.assertTrue('|pSphere1' in layerObjs)
        self.assertTrue('|stage1|stageShape1,/Sphere1' in layerObjs)

        # We can also directly query the attribute value.
        ufeAttr = cmds.getAttr('layer1.ufem')
        self.assertTrue('|stage1|stageShape1,/Sphere1' in ufeAttr)
