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
from pxr import Usd, Sdf, UsdUtils
import fixturesUtils
import os
import unittest
import mayaUsdAPI.lib as mayaUsdAPILib

class testMayaUsdAPIProxyStage(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)
        cls.usdFilePath = os.path.join(inputPath, 'StageTest','CubeModel.usda')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testProxyStage(self):
        '''
        Verify mayaUSDAPI lib ProxyStage methods
        '''
        cmds.file(new=True, force=True)

        #Create a mayaUsdProxyShape node and load a usd stage in it from a file.
        proxyShapeNodeName = cmds.createNode('mayaUsdProxyShape', name='UsdStageShape')
        cmds.setAttr(proxyShapeNodeName+ '.filePath', self.usdFilePath, type='string')
        #Connect time
        cmds.connectAttr('time1.outTime', proxyShapeNodeName+'.time')

        #Check mayaUsdAPILib.ProxyStage.getTime
        usdTime = mayaUsdAPILib.ProxyStage.getTime(proxyShapeNodeName)
        self.assertFalse(usdTime.IsDefault())
        self.assertEqual(usdTime.GetValue(), 1.0)

        # Set the current time to frame 10 and recheck mayaUsdAPILib.ProxyStage.getTime
        cmds.currentTime(10)
        usdTime = mayaUsdAPILib.ProxyStage.getTime(proxyShapeNodeName)
        self.assertFalse(usdTime.IsDefault())
        self.assertEqual(usdTime.GetValue(), 10.0)

        #Check mayaUsdAPILib.ProxyStage.getUsdStage
        stage = mayaUsdAPILib.ProxyStage.getUsdStage(proxyShapeNodeName)
        originalRootIdentifier = stage.GetRootLayer().identifier
        self.assertNotEqual(originalRootIdentifier, "")
        prim = stage.GetPrimAtPath('/CubeModel')
        self.assertTrue(prim.IsValid())
    
if __name__ == '__main__':
    unittest.main(verbosity=2)
