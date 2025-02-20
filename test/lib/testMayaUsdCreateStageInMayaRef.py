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

from maya import cmds
from maya import OpenMaya as om
from maya import standalone

import mayaUsd.lib
import mayaUsd.ufe
import mayaUsd_createStageWithNewLayer

from pxr import Sdf


# Note: for some reason, when Maya processes file path for Maya references,
#       it really does not like backward slashes on Windows. So make sure
#       all paths use forward slashes.

def createCubeUsdFile(root_path):
    cmds.file(new=True, force=True)
    cmds.polyCube(name="cube1")
    usd_cube_file = os.path.join(root_path, "cube.usd").replace('\\', '/')
    cmds.file(usd_cube_file, exportAll=True, force=True, type="USD Export")
    return usd_cube_file

def createMayaSceneWithStageWithUsdRef(root_path, usd_cube_file):
    # Create a Maya scene with a reference to the usd scene
    cmds.file(new=True, force=True)# Create USD stage
    proxy_shape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
    stage = mayaUsd.lib.GetPrim(proxy_shape).GetStage()

    # Add the reference
    usd_cube_path = "/usd_cube"
    ref_prim = stage.DefinePrim(usd_cube_path)
    ref_prim.GetReferences().AddReference(usd_cube_file)

    maya_with_usd_ref = os.path.join(root_path, "maya_with_usd_ref.ma").replace('\\', '/')
    # Save the scene
    SAVE_USD_TO_MAYA_SCENE = 2
    cmds.optionVar(iv=("mayaUsd_SerializedUsdEditsLocation", SAVE_USD_TO_MAYA_SCENE))
    om.MFileIO.saveAs(maya_with_usd_ref, "mayaAscii")
    return proxy_shape, usd_cube_path, maya_with_usd_ref

def createMayaSceneWithMayaRef(root_path, maya_with_usd_ref, namespace):
    # Reference the Maya scene in a new scene
    maya_with_maya_ref = os.path.join(root_path, "maya_with_maya_ref.ma").replace('\\', '/')
    cmds.file(new=True, force=True)
    ref_file = cmds.file(maya_with_usd_ref, reference=True, namespace=namespace)

def saveAndOpenMayaSceneWithMayaRef(root_path):
    maya_with_maya_ref = os.path.join(root_path, "maya_with_maya_ref.ma").replace('\\', '/')
    om.MFileIO.saveAs(maya_with_maya_ref, "mayaAscii")
    cmds.file(new=True, force=True)
    cmds.file(maya_with_maya_ref, open=True)


class MayaUsdCreateStageInMayaRefTestCase(unittest.TestCase):
    """
    Test reloading a Maya scene that contains a Maya reference which contain a USD stage.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testCreateStageInMayaRef(self):
        '''
        Test reloading a Maya scene that contains a Maya reference which contain a USD stage.
        '''
        testDir = os.path.join(os.path.abspath('.'),'testMayaUsdCreateStageInMayaRef')

        usd_cube_file = createCubeUsdFile(testDir)
        proxy_shape, usd_cube_path, maya_with_usd_ref = createMayaSceneWithStageWithUsdRef(testDir, usd_cube_file)
        namespace = "my_ref"
        createMayaSceneWithMayaRef(testDir, maya_with_usd_ref, namespace)

        # To debug the test, it can be useful to save the top Maya scene.
        # saveAndOpenMayaSceneWithMayaRef(testDir)

        proxy_shape_in_ref = proxy_shape.replace('|', '|%s:' % namespace)

        stage = mayaUsd.lib.GetPrim(proxy_shape_in_ref).GetStage()
        self.assertTrue(stage)
        self.assertTrue(Sdf.Layer.IsAnonymousLayerIdentifier(stage.GetRootLayer().identifier))
        self.assertTrue(Sdf.Layer.IsAnonymousLayerIdentifier(stage.GetSessionLayer().identifier))
        ref_prim = stage.GetPrimAtPath(usd_cube_path)
        self.assertTrue(ref_prim)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())