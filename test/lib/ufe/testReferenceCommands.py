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

#####################################################################
#
# Tests

class ReferenceCommandsTestCase(unittest.TestCase):
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

    def testAddAndClearReferenceCommands(self):
        '''
        Test add and clear reference commands.
        '''
        # Get the session layer
        prim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A")

        self.assertFalse(prim.HasAuthoredReferences())
        originalRootContents = filterUsdStr(self.stage.GetRootLayer().ExportToString())

        referencedFile = testUtils.getTestScene('twoSpheres', 'sphere.usda')
        cmd = usdUfe.AddReferenceCommand(prim, referencedFile, True)

        cmd.execute()
        self.assertTrue(prim.HasAuthoredReferences())

        cmd.undo()
        self.assertFalse(prim.HasAuthoredReferences())
        self.assertEqual(originalRootContents, filterUsdStr(self.stage.GetRootLayer().ExportToString()))

        cmd.redo()
        self.assertTrue(prim.HasAuthoredReferences())

        cmd = usdUfe.ClearReferencesCommand(prim)

        cmd.execute()
        self.assertFalse(prim.HasAuthoredReferences())
        self.assertEqual(originalRootContents, filterUsdStr(self.stage.GetRootLayer().ExportToString()))

        cmd.undo()
        self.assertTrue(prim.HasAuthoredReferences())

        cmd.redo()
        self.assertFalse(prim.HasAuthoredReferences())
        self.assertEqual(originalRootContents, filterUsdStr(self.stage.GetRootLayer().ExportToString()))


    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2023, 'Delete restriction on delete requires Maya 2023 or greater.')
    def testDeletePrimContainingReference(self):
        '''
        Test adding a reference to a prim, then deleting that prim.
        '''
        # Get the session layer
        prim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A")

        self.assertFalse(prim.HasAuthoredReferences())

        referencedFile = testUtils.getTestScene('twoSpheres', 'sphere.usda')
        cmd = usdUfe.AddReferenceCommand(prim, referencedFile, True)

        cmd.execute()
        self.assertTrue(prim.HasAuthoredReferences())

        # Delete the prim containing the reference. The ref should not block deletion.
        cmds.delete("|stage1|stageShape1,/A")
        prim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A")
        self.assertFalse(prim)


if __name__ == '__main__':
    unittest.main(verbosity=2)
