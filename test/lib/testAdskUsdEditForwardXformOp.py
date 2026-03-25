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


    def _runEditForwarding(self, cmdFunc, xformOpPropPath, is_using_execute: bool, had_xform_already=False):
        '''
        Run the provided command function that performs edits on the prim,
        then flush the idle queue to trigger continuous forwarding, and finally
        call the provided verify function to check the expected state after forwarding.
        '''
        def verifyDone():
            self.assertIsNotNone(self._testLayer.GetPrimAtPath(self._primPath))
            sessionLayerXformOp = self._sessionLayer.GetPropertyAtPath(xformOpPropPath)
            self.assertIsNone(sessionLayerXformOp, "Expected original session-layer edit to be forwarded")
            testLayerXformOp = self._testLayer.GetPropertyAtPath(xformOpPropPath)
            self.assertIsNotNone(testLayerXformOp, "Expected scale xformOp to be forwarded to TEST layer")
        
        def verifyUndone(had_xform_already):
            self.assertIsNotNone(self._testLayer.GetPrimAtPath(self._primPath))
            # Note: the UFE command above was not added to the Maya undo queue,
            #       so it is not undone by the cmds.undo() below, only the forwarding command is undone.
            # sessionLayerXformOp = self._sessionLayer.GetPropertyAtPath(xformOpPropPath)
            # self.assertIsNone(sessionLayerXformOp, "Expected original session-layer edit to also be undone")
            testLayerXformOp = self._testLayer.GetPropertyAtPath(xformOpPropPath)
            if had_xform_already:
                self.assertIsNotNone(testLayerXformOp, "Expected property to be kept in TEST layer after undo")
            else:
                self.assertIsNone(testLayerXformOp, "Expected property to be removed from TEST layer after undo")


        # Translate the prim using UFE and onyl its set() function
        # which mimicks how Maya viewport manipulators work.
        ufeCmds = cmdFunc()
        verifyDone()

        # If the UFE command is using execute(), then the edit-forwarding will use
        # a Maya command (undo chunk), so we must call cmds. If the UFE command is
        # using set(), then we only undo the UFE command (as Maya would have done)
        # a the edit-forwarding will be done *not* in a command, so we must *not*
        # call cmds.undo.
        if is_using_execute:
            ufeCmds[1].undo()
        else:
            ufeCmds[1].set(1, 2, 3)
        cmds.flushIdleQueue()
        # Note: the was a UFE command before this one that also set the value,
        #       so after undo there is *still* a value that was forwarded by that
        #       first command, so we call verifyDone() here even though we just undid
        #       the second command, because the first command is still in effect.
        verifyDone()

        # For the very first edit, the command always uses a UsdUndoBlock to capture
        # the creation of the xformOp, so the edit-forwarding will always use a Maya
        # command for the forwarding, so we must call cmds.undo to undo the forwarding,
        if had_xform_already:
            ufeCmds[0].undo()
        else:
            cmds.undo()
            ufeCmds[0].undo()
        cmds.flushIdleQueue()
        verifyUndone(had_xform_already)

        # Redo should restore the forwarded state.

        # For The first redo, The first command is always creating the xformOp,
        # so we always call cmds.redo, but if using set(), we must also call
        # the redo of the UFE command.
        ufeCmds[0].redo()
        if not had_xform_already:
            cmds.redo()
        cmds.flushIdleQueue()
        verifyDone()

        # For the second redo, if using execute(), then the edit forwarding will
        # be done in a Maya command, so we must call cmds.redo, but if using set(),
        # then the edit-forwarding will be done *not* in a command, so we must *not*
        # call cmds.redo.
        ufeCmds[1].redo()
        cmds.flushIdleQueue()
        verifyDone()


    #################################################################
    #
    # Tests for MayaXformStack USD transform stack style.


    def testEditForwardingTranslateSet(self):
        self._prepareStage()
        xformOpPropPath = self._primPath.AppendProperty('xformOp:translate')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.translateCmd()
                self.assertIsNotNone(cmd, "Expected translateCmd to be available")
                cmd.set(i * 1, i * 2, i * 3)
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=False)


    def testEditForwardingTranslateExecute(self):
        self._prepareStage()
        xformOpPropPath = self._primPath.AppendProperty('xformOp:translate')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.translateCmd(i * 1, i * 2, i * 3)
                self.assertIsNotNone(cmd, "Expected translateCmd to be available")
                cmd.execute()
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=True)


    def testEditForwardingScaleSet(self):
        self._prepareStage()
        xformOpPropPath = self._primPath.AppendProperty('xformOp:scale')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.scaleCmd()
                self.assertIsNotNone(cmd, "Expected scaleCmd to be available")
                cmd.set(i * 1, i * 2, i * 3)
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=False)


    def testEditForwardingScaleExecute(self):
        self._prepareStage()
        xformOpPropPath = self._primPath.AppendProperty('xformOp:scale')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.scaleCmd(i * 1, i * 2, i * 3)
                self.assertIsNotNone(cmd, "Expected scaleCmd to be available")
                cmd.execute()
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=True)


    def testEditForwardingRotateSet(self):
        self._prepareStage()
        xformOpPropPath = self._primPath.AppendProperty('xformOp:rotateXYZ')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.rotateCmd()
                self.assertIsNotNone(cmd, "Expected rotateCmd to be available")
                cmd.set(i * 1, i * 2, i * 3)
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=False)


    def testEditForwardingRotateExecute(self):
        self._prepareStage()
        xformOpPropPath = self._primPath.AppendProperty('xformOp:rotateXYZ')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.rotateCmd(i * 1, i * 2, i * 3)
                self.assertIsNotNone(cmd, "Expected rotateCmd to be available")
                cmd.execute()
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=True)


    #################################################################
    #
    # Tests for FallbackMayaXformStack USD transform stack style.


    def _createNonMayaXformStack(self):
        # Author a non-Maya-style xform stack with multiple scales.
        # Give them unique op-names so that they don't clash with standard ops.
        self._stage.SetEditTarget(self._testLayer)
        self._stage.GetPrimAtPath(self._primPath).CreateAttribute('xformOp:scale:first', Sdf.ValueTypeNames.Float3)
        self._stage.GetPrimAtPath(self._primPath).CreateAttribute('xformOp:rotateX:onlyX', Sdf.ValueTypeNames.Float)
        self._stage.GetPrimAtPath(self._primPath).CreateAttribute('xformOp:scale:second', Sdf.ValueTypeNames.Float3)

        xformOpOrderProp = self._stage.GetPrimAtPath(self._primPath).CreateAttribute('xformOpOrder', Sdf.ValueTypeNames.TokenArray)
        xformOpOrderProp.Set(['xformOp:scale:first', 'xformOp:rotateX:onlyX', 'xformOp:scale:second'])
        self._stage.SetEditTarget(self._sessionLayer)


    def testEditForwardingNonMayaTranslateSet(self):
        self._prepareStage()
        self._createNonMayaXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:translate')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.translateCmd()
                self.assertIsNotNone(cmd, "Expected translateCmd to be available")
                cmd.set(i * 1, i * 2, i * 3)
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=False, had_xform_already=True)


    def testEditForwardingNonMayaTranslateExecute(self):
        self._prepareStage()
        self._createNonMayaXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:translate')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.translateCmd(i * 1, i * 2, i * 3)
                self.assertIsNotNone(cmd, "Expected translateCmd to be available")
                cmd.execute()
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=True, had_xform_already=True)


    def testEditForwardingNonMayaScaleSet(self):
        self._prepareStage()
        self._createNonMayaXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:scale')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.scaleCmd()
                self.assertIsNotNone(cmd, "Expected scaleCmd to be available")
                cmd.set(i * 1, i * 2, i * 3)
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=False, had_xform_already=True)


    def testEditForwardingNonMayaScaleExecute(self):
        self._prepareStage()
        self._createNonMayaXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:scale')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.scaleCmd(i * 1, i * 2, i * 3)
                self.assertIsNotNone(cmd, "Expected scaleCmd to be available")
                cmd.execute()
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=True, had_xform_already=True)


    def testEditForwardingNonMayaRotateSet(self):
        self._prepareStage()
        self._createNonMayaXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:rotateXYZ')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.rotateCmd()
                self.assertIsNotNone(cmd, "Expected rotateCmd to be available")
                cmd.set(i * 1, i * 2, i * 3)
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=False, had_xform_already=True)


    def testEditForwardingNonMayaRotateExecute(self):
        self._prepareStage()
        self._createNonMayaXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:rotateXYZ')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.rotateCmd(i * 1, i * 2, i * 3)
                self.assertIsNotNone(cmd, "Expected rotateCmd to be available")
                cmd.execute()
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=True, had_xform_already=True)


    #################################################################
    #
    # Tests for Matrix USD transform stack style.


    def _createMatrixXformStack(self):
        # Author a non-Maya-style xform stack with multiple scales.
        # Give them unique op-names so that they don't clash with standard ops.
        self._stage.SetEditTarget(self._testLayer)
        self._stage.GetPrimAtPath(self._primPath).CreateAttribute('xformOp:transform', Sdf.ValueTypeNames.Matrix4d)

        xformOpOrderProp = self._stage.GetPrimAtPath(self._primPath).CreateAttribute('xformOpOrder', Sdf.ValueTypeNames.TokenArray)
        xformOpOrderProp.Set(['xformOp:transform'])
        self._stage.SetEditTarget(self._sessionLayer)


    def testEditForwardingMatrixTranslateSet(self):
        self._prepareStage()
        self._createMatrixXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:translate')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.translateCmd()
                self.assertIsNotNone(cmd, "Expected translateCmd to be available")
                cmd.set(i * 1, i * 2, i * 3)
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=False, had_xform_already=True)


    def testEditForwardingMatrixTranslateExecute(self):
        self._prepareStage()
        self._createMatrixXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:translate')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.translateCmd(i * 1, i * 2, i * 3)
                self.assertIsNotNone(cmd, "Expected translateCmd to be available")
                cmd.execute()
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=True, had_xform_already=True)


    def testEditForwardingMatrixScaleSet(self):
        self._prepareStage()
        self._createMatrixXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:scale')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.scaleCmd()
                self.assertIsNotNone(cmd, "Expected scaleCmd to be available")
                cmd.set(i * 1, i * 2, i * 3)
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=False, had_xform_already=True)


    def testEditForwardingMatrixScaleExecute(self):
        self._prepareStage()
        self._createMatrixXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:scale')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.scaleCmd(i * 1, i * 2, i * 3)
                self.assertIsNotNone(cmd, "Expected scaleCmd to be available")
                cmd.execute()
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=True, had_xform_already=True)


    def testEditForwardingMatrixRotateSet(self):
        self._prepareStage()
        self._createMatrixXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:rotateXYZ')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.rotateCmd()
                self.assertIsNotNone(cmd, "Expected rotateCmd to be available")
                cmd.set(i * 1, i * 2, i * 3)
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=False, had_xform_already=True)


    def testEditForwardingMatrixRotateExecute(self):
        self._prepareStage()
        self._createMatrixXformStack()

        xformOpPropPath = self._primPath.AppendProperty('xformOp:rotateXYZ')

        def cmdFunc():
            ufeCmds = []
            for i in range(2):
                cmd = self._primUfe3d.rotateCmd(i * 1, i * 2, i * 3)
                self.assertIsNotNone(cmd, "Expected rotateCmd to be available")
                cmd.execute()
                ufeCmds.append(cmd)
                # Continuous forwarding happens on the next idle.
                cmds.flushIdleQueue()
            return ufeCmds

        self._runEditForwarding(cmdFunc, xformOpPropPath, is_using_execute=True, had_xform_already=True)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
    