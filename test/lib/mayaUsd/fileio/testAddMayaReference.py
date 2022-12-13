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
import ufeUtils
import usdUtils

from pxr import Tf, Usd, Kind

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as om

import mayaUsdAddMayaReference
import mayaUsdMayaReferenceUtils as mayaRefUtils
from mayaUsd.lib import cacheToUsd

import ufe

import os, unittest

class AddMayaReferenceTestCase(unittest.TestCase):
    '''Test Add Maya Reference.
    '''
    pluginsLoaded = False
    mayaSceneStr = None
    stage = None
    kDefaultNamespace = 'simpleSphere'

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

        # Create a pure Maya scene to reference in.
        import os
        cls.mayaSceneStr = mayaUtils.createSingleSphereMayaScene(os.getcwd())

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def getCacheFileName(self):
        return 'testAddMayaRefCache.usda'

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
        import mayaUsd_createStageWithNewLayer
        self.proxyShapePathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        self.stage = mayaUsd.lib.GetPrim(self.proxyShapePathStr).GetStage()
        self.removeCacheFile()

    def tearDown(self):
        self.removeCacheFile()

    def testDefault(self):
        '''Test the default options for Add Maya Reference.

        Add a Maya Reference using the defaults (no group or variant).
        '''
        kDefaultPrimName = mayaRefUtils.defaultMayaReferencePrimName()

        # Since this is a brand new prim, it should not have variant sets.
        primTestDefault = self.stage.DefinePrim('/Test_Default', 'Xform')
        primPathStr = self.proxyShapePathStr + ',/Test_Default'
        self.assertFalse(primTestDefault.HasVariantSets())

        mayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            primPathStr,
            self.mayaSceneStr,
            self.kDefaultNamespace)

        # The prim should not have any variant set.
        self.assertFalse(primTestDefault.HasVariantSets())

        # Verify that a Maya Reference prim was created.
        self.assertTrue(mayaRefPrim.IsValid())
        self.assertEqual(str(mayaRefPrim.GetName()), kDefaultPrimName)
        self.assertEqual(mayaRefPrim, primTestDefault.GetChild(kDefaultPrimName))
        self.assertTrue(mayaRefPrim.GetPrimTypeInfo().GetTypeName(), 'MayaReference')

        # Test an error creating the Maya reference prim by disabling permission to edit on the
        # edit target layer.
        editTarget = self.stage.GetEditTarget()
        editLayer = editTarget.GetLayer()
        editLayer.SetPermissionToEdit(False)
        badMayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            primPathStr,
            self.mayaSceneStr,
            self.kDefaultNamespace,
            mayaReferencePrimName='BadMayaReference')
        self.assertFalse(badMayaRefPrim.IsValid())
        editLayer.SetPermissionToEdit(True)

    def testDefineInVariant(self):
        '''Test the "Define in Variant" options.

        Add a Maya Reference with a (default) variant set.
        '''
        kDefaultPrimName = mayaRefUtils.defaultMayaReferencePrimName()
        kDefaultVariantSetName = mayaRefUtils.defaultVariantSetName()
        kDefaultVariantName = mayaRefUtils.defaultVariantName()

        # Create another prim with default variant set and name.
        primTestVariant = self.stage.DefinePrim('/Test_Variant', 'Xform')
        primPathStr = self.proxyShapePathStr + ',/Test_Variant'

        mayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            primPathStr,
            self.mayaSceneStr,
            self.kDefaultNamespace,
            variantSet=(kDefaultVariantSetName, kDefaultVariantName),
            mayaAutoEdit=True)

        # Make sure the prim has the variant set and variant.
        self.assertTrue(primTestVariant.HasVariantSets())
        vset = primTestVariant.GetVariantSet(kDefaultVariantSetName)
        self.assertTrue(vset.IsValid())
        self.assertEqual(vset.GetName(), kDefaultVariantSetName)
        self.assertTrue(vset.GetVariantNames())
        self.assertTrue(vset.HasAuthoredVariant(kDefaultVariantName))
        self.assertEqual(vset.GetVariantSelection(), kDefaultVariantName)

        # Verify that a Maya Reference prim was created.
        self.assertTrue(mayaRefPrim.IsValid())
        self.assertEqual(str(mayaRefPrim.GetName()), kDefaultPrimName)
        self.assertEqual(mayaRefPrim, primTestVariant.GetChild(kDefaultPrimName))
        self.assertTrue(mayaRefPrim.GetPrimTypeInfo().GetTypeName(), 'MayaReference')

        # Verify that the Maya reference prim is inside the variant,
        # and that it has the expected metadata.
        attr = mayaRefPrim.GetAttribute('mayaReference')
        self.assertTrue(attr.IsValid())
        self.assertTrue(os.path.samefile(attr.Get().resolvedPath, self.mayaSceneStr))
        attr = mayaRefPrim.GetAttribute('mayaNamespace')
        self.assertTrue(attr.IsValid())
        self.assertEqual(attr.Get(), self.kDefaultNamespace)
        attr = mayaRefPrim.GetAttribute('mayaAutoEdit')
        self.assertTrue(attr.IsValid())
        self.assertEqual(attr.Get(),True)

        # Test an error creating the Variant Set by disabling permission to edit on the
        # edit target layer.
        editTarget = self.stage.GetEditTarget()
        editLayer = editTarget.GetLayer()
        editLayer.SetPermissionToEdit(False)
        badMayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            primPathStr,
            self.mayaSceneStr,
            self.kDefaultNamespace,
            mayaReferencePrimName='PrimVariantFail',
            variantSet=('VariantFailSet', 'VariantNameFail'),
            mayaAutoEdit=False)
        self.assertFalse(badMayaRefPrim.IsValid())
        editLayer.SetPermissionToEdit(True)

    def testEditAndDiscardMayaRef(self):
        '''Test editing then discarding a Maya Reference.

        Add a Maya Reference using auto-edit, then discard the edits.
        '''
        kDefaultPrimName = mayaRefUtils.defaultMayaReferencePrimName()

        # Since this is a brand new prim, it should not have variant sets.
        primTestDefault = self.stage.DefinePrim('/Test_Default', 'Xform')
        primPathStr = self.proxyShapePathStr + ',/Test_Default'
        self.assertFalse(primTestDefault.HasVariantSets())

        mayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            primPathStr,
            self.mayaSceneStr,
            self.kDefaultNamespace,
            mayaAutoEdit=True)

        # The prim should not have any variant set.
        self.assertFalse(primTestDefault.HasVariantSets())

        # Verify that a Maya Reference prim was created.
        self.assertTrue(mayaRefPrim.IsValid())
        self.assertEqual(str(mayaRefPrim.GetName()), kDefaultPrimName)
        self.assertEqual(mayaRefPrim, primTestDefault.GetChild(kDefaultPrimName))
        self.assertTrue(mayaRefPrim.GetPrimTypeInfo().GetTypeName(), 'MayaReference')

        attr = mayaRefPrim.GetAttribute('mayaAutoEdit')
        self.assertTrue(attr.IsValid())
        self.assertEqual(attr.Get(), True)

        # Discard Maya edits.
        aMayaItem = ufe.GlobalSelection.get().front()
        aMayaPath = aMayaItem.path()
        aMayaPathStr = ufe.PathString.string(aMayaPath)
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.discardEdits(aMayaPathStr))
        
        # Verify that the auto-edit has been turned off
        attr = mayaRefPrim.GetAttribute('mayaAutoEdit')
        self.assertTrue(attr.IsValid())
        self.assertEqual(attr.Get(), False)


    def testDiscardMayaRefUnderDeactivatedPrim(self):
        '''
        Test editing a Maya Reference, then deactivitng its parent, then discarding.
        '''
        kDefaultPrimName = mayaRefUtils.defaultMayaReferencePrimName()

        # Since this is a brand new prim, it should not have variant sets.
        primTestDefault = self.stage.DefinePrim('/Test_Default', 'Xform')
        primPathStr = self.proxyShapePathStr + ',/Test_Default'
        self.assertFalse(primTestDefault.HasVariantSets())

        mayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            primPathStr,
            self.mayaSceneStr,
            self.kDefaultNamespace,
            mayaAutoEdit=True)
        mayaRefPrimPathStr = self.proxyShapePathStr + ',' + str(mayaRefPrim.GetPath())

        # The prim should not have any variant set.
        self.assertFalse(primTestDefault.HasVariantSets())

        # Verify that a Maya Reference prim was created.
        self.assertTrue(mayaRefPrim.IsValid())
        self.assertEqual(str(mayaRefPrim.GetName()), kDefaultPrimName)
        self.assertEqual(mayaRefPrim, primTestDefault.GetChild(kDefaultPrimName))
        self.assertTrue(mayaRefPrim.GetPrimTypeInfo().GetTypeName(), 'MayaReference')

        attr = mayaRefPrim.GetAttribute('mayaAutoEdit')
        self.assertTrue(attr.IsValid())
        self.assertEqual(attr.Get(), True)

        # Deactivate the parent of the Maya reference.
        mayaRefPrim = None
        primTestDefault.SetActive(False)
        self.assertFalse(primTestDefault.IsActive())

        # Discard Maya edits.
        aMayaItem = ufe.GlobalSelection.get().front()
        aMayaPath = aMayaItem.path()
        aMayaPathStr = ufe.PathString.string(aMayaPath)
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.discardEdits(aMayaPathStr))
        
        # Activate the parent of the Maya reference.
        primTestDefault.SetActive(True)
        self.assertTrue(primTestDefault.IsActive())

        # Verify that the auto-edit has *not* been turned off since it could not
        # be edited when discarding edits since the prim was not accessible since
        # its parent prim was deactivated.
        #
        # Note: we have to retrieved the Maya ref prim again because when its parent
        #       was deactivated, the UsdPrim object became invalid and cannot be used
        #       anymore.
        mayaRefUsdItem = ufeUtils.createItem(mayaRefPrimPathStr)
        mayaRefPrim = usdUtils.getPrimFromSceneItem(mayaRefUsdItem)
        attr = mayaRefPrim.GetAttribute('mayaAutoEdit')
        self.assertTrue(attr.IsValid())
        self.assertEqual(attr.Get(), True)
        self.assertFalse(mayaRefPrim.IsActive())


    def testEditAndMergeMayaRef(self):
        '''Test editing then merge a Maya Reference.

        Add a Maya Reference using auto-edit, then merge the edits.
        '''
        kDefaultPrimName = mayaRefUtils.defaultMayaReferencePrimName()

        # Since this is a brand new prim, it should not have variant sets.
        primTestDefault = self.stage.DefinePrim('/Test_Default', 'Xform')
        primPathStr = self.proxyShapePathStr + ',/Test_Default'
        self.assertFalse(primTestDefault.HasVariantSets())

        variantSetName = 'new_variant_set'
        variantName =  'new_variant'
        cacheVariantName = 'new_cache'

        mayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            primPathStr,
            self.mayaSceneStr,
            self.kDefaultNamespace,
            variantSet=(variantSetName, variantName),
            mayaAutoEdit=True)

        # The prim should have a variant set.
        self.assertTrue(primTestDefault.HasVariantSets())
        vs = primTestDefault.GetVariantSets()
        variantSet = vs.GetVariantSet(variantSetName)
        self.assertEqual(variantSet.GetVariantSelection(), variantName)

        # Verify that a Maya Reference prim was created.
        self.assertTrue(mayaRefPrim.IsValid())
        self.assertEqual(str(mayaRefPrim.GetName()), kDefaultPrimName)
        self.assertEqual(mayaRefPrim, primTestDefault.GetChild(kDefaultPrimName))
        self.assertTrue(mayaRefPrim.GetPrimTypeInfo().GetTypeName(), 'MayaReference')

        attr = mayaRefPrim.GetAttribute('mayaAutoEdit')
        self.assertTrue(attr.IsValid())
        self.assertEqual(attr.Get(), True)

        # Merge Maya edits.
        # Cache to USD, using a payload composition arc.
        defaultExportOptions = cacheToUsd.getDefaultExportOptions()
        cacheFile = self.getCacheFileName()
        cachePrimName = 'cachePrimName'
        payloadOrReference = 'Payload'
        listEditType = 'Prepend'
        # In the sibling cache case variantSetName and cacheVariantName will be
        # None.
        cacheOptions = cacheToUsd.createCacheCreationOptions(
            defaultExportOptions, cacheFile, cachePrimName,
            payloadOrReference, listEditType, variantSetName, cacheVariantName)

        aMayaItem = ufe.GlobalSelection.get().front()
        aMayaPath = aMayaItem.path()
        aMayaPathStr = ufe.PathString.string(aMayaPath)
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.mergeToUsd(aMayaPathStr, cacheOptions))

        # Verify the cache variant is active and the prim
        # is active.
        self.assertTrue(primTestDefault.HasVariantSets())
        vs = primTestDefault.GetVariantSets()
        variantSet = vs.GetVariantSet(variantSetName)
        self.assertEqual(variantSet.GetVariantSelection(), cacheVariantName)
        self.assertTrue(mayaRefPrim.IsActive())

        # Switch to Maya Ref variant and verify that the auto-edit is
        # still on and the prim is now inactive.
        variantSet.SetVariantSelection(variantName)
        self.assertFalse(mayaRefPrim.IsActive())
        attr = mayaRefPrim.GetAttribute('mayaAutoEdit')
        self.assertTrue(attr.IsValid())
        self.assertEqual(attr.Get(), True)


    def testBadNames(self):
        '''Test using bad prim and variant names.

        Add a Maya Reference using a bad Maya Reference prim name and
        bad Variant Set and Variant name.
        '''
        kDefaultPrimName = mayaRefUtils.defaultMayaReferencePrimName()

        # Create another prim to test sanitizing variant set and name.
        primTestSanitizeVariant = self.stage.DefinePrim('/Test_SanitizeVariant', 'Xform')
        primPathStr = self.proxyShapePathStr + ',/Test_SanitizeVariant'

        kBadPrimName = ('3'+kDefaultPrimName+'$')
        kGoodPrimName = Tf.MakeValidIdentifier(kBadPrimName)
        kBadVariantSetName = 'No Spaces or Special#Chars'
        kGoodVariantSetName = Tf.MakeValidIdentifier(kBadVariantSetName)
        kBadVariantName = '3no start digits'
        kGoodVariantName = Tf.MakeValidIdentifier(kBadVariantName)
        mayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            primPathStr,
            self.mayaSceneStr,
            self.kDefaultNamespace,
            mayaReferencePrimName=kBadPrimName,
            variantSet=(kBadVariantSetName, kBadVariantName))

        # Make sure the prim has the variant set and variant with
        # the sanitized names.
        self.assertTrue(primTestSanitizeVariant.HasVariantSets())
        vset = primTestSanitizeVariant.GetVariantSet(kGoodVariantSetName)
        self.assertTrue(vset.IsValid())
        self.assertEqual(vset.GetName(), kGoodVariantSetName)
        self.assertTrue(vset.GetVariantNames())
        self.assertTrue(vset.HasAuthoredVariant(kGoodVariantName))
        self.assertEqual(vset.GetVariantSelection(), kGoodVariantName)

        # Verify that the prim was created with the good name.
        self.assertTrue(mayaRefPrim.IsValid())
        self.assertEqual(str(mayaRefPrim.GetName()), kGoodPrimName)
        self.assertEqual(mayaRefPrim, primTestSanitizeVariant.GetChild(kGoodPrimName))
        self.assertTrue(mayaRefPrim.GetPrimTypeInfo().GetTypeName(), 'MayaReference')

        # Adding a Maya Reference with the same name should produce an error.
        mayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            primPathStr,
            self.mayaSceneStr,
            self.kDefaultNamespace,
            mayaReferencePrimName=kGoodPrimName)
        self.assertFalse(mayaRefPrim.IsValid())

    def testGroup(self):
        '''Test the "Group" options.

        Add a Maya Reference using a group.
        '''
        kDefaultPrimName = mayaRefUtils.defaultMayaReferencePrimName()
        kDefaultVariantSetName = mayaRefUtils.defaultVariantSetName()
        kDefaultVariantName = mayaRefUtils.defaultVariantName()

        # Create another prim to test adding a group prim (with variant).
        primTestGroup = self.stage.DefinePrim('/Test_Group', 'Xform')
        primPathStr = self.proxyShapePathStr + ',/Test_Group'

        mayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            primPathStr,
            self.mayaSceneStr,
            self.kDefaultNamespace,
            groupPrim=('Xform', Kind.Tokens.group),
            variantSet=(kDefaultVariantSetName, kDefaultVariantName))

        # Make sure a group prim was created.
        # Since we did not provide a group name, one will have been auto-generated for us.
        #   "namespace" + "RN" + "group"
        primGroup = primTestGroup.GetChild(self.kDefaultNamespace+'RNgroup')
        self.assertTrue(primGroup.IsValid())
        self.assertTrue(primGroup.GetPrimTypeInfo().GetTypeName(), 'Xform')
        model = Usd.ModelAPI(primGroup)
        self.assertEqual(model.GetKind(), Kind.Tokens.group)

        # Make sure the group prim has the variant set and variant.
        self.assertTrue(primGroup.HasVariantSets())
        vset = primGroup.GetVariantSet(kDefaultVariantSetName)
        self.assertTrue(vset.IsValid())
        self.assertEqual(vset.GetName(), kDefaultVariantSetName)
        self.assertTrue(vset.GetVariantNames())
        self.assertTrue(vset.HasAuthoredVariant(kDefaultVariantName))
        self.assertEqual(vset.GetVariantSelection(), kDefaultVariantName)

        # Verify that a Maya Reference prim was created under the new group prim.
        self.assertTrue(mayaRefPrim.IsValid())
        self.assertEqual(str(mayaRefPrim.GetName()), kDefaultPrimName)
        self.assertEqual(mayaRefPrim, primGroup.GetChild(kDefaultPrimName))
        self.assertTrue(mayaRefPrim.GetPrimTypeInfo().GetTypeName(), 'MayaReference')

        # Add another Maya reference with group, but name the group this time and
        # use a 'Scope' prim instead.
        kGroupName = 'NewGroup'
        mayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            primPathStr,
            self.mayaSceneStr,
            self.kDefaultNamespace,
            groupPrim=(kGroupName, 'Scope', Kind.Tokens.group))

        # Make sure a group prim was created and what we named it.
        prim2ndGroup = primTestGroup.GetChild(kGroupName)
        self.assertTrue(prim2ndGroup.IsValid())
        self.assertTrue(prim2ndGroup.GetPrimTypeInfo().GetTypeName(), 'Scope')
        model = Usd.ModelAPI(prim2ndGroup)
        self.assertEqual(model.GetKind(), Kind.Tokens.group)

        # Verify that a Maya Reference prim was created under the new group prim.
        self.assertTrue(mayaRefPrim.IsValid())
        self.assertEqual(str(mayaRefPrim.GetName()), kDefaultPrimName)
        self.assertEqual(mayaRefPrim, prim2ndGroup.GetChild(kDefaultPrimName))
        self.assertTrue(mayaRefPrim.GetPrimTypeInfo().GetTypeName(), 'MayaReference')

        # Adding a group with the same name should produce an error.
        mayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            primPathStr,
            self.mayaSceneStr,
            self.kDefaultNamespace,
            groupPrim=(kGroupName, 'Scope', Kind.Tokens.group))
        self.assertFalse(mayaRefPrim.IsValid())

        # Test an error creating the group prim by disabling permission to edit on the edit target layer.
        editTarget = self.stage.GetEditTarget()
        editLayer = editTarget.GetLayer()
        editLayer.SetPermissionToEdit(False)
        badMayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            primPathStr,
            self.mayaSceneStr,
            self.kDefaultNamespace,
            groupPrim=('NoGroup', 'Xform', Kind.Tokens.group))
        self.assertFalse(badMayaRefPrim.IsValid())
        invalidGroupPrim = primTestGroup.GetChild('NoGroup')
        self.assertFalse(invalidGroupPrim.IsValid())
        editLayer.SetPermissionToEdit(True)

    def testProxyShape(self):
        '''Test adding a Maya Reference directly undereath the proxy shape.

        Add a Maya Reference using the defaults (no group or variant).
        '''
        kDefaultPrimName = mayaRefUtils.defaultMayaReferencePrimName()

        mayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            self.proxyShapePathStr,
            self.mayaSceneStr,
            self.kDefaultNamespace)

        # Verify that a Maya Reference prim was created.
        self.assertTrue(mayaRefPrim.IsValid())
        self.assertEqual(str(mayaRefPrim.GetName()), kDefaultPrimName)
        self.assertTrue(mayaRefPrim.GetPrimTypeInfo().GetTypeName(), 'MayaReference')

        # We should get an error (invalid prim) when adding a Maya reference under
        # the proxy shape when we also add a variant set.
        kDefaultVariantSetName = mayaRefUtils.defaultVariantSetName()
        kDefaultVariantName = mayaRefUtils.defaultVariantName()
        mayaRefPrim = mayaUsdAddMayaReference.createMayaReferencePrim(
            self.proxyShapePathStr,
            self.mayaSceneStr,
            self.kDefaultNamespace,
            variantSet=(kDefaultVariantSetName, kDefaultVariantName))
        self.assertFalse(mayaRefPrim.IsValid())


if __name__ == '__main__':
    unittest.main(verbosity=2)
