#!/usr/bin/env python

#
# Copyright 2021 Autodesk
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

import fixturesUtils

from mayaUtils import setMayaTranslation
from usdUtils import createSimpleXformScene
from ufeUtils import ufeFeatureSetVersion

import mayaUsd.lib

import mayaUtils
import mayaUsd.ufe
import usdUtils

from pxr import UsdGeom, Gf

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as om

import ufe

import unittest

from testUtils import assertVectorAlmostEqual

import os

class MergeToUsdTestCase(unittest.TestCase):
    '''Test discard Maya edits: discard edits done as Maya data to USD prims.
    '''

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
        cmds.file(new=True, force=True)

    def _GetMayaDependencyNode(self, objectName):
        selectionList = om.MSelectionList()
        try:
            selectionList.add(objectName)
        except:
            return None
        mObj = selectionList.getDependNode(0)

        return om.MFnDependencyNode(mObj)

    @unittest.skipUnless(ufeFeatureSetVersion() >= 3, 'Test only available in UFE v3 or greater.')
    def testDiscardEdits(self):
        '''Discard edits on a USD transform.'''

        # Edit as Maya first.
        (ps, xlateOp, usdXlation, aUsdUfePathStr, aUsdUfePath, aUsdItem,
         _, _, _, _, _) = createSimpleXformScene()

        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(aUsdUfePathStr))

        aMayaItem = ufe.GlobalSelection.get().front()
        mayaXlation = om.MVector(4, 5, 6)
        (aMayaPath, aMayaPathStr, aFn, mayaMatrix) = \
            setMayaTranslation(aMayaItem, mayaXlation)

        self.assertEqual(aFn.translation(om.MSpace.kObject), mayaXlation)

        mayaToUsd = ufe.PathMappingHandler.pathMappingHandler(aMayaItem)
        self.assertEqual(mayaToUsd.fromHost(aMayaPath), aUsdUfePath)

        psHier = ufe.Hierarchy.hierarchy(ps)
        self.assertNotIn(aUsdItem, psHier.children())
        self.assertIn(aMayaItem, psHier.children())

        # Discard Maya edits.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.discardEdits(aMayaPathStr))

        # Original USD translation values are preserved.
        usdMatrix = xlateOp.GetOpTransform(mayaUsd.ufe.getTime(aUsdUfePathStr))
        self.assertEqual(usdMatrix.ExtractTranslation(), usdXlation)

        # There no longer is a path mapping from the Maya path to the USD path.
        self.assertEqual(len(mayaToUsd.fromHost(aMayaPath)), 0)

        # Hierarchy is restored: USD item is child of proxy shape, Maya item is
        # not.  Be careful to use the Maya path rather than the Maya item, which
        # should no longer exist.
        self.assertIn(aUsdItem, psHier.children())
        self.assertNotIn(aMayaPath, [child.path() for child in psHier.children()])

        # Maya node is removed.
        with self.assertRaises(RuntimeError):
            om.MSelectionList().add(aMayaPathStr)

    @unittest.skipUnless(ufeFeatureSetVersion() >= 3, 'Test only available in UFE v3 or greater.')
    def testDiscardEditsUndoRedo(self):
        '''Discard edits on a USD transform then undo and redo.'''

        # Edit as Maya first.
        (ps, xlateOp, usdXlation, aUsdUfePathStr, aUsdUfePath, aUsdItem,
         _, _, _, _, _) = createSimpleXformScene()

        cmds.mayaUsdEditAsMaya(aUsdUfePathStr)

        # Modify the scene.
        aMayaItem = ufe.GlobalSelection.get().front()
        mayaXlation = om.MVector(4, 5, 6)
        (aMayaPath, aMayaPathStr, aFn, mayaMatrix) = \
            setMayaTranslation(aMayaItem, mayaXlation)

        # Verify the scene modifications.
        def verifyScenesModifications():
            self.assertEqual(aFn.translation(om.MSpace.kObject), mayaXlation)
            mayaToUsd = ufe.PathMappingHandler.pathMappingHandler(aMayaItem)
            self.assertEqual(
                ufe.PathString.string(mayaToUsd.fromHost(aMayaPath)),
                ufe.PathString.string(aUsdUfePath))

        verifyScenesModifications()

        # Make a selection before discard Maya edits.
        cmds.select('persp')
        previousSn = cmds.ls(sl=True, ufe=True, long=True)

        # Discard Maya edits.
        cmds.mayaUsdDiscardEdits(aMayaPathStr)

        def verifyDiscard():
            # Original USD translation values are preserved.
            usdMatrix = xlateOp.GetOpTransform(mayaUsd.ufe.getTime(aUsdUfePathStr))
            self.assertEqual(usdMatrix.ExtractTranslation(), usdXlation)

            # Maya node is removed.
            with self.assertRaises(RuntimeError):
                om.MSelectionList().add(aMayaPathStr)

            # Selection is on the USD object.
            sn = cmds.ls(sl=True, ufe=True, long=True)
            self.assertEqual(len(sn), 1)
            self.assertEqual(sn[0], aUsdUfePathStr)

        verifyDiscard()

        # undo
        cmds.undo()

        verifyScenesModifications()
        # Selection is restored.
        self.assertEqual(cmds.ls(sl=True, ufe=True, long=True), previousSn)

        # redo
        cmds.redo()

        verifyDiscard()

    @unittest.skipUnless(ufeFeatureSetVersion() >= 3, 'Test only available in UFE v3 or greater.')
    def testDiscardOrphaned(self):
        '''Discard orphaned edits due to prim inactivation'''
        
        # open appleBite.ma scene in testSamples
        mayaUtils.openAppleBiteScene()

        mayaPathSegment = mayaUtils.createUfePathSegment('|Asset_flattened_instancing_and_class_removed_usd|Asset_flattened_instancing_and_class_removed_usdShape')
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))
        self.assertTrue(stage)
        
        usdPathSegment = usdUtils.createUfePathSegment('/apple/payload/geo/skin')
        geoPath = ufe.Path([mayaPathSegment, usdPathSegment])
        geoPathStr = ufe.PathString.string(geoPath)
        
        # pull the skin geo for editing
        cmds.mayaUsdEditAsMaya(geoPathStr)
        
        pulledDagPath = '|__mayaUsd__|skinParent|skin'
        self.assertTrue(self._GetMayaDependencyNode(pulledDagPath))
        
        # now we will make the pulled prim go away...which makes state in DG orphaned
        primToDeactivate = stage.GetPrimAtPath('/apple/payload')
        primToDeactivate.SetActive(False)
        self.assertFalse(stage.GetPrimAtPath('/apple/payload/geo/skin'))
        # we keep the state in Maya untouched to prevent data loss
        self.assertTrue(self._GetMayaDependencyNode(pulledDagPath))
        
        # discard orphaned edits
        cmds.mayaUsdDiscardEdits(pulledDagPath)
        self.assertFalse(self._GetMayaDependencyNode(pulledDagPath))
        
        # validate undo
        cmds.undo()
        self.assertTrue(self._GetMayaDependencyNode(pulledDagPath))
        
        # validate redo
        cmds.redo()
        self.assertFalse(self._GetMayaDependencyNode(pulledDagPath))

if __name__ == '__main__':
    unittest.main(verbosity=2)
