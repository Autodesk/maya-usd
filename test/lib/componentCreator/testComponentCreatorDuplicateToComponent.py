import unittest

import fixturesUtils
import mayaUsd.lib
import mayaUsd.ufe
import mayaUsd_createStageWithNewLayer
from maya import cmds
from pxr import Sdf, Usd
import ufe
from maya.internal.ufeSupport import ufeCmdWrapper as ufeCmd

from testComponentCreatorBase import _ComponentCreatorTestBase

class DuplicateToComponentTestCase(_ComponentCreatorTestBase, unittest.TestCase):
    """
    Test duplicating Maya nodes to an Adsk USD Component stage via the
    'Duplicate as USD' command.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def setUp(self):
        self._setUpCC()

    def testDuplicateAsUsdToComponentNoVariantTarget(self):
        '''Duplicate to an Adsk USD Component with no variant targeted — should fail with an error.'''

        cmds.polyCube(name='pCube1')
        cubePath = cmds.ls('pCube1', long=True)[0]

        proxyShapePath, _ = self._createComponent()

        # No variant target is set — duplicate should return False and emit a error.
        with mayaUsd.lib.OpUndoItemList():
            result = mayaUsd.lib.PrimUpdaterManager.duplicate(cubePath, proxyShapePath)
        self.assertFalse(result)

    def testDuplicateAsUsdToComponent(self):
        '''Duplicate a Maya mesh to an Adsk USD Component with a variant targeted.'''
        import AdskUsdComponentCreator

        cmds.polyCube(name='pCube1')
        cubePath = cmds.ls('pCube1', long=True)[0]

        proxyShapePath, desc = self._createComponent()

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Duplicate the Maya mesh to the component.
        with mayaUsd.lib.OpUndoItemList():
            result = mayaUsd.lib.PrimUpdaterManager.duplicate(cubePath, proxyShapePath)
        self.assertTrue(result)

        # The cube should now appear in the component's geo scope.
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage.GetPrimAtPath('/root/geo/pCube1').IsValid())



if __name__ == '__main__':
    fixturesUtils.runTests(globals())

