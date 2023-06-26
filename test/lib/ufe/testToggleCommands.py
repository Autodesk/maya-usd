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
from usdUtils import filterUsdStr

from pxr import Usd


#####################################################################
#
# Tests

class ToggleCommandsTestCase(unittest.TestCase):
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

    def testToggleActiveCommand(self):
        '''
        Test toggle active commands.
        '''
        prim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A")

        self.assertTrue(prim.IsActive())
        originalRootContents = filterUsdStr(self.stage.GetRootLayer().ExportToString())

        cmd = usdUfe.ToggleActiveCommand(prim)
        self.assertIsNotNone(cmd)

        cmd.execute()
        self.assertFalse(prim.IsActive())

        cmd.undo()
        self.assertTrue(prim.IsActive())
        self.assertEqual(originalRootContents, filterUsdStr(self.stage.GetRootLayer().ExportToString()))

        cmd.redo()
        self.assertFalse(prim.IsActive())

    def testToggleInstanceableCommand(self):
        '''
        Test toggle instanceable commands.
        '''
        prim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A")

        self.assertFalse(prim.IsInstanceable())
        originalRootContents = filterUsdStr(self.stage.GetRootLayer().ExportToString())

        cmd = usdUfe.ToggleInstanceableCommand(prim)
        self.assertIsNotNone(cmd)

        cmd.execute()
        self.assertTrue(prim.IsInstanceable())

        cmd.undo()
        self.assertFalse(prim.IsInstanceable())
        self.assertEqual(originalRootContents, filterUsdStr(self.stage.GetRootLayer().ExportToString()))

        cmd.redo()
        self.assertTrue(prim.IsInstanceable())


if __name__ == '__main__':
    unittest.main(verbosity=2)
