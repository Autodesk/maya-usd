#!/usr/bin/env python

#
# Copyright 2022 Autodesk
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

import mayaUsd
import mayaUtils

from maya import cmds
from maya import standalone
from mayaUsdLibRegisterStrings import getMayaUsdLibString
from mayaUSDRegisterStrings import getMayaUsdString

import unittest

class AddMayaReferenceTestCase(unittest.TestCase):
    '''Test Add Maya Reference.
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

    def createSimpleMayaScene(self):
        import os
        import maya.cmds as cmds
        import tempfile

        cmds.CreatePolygonSphere()
        tempMayaFile = os.path.join(tempfile.gettempdir(), 'simpleSphere.ma')
        cmds.file(rename=tempMayaFile)
        cmds.file(save=True, force=True, type='mayaAscii')
        return tempMayaFile

    def testDefineInVariant(self):
        '''Test the "Define in Variant" options for Add Maya Reference.'''

        # First we need a pure Maya scene to reference in.
        mayaSceneStr = self.createSimpleMayaScene()

        # Then create an Xform and add the maya reference.
        cmds.file(new=True, force=True)
        import mayaUsd_createStageWithNewLayer
        proxyShapePathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()
        primA = stage.DefinePrim('/A', 'Xform')
        primPathStr = proxyShapePathStr + ',/A'

        # Since this is a brand new prim, it should not have variant sets.
        primA = mayaUsd.ufe.ufePathToPrim(primPathStr)
        self.assertFalse(primA.HasVariantSets())

        kDefaultVariantSetName = getMayaUsdLibString('kMayaRefDefaultVariantSetName')
        kDefaultVariantName = getMayaUsdLibString('kMayaRefDefaultVariantName')

        import mayaUsdAddMayaReference
        mayaUsdAddMayaReference.createMayaReferencePrim(
            primPathStr,
            mayaSceneStr,
            'simpleSphere', kDefaultVariantSetName, kDefaultVariantName)

        # Make sure the prim has the variant set and variant.
        self.assertTrue(primA.HasVariantSets())
        vset = primA.GetVariantSet(kDefaultVariantSetName)
        self.assertTrue(vset.IsValid())
        self.assertEqual(vset.GetName(), kDefaultVariantSetName)
        self.assertTrue(vset.GetVariantNames())
        self.assertTrue(vset.HasAuthoredVariant(kDefaultVariantName))
        self.assertEqual(vset.GetVariantSelection(), kDefaultVariantName)

        # Create another prim to test sanitizing variant name.
        primB = stage.DefinePrim('/B', 'Xform')
        primPathStr = proxyShapePathStr + ',/B'
        primB = mayaUsd.ufe.ufePathToPrim(primPathStr)

        kBadVariantSetName = 'No Spaces or Special Chars#$%^&'
        kGoodVariantSetName = 'No_Spaces_or_Special_Chars_'
        kBadVariantName = '39no start digits+-*/='
        kGoodVariantName = 'no_start_digits_'
        mayaUsdAddMayaReference.createMayaReferencePrim(
            primPathStr,
            mayaSceneStr,
            'simpleSphere', kBadVariantSetName, kBadVariantName)

        # Make sure the prim has the variant set and variant with
        # the sanitized names.
        self.assertTrue(primB.HasVariantSets())
        vset = primB.GetVariantSet(kGoodVariantSetName)
        self.assertTrue(vset.IsValid())
        self.assertEqual(vset.GetName(), kGoodVariantSetName)
        self.assertTrue(vset.GetVariantNames())
        self.assertTrue(vset.HasAuthoredVariant(kGoodVariantName))
        self.assertEqual(vset.GetVariantSelection(), kGoodVariantName)

if __name__ == '__main__':
    unittest.main(verbosity=2)
