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

from pxr import Sdf, Usd
from maya import cmds
import mayaUsd
import fixturesUtils
import mayaUtils
import ufe


class AdskUsdEditForwardXformOpTestCase(unittest.TestCase):

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

    def _prepareStage(self):
        self._shapeNode = cmds.createNode('mayaUsdProxyShape')
        self._shapeNode = cmds.ls(self._shapeNode, long=True)[0]
        self._stage = mayaUsd.lib.GetPrim(self._shapeNode).GetStage()
        
        # Create a sublayer with a recognizable tag, that we will forward to.
        rootLayer = self._stage.GetRootLayer()
        self._sessionLayer = self._stage.GetSessionLayer()
        self._testLayer = Sdf.Layer.CreateAnonymous("TEST_layer")
        self.assertIsNotNone(self._testLayer, "Could not create test layer")

        rootLayer.subLayerPaths.append(self._testLayer.identifier)

        # Create a prim in the test layer that will be manipulated.
        self._stage.SetEditTarget(self._testLayer)
        self._primPath = Sdf.Path('/Xform1')
        self._stage.DefinePrim(self._primPath, 'Xform')
        prim = self._stage.GetPrimAtPath(self._primPath)
        self.assertTrue(prim.IsValid(), "Could not create prim")

        # Target the session layer because only this layer
        # is monitored by the edit forwarding.
        self._stage.SetEditTarget(self._sessionLayer)

        # Configure forwarding to target layers matching .*TEST.*
        # TODO : Would ideally use edit forwarding python API to configure the rule.
        # Update the test when these are available.
        customData = rootLayer.customLayerData
        customData['adsk_forward_continuously'] = True
        customData['adsk_forward_ruleset'] = {
            'rules': {
                'rule_0': {
                    'description': 'test rule',
                    'id': 'rule_0',
                    'input_path_regex': '.*',
                    'target_layer_regex': '.*TEST.*'
                }
            },
            'version': 1
        }
        rootLayer.customLayerData = customData
        
        primUfePathString = '|' + str(self._shapeNode) + ',' + str(self._primPath)
        primUfePath = ufe.PathString.path(primUfePathString)
        self.assertIsNotNone(primUfePath, "Could not create UFE path from string: {}".format(primUfePathString))
        primUfeSceneItem = ufe.SceneItem(primUfePath)
        self.assertIsNotNone(primUfeSceneItem, "Could not create UFE scene item from path: {}".format(primUfePathString))
        self._primUfe3d = ufe.Transform3d.transform3d(primUfeSceneItem)


    def _runEditForwarding(self, cmdFunc, xformOpPropPath):
        '''
        Run the provided command function that performs edits on the prim,
        then flush the idle queue to trigger continuous forwarding, and finally
        call the provided verify function to check the expected state after forwarding.
        '''
        def verifyDone():
            sessionLayerXformOp = self._sessionLayer.GetPropertyAtPath(xformOpPropPath)
            self.assertIsNone(sessionLayerXformOp, "Expected original session-layer edit to be forwarded")
            testLayerXformOp = self._testLayer.GetPropertyAtPath(xformOpPropPath)
            self.assertIsNotNone(testLayerXformOp, "Expected scale xformOp to be forwarded to TEST layer")
        
        def verifyUndone():
            # Note: the UFE command above was not added to the Maya undo queue,
            #       so it is not undone by the cmds.undo() below, only the forwarding command is undone.
            # sessionLayerXformOp = self._sessionLayer.GetPropertyAtPath(xformOpPropPath)
            # self.assertIsNone(sessionLayerXformOp, "Expected original session-layer edit to also be undone")
            testLayerXformOp = self._testLayer.GetPropertyAtPath(xformOpPropPath)
            self.assertIsNone(testLayerXformOp, "Expected prim to be removed from TEST layer after undo")


        # Translate the prim using UFE and onyl its set() function
        # which mimicks how Maya viewport manipulators work.
        cmdFunc()

        # Continuous forwarding happens on the next idle.
        cmds.flushIdleQueue()
        verifyDone()

        # Undo should revert both the forward command and the original session-layer
        # edit as a single operation (they are grouped in one Maya undo chunk).
        cmds.undo()
        verifyUndone()

        # Redo should restore the forwarded state.
        cmds.redo()
        verifyDone()


    #################################################################
    #
    # Tests for MayaXformStack USD transform stack style.


    def testEditForwardingTranslateSet(self):
        self._prepareStage()
        xformOpPropPath = self._primPath.AppendProperty('xformOp:translate')

        def cmdFunc():
            primUfeTranslateCmd = self._primUfe3d.translateCmd()
            self.assertIsNotNone(primUfeTranslateCmd, "Expected translateCmd to be available")
            primUfeTranslateCmd.set(1, 2, 3)

        self._runEditForwarding(cmdFunc, xformOpPropPath)


    def testEditForwardingTranslateExecute(self):
        self._prepareStage()
        xformOpPropPath = self._primPath.AppendProperty('xformOp:translate')

        def cmdFunc():
            primUfeTranslateCmd = self._primUfe3d.translateCmd(1, 2, 3)
            self.assertIsNotNone(primUfeTranslateCmd, "Expected translateCmd to be available")
            primUfeTranslateCmd.execute()

        self._runEditForwarding(cmdFunc, xformOpPropPath)


    def testEditForwardingScaleSet(self):
        self._prepareStage()
        xformOpPropPath = self._primPath.AppendProperty('xformOp:scale')

        def cmdFunc():
            primUfeScaleCmd = self._primUfe3d.scaleCmd()
            self.assertIsNotNone(primUfeScaleCmd, "Expected scaleCmd to be available")
            primUfeScaleCmd.set(1, 2, 3)

        self._runEditForwarding(cmdFunc, xformOpPropPath)


    def testEditForwardingScaleExecute(self):
        self._prepareStage()
        xformOpPropPath = self._primPath.AppendProperty('xformOp:scale')

        def cmdFunc():
            primUfeScaleCmd = self._primUfe3d.scaleCmd(1, 2, 3)
            self.assertIsNotNone(primUfeScaleCmd, "Expected scaleCmd to be available")
            primUfeScaleCmd.execute()

        self._runEditForwarding(cmdFunc, xformOpPropPath)


    def testEditForwardingRotateSet(self):
        self._prepareStage()
        xformOpPropPath = self._primPath.AppendProperty('xformOp:rotateXYZ')

        def cmdFunc():
            primUfeRotateCmd = self._primUfe3d.rotateCmd()
            primUfeRotateCmd.set(1, 2, 3)

        self._runEditForwarding(cmdFunc, xformOpPropPath)


    def testEditForwardingRotateExecute(self):
        self._prepareStage()
        xformOpPropPath = self._primPath.AppendProperty('xformOp:rotateXYZ')

        def cmdFunc():
            primUfeRotateCmd = self._primUfe3d.rotateCmd(1, 2, 3)
            self.assertIsNotNone(primUfeRotateCmd, "Expected rotateCmd to be available")
            primUfeRotateCmd.execute()

        self._runEditForwarding(cmdFunc, xformOpPropPath)


    #################################################################
    #
    # Tests for FallbackMayaXformStack USD transform stack style.


    def _createNonMayaXformStack(self):
        # Author a non-Maya-style xform stack with multiple scales.
        # Give them unique op-names so that they don't clash with standard ops.
        self._stage.GetPrimAtPath(self._primPath).CreateAttribute('xformOp:scale:first', Sdf.ValueTypeNames.Float3)
        self._stage.GetPrimAtPath(self._primPath).CreateAttribute('xformOp:rotateX:onlyX', Sdf.ValueTypeNames.Float)
        self._stage.GetPrimAtPath(self._primPath).CreateAttribute('xformOp:scale:second', Sdf.ValueTypeNames.Float3)

        xformOpOrderProp = self._stage.GetPrimAtPath(self._primPath).CreateAttribute('xformOpOrder', Sdf.ValueTypeNames.TokenArray)
        xformOpOrderProp.Set(['xformOp:scale:first', 'xformOp:rotateX:onlyX', 'xformOp:scale:second'])


    def testEditForwardingNonMayaTranslateSet(self):
        self._prepareStage()
        self._createNonMayaXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:translate')

        def cmdFunc():
            primUfeTranslateCmd = self._primUfe3d.translateCmd()
            self.assertIsNotNone(primUfeTranslateCmd, "Expected translateCmd to be available")
            primUfeTranslateCmd.set(1, 2, 3)

        self._runEditForwarding(cmdFunc, xformOpPropPath)


    def testEditForwardingNonMayaTranslateExecute(self):
        self._prepareStage()
        self._createNonMayaXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:translate')

        def cmdFunc():
            primUfeTranslateCmd = self._primUfe3d.translateCmd(1, 2, 3)
            self.assertIsNotNone(primUfeTranslateCmd, "Expected translateCmd to be available")
            primUfeTranslateCmd.execute()

        self._runEditForwarding(cmdFunc, xformOpPropPath)


    def testEditForwardingNonMayaScaleSet(self):
        self._prepareStage()
        self._createNonMayaXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:scale')

        def cmdFunc():
            primUfeScaleCmd = self._primUfe3d.scaleCmd()
            self.assertIsNotNone(primUfeScaleCmd, "Expected scaleCmd to be available")
            primUfeScaleCmd.set(1, 2, 3)

        self._runEditForwarding(cmdFunc, xformOpPropPath)


    def testEditForwardingNonMayaScaleExecute(self):
        self._prepareStage()
        self._createNonMayaXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:scale')

        def cmdFunc():
            primUfeScaleCmd = self._primUfe3d.scaleCmd(1, 2, 3)
            self.assertIsNotNone(primUfeScaleCmd, "Expected scaleCmd to be available")
            primUfeScaleCmd.execute()

        self._runEditForwarding(cmdFunc, xformOpPropPath)


    def testEditForwardingNonMayaRotateSet(self):
        self._prepareStage()
        self._createNonMayaXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:rotateXYZ')

        def cmdFunc():
            primUfeRotateCmd = self._primUfe3d.rotateCmd()
            self.assertIsNotNone(primUfeRotateCmd, "Expected rotateCmd to be available")
            primUfeRotateCmd.set(1, 2, 3)

        self._runEditForwarding(cmdFunc, xformOpPropPath)


    def testEditForwardingNonMayaRotateExecute(self):
        self._prepareStage()
        self._createNonMayaXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:rotateXYZ')

        def cmdFunc():
            primUfeRotateCmd = self._primUfe3d.rotateCmd(1, 2, 3)
            self.assertIsNotNone(primUfeRotateCmd, "Expected rotateCmd to be available")
            primUfeRotateCmd.execute()

        self._runEditForwarding(cmdFunc, xformOpPropPath)


    #################################################################
    #
    # Tests for Matrix USD transform stack style.


    def _createMatrixXformStack(self):
        # Author a non-Maya-style xform stack with multiple scales.
        # Give them unique op-names so that they don't clash with standard ops.
        self._stage.GetPrimAtPath(self._primPath).CreateAttribute('xformOp:transform', Sdf.ValueTypeNames.Matrix4d)

        xformOpOrderProp = self._stage.GetPrimAtPath(self._primPath).CreateAttribute('xformOpOrder', Sdf.ValueTypeNames.TokenArray)
        xformOpOrderProp.Set(['xformOp:transform'])


    def testEditForwardingMatrixTranslateSet(self):
        self._prepareStage()
        self._createMatrixXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:translate')

        def cmdFunc():
            primUfeTranslateCmd = self._primUfe3d.translateCmd()
            self.assertIsNotNone(primUfeTranslateCmd, "Expected translateCmd to be available")
            primUfeTranslateCmd.set(1, 2, 3)

        self._runEditForwarding(cmdFunc, xformOpPropPath)


    def testEditForwardingMatrixTranslateExecute(self):
        self._prepareStage()
        self._createMatrixXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:translate')

        def cmdFunc():
            primUfeTranslateCmd = self._primUfe3d.translateCmd(1, 2, 3)
            self.assertIsNotNone(primUfeTranslateCmd, "Expected translateCmd to be available")
            primUfeTranslateCmd.execute()

        self._runEditForwarding(cmdFunc, xformOpPropPath)


    def testEditForwardingMatrixScaleSet(self):
        self._prepareStage()
        self._createMatrixXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:scale')

        def cmdFunc():
            primUfeScaleCmd = self._primUfe3d.scaleCmd()
            self.assertIsNotNone(primUfeScaleCmd, "Expected scaleCmd to be available")
            primUfeScaleCmd.set(1, 2, 3)

        self._runEditForwarding(cmdFunc, xformOpPropPath)


    def testEditForwardingMatrixScaleExecute(self):
        self._prepareStage()
        self._createMatrixXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:scale')

        def cmdFunc():
            primUfeScaleCmd = self._primUfe3d.scaleCmd(1, 2, 3)
            self.assertIsNotNone(primUfeScaleCmd, "Expected scaleCmd to be available")
            primUfeScaleCmd.execute()

        self._runEditForwarding(cmdFunc, xformOpPropPath)


    def testEditForwardingMatrixRotateSet(self):
        self._prepareStage()
        self._createMatrixXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:rotateXYZ')

        def cmdFunc():
            primUfeRotateCmd = self._primUfe3d.rotateCmd()
            self.assertIsNotNone(primUfeRotateCmd, "Expected rotateCmd to be available")
            primUfeRotateCmd.set(1, 2, 3)

        self._runEditForwarding(cmdFunc, xformOpPropPath)


    def testEditForwardingMatrixRotateExecute(self):
        self._prepareStage()
        self._createMatrixXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:rotateXYZ')

        def cmdFunc():
            primUfeRotateCmd = self._primUfe3d.rotateCmd(1, 2, 3)
            self.assertIsNotNone(primUfeRotateCmd, "Expected rotateCmd to be available")
            primUfeRotateCmd.execute()

        self._runEditForwarding(cmdFunc, xformOpPropPath)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
    