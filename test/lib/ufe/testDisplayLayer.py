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
import maya.api.OpenMaya as om

import mayaUtils
import mayaUsd
import mayaUsd_createStageWithNewLayer

import unittest

class DisplayLayerTestCase(unittest.TestCase):
    '''
    Test Display Layer Ufe items (non-Maya) operations such as rename/reparent.
    '''

    pluginsLoaded = False

    DEFAULT_LAYER = 'defaultLayer'
    LAYER1 = 'layer1'
    CUBE1 = '|stage1|stageShape1,/Cube1'
    SPHERE1 = '|stage1|stageShape1,/Sphere1'
    XFORM1 = '|stage1|stageShape1,/Xform1'
    NEW_XFORM1 = '|stage1|stageShape1,/NewXform1'
    NEW_SPHERE1 = '|stage1|stageShape1,/NewSphere1'
    XFORM1_SPHERE1 = '|stage1|stageShape1,/Xform1/Sphere1'
    NEW_XFORM1_SPHERE1 = '|stage1|stageShape1,/NewXform1/Sphere1'
    INVALID_PRIM = '|stage1|stageShape1,/BogusPrim'

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

        # Get the Maya display layer manager object.
        mo = om.MFnDisplayLayerManager.currentDisplayLayerManager() if mayaUtils.ufeSupportFixLevel() >= 2 else om.MFnDisplayLayerManager().currentDisplayLayerManager()
        self.dlm = om.MFnDisplayLayerManager(mo)

    def displayLayer(self, layer_name):
        displayLayerObjs = self.dlm.getAllDisplayLayers()
        for dl in displayLayerObjs:
            if om.MFnDisplayLayer(dl).name() == layer_name:
                return om.MFnDisplayLayer(dl)

    def _testLayerFromPath(self, pathStr, layerName):
        try:
            mo = self.dlm.getLayer(pathStr)
        except RuntimeError:
            self.assertIsNone(layerName)
            return
        layer = om.MFnDisplayLayer(mo)
        self.assertEqual(layerName, layer.name())
        self.assertTrue(layer.contains(pathStr))
        self.assertTrue(pathStr in layer.getMembers().getSelectionStrings())

    @unittest.skipUnless(mayaUtils.ufeSupportFixLevel() >= 2, "Requires Display Layer Ufe item rename fix.")
    def testDisplayLayerItemRename(self):
        # First create Display Layer and add some prims to it.
        cmds.createDisplayLayer(name=self.LAYER1, number=1, empty=True)
        defaultLayer = self.displayLayer(self.DEFAULT_LAYER)
        layer1 = self.displayLayer(self.LAYER1)
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/Cube1', 'Cube')
        stage.DefinePrim('/Sphere1', 'Sphere')

        # Add the two prims to the layer.
        cmds.editDisplayLayerMembers(self.LAYER1, self.SPHERE1, self.CUBE1, noRecurse=True)

        # Verify they are in layer.
        layerObjs = cmds.editDisplayLayerMembers(self.LAYER1, query=True, fn=True)
        self.assertTrue(self.CUBE1 in layerObjs)
        self.assertTrue(self.SPHERE1 in layerObjs)
        self.assertFalse(self.NEW_SPHERE1 in layerObjs)

        self._testLayerFromPath(self.CUBE1, self.LAYER1)
        self._testLayerFromPath(self.SPHERE1, self.LAYER1)
        if mayaUtils.ufeSupportFixLevel() >= 2:
            # Bug in Maya code which incorrectly returned default layer for invalid ufe path.
            self._testLayerFromPath(self.NEW_SPHERE1, None)

        # Rename the Sphere and make sure it is still in the layer.
        cmds.select(self.SPHERE1, replace=True)
        cmds.rename('NewSphere1')
        layerObjs = cmds.editDisplayLayerMembers(self.LAYER1, query=True, fn=True)
        self.assertTrue(self.CUBE1 in layerObjs)
        self.assertFalse(self.SPHERE1 in layerObjs)
        self.assertTrue(self.NEW_SPHERE1 in layerObjs)

        self._testLayerFromPath(self.CUBE1, self.LAYER1)
        if mayaUtils.ufeSupportFixLevel() >= 2:
            self._testLayerFromPath(self.SPHERE1, None)
        self._testLayerFromPath(self.NEW_SPHERE1, self.LAYER1)

        # Undo the rename.
        cmds.undo()
        layerObjs = cmds.editDisplayLayerMembers(self.LAYER1, query=True, fn=True)
        self.assertTrue(self.CUBE1 in layerObjs)
        self.assertTrue(self.SPHERE1 in layerObjs)
        self.assertFalse(self.NEW_SPHERE1 in layerObjs)

        self._testLayerFromPath(self.CUBE1, self.LAYER1)
        self._testLayerFromPath(self.SPHERE1, self.LAYER1)
        if mayaUtils.ufeSupportFixLevel() >= 2:
            self._testLayerFromPath(self.NEW_SPHERE1, None)

        # Redo the rename.
        cmds.redo()
        layerObjs = cmds.editDisplayLayerMembers(self.LAYER1, query=True, fn=True)
        self.assertTrue(self.CUBE1 in layerObjs)
        self.assertFalse(self.SPHERE1 in layerObjs)
        self.assertTrue(self.NEW_SPHERE1 in layerObjs)

        self._testLayerFromPath(self.CUBE1, self.LAYER1)
        if mayaUtils.ufeSupportFixLevel() >= 2:
            self._testLayerFromPath(self.SPHERE1, None)
        self._testLayerFromPath(self.NEW_SPHERE1, self.LAYER1)

    @unittest.skipUnless(mayaUtils.ufeSupportFixLevel() >= 2, "Requires Display Layer Ufe item rename fix.")
    def testDisplayLayerItemRenameParent(self):
        # First create Display Layer and add some prims to it.
        cmds.createDisplayLayer(name=self.LAYER1, number=1, empty=True)
        defaultLayer = self.displayLayer(self.DEFAULT_LAYER)
        layer1 = self.displayLayer(self.LAYER1)
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/Xform1', 'Xform')
        stage.DefinePrim('/Xform1/Sphere1', 'Sphere')

        # Add the Xform1/Sphere1 prim to the layer.
        cmds.editDisplayLayerMembers(self.LAYER1, self.XFORM1_SPHERE1, noRecurse=True)

        # Rename the Xform1 (parent of Sphere1) and make sure Sphere1 is still in the layer.
        cmds.select(self.XFORM1, replace=True)
        cmds.rename('NewXform1')
        layerObjs = cmds.editDisplayLayerMembers(self.DEFAULT_LAYER, query=True, fn=True)
        self.assertFalse(self.XFORM1 in layerObjs)
        self.assertTrue(self.NEW_XFORM1 in layerObjs)
        self.assertFalse(self.XFORM1_SPHERE1 in layerObjs)
        self.assertFalse(self.NEW_XFORM1_SPHERE1 in layerObjs)

        layerObjs = cmds.editDisplayLayerMembers(self.LAYER1, query=True, fn=True)
        self.assertFalse(self.XFORM1 in layerObjs)
        self.assertFalse(self.NEW_XFORM1 in layerObjs)
        self.assertFalse(self.XFORM1_SPHERE1 in layerObjs)
        self.assertTrue(self.NEW_XFORM1_SPHERE1 in layerObjs)

        self._testLayerFromPath(self.XFORM1, None)
        self._testLayerFromPath(self.NEW_XFORM1, self.DEFAULT_LAYER)
        self._testLayerFromPath(self.XFORM1_SPHERE1, None)
        self._testLayerFromPath(self.NEW_XFORM1_SPHERE1, self.LAYER1)

    def testDisplayLayerItemReparent(self):
        # First create Display Layer and add some prims to it.
        cmds.createDisplayLayer(name=self.LAYER1, number=1, empty=True)
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/Xform1', 'Xform')
        stage.DefinePrim('/Cube1', 'Cube')
        stage.DefinePrim('/Sphere1', 'Sphere')

        # Add the two prims to the layer.
        cmds.editDisplayLayerMembers(self.LAYER1, self.SPHERE1, self.CUBE1, noRecurse=True)

        # Verify they are in layer.
        layerObjs = cmds.editDisplayLayerMembers(self.LAYER1, query=True, fn=True)
        self.assertTrue(self.CUBE1 in layerObjs)
        self.assertTrue(self.SPHERE1 in layerObjs)
        self.assertFalse(self.XFORM1 in layerObjs)

        self._testLayerFromPath(self.CUBE1, self.LAYER1)
        self._testLayerFromPath(self.SPHERE1, self.LAYER1)
        self._testLayerFromPath(self.XFORM1, self.DEFAULT_LAYER)

        # Reparent the Sphere and make sure it is still in the layer.
        cmds.parent(self.SPHERE1, self.XFORM1)
        layerObjs = cmds.editDisplayLayerMembers(self.LAYER1, query=True, fn=True)
        self.assertTrue(self.CUBE1 in layerObjs)
        self.assertTrue(self.XFORM1_SPHERE1 in layerObjs)
        self.assertFalse(self.XFORM1 in layerObjs)

        self._testLayerFromPath(self.CUBE1, self.LAYER1)
        self._testLayerFromPath(self.XFORM1_SPHERE1, self.LAYER1)
        self._testLayerFromPath(self.XFORM1, self.DEFAULT_LAYER)

        # Undo the reparent.
        cmds.undo()
        layerObjs = cmds.editDisplayLayerMembers(self.LAYER1, query=True, fn=True)
        self.assertTrue(self.CUBE1 in layerObjs)
        self.assertTrue(self.SPHERE1 in layerObjs)
        self.assertFalse(self.XFORM1 in layerObjs)

        self._testLayerFromPath(self.CUBE1, self.LAYER1)
        self._testLayerFromPath(self.SPHERE1, self.LAYER1)
        self._testLayerFromPath(self.XFORM1, self.DEFAULT_LAYER)

        # Redo the reparent.
        cmds.redo()
        layerObjs = cmds.editDisplayLayerMembers(self.LAYER1, query=True, fn=True)
        self.assertTrue(self.CUBE1 in layerObjs)
        self.assertTrue(self.XFORM1_SPHERE1 in layerObjs)
        self.assertFalse(self.XFORM1 in layerObjs)

        self._testLayerFromPath(self.CUBE1, self.LAYER1)
        self._testLayerFromPath(self.XFORM1_SPHERE1, self.LAYER1)
        self._testLayerFromPath(self.XFORM1, self.DEFAULT_LAYER)

    def testDisplayLayerItemDelete(self):
        # First create Display Layer and add some prims to it.
        cmds.createDisplayLayer(name=self.LAYER1, number=1, empty=True)
        defaultLayer = self.displayLayer(self.DEFAULT_LAYER)
        layer1 = self.displayLayer(self.LAYER1)
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/Cube1', 'Cube')
        stage.DefinePrim('/Sphere1', 'Sphere')

        # Add the two prims to the layer.
        cmds.editDisplayLayerMembers(self.LAYER1, self.SPHERE1, self.CUBE1, noRecurse=True)

        # Verify they are in layer.
        layerObjs = cmds.editDisplayLayerMembers(self.LAYER1, query=True, fn=True)
        self.assertTrue(self.CUBE1 in layerObjs)
        self.assertTrue(self.SPHERE1 in layerObjs)
        self.assertFalse(self.NEW_SPHERE1 in layerObjs)

        self._testLayerFromPath(self.CUBE1, self.LAYER1)
        self._testLayerFromPath(self.SPHERE1, self.LAYER1)

        # Delete the Sphere and make sure it is removed from the layer.
        cmds.delete(self.SPHERE1)
        layerObjs = cmds.editDisplayLayerMembers(self.LAYER1, query=True, fn=True)
        self.assertTrue(self.CUBE1 in layerObjs)
        self.assertFalse(self.SPHERE1 in layerObjs)

        self._testLayerFromPath(self.CUBE1, self.LAYER1)
        if mayaUtils.ufeSupportFixLevel() >= 2:
            # Bug in Maya code which incorrectly returned default layer for invalid ufe path.
            self.assertFalse(defaultLayer.contains(self.SPHERE1))
        self.assertFalse(layer1.contains(self.SPHERE1))

        # TODO - test the undo/redo for delete of Ufe item in Layer.
        # This currently doesn't work.

    def testDisplayLayerClear(self):
        cmdHelp = cmds.help('editDisplayLayerMembers')
        if '-clear' not in cmdHelp:
            self.skipTest('Requires clear flag in editDisplayLayerMembers command.')

        # First create Display Layer.
        cmds.createDisplayLayer(name=self.LAYER1, number=1, empty=True)
        layer1 = self.displayLayer(self.LAYER1)
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()

        # Add the one valid and one invalid prim to the layer.
        # Note: we cannot use the command to add an invalid prim since it verifies that a valid
        #       scene item can be created. Instead we use the Maya API to add it.
        # Note: you can get invalid prims thru different operations such as loading USD file
        #       or changing variants.
        stage.DefinePrim('/Cube1', 'Cube')
        cmds.editDisplayLayerMembers(self.LAYER1, self.CUBE1, noRecurse=True)
        layer1.add(self.INVALID_PRIM)

        # Verify that both prims are in layer.
        # Note: the editDisplayLayerMembers command only returns valid prims.
        #       But the MFnDisplayLayer will return all prims (including invalid ones).
        layerObjs = cmds.editDisplayLayerMembers(self.LAYER1, query=True, fn=True)
        self.assertTrue(self.CUBE1 in layerObjs)
        self.assertFalse(self.INVALID_PRIM in layerObjs)
        self._testLayerFromPath(self.CUBE1, self.LAYER1)
        self._testLayerFromPath(self.INVALID_PRIM, self.LAYER1)

        # Now clear the layer and make sure both prims (valid and invalid) got removed.
        cmds.editDisplayLayerMembers(self.LAYER1, clear=True)
        layerObjs = cmds.editDisplayLayerMembers(self.LAYER1, query=True, fn=True)
        self.assertIsNone(layerObjs)
        self.assertFalse(layer1.contains(self.CUBE1))
        self.assertFalse(layer1.contains(self.INVALID_PRIM))
