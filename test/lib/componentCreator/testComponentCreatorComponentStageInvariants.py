import unittest

import fixturesUtils
import mayaUsd.lib
import mayaUsd.ufe
from maya import cmds
import ufe

from testComponentCreatorBase import _ComponentCreatorTestBase

class ComponentStageInvariantsTestCase(_ComponentCreatorTestBase, unittest.TestCase):
    """
    Validate structural invariants that hold for every Adsk USD Component stage.

    1. Root layer is locked (permissionToEdit == False) — the MayaComponentManager
       locks it via mayaUsdLayerEditor when the component is registered, preventing
       direct edits to the on-disk file through the proxy shape.

    2. Default edit target is the session layer — stageNode.cpp sets this
       automatically whenever the root layer is not editable.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def setUp(self):
        self._setUpCC()

    def testComponentRootLayerIsLocked(self):
        '''Validate that the component root layer is locked (permissionToEdit == False).

        The MayaComponentManager locks the root layer via mayaUsdLayerEditor
        when the component stage is registered.  This prevents direct edits
        to the on-disk component file through the proxy shape.
        '''
        proxyShapePath, _ = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        rootLayer = stage.GetRootLayer()
        self.assertFalse(
            rootLayer.permissionToEdit,
            'Component root layer should be locked (permissionToEdit == False)')

    def testComponentDefaultEditTargetIsSessionLayer(self):
        '''Validate that the default edit target of a component stage is the session layer.

        stageNode.cpp automatically targets the session layer when the root
        layer is not editable (permissionToEdit == False), so every new edit
        made without an explicit edit-context goes to the session layer.
        '''
        proxyShapePath, _ = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        self.assertEqual(
            stage.GetEditTarget().GetLayer(),
            stage.GetSessionLayer(),
            'Component stage default edit target should be the session layer')



if __name__ == '__main__':
    fixturesUtils.runTests(globals())

