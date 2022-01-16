#!/usr/bin/env mayapy
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

import mayaUsd.lib as mayaUsdLib

from mayaUtils import setMayaTranslation
from usdUtils import createSimpleXformScene

from pxr import Usd

from maya import cmds
from maya import standalone

from maya.api import OpenMaya as om

import ufe

import fixturesUtils, os

import unittest

class primUpdaterTest(mayaUsdLib.PrimUpdater):
    pushCopySpecsCalled = False
    discardEditsCalled = False
    editAsMayaCalled = False
    pushEndCalled = False

    def __init__(self, *args, **kwargs):
        super(primUpdaterTest, self).__init__(*args, **kwargs)

    def pushCopySpecs(self, srcStage, srcLayer, srcSdfPath, dstStage, dstLayer, dstSdfPath):
        primUpdaterTest.pushCopySpecsCalled = True
        return super(primUpdaterTest, self).pushCopySpecs(srcStage, srcLayer, srcSdfPath, dstStage, dstLayer, dstSdfPath)

    def editAsMaya(self,context):
        primUpdaterTest.editAsMayaCalled = True
        return super(primUpdaterTest, self).editAsMaya(context)

    def discardEdits(self,context):
        primUpdaterTest.discardEditsCalled = True
        return super(primUpdaterTest, self).discardEdits(context)

    def pushEnd(self,context):
        primUpdaterTest.pushEndCalled = True
        return super(primUpdaterTest, self).pushEnd(context)

class testPrimUpdater(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testSimplePrimUpdater(self):
        mayaUsdLib.PrimUpdater.Register(primUpdaterTest, "UsdGeomXform", "transform", primUpdaterTest.Supports.All.value) # primUpdaterTest.Supports.Push.value + primUpdaterTest.Supports.Clear.value + primUpdaterTest.Supports.AutoPull.value)

        # Edit as Maya first time.
        (ps, xlateOp, usdXlation, aUsdUfePathStr, aUsdUfePath, aUsdItem, _, _, _, _, _) = createSimpleXformScene()
        with mayaUsdLib.OpUndoItemList():
            self.assertTrue(mayaUsdLib.PrimUpdaterManager.editAsMaya(aUsdUfePathStr))

        aMayaItem = ufe.GlobalSelection.get().front()
        (aMayaPath, aMayaPathStr, _, aMayaMatrix) = setMayaTranslation(aMayaItem, om.MVector(4, 5, 6))

        # Discard Maya edits.
        with mayaUsdLib.OpUndoItemList():
            self.assertTrue(mayaUsdLib.PrimUpdaterManager.discardEdits(aMayaPathStr))

        cmds.file(new=True, force=True)

        # Edit as Maya second time.
        (ps, xlateOp, usdXlation, aUsdUfePathStr, aUsdUfePath, aUsdItem, _, _, _, _, _) = createSimpleXformScene()
        with mayaUsdLib.OpUndoItemList():
            self.assertTrue(mayaUsdLib.PrimUpdaterManager.editAsMaya(aUsdUfePathStr))

        aMayaItem = ufe.GlobalSelection.get().front()
        (aMayaPath, aMayaPathStr, _, aMayaMatrix) = setMayaTranslation(aMayaItem, om.MVector(4, 5, 6))

        # Merge edits back to USD.
        with mayaUsdLib.OpUndoItemList():
            self.assertTrue(mayaUsdLib.PrimUpdaterManager.mergeToUsd(aMayaPathStr))

        self.assertTrue(primUpdaterTest.editAsMayaCalled)
        self.assertTrue(primUpdaterTest.discardEditsCalled)
        self.assertTrue(primUpdaterTest.pushCopySpecsCalled)
        self.assertTrue(primUpdaterTest.pushEndCalled)

if __name__ == '__main__':
    unittest.main(verbosity=2)
