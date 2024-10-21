#!/usr/bin/env python

#
# Copyright 2024 Autodesk
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
import os.path

import fixturesUtils
from ufeUtils import ufeFeatureSetVersion

from maya import cmds
from maya import OpenMaya as om
from maya import standalone
import maya.mel as mel

import mayaUsd.lib
import mayaUsd.ufe
import mayaUsd_createStageWithNewLayer


class MayaUsdCreateStageInMayaRefTestCase(unittest.TestCase):
    """
    Test reloading a Maya scene that contains a Maya reference which contain a USD stage.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)
        cmds.loadPlugin('mayaUsdPlugin')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testCreateStageInMayaRef(self):
        '''
        Test reloading a Maya scene that contains a Maya reference which contain a USD stage.
        '''
        testDir = os.path.join(os.path.abspath('.'),'testMayaUsdCreateStageInMayaRef')

        # Note: for some reason, when Maya processes file path for Maya references,
        #       it really does not like backward slashes on Windows. So make sure
        #       all paths use forward slashes.
        usd_cube_scene = os.path.join(testDir, "cube.usd").replace('\\', '/')
        maya_with_usd_ref = os.path.join(testDir, "maya_with_usd_ref.ma").replace('\\', '/')
        maya_with_maya_ref = os.path.join(testDir, "maya_with_maya_ref.ma").replace('\\', '/')

        # Create a USD scene with a cube
        cmds.file(new=True, force=True)
        cmds.polyCube(name="cube1")
        USD_EXPORT_SETTING = 1
        cmds.optionVar(iv=("mayaUsd_SerializedUsdEditsLocation", USD_EXPORT_SETTING))
        cmds.file(usd_cube_scene, exportAll=True, force=True, type="USD Export")

        # Create a Maya scene with an empty USD stage 
        cmds.file(new=True, force=True)
        proxy_shape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(proxy_shape).GetStage()
        self.assertTrue(stage)

        # Add a reference to the usd scene with the cube
        usdCubePath = "/usd_cube"
        ref_prim = stage.DefinePrim(usdCubePath)
        self.assertTrue(ref_prim)
        ref_prim.GetReferences().AddReference(usd_cube_scene)

        # Save the scene
        cmds.optionVar(iv=("mayaUsd_SerializedUsdEditsLocation", USD_EXPORT_SETTING))
        om.MFileIO.saveAs(maya_with_usd_ref, "mayaAscii")

        # Make sure we don't retain references to USD data.
        stage = None
        ref_prim = None

        # Reference the Maya scene in a new scene
        namespace = 'my_ref'
        cmds.file(new=True, force=True)
        cmds.file(maya_with_usd_ref, reference=True, namespace=namespace)
        om.MFileIO.saveAs(maya_with_maya_ref, "mayaAscii")
        
        cmds.file(new=True, force=True)
        cmds.file(maya_with_maya_ref, open=True)

        proxy_shape_in_ref = proxy_shape.replace('|', '|%s:' % namespace)

        stage = mayaUsd.lib.GetPrim(proxy_shape_in_ref).GetStage()
        self.assertTrue(stage)
        ref_prim = stage.DefinePrim(usdCubePath)
        self.assertTrue(ref_prim)
