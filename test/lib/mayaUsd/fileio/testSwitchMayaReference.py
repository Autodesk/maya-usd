#!/usr/bin/env python

#
# Copyright 2023 Autodesk
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
import ufeUtils
import usdUtils
import testUtils

from pxr import Tf, Usd, Kind, Sdf

from maya import cmds
import maya.mel as mel
from maya import standalone
from maya.api import OpenMaya as om

import mayaUsdAddMayaReference
import mayaUsdMayaReferenceUtils as mayaRefUtils
from mayaUsd.lib import cacheToUsd

import ufe

import os, unittest

class SwitchMayaReferenceTestCase(unittest.TestCase):
    '''Test Switching Maya Reference when switching variants.
    '''
    pluginsLoaded = False
    mayaSceneStr = None
    stage = None
    kDefaultNamespace = 'simpleSphere'

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def getCacheFileName(self):
        return 'testSwitchMayaRefCache.usda'

    def removeCacheFile(self):
        '''
        Remove the cache file if it exists. Ignore error if it does not exists.
        '''
        try:
            os.remove(self.getCacheFileName())
        except:
            pass

    def setUp(self):
        # Start each test with a new scene with empty stage.
        cmds.file(new=True, force=True)
        self.removeCacheFile()

    def tearDown(self):
        self.removeCacheFile()

    def testMultiRefInMultiVariants(self):
        '''
        Test that switching variant in a stage that contains multiple prims
        of the Maya Reference type with the same name but under different
        variant works:
            - The Maya references are loaded and unload as the variant are switched,
            - The Maya reference can be cached.
        '''
        test_file = testUtils.getTestScene("switchVariants", "asset.usda")
        ps, stage = mayaUtils.createProxyFromFile(test_file)

        prim_with_variant = stage.GetPrimAtPath('/cube')
        self.assertTrue(prim_with_variant)

        variant_set = 'rig'
        no_rig_variant = 'none'
        anim_rig_variant = 'anim'
        layout_rig_variant = 'layout'

        anim_rig_path = '|__mayaUsd__|rigParent'
        layout_rig_path = '|__mayaUsd__|rigParent1'

        # Verify none of the rigs are loaded.
        def verify_rig_loaded(name, loaded):
            item = ufeUtils.createItem(name)
            if item:
                self.assertEqual(cmds.getAttr(name + '.visibility'), loaded)
            else:
                self.assertFalse(loaded)

        verify_rig_loaded(anim_rig_path, False)
        verify_rig_loaded(layout_rig_path, False)

        # Switch to anim variant and verify the presence of the edited
        # Maya reference.
        prim_with_variant.GetVariantSet(variant_set).SetVariantSelection(anim_rig_variant)
        verify_rig_loaded(anim_rig_path, True)
        verify_rig_loaded(layout_rig_path, False)

        # Switch to layout variant and verify the presence of the edited
        # Maya reference.
        prim_with_variant.GetVariantSet(variant_set).SetVariantSelection(layout_rig_variant)
        verify_rig_loaded(anim_rig_path, False)
        verify_rig_loaded(layout_rig_path, True)

        # Switch to none variant and verify the absence of all references.
        prim_with_variant.GetVariantSet(variant_set).SetVariantSelection(no_rig_variant)
        verify_rig_loaded(anim_rig_path, False)
        verify_rig_loaded(layout_rig_path, False)

        # Switch to anim variant and verify the presence of the edited
        # Maya reference.
        prim_with_variant.GetVariantSet(variant_set).SetVariantSelection(anim_rig_variant)
        verify_rig_loaded(anim_rig_path, True)
        verify_rig_loaded(layout_rig_path, False)

        # Cache to USD anim rig and verify the Maya rig is gone.
        defaultExportOptions = cacheToUsd.getDefaultExportOptions()
        cacheFile = self.getCacheFileName()
        cachePrimName = 'cachePrimName'
        payloadOrReference = 'Payload'
        listEditType = 'Prepend'
        cacheVariantName = 'cache'
        relativePath = True
        # In the sibling cache case variantSetName and cacheVariantName will be
        # None.
        cacheOptions = cacheToUsd.createCacheCreationOptions(
            defaultExportOptions, cacheFile, cachePrimName,
            payloadOrReference, listEditType, variant_set, cacheVariantName, relativePath)
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.mergeToUsd(anim_rig_path + '|rig', cacheOptions))
        
        verify_rig_loaded(anim_rig_path, False)
        verify_rig_loaded(layout_rig_path, False)

        # Switch to layout variant and verify the presence of the edited
        # Maya reference.
        prim_with_variant.GetVariantSet(variant_set).SetVariantSelection(layout_rig_variant)
        verify_rig_loaded(anim_rig_path, False)
        verify_rig_loaded(layout_rig_path, True)

        # Discard edits on layout rig and verify the Maya rig is gone.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.discardEdits(layout_rig_path + '|rig'))
        verify_rig_loaded(anim_rig_path, False)
        verify_rig_loaded(layout_rig_path, False)


if __name__ == '__main__':
    unittest.main(verbosity=2)
