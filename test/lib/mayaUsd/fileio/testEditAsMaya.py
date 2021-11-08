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

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '3006', 'Test only available in UFE preview version 0.3.6 and greater')
    def testTransformEditAsMaya(self):
        '''Edit a USD transform as a Maya object.'''

        (ps, xlateOp, xlation, aUsdUfePathStr, aUsdUfePath, aUsdItem,
         _, _, _, _, _) = createSimpleXformScene()

        # Edit aPrim as Maya data.
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

if __name__ == '__main__':
    unittest.main(verbosity=2)
