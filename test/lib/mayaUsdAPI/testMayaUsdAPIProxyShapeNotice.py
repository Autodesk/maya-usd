#!/usr/bin/env mayapy
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

from maya import cmds
from maya import standalone
from pxr import Usd, Sdf, UsdUtils, UsdGeom, Tf
import fixturesUtils
import os
import unittest
import mayaUsdAPI.lib as mayaUsdAPILib

class testMayaUsdAPIProxyShapeNotice(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)
        cls.usdFilePath = os.path.join(inputPath, 'StageTest','CubeModel.usda')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testProxyShapeNotices(self):
        '''
        Verify mayaUSDAPI lib ProxyShapeNotice classes
        '''
        cmds.file(new=True, force=True)

        #Reset the calls from any previous stage notices
        mayaUsdAPILib.resetStageSet()
        mayaUsdAPILib.resetStageInvalidated()
        mayaUsdAPILib.resetStageObjectsChanged()
        self.assertFalse(mayaUsdAPILib.stageSet())
        self.assertFalse(mayaUsdAPILib.stageInvalidated())
        self.assertFalse(mayaUsdAPILib.stageObjectsChanged())
        
        #create a proxy shape node and load a stage
        proxyShapeNodeName = cmds.createNode('mayaUsdProxyShape', name='UsdStageShape')
        cmds.setAttr(proxyShapeNodeName+ '.filePath', self.usdFilePath, type='string')
        cmds.connectAttr('time1.outTime', proxyShapeNodeName+'.time')
        
        #Check that the stage was correctly loaded
        stage = mayaUsdAPILib.ProxyStage.getUsdStage(proxyShapeNodeName)
        originalRootIdentifier = stage.GetRootLayer().identifier
        self.assertNotEqual(originalRootIdentifier, "")
        prim = stage.GetPrimAtPath('/CubeModel')
        self.assertTrue(prim.IsValid())

        #verify that the stage notices callbakcs were called
        self.assertTrue(mayaUsdAPILib.stageSet())
        self.assertTrue(mayaUsdAPILib.stageInvalidated())

        #For StageObjectsChangedHasBeenCalled we need to modify the stage, so add a new Prim
        stage.DefinePrim("/root/thisNodeShouldTriggerAStageObjectsChanged")
        self.assertTrue(mayaUsdAPILib.stageObjectsChanged())

        mayaUsdAPILib.resetStageSet()
        mayaUsdAPILib.resetStageInvalidated()
        mayaUsdAPILib.resetStageObjectsChanged()

if __name__ == '__main__':
    unittest.main(verbosity=2)
