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

import unittest

from pxr import Sdf, Usd
import AdskUsdEditForward
from maya import cmds
import mayaUsd
import fixturesUtils
import mayaUtils
import ufe
from maya.internal.ufeSupport import ufeCmdWrapper as ufeCmd


class EditForwardLayerEditorTestCase(unittest.TestCase):

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)
        cmds.flushIdleQueue(resume=True)
        cmds.flushIdleQueue()
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    def setUp(self):
        cmds.file(new=True, force=True)
        # Don't prompt for where to save USD edits (the modal dialog blocks the tests).
        cmds.optionVar(intValue=('mayaUsd_SerializedUsdEditsLocationPrompt', 0))

    def _createStageWithSublayers(self):
        """Create a proxy shape with two named sublayers."""
        shapeNode = cmds.createNode('mayaUsdProxyShape')
        shapeNodePath = cmds.ls(shapeNode, long=True)[0]
        stage = mayaUsd.lib.GetPrim(shapeNode).GetStage()
        layerA = Sdf.Layer.CreateAnonymous("TARGET_layerA")
        layerB = Sdf.Layer.CreateAnonymous("TARGET_layerB")
        stage.GetRootLayer().subLayerPaths.append(layerA.identifier)
        stage.GetRootLayer().subLayerPaths.append(layerB.identifier)
        return shapeNodePath, stage, layerA, layerB

    def _writeRules(self, rootLayer, continuous=True, inputExpr='.*'):
        """Write an EF rule targeting TARGET_layerA """
        rule = AdskUsdEditForward.RuleDef()
        rule.id = 'test_rule'
        rule.input_object_expression = AdskUsdEditForward.RuleExpression(inputExpr)
        rule.target_layer_expression = AdskUsdEditForward.RuleExpression('.*TARGET_layerA.*')
        rule_set = AdskUsdEditForward.RuleSet([rule])
        rule_set.continuous = continuous
        AdskUsdEditForward.RuleDef.WriteRuleSetToLayerCustomData(rootLayer, rule_set)

    def _clearRules(self, rootLayer):
        """Remove all authored EF rules from the root layer."""
        AdskUsdEditForward.RuleDef.WriteRuleSetToLayerCustomData(
            rootLayer, AdskUsdEditForward.RuleSet([]))

    def testEFModeEditTargetIsSessionLayer(self):
        """In EF mode the actual stage edit target (USD API) is the session layer,
        while the command query reports the EF fallback target."""
        shapeNodePath, stage, layerA, layerB = self._createStageWithSublayers()

        # Set an explicit edit target so it becomes the fallback when EF activates.
        cmds.mayaUsdEditTarget(shapeNodePath, edit=True, editTarget=layerA.identifier)
        self._writeRules(stage.GetRootLayer())

        self.assertEqual(
            stage.GetEditTarget().GetLayer(),
            stage.GetSessionLayer(),
            "In EF mode the actual stage edit target must be the session layer")
        self.assertEqual(
            cmds.mayaUsdEditTarget(shapeNodePath, query=True, editTarget=True)[0],
            layerA.identifier,
            "In EF mode the queried edit target must be the EF fallback layer")

    def testAutoActivation(self):
        """Writing continuous EF rules forces the edit target to the session layer, activating edit forwarding."""
        shapeNodePath, stage, layerA, layerB = self._createStageWithSublayers()

        cmds.mayaUsdEditTarget(shapeNodePath, edit=True, editTarget=layerA.identifier)
        self.assertEqual(stage.GetEditTarget().GetLayer(), layerA)

        self._writeRules(stage.GetRootLayer())
        self.assertEqual(
            stage.GetEditTarget().GetLayer(),
            stage.GetSessionLayer(),
            "EF mode should force edit target to session layer when continuous rules exist")
        self.assertEqual(
            cmds.mayaUsdEditTarget(shapeNodePath, query=True, editTarget=True)[0],
            layerA.identifier)

    def testAutoDeactivation(self):
        """Clearing EF rules restores the last fallback target."""
        shapeNodePath, stage, layerA, layerB = self._createStageWithSublayers()

        cmds.mayaUsdEditTarget(shapeNodePath, edit=True, editTarget=layerA.identifier)
        self._writeRules(stage.GetRootLayer())
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetSessionLayer())

        self._clearRules(stage.GetRootLayer())
        self.assertEqual(
            stage.GetEditTarget().GetLayer(),
            layerA,
            "Clearing rules should restore the last fallback target")
        self.assertEqual(
            cmds.mayaUsdEditTarget(shapeNodePath, query=True, editTarget=True)[0],
            layerA.identifier)

    def testContinuousToggleOff(self):
        """Rewriting rules with continuous=False exits EF mode and restores the edit target."""
        shapeNodePath, stage, layerA, layerB = self._createStageWithSublayers()

        cmds.mayaUsdEditTarget(shapeNodePath, edit=True, editTarget=layerA.identifier)
        self._writeRules(stage.GetRootLayer(), continuous=True)
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetSessionLayer())

        self._writeRules(stage.GetRootLayer(), continuous=False)
        self.assertEqual(
            stage.GetEditTarget().GetLayer(),
            layerA,
            "Disabling continuous mode should exit EF mode and restore the last fallback target")

    def testContinuousToggleOnAgain(self):
        """Re-enabling continuous=True after it was off re-enters EF mode automatically."""
        shapeNodePath, stage, layerA, layerB = self._createStageWithSublayers()

        cmds.mayaUsdEditTarget(shapeNodePath, edit=True, editTarget=layerA.identifier)
        self._writeRules(stage.GetRootLayer(), continuous=True)
        self._writeRules(stage.GetRootLayer(), continuous=False)
        self.assertEqual(stage.GetEditTarget().GetLayer(), layerA)
        self._writeRules(stage.GetRootLayer(), continuous=True)

        self.assertEqual(
            stage.GetEditTarget().GetLayer(),
            stage.GetSessionLayer(),
            "Re-enabling continuous mode should re-enter EF mode automatically")

    def testExternalEditTargetChange(self):
        """Changing the edit target externally while EF is on exits EF mode; new target stands."""
        shapeNodePath, stage, layerA, layerB = self._createStageWithSublayers()

        cmds.mayaUsdEditTarget(shapeNodePath, edit=True, editTarget=layerA.identifier)
        self._writeRules(stage.GetRootLayer())
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetSessionLayer())

        # Bypass the layer editor command to simulate an external change.
        stage.SetEditTarget(Usd.EditTarget(layerB))

        self.assertEqual(
            stage.GetEditTarget().GetLayer(),
            layerB,
            "External edit target change should exit EF mode; the new target should stand")
        self.assertEqual(
            cmds.mayaUsdEditTarget(shapeNodePath, query=True, editTarget=True)[0],
            layerB.identifier)

    def testEFTargetRouting(self):
        """In EF mode mayaUsdEditTarget routes to the fallback target, not the stage target."""
        shapeNodePath, stage, layerA, layerB = self._createStageWithSublayers()

        # Use a rule whose input expression does not match the prim created below
        # (/Xform1). EF still activates because a continuous rule exists, but the
        # prim falls through to the fallback target rather than being captured by
        # this rule — which is what lets us verify fallback routing.
        self._writeRules(stage.GetRootLayer(), inputExpr='/NoMatch.*')
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetSessionLayer())

        # In EF mode mayaUsdEditTarget must NOT change the stage edit target.
        # It should silently route to the provider's fallback target instead.
        cmds.mayaUsdEditTarget(shapeNodePath, edit=True, editTarget=layerB.identifier)

        # Stage edit target must still be session layer — the command must not have
        # moved it to layerB as it would in non-EF mode.
        self.assertEqual(
            stage.GetEditTarget().GetLayer(),
            stage.GetSessionLayer(),
            "In EF mode, mayaUsdEditTarget must not change the stage edit target")

        # Querying the target via the layer editor command returns the EF fallback (layerB), the user-facing target.
        currentTarget = cmds.mayaUsdEditTarget(shapeNodePath, query=True, editTarget=True)[0]
        self.assertEqual(
            currentTarget,
            layerB.identifier,
            "In EF mode the queried edit target must be the routed fallback (layerB)")

        # Verify edits actually land in layerB (the routed fallback), not the session layer.
        shapeItem = ufe.Hierarchy.createItem(ufe.PathString.path(shapeNodePath))
        contextOps = ufe.ContextOps.contextOps(shapeItem)
        ufeCmd.execute(contextOps.doOpCmd(['Add New Prim', 'Xform']))
        cmds.flushIdleQueue()

        primPath = Sdf.Path('/Xform1')
        self.assertIsNotNone(
            layerB.GetPrimAtPath(primPath),
            "In EF mode, UFE edits must be forwarded to the routed fallback (layerB)")
        self.assertIsNone(
            stage.GetSessionLayer().GetPrimAtPath(primPath),
            "In EF mode, UFE edits must not remain on the session layer")

    def testEFTargetUndoable(self):
        """mayaUsdEditTarget in EF mode is undoable; stage target stays session layer throughout."""
        shapeNodePath, stage, layerA, layerB = self._createStageWithSublayers()

        self._writeRules(stage.GetRootLayer())
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetSessionLayer())

        # Set fallback to layerB then to layerA.
        cmds.mayaUsdEditTarget(shapeNodePath, edit=True, editTarget=layerB.identifier)
        cmds.mayaUsdEditTarget(shapeNodePath, edit=True, editTarget=layerA.identifier)

        # Both commands must keep the stage target on the session layer.
        self.assertEqual(
            stage.GetEditTarget().GetLayer(),
            stage.GetSessionLayer(),
            "Stage edit target must remain session layer after two EF routing commands")

        # The query reports the latest fallback target (layerA).
        self.assertEqual(
            cmds.mayaUsdEditTarget(shapeNodePath, query=True, editTarget=True)[0],
            layerA.identifier,
            "Queried edit target must be the latest fallback (layerA)")

        # Undo the second routing command.
        cmds.undo()

        # After undo, EF mode must still be active (stage target is still session layer).
        self.assertEqual(
            stage.GetEditTarget().GetLayer(),
            stage.GetSessionLayer(),
            "EF mode must remain active after undoing an EF routing command")

        # Undo must also restore the previous fallback target (layerB).
        self.assertEqual(
            cmds.mayaUsdEditTarget(shapeNodePath, query=True, editTarget=True)[0],
            layerB.identifier,
            "Undo must restore the previous fallback target (layerB)")

    def testEFFallbackRuleBlocksWeakOpinion(self):
        """The EF fallback rule sets blockWeakOpinion: a forwarded edit is dropped when a
        stronger layer already holds an opinion for the same attribute."""
        shapeNodePath, stage, layerA, layerB = self._createStageWithSublayers()
        # layerA is appended before layerB, so layerA is the stronger sublayer.

        # Author a strong opinion in layerA: create /Xform1 there and translate it.
        cmds.mayaUsdEditTarget(shapeNodePath, edit=True, editTarget=layerA.identifier)
        shapeItem = ufe.Hierarchy.createItem(ufe.PathString.path(shapeNodePath))
        ufeCmd.execute(ufe.ContextOps.contextOps(shapeItem).doOpCmd(['Add New Prim', 'Xform']))
        
        attrPath = '/Xform1.xformOp:translate'
        primItem = ufe.Hierarchy.createItem(ufe.PathString.path(shapeNodePath + ',/Xform1'))
        ufe.Transform3d.transform3d(primItem).translate(1.0, 2.0, 3.0)
        self.assertIsNotNone(
            layerA.GetAttributeAtPath(attrPath),
            "Strong opinion should be authored in layerA")

        # Activate EF (rule that won't match the prim) and route the fallback to the weaker
        # layerB, so the catch-all fallback rule (blockWeakOpinion) handles forwarded edits.
        self._writeRules(stage.GetRootLayer(), inputExpr='/NoMatch.*')
        cmds.mayaUsdEditTarget(shapeNodePath, edit=True, editTarget=layerB.identifier)
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetSessionLayer())

        # Forward an edit to the same attribute. layerA (stronger than the fallback layerB)
        # already has an opinion, so blockWeakOpinion must drop the forwarded edit.
        ufe.Transform3d.transform3d(primItem).translate(9.0, 9.0, 9.0)
        cmds.flushIdleQueue()

        self.assertIsNone(
            layerB.GetAttributeAtPath(attrPath),
            "blockWeakOpinion must drop the weak forwarded opinion to the fallback layer")

    def testEFToggleOffRestoresLastFallback(self):
        """Toggling EF off restores the last fallback target, not the pre-EF edit target."""
        shapeNodePath, stage, layerA, layerB = self._createStageWithSublayers()

        # Activate EF, then route the fallback to layerA and then layerB.
        self._writeRules(stage.GetRootLayer())
        cmds.mayaUsdEditTarget(shapeNodePath, edit=True, editTarget=layerA.identifier)
        cmds.mayaUsdEditTarget(shapeNodePath, edit=True, editTarget=layerB.identifier)
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetSessionLayer())

        # Toggling EF off restores the last fallback (layerB), not the pre-EF target (root).
        self._clearRules(stage.GetRootLayer())
        self.assertEqual(
            stage.GetEditTarget().GetLayer(),
            layerB,
            "Toggling EF off must restore the last fallback target (layerB)")

    def testGetRulesEmptyWithoutContinuous(self):
        """Non-continuous rules do not activate EF mode; the edit target is unchanged."""
        shapeNodePath, stage, layerA, layerB = self._createStageWithSublayers()

        cmds.mayaUsdEditTarget(shapeNodePath, edit=True, editTarget=layerA.identifier)
        self._writeRules(stage.GetRootLayer(), continuous=False)
        
        self.assertEqual(
            stage.GetEditTarget().GetLayer(),
            layerA,
            "Non-continuous rules must not activate EF mode or change the edit target")

    def testLockFallbackLayerSwitchesToSessionLayer(self):
        """Locking the EF fallback layer keeps EF active and routes writes to the session layer."""
        shapeNodePath, stage, layerA, layerB = self._createStageWithSublayers()
        sessionLayer = stage.GetSessionLayer()

        # Activate EF mode and route the fallback to layerA.
        self._writeRules(stage.GetRootLayer())
        self.assertEqual(stage.GetEditTarget().GetLayer(), sessionLayer)
        cmds.mayaUsdEditTarget(shapeNodePath, edit=True, editTarget=layerA.identifier)

        # Lock layerA via the layer editor command.
        # lockLayer flag: (lockType=1 locked, includeSublayers=0, proxyShapePath)
        cmds.mayaUsdLayerEditor(layerA.identifier, edit=True,
                                lockLayer=(1, 0, shapeNodePath))

        # EF must still be active.
        self.assertEqual(
            stage.GetEditTarget().GetLayer(), sessionLayer,
            "EF mode must remain active after locking the fallback layer")
        self.assertEqual(
            cmds.mayaUsdEditTarget(shapeNodePath, query=True, editTarget=True)[0],
            sessionLayer.identifier)

        # A prim authored now must land in the session layer, not in the locked layerA.
        stage.DefinePrim('/TestLockFallback', 'Xform')
        cmds.flushIdleQueue()
        self.assertTrue(
            bool(sessionLayer.GetPrimAtPath('/TestLockFallback')),
            "After locking the fallback, new edits must go to the session layer")
        self.assertFalse(
            bool(layerA.GetPrimAtPath('/TestLockFallback')),
            "After locking the fallback, new edits must NOT go to layerA")

    def testLockFallbackLayerUndoable(self):
        """Undoing the lock of the EF fallback layer keeps writes on the session layer."""
        shapeNodePath, stage, layerA, layerB = self._createStageWithSublayers()
        sessionLayer = stage.GetSessionLayer()

        # Activate EF and route fallback to layerA.
        self._writeRules(stage.GetRootLayer())
        cmds.mayaUsdEditTarget(shapeNodePath, edit=True, editTarget=layerA.identifier)

        # Lock layerA, which redirects the fallback to session layer.
        cmds.mayaUsdLayerEditor(layerA.identifier, edit=True,
                                lockLayer=(1, 0, shapeNodePath))
        self.assertEqual(
            cmds.mayaUsdEditTarget(shapeNodePath, query=True, editTarget=True)[0],
            sessionLayer.identifier)

        # Undo the lock — layerA is writable again, but writes stay on
        # session layer (consistent with how edit-target auto-targeting works on undo).
        cmds.undo()
        self.assertEqual(
            stage.GetEditTarget().GetLayer(), sessionLayer,
            "EF mode must remain active after undoing the lock")
        self.assertEqual(
            cmds.mayaUsdEditTarget(shapeNodePath, query=True, editTarget=True)[0],
            sessionLayer.identifier)

        stage.DefinePrim('/TestUndoLockFallback', 'Xform')
        cmds.flushIdleQueue()
        self.assertTrue(
            bool(sessionLayer.GetPrimAtPath('/TestUndoLockFallback')),
            "After undoing the lock, writes must still go to the session layer")
        self.assertFalse(
            bool(layerA.GetPrimAtPath('/TestUndoLockFallback')),
            "After undoing the lock, writes must NOT revert to layerA")

    def testSaveReloadWithEFActive(self):
        """Saving while EF is active restores the edit target and fallback layer on reload."""
        import os
        import shutil
        import tempfile

        tmpDir = tempfile.mkdtemp(prefix='testEFSaveReload_')
        self.addCleanup(shutil.rmtree, tmpDir, ignore_errors=True)
        rootLayerPath = os.path.join(tmpDir, 'testEFSaveReload_root.usda')
        layerAPath = os.path.join(tmpDir, 'testEFSaveReload_TARGET_layerA.usda')
        layerBPath = os.path.join(tmpDir, 'testEFSaveReload_TARGET_layerB.usda')

        rootLayer = Sdf.Layer.CreateNew(rootLayerPath)
        layerA = Sdf.Layer.CreateNew(layerAPath)
        layerB = Sdf.Layer.CreateNew(layerBPath)
        rootLayer.subLayerPaths.append(layerAPath)
        rootLayer.subLayerPaths.append(layerBPath)
        rootLayer.Save()
        shapeNode = cmds.createNode('mayaUsdProxyShape')
        shapeNodePath = cmds.ls(shapeNode, long=True)[0]
        cmds.setAttr('{}.filePath'.format(shapeNode), rootLayerPath, type='string')
        stage = mayaUsd.lib.GetPrim(shapeNode).GetStage()

        # Set edit target to layerA before activating EF so it becomes the fallback.
        cmds.mayaUsdEditTarget(shapeNodePath, edit=True, editTarget=layerA.identifier)
        self.assertEqual(stage.GetEditTarget().GetLayer(), layerA)

        # Activate EF with a rule that won't match /Xform1, so edits fall through to the fallback.
        self._writeRules(stage.GetRootLayer(), inputExpr='/NoMatch.*')
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetSessionLayer())
        self.assertEqual(
            cmds.mayaUsdEditTarget(shapeNodePath, query=True, editTarget=True)[0],
            layerA.identifier)

        # Persist the EF rules to the root .usda so the file-backed layer is clean at scene
        # save and mayaUsd does not prompt to save it.
        stage.GetRootLayer().Save()

        # Save and reload the Maya scene.
        tmpFile = os.path.join(tmpDir, 'testEFSaveReload.ma')
        cmds.file(rename=tmpFile)
        cmds.file(save=True, type='mayaAscii', force=True)
        cmds.file(new=True, force=True)
        cmds.file(tmpFile, open=True, force=True)

        # Re-acquire handles to the reloaded shape and stage.
        shapeNodePath = cmds.ls(type='mayaUsdProxyShape', long=True)[0]
        stage = mayaUsd.lib.GetPrim(shapeNodePath).GetStage()
        sessionLayer = stage.GetSessionLayer()

        # EF must still be active after reload — edit target must be the session layer.
        self.assertEqual(
            stage.GetEditTarget().GetLayer(),
            sessionLayer,
            "After reload, EF must still be active (edit target = session layer)")

        # Find layerA in the reloaded stage.
        reloadedLayerA = None
        for layer in stage.GetUsedLayers():
            if 'TARGET_layerA' in layer.identifier:
                reloadedLayerA = layer
                break

        self.assertIsNotNone(
            reloadedLayerA, "Could not find TARGET_layerA after reload")

        # The query must report the restored fallback after reload.
        self.assertEqual(
            cmds.mayaUsdEditTarget(shapeNodePath, query=True, editTarget=True)[0],
            reloadedLayerA.identifier,
            "After reload, the queried edit target must be the restored fallback (TARGET_layerA)")

        # Verify that EF actually routes edits to the restored fallback layer by
        # creating a prim via a UFE undoable command (EF only intercepts those).
        shapeItem = ufe.Hierarchy.createItem(ufe.PathString.path(shapeNodePath))
        contextOps = ufe.ContextOps.contextOps(shapeItem)
        ufeCmd.execute(contextOps.doOpCmd(['Add New Prim', 'Xform']))
        cmds.flushIdleQueue()

        primPath = Sdf.Path('/Xform1')
        self.assertIsNotNone(
            reloadedLayerA.GetPrimAtPath(primPath),
            "After reload, EF must forward UFE edits to the restored fallback (TARGET_layerA)")
        self.assertIsNone(
            sessionLayer.GetPrimAtPath(primPath),
            "After EF forwarding, the prim must not remain on the session layer")

        # Deactivating EF must restore the edit target to the fallback (layerA), not rootLayer.
        self._clearRules(stage.GetRootLayer())

        self.assertEqual(
            stage.GetEditTarget().GetLayer(),
            reloadedLayerA,
            "After EF deactivation, edit target must return to the restored fallback (TARGET_layerA). "
            "Got: {}".format(stage.GetEditTarget().GetLayer().identifier))


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
