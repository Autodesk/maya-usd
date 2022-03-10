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

from usdUtils import createSimpleXformScene
from ufeUtils import ufeFeatureSetVersion

import mayaUsd.lib

import mayaUtils
import mayaUsd.ufe

from pxr import Usd, UsdGeom, Gf

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as om

import ufe

import unittest

from testUtils import assertVectorAlmostEqual

import os

class EditAsMayaTestCase(unittest.TestCase):
    '''Test edit as Maya: bring USD data into Maya to edit.

    Also known as Pull to DG.
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

    @unittest.skipUnless(ufeFeatureSetVersion() >= 3, 'Test only available in UFE v3 or greater.')
    def testTransformEditAsMaya(self):
        '''Edit a USD transform as a Maya object.'''

        (ps, xlateOp, xlation, aUsdUfePathStr, aUsdUfePath, aUsdItem,
         _, _, _, _, _) = createSimpleXformScene()

        # Edit aPrim as Maya data.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.canEditAsMaya(aUsdUfePathStr))
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(aUsdUfePathStr))

        # Test the path mapping services.
        #
        # Unfortunately, toHost is unimplemented as of 20-Sep-2021.  Use the
        # selection to retrieve the Maya object.
        #
        # usdToMaya = ufe.PathMappingHandler.pathMappingHandler(aUsdItem)
        # aMayaUfePath = usdToMaya.toHost(aUsdUfePath)
        # self.assertEqual(aMayaUfePath.nbSegments(), 1)

        aMayaItem = ufe.GlobalSelection.get().front()
        aMayaPath = aMayaItem.path()
        self.assertEqual(aMayaPath.nbSegments(), 1)
        mayaToUsd = ufe.PathMappingHandler.pathMappingHandler(aMayaItem)
        self.assertEqual(mayaToUsd.fromHost(aMayaPath), aUsdUfePath)

        # Confirm the hierarchy is preserved through the Hierarchy interface:
        # one child, the parent of the pulled item is the proxy shape, and
        # the proxy shape has the pulled item as a child, not the original USD
        # scene item.
        aMayaHier = ufe.Hierarchy.hierarchy(aMayaItem)
        self.assertEqual(len(aMayaHier.children()), 1)
        self.assertEqual(ps, aMayaHier.parent())
        psHier = ufe.Hierarchy.hierarchy(ps)
        self.assertIn(aMayaItem, psHier.children())
        self.assertNotIn(aUsdItem, psHier.children())

        # Confirm the translation has been transferred, and that the local
        # transformation is only a translation.
        aDagPath = om.MSelectionList().add(ufe.PathString.string(aMayaPath)).getDagPath(0)
        aFn= om.MFnTransform(aDagPath)
        self.assertEqual(aFn.translation(om.MSpace.kObject), om.MVector(*xlation))
        mayaMatrix = aFn.transformation().asMatrix()
        usdMatrix = xlateOp.GetOpTransform(mayaUsd.ufe.getTime(aUsdUfePathStr))
        mayaValues = [v for v in mayaMatrix]
        usdValues = [v for row in usdMatrix for v in row]

        assertVectorAlmostEqual(self, mayaValues, usdValues)

    @unittest.skipUnless(ufeFeatureSetVersion() >= 3, 'Test only available in UFE v3 or greater.')
    def testEditAsMayaUndoRedo(self):
        '''Edit a USD transform as a Maya object and apply undo and redo.'''

        (ps, xlateOp, xlation, aUsdUfePathStr, aUsdUfePath, aUsdItem,
         _, _, _, _, _) = createSimpleXformScene()

        aPrim = mayaUsd.ufe.ufePathToPrim(aUsdUfePathStr)

        # Edit aPrim as Maya data.
        self.assertTrue(mayaUsd.lib.PrimUpdaterManager.canEditAsMaya(aUsdUfePathStr))

        # Make a selection before edit as Maya.
        cmds.select('persp')
        previousSn = cmds.ls(sl=True, ufe=True, long=True)

        cmds.mayaUsdEditAsMaya(aUsdUfePathStr)

        def getMayaPathStr():
            aMayaItem = ufe.GlobalSelection.get().front()
            aMayaPath = aMayaItem.path()
            aMayaPathStr = ufe.PathString.string(aMayaPath)
            return aMayaPathStr

        aMayaPathStr = getMayaPathStr()

        def verifyEditedScene():
            aMayaItem = ufe.GlobalSelection.get().front()
            aMayaPath = aMayaItem.path()
            aMayaPathStr = ufe.PathString.string(aMayaPath)

            # Confirm the hierarchy is preserved through the Hierarchy interface:
            # one child, the parent of the pulled item is the proxy shape, and
            # the proxy shape has the pulled item as a child, not the original USD
            # scene item.
            aMayaHier = ufe.Hierarchy.hierarchy(aMayaItem)
            self.assertEqual(len(aMayaHier.children()), 1)
            self.assertEqual(ps, aMayaHier.parent())
            psHier = ufe.Hierarchy.hierarchy(ps)
            self.assertIn(aMayaItem, psHier.children())
            self.assertNotIn(aUsdItem, psHier.children())

            # Confirm the translation has been transferred, and that the local
            # transformation is only a translation.
            aDagPath = om.MSelectionList().add(ufe.PathString.string(aMayaPath)).getDagPath(0)
            aFn= om.MFnTransform(aDagPath)
            self.assertEqual(aFn.translation(om.MSpace.kObject), om.MVector(*xlation))
            mayaMatrix = aFn.transformation().asMatrix()
            usdMatrix = xlateOp.GetOpTransform(mayaUsd.ufe.getTime(aUsdUfePathStr))
            mayaValues = [v for v in mayaMatrix]
            usdValues = [v for row in usdMatrix for v in row]

            assertVectorAlmostEqual(self, mayaValues, usdValues)

            # Selection is on the edited Maya object.
            sn = cmds.ls(sl=True, ufe=True, long=True)
            self.assertEqual(len(sn), 1)
            self.assertEqual(sn[0], aMayaPathStr)

            # Read the pull information from the pulled prim.
            aMayaPullPathStr = mayaUsd.lib.PrimUpdaterManager.readPullInformation(aPrim)
            self.assertEqual(aMayaPathStr, aMayaPullPathStr)

        verifyEditedScene()

        # Undo
        cmds.undo()

        def verifyNoLongerEdited():
            # Maya node is removed.
            with self.assertRaises(RuntimeError):
                om.MSelectionList().add(aMayaPathStr)
            # Selection is restored.
            self.assertEqual(cmds.ls(sl=True, ufe=True, long=True), previousSn)
            # No more pull information on the prim.
            self.assertEqual(len(mayaUsd.lib.PrimUpdaterManager.readPullInformation(aPrim)), 0)

        verifyNoLongerEdited()
        
        # Redo
        cmds.redo()

        verifyEditedScene()

    @unittest.skipUnless(ufeFeatureSetVersion() >= 3, 'Test only available in UFE v3 or greater.')
    def testIllegalEditAsMaya(self):
        '''Trying to edit as Maya on object that doesn't support it.'''
        
        import mayaUsd_createStageWithNewLayer

        proxyShapePathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()
        blendShape = stage.DefinePrim('/BlendShape1', 'BlendShape')
        scope = stage.DefinePrim('/Scope1', 'Scope')

        blendShapePathStr = proxyShapePathStr + ',/BlendShape1'
        scopePathStr = proxyShapePathStr + ',/Scope1'

        # Blend shape cannot be edited as Maya: it has no importer.
        with mayaUsd.lib.OpUndoItemList():
            self.assertFalse(mayaUsd.lib.PrimUpdaterManager.canEditAsMaya(blendShapePathStr))
            self.assertFalse(mayaUsd.lib.PrimUpdaterManager.editAsMaya(blendShapePathStr))

        # Scope cannot be edited as Maya: it has no exporter.
        # Unfortunately, as of 17-Nov-2021, we cannot determine how a prim will
        # round-trip, so we cannot use the information that scope has no
        # exporter.
        # with mayaUsd.lib.OpUndoItemList():
        #     self.assertFalse(mayaUsd.lib.PrimUpdaterManager.canEditAsMaya(scopePathStr))
        #     self.assertFalse(mayaUsd.lib.PrimUpdaterManager.editAsMaya(scopePathStr))

    @unittest.skipUnless(ufeFeatureSetVersion() >= 3, 'Test only available in UFE v3 or greater.')
    def testSessionLayer(self):
        '''Verify that the edit gets on the sessionLayer instead of the editTarget layer.'''
        
        import mayaUsd_createStageWithNewLayer

        proxyShapePathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()
        sessionLayer = stage.GetSessionLayer()
        prim = stage.DefinePrim('/A', 'Xform')

        primPathStr = proxyShapePathStr + ',/A'

        self.assertTrue(stage.GetSessionLayer().empty)

        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.canEditAsMaya(primPathStr))
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(primPathStr))

        self.assertFalse(stage.GetSessionLayer().empty)

        kPullPrimMetadataKey = "Maya:Pull:DagPath"
        self.assertEqual(prim.GetCustomDataByKey(kPullPrimMetadataKey), "|__mayaUsd__|AParent|A")

        # Discard Maya edits, but there is nothing to discard.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.discardEdits("A"))

        self.assertTrue(stage.GetSessionLayer().empty)

        self.assertEqual(prim.GetCustomDataByKey(kPullPrimMetadataKey), None)

if __name__ == '__main__':
    unittest.main(verbosity=2)
