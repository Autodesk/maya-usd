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

import mayaUsd.lib

import mayaUtils
import mayaUsd.ufe

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

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '3006', 'Test only available in UFE preview version 0.3.6 and greater')
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

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '3006', 'Test only available in UFE preview version 0.3.6 and greater')
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

        # Discard Maya edits.
        cmds.mayaUsdDiscardEdits(aMayaPathStr)

        def verifyDiscard():
            # Original USD translation values are preserved.
            usdMatrix = xlateOp.GetOpTransform(mayaUsd.ufe.getTime(aUsdUfePathStr))
            self.assertEqual(usdMatrix.ExtractTranslation(), usdXlation)

            # Maya node is removed.
            with self.assertRaises(RuntimeError):
                om.MSelectionList().add(aMayaPathStr)

        verifyDiscard()

        # undo
        cmds.undo()

        verifyScenesModifications()

        # redo
        cmds.redo()

        verifyDiscard()


if __name__ == '__main__':
    unittest.main(verbosity=2)
