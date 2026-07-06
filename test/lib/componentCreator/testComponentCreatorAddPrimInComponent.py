import unittest

import fixturesUtils
import mayaUsd.ufe
import tempfile
from maya import cmds
from pxr import Sdf
import ufe
from maya.internal.ufeSupport import ufeCmdWrapper as ufeCmd

from testComponentCreatorBase import _ComponentCreatorTestBase

class AddPrimInComponentTestCase(_ComponentCreatorTestBase, unittest.TestCase):
    """
    Test adding prims to an Adsk USD Component stage via different authoring paths.

    1. Authoring via the pxr API (stage.DefinePrim) with no UsdUndoBlock →
       no edit-forwarding fires → the prim lands on the session layer.

    2. Authoring via the UFE 'Add New Prim' context-op on a valid scope (geo/mtl)
       → edit-forwarding fires on idle → prim is forwarded to the variant layer
       and removed from the session layer.

    3. Authoring via the UFE 'Add New Prim' context-op at the root level
       (outside geo/mtl) → the component's block rule rolls back the entire
       transaction → the prim ends up in neither the session layer nor the root layer.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)
        # Tests in this class drive edit-forwarding through `ufeCmd.execute`, which
        # wraps `cmds.ufeCmd` — a command mayaUsdPlugin registers on idle.  Resume
        # and drain the idle queue once at class setup so the command is available.
        cmds.flushIdleQueue(resume=True)
        cmds.flushIdleQueue()

    def setUp(self):
        self._setUpCC()

    def _createComponent(self):
        """Creates a new Adsk USD Component and a proxy shape pointing to it.

        Returns (proxyShapePath, ComponentDescription).
        """
        import AdskUsdComponentCreator

        tempDir = tempfile.mkdtemp()
        opts = AdskUsdComponentCreator.Options()
        opts.component_folder = tempDir
        opts.component_name = 'TestComp'
        opts.component_filename = 'TestComp'
        opts.file_extension = 'usda'
        opts.component_variants = [('model', 'default')]
        opts.is_default_variant = True

        info = AdskUsdComponentCreator.ComponentAPI.CreateFromNothing(opts)
        desc = AdskUsdComponentCreator.ComponentDescription.CreateFromInfo(info)

        rootLayerPath = info.stage.GetRootLayer().realPath

        transform = cmds.createNode('transform', name='TestComp')
        shape = cmds.createNode('mayaUsdProxyShape', name='TestCompShape', parent=transform)
        cmds.setAttr(shape + '.filePath', rootLayerPath, type='string')

        proxyShapePath = cmds.ls(shape, long=True)[0]

        # Give the CC plugin an idle tick to detect the new proxy shape and register
        # its edit-forwarding callbacks before the test proceeds.
        cmds.flushIdleQueue()

        return proxyShapePath, desc

    def _setDefaultVariantTarget(self, desc):
        """Target the `model=default` variant on the given component description."""
        import AdskUsdComponentCreator
        vsDesc = desc.GetVariantSets()['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

    def testAddPrimViaApiLandsInSessionLayer(self):
        '''Validate that authoring via the pxr API puts the prim in the session layer.

        stage.DefinePrim() does not create a UsdUndoBlock, so edit-forwarding
        never fires.  The prim is therefore authored directly on the current
        edit target, which for component stages is the session layer.
        '''
        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant so the geo scope is visible.
        self._setDefaultVariantTarget(desc)

        primPath = Sdf.Path('/root/geo/ApiPrim')

        # Author directly via the pxr API — no UsdUndoBlock, no forwarding.
        newPrim = stage.DefinePrim(primPath, 'Xform')
        self.assertTrue(newPrim.IsValid())

        # Prim must be in the session layer.
        sessionLayer = stage.GetSessionLayer()
        self.assertIsNotNone(
            sessionLayer.GetPrimAtPath(primPath),
            'Prim authored via pxr API should land in the session layer')

        # Prim must NOT be in the locked root layer.
        rootLayer = stage.GetRootLayer()
        self.assertIsNone(
            rootLayer.GetPrimAtPath(primPath),
            'Prim authored via pxr API should not be written to the locked root layer')

    def testAddPrimViaContextOpForwardsToVariantLayerInComponentStage(self):
        '''Validate that 'Add New Prim' under a valid scope is forwarded to the variant layer.

        When contextOps is invoked on the geo scope item the new prim lands under
        /root/geo, which matches the forwarding rule.  The prim is first authored
        in the session layer (UsdUndoBlock), then edit-forwarding fires on the next
        idle tick and moves it to the active variant layer.  After the flush the
        prim must NOT remain in the session layer, and must be visible in the
        composed stage via the active variant.
        '''
        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        self._setDefaultVariantTarget(desc)

        # Build the UFE item for the geo scope so the new prim lands under it.
        geoItem = ufe.Hierarchy.createItem(
            ufe.PathString.path(proxyShapePath + ',/root/geo'))
        self.assertIsNotNone(geoItem)

        contextOps = ufe.ContextOps.contextOps(geoItem)
        self.assertIsNotNone(contextOps)

        ufeCmd.execute(contextOps.doOpCmd(['Add New Prim', 'Xform']))

        primPath = Sdf.Path('/root/geo/Xform1')
        sessionLayer = stage.GetSessionLayer()

        # Before forwarding: prim is in the session layer and visible in the composed stage.
        self.assertIsNotNone(
            sessionLayer.GetPrimAtPath(primPath),
            'Prim should be in the session layer immediately after contextOps command')
        self.assertTrue(
            stage.GetPrimAtPath(primPath).IsValid(),
            'Prim should be visible in the composed stage before forwarding')

        # Flush idle queue so edit-forwarding fires and moves the edit.
        cmds.flushIdleQueue()

        # After forwarding: prim has left the session layer (moved to the variant sublayer).
        self.assertIsNone(
            sessionLayer.GetPrimAtPath(primPath),
            'Prim should have been forwarded out of the session layer')
        # The prim must remain visible in the composed stage via the active variant
        # payload (the root-layer variant selection keeps the variant active).
        self.assertTrue(
            stage.GetPrimAtPath(primPath).IsValid(),
            'Prim should remain visible in the composed stage after forwarding')

    def testAddPrimViaContextOpIsBlockedByForwardingRuleInComponentStage(self):
        '''Validate that 'Add New Prim' at root level is rejected by the edit-forward block rule.

        The context-op creates a UsdUndoBlock internally.  Edit-forwarding fires
        on the next idle tick.  The component's block rule (EditForwardRules.cpp,
        isTargetLayerBlock=true) matches prims outside the geo/mtl scopes and
        rolls back the entire transaction — the session-layer edit is undone.
        After the flush the prim exists in neither the session layer nor the root layer,
        and is not visible in the composed stage.
        '''
        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        self._setDefaultVariantTarget(desc)

        # Build the UFE item for the proxy shape — 'Add New Prim' will land at /Xform1,
        # which is outside /root/geo and /root/mtl.
        shapeItem = ufe.Hierarchy.createItem(ufe.PathString.path(proxyShapePath))
        self.assertIsNotNone(shapeItem)

        contextOps = ufe.ContextOps.contextOps(shapeItem)
        self.assertIsNotNone(contextOps)

        ufeCmd.execute(contextOps.doOpCmd(['Add New Prim', 'Xform']))

        primPath = Sdf.Path('/Xform1')
        sessionLayer = stage.GetSessionLayer()

        # Before block fires: prim is in the session layer and visible in the composed stage.
        self.assertIsNotNone(
            sessionLayer.GetPrimAtPath(primPath),
            'Prim should be in the session layer immediately after contextOps command')
        self.assertTrue(
            stage.GetPrimAtPath(primPath).IsValid(),
            'Prim should be visible in the composed stage before the block rule fires')

        # Flush idle queue so the block rule fires and rolls back the transaction.
        cmds.flushIdleQueue()

        # After rollback: prim is gone from the composed stage.
        self.assertFalse(
            stage.GetPrimAtPath(primPath).IsValid(),
            'Prim must not be visible in the composed stage after rollback')



if __name__ == '__main__':
    fixturesUtils.runTests(globals())

