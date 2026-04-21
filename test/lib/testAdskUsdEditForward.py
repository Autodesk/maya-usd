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
import AdskUsdEditForward
from maya import cmds
import maya.utils
import mayaUsd
import fixturesUtils
import mayaUtils
import ufe
from maya.internal.ufeSupport import ufeCmdWrapper as ufeCmd

class AdskUsdEditForwardTestCase(unittest.TestCase):

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)
        # Ensure the idle queue is running so that MGlobal::executeTaskOnIdle
        # callbacks fire when cmds.flushIdleQueue() is called (needed on Linux).
        cmds.flushIdleQueue(resume=True)
        # Ensure the queue is clear before starting the test so that undo stack is predictable,
        # some idle jobs can append commands on the stack.
        cmds.flushIdleQueue()
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()
                        
    def setUp(self):
        cmds.file(new=True, force=True)

    def testEditForwarding(self):
        shapeNode = cmds.createNode('mayaUsdProxyShape')
        stage = mayaUsd.lib.GetPrim(shapeNode).GetStage()
        
        # Create a sublayer with a recognizable tag, that we will forward to.
        rootLayer = stage.GetRootLayer()
        sessionLayer = stage.GetSessionLayer()
        testLayer = Sdf.Layer.CreateAnonymous("TEST_layer")
        self.assertIsNotNone(testLayer, "Could not create test layer")

        rootLayer.subLayerPaths.append(testLayer.identifier)

        # Configure forwarding to target layers matching .*TEST.*
        rule_def = AdskUsdEditForward.RuleDef()
        rule_def.id = 'rule_0'
        rule_def.description = 'test rule'
        rule_def.input_object_expression = AdskUsdEditForward.RuleExpression('.*')
        rule_def.target_layer_expression = AdskUsdEditForward.RuleExpression('.*TEST.*')

        rule_set = AdskUsdEditForward.RuleSet([rule_def])
        rule_set.continuous = True
        AdskUsdEditForward.RuleDef.WriteRuleSetToLayerCustomData(rootLayer, rule_set)

        # Create a prim on the session layer via a UFE undoable command, so that
        # cmds.undo() can revert it along with the forward command as one unit.
        stage.SetEditTarget(sessionLayer)
        shapeNodePath = cmds.ls(shapeNode, long=True)[0]
        shapeItem = ufe.Hierarchy.createItem(ufe.PathString.path(shapeNodePath))
        
        # Use the context op as the add prim command is not exposed in python.
        contextOps = ufe.ContextOps.contextOps(shapeItem)
        ufeCmd.execute(contextOps.doOpCmd(['Add New Prim', 'Xform']))

        primPath = Sdf.Path('/Xform1')
        prim = stage.GetPrimAtPath(primPath)
        self.assertTrue(prim.IsValid(), "Could not create prim")

        # Continuous forwarding happens on the next idle.
        cmds.flushIdleQueue()
        
        testLayerHasPrim = testLayer.GetPrimAtPath(primPath) is not None
        self.assertTrue(testLayerHasPrim, "Expected prim to be forwarded to TEST layer")
        
        # Undo should revert both the forward command and the original session-layer
        # edit as a single operation (they are grouped in one Maya undo chunk).
        cmds.undo()
        
        self.assertIsNone(
            testLayer.GetPrimAtPath(primPath),
            "Expected prim to be removed from TEST layer after undo")
        self.assertIsNone(
            sessionLayer.GetPrimAtPath(primPath),
            "Expected original session-layer edit to also be undone")

        # Redo should restore the forwarded state.
        cmds.redo()
        
        self.assertIsNotNone(
            testLayer.GetPrimAtPath(primPath),
            "Expected prim to be forwarded back to TEST layer after redo")
        self.assertIsNone(
            sessionLayer.GetPrimAtPath(primPath),
            "Expected prim to have been moved out of session layer after redo")
        
        # Create a prim on the session layer outside of a command, nothing should happen.
        primPath = Sdf.Path('/TestPrim')
        with Usd.EditContext(stage, sessionLayer):
            prim = stage.DefinePrim(primPath, 'Xform')
            self.assertTrue(prim.IsValid(), "Could not create prim")
        
        cmds.flushIdleQueue()
        
        self.assertIsNotNone(
            sessionLayer.GetPrimAtPath(primPath),
            "Expected prim to stay on the session layer if outside a command.")
        self.assertIsNone(
            testLayer.GetPrimAtPath(primPath),
            "Expected prim to stay on the session layer if outside a command.")
        

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
    