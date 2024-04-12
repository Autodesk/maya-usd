#!/usr/bin/env python 

import fixturesUtils
from maya import cmds
from maya import standalone
import mayaUsd.ufe
import mayaUtils
import usdUfe
import ufe
import unittest
import usdUtils
import testUtils
from usdUtils import filterUsdStr

from pxr import Usd


#####################################################################
#
# Tests

class PayloadCommandsTestCase(unittest.TestCase):
    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        self.assertTrue(self.pluginsLoaded)

        cmds.file(new=True, force=True)
        import mayaUsd_createStageWithNewLayer
 
        # Create the following hierarchy:
        #
        # proxy shape
        #  |_ A
        #  |_ B
        #
 
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        self.stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        self.stage.DefinePrim('/A', 'Xform')
        self.stage.DefinePrim('/B', 'Xform')
 
        psPath = ufe.PathString.path(psPathStr)
        psPathSegment = psPath.segments[0]
        aPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/A')])
        bPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/B')])
        self.a = ufe.Hierarchy.createItem(aPath)
        self.b = ufe.Hierarchy.createItem(bPath)
 
        cmds.select(clear=True)

    def testAddAndClearPayloadCommands(self):
        '''
        Test add and clear payload commands.
        '''
        prim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A")

        # Verify the initial state
        self.assertFalse(prim.HasPayload())
        originalRootContents = filterUsdStr(self.stage.GetRootLayer().ExportToString())

        # Verify add payload
        payloadFile = testUtils.getTestScene('twoSpheres', 'sphere.usda')
        cmd = usdUfe.AddPayloadCommand(prim, payloadFile, True)
        self.assertIsNotNone(cmd)

        cmd.execute()
        self.assertTrue(prim.HasPayload())

        cmd.undo()
        self.assertFalse(prim.HasPayload())
        self.assertEqual(originalRootContents, filterUsdStr(self.stage.GetRootLayer().ExportToString()))

        cmd.redo()
        self.assertTrue(prim.HasPayload())

        # Verify clear payload
        cmd = usdUfe.ClearPayloadsCommand(prim)
        self.assertIsNotNone(cmd)

        cmd.execute()
        self.assertFalse(prim.HasPayload())
        self.assertEqual(originalRootContents, filterUsdStr(self.stage.GetRootLayer().ExportToString()))

        cmd.undo()
        self.assertTrue(prim.HasPayload())

        cmd.redo()
        self.assertFalse(prim.HasPayload())
        self.assertEqual(originalRootContents, filterUsdStr(self.stage.GetRootLayer().ExportToString()))

    def testLoadAndUnloadPayloadCommands(self):
        '''
        Test unload and load payload commands.
        '''
        primUfePath = "|stage1|stageShape1,/A"
        prim = mayaUsd.ufe.ufePathToPrim(primUfePath)

        # Verify initial state
        self.assertFalse(prim.HasPayload())

        payloadFile = testUtils.getTestScene('twoSpheres', 'sphere.usda')
        cmd = usdUfe.AddPayloadCommand(prim, payloadFile, True)
        self.assertIsNotNone(cmd)

        # Verify state after add payload
        cmd.execute()
        self.assertTrue(prim.HasPayload())
        self.assertTrue(prim.IsLoaded())

        # Verify unload payload
        cmd = usdUfe.UnloadPayloadCommand(prim)
        self.assertIsNotNone(cmd)

        cmd.execute()
        self.assertFalse(prim.IsLoaded())

        cmd.undo()
        self.assertTrue(prim.IsLoaded())

        cmd.redo()
        self.assertFalse(prim.IsLoaded())

        # Verify load payload
        cmd = usdUfe.LoadPayloadCommand(prim, Usd.LoadWithDescendants)
        self.assertIsNotNone(cmd)

        cmd.execute()
        self.assertTrue(prim.IsLoaded())

        cmd.undo()
        self.assertFalse(prim.IsLoaded())

        cmd.redo()
        self.assertTrue(prim.IsLoaded())

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2023, 'Delete restriction on delete requires Maya 2023 or greater.')
    def testDeletePrimContainingPayload(self):
        '''
        Test adding a payload to a prim, then deleting that prim.
        '''
        # Get the session layer
        prim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A")

        self.assertFalse(prim.HasPayload())

        payloadFile = testUtils.getTestScene('twoSpheres', 'sphere.usda')
        cmd = usdUfe.AddPayloadCommand(prim, payloadFile, True)
        self.assertIsNotNone(cmd)

        # Verify state after add payload
        cmd.execute()
        self.assertTrue(prim.HasPayload())
        self.assertTrue(prim.IsLoaded())

        # Delete the prim containing the payload. The payload should not block deletion.
        cmds.delete("|stage1|stageShape1,/A")
        prim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A")
        self.assertFalse(prim)
        

if __name__ == '__main__':
    unittest.main(verbosity=2)
