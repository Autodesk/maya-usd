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
import shutil
import os
import time

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

    def testReloadReferencesCommands(self):
        '''
        Test reload prim by simulating external changes happening to the reference.
        '''

        # paths to the files used in this test
        newFile = testUtils.getTestScene('twoSpheres', 'spherexform.usda')
        oldFile = testUtils.getTestScene('twoSpheres', 'sphere.usda')
        bkFile  = testUtils.getTestScene('twoSpheres', 'sphere_bk.usda')
        refFile = testUtils.getTestScene('twoSpheres', 'spheres_ref.usda')

        # Added a file with nested reference so that can also be tested
        prim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A")

        # make sure to revert changes to the test file with original content
        shutil.copyfile(bkFile, oldFile)

        cmd = usdUfe.AddReferenceCommand(prim, refFile, True)
        cmd.execute()

        spherePrim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A/sphere")
        sphereXformPrim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A/test")

        self.assertTrue(spherePrim.IsValid())
        self.assertFalse(sphereXformPrim.IsValid())

        # Sleep here to make sure that the time stamp from the next copy will be different
        # a delta time less than 1 second won't be enough to be detected for reloading
        time.sleep(1.1)

        # replace sphere file with a different version so that the "reload" can be tested
        shutil.copyfile(newFile, oldFile)

        reloadCmd = usdUfe.ReloadReferenceCommand(prim)
        reloadCmd.execute()

        newSphereXformPrim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A/test")
        self.assertTrue(newSphereXformPrim.IsValid())


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
