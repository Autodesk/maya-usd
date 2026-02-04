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

import fixturesUtils
from testUtils import getTestScene

from maya import cmds
from maya import standalone

import mayaUtils
import mayaUsd
import mayaUsd_createStageWithNewLayer
import ufe

import usdUtils

from pxr import Usd, Sdf

import unittest

class SaveConnectedStageTest(unittest.TestCase):
    '''
    Test saving a stage that is connected to a prim.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)
        cls.inputPath = fixturesUtils.setUpClass(__file__)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testSaveLayerInMaya(self):
        '''
        The goal is to create stage and create a custom attribute on it that we connect
        to a normal Maya node. This should not make the proxy shape believe to have an
        incoming connection.
        '''
        # Create a stage.
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()

        # Add a prim in the root and in the sub-layer.
        with Usd.EditContext(stage, Usd.EditTarget(stage.GetRootLayer())):
            stage.DefinePrim('/InRoot', 'Sphere')

        # Add a maya cube
        cmds.polyCube(name='pCube1')[1]
        cubePathStr = '|pCube1|pCube1Shape'

        # Add a custom attribute on the proxy shape and cube and connect to the cube.
        attrLongName = "testBoolAttr"
        cmds.addAttr(psPathStr, ln=attrLongName, at="bool")
        cmds.addAttr(cubePathStr, ln=attrLongName, at="bool")
        cmds.connectAttr(f"{psPathStr}.testBoolAttr", f"{cubePathStr}.testBoolAttr")

        self.assertEqual(len(cmds.listConnections(psPathStr, t="shape", shapes=True)), 1)

        # Modify the prim in the session layer.
        with Usd.EditContext(stage, Usd.EditTarget(stage.GetSessionLayer())):
            ufePath = ufe.Path([
                mayaUtils.createUfePathSegment(psPathStr),
                usdUtils.createUfePathSegment('/InRoot')])
            ufeItem = ufe.Hierarchy.createItem(ufePath)

            globalSelection = ufe.GlobalSelection.get()
            globalSelection.clear()
            globalSelection.append(ufeItem)
            cmds.move(5, 0, 0, absolute=True)
            globalSelection.clear()
            ufePath = None
            ufeItem = None

        def verifyPrims(stage):
            self.assertIsNotNone(stage)
            inRoot = stage.GetPrimAtPath("/InRoot")
            self.assertTrue(inRoot)
            xformOp = inRoot.GetAttribute('xformOp:translate')
            self.assertTrue(xformOp)
            self.assertEqual(xformOp.Get(), (5.0, 0.0, 0.0))
            inRoot = None
            xformOp = None

        verifyPrims(stage)

        tempMayaFile = 'saveConnectedStageTest.ma'
        cmds.file(rename=tempMayaFile)
        cmds.file(save=True, force=True, type='mayaAscii')

        # Clear and reopen the file.
        stage = None
        cmds.file(new=True, force=True)
        cmds.file(tempMayaFile, open=True)
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()

        # Verify the two objects are still present and moved.
        verifyPrims(stage)


if __name__ == '__main__':
    unittest.main(verbosity=2)
