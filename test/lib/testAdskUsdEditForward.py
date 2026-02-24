#!/usr/bin/env python

#
# Copyright 2026 Autodesk
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
import os
import time

from pxr import Sdf, Usd
from maya import cmds
import maya.utils
import mayaUsd
import fixturesUtils

class AdskUsdEditForwardTestCase(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)
        # Ensure the idle queue is running so that MGlobal::executeTaskOnIdle
        # callbacks fire when cmds.flushIdleQueue() is called (needed on Linux).
        cmds.flushIdleQueue(resume=True)
                        
    def setUp(self):
        cmds.file(new=True, force=True)

    def testEditForwarding(self):
        if not cmds.pluginInfo('mayaUsdPlugin', query=True, loaded=True):
            cmds.loadPlugin('mayaUsdPlugin')

        shapeNode = cmds.createNode('mayaUsdProxyShape')
        stage = mayaUsd.lib.GetPrim(shapeNode).GetStage()
        
        # Create a sublayer with a recognizable tag, that we will forward to.
        rootLayer = stage.GetRootLayer()
        sessionLayer = stage.GetSessionLayer()
        testLayer = Sdf.Layer.CreateAnonymous("TEST_layer")
        self.assertIsNotNone(testLayer, "Could not create test layer")

        rootLayer.subLayerPaths.append(testLayer.identifier)

        # Configure forwarding to target layers matching .*TEST.*
        # TODO : Would ideally use edit forwarding python API to configure the rule.
        # Update the test when these are available.
        customData = rootLayer.customLayerData
        customData['adsk_forward_continuously'] = True
        customData['adsk_forward_rules'] = {
            'paths': {
                'rule_0': {
                    'description': 'test rule',
                    'id': 'rule_0',
                    'input_path_regex': '.*',
                    'target_layer_regex': '.*TEST.*'
                }
            }
        }
        rootLayer.customLayerData = customData

        # Create a prim on the session layer, should be forwarded to the TEST layer.
        primPath = Sdf.Path('/TestPrim')
        with Usd.EditContext(stage, sessionLayer):
            prim = stage.DefinePrim(primPath, 'Xform')
            self.assertTrue(prim.IsValid(), "Could not create prim")

        # Continuous forwarding happens on the next idle.
        cmds.flushIdleQueue()
        
        testLayerHasPrim = testLayer.GetPrimAtPath(primPath) is not None
        
        if not testLayerHasPrim:
            forceTest = os.environ.get('MAYAUSD_FORCE_EF_TEST', '0') == '1'
            if forceTest:
                self.fail("Edit forwarding did not work but MAYAUSD_FORCE_EF_TEST is set")
            else:
                self.skipTest('Edit forwarding functionality not available (set MAYAUSD_FORCE_EF_TEST=1 to fail)')

        self.assertTrue(testLayerHasPrim, "Expected prim to be forwarded to TEST layer")

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
    