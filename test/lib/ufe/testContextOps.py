#!/usr/bin/env python

#
# Copyright 2020 Autodesk
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
import mayaUtils
import usdUtils
import ufeUtils

from pxr import UsdGeom
from pxr import UsdShade
from pxr import Sdf
from pxr import Usd

from maya import cmds
from maya import standalone
from maya.internal.ufeSupport import ufeCmdWrapper as ufeCmd

import ufe

import os
import unittest


class TestAddPrimObserver(ufe.Observer):
    def __init__(self):
        super(TestAddPrimObserver, self).__init__()
        self.deleteNotif = 0
        self.addNotif = 0

    def __call__(self, notification):
        if isinstance(notification, ufe.ObjectDelete):
            self.deleteNotif += 1
        if isinstance(notification, ufe.ObjectAdd):
            self.addNotif += 1

    def nbDeleteNotif(self):
        return self.deleteNotif

    def nbAddNotif(self):
        return self.addNotif

    def reset(self):
        self.addNotif = 0
        self.deleteNotif = 0

class ContextOpsTestCase(unittest.TestCase):
    '''Verify the ContextOps interface for the USD runtime.'''

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
        ''' Called initially to set up the maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

        # These tests requires no additional setup.
        if self._testMethodName in ['testAddNewPrim', 'testAddNewPrimWithDelete']:
            return

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

        # Clear selection to start off.
        ufe.GlobalSelection.get().clear()
        
        # Select Ball_35.
        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment("|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        self.ball35Item = ufe.Hierarchy.createItem(ball35Path)
        self.ball35Prim = usdUtils.getPrimFromSceneItem(self.ball35Item)

        ufe.GlobalSelection.get().append(self.ball35Item)

        # Create a ContextOps interface for it.
        self.contextOps = ufe.ContextOps.contextOps(self.ball35Item)

    def testGetItems(self):
        # The top-level context items are:
        # - working set management for loadable prims and descendants
        # - visibility (for all prims with visibility attribute)
        # - variant sets, for those prims that have them (such as Ball_35)
        # - add new prim (for all prims)
        # - prim activate/deactivate
        # - prim mark/unmark as instanceable
        contextItems = self.contextOps.getItems([])
        contextItemStrings = [c.item for c in contextItems]

        attrs = ufe.Attributes.attributes(self.contextOps.sceneItem())
        self.assertIsNotNone(attrs)
        hasVisibility = attrs.hasAttribute(UsdGeom.Tokens.visibility)

        # The proxy shape specifies loading all payloads, so Ball_35 should be
        # loaded, and since it has a payload, there should be an item for
        # unloading it.
        self.assertIn('Unload', contextItemStrings)
        self.assertIn('Variant Sets', contextItemStrings)
        if hasVisibility:
            self.assertIn('Toggle Visibility', contextItemStrings)
        else:
            self.assertNotIn('Toggle Visibility', contextItemStrings)
        self.assertIn('Toggle Active State', contextItemStrings)
        self.assertIn('Toggle Instanceable State', contextItemStrings)
        self.assertIn('Add New Prim', contextItemStrings)

        # Ball_35 has a single variant set, which has children.
        contextItems = self.contextOps.getItems(['Variant Sets'])
        contextItemStrings = [c.item for c in contextItems]

        self.assertListEqual(['shadingVariant'], contextItemStrings)
        self.assertTrue(contextItems[0].hasChildren)

        # Initial shadingVariant is "Ball_8"
        contextItems = self.contextOps.getItems(
            ['Variant Sets', 'shadingVariant'])
        self.assertGreater(len(contextItems), 1)
        for c in contextItems:
            self.assertFalse(c.hasChildren)
            self.assertTrue(c.checkable)
            if c.checked:
                self.assertEqual(c.item, 'Ball_8')

    def testDoOp(self):

        # Change visibility, undo / redo.
        cmd = self.contextOps.doOpCmd(['Toggle Visibility'])
        self.assertIsNotNone(cmd)

        attrs = ufe.Attributes.attributes(self.ball35Item)
        self.assertIsNotNone(attrs)
        visibility = attrs.attribute(UsdGeom.Tokens.visibility)
        self.assertIsNotNone(visibility)

        # Initially, Ball_35 has inherited visibility.
        self.assertEqual(visibility.get(), UsdGeom.Tokens.inherited)
        
        ufeCmd.execute(cmd)

        self.assertEqual(visibility.get(), UsdGeom.Tokens.invisible)
        
        cmds.undo()

        self.assertEqual(visibility.get(), UsdGeom.Tokens.inherited)

        cmds.redo()

        self.assertEqual(visibility.get(), UsdGeom.Tokens.invisible)
        
        cmds.undo()

        # Active / Deactivate Prim
        cmd = self.contextOps.doOpCmd(['Toggle Active State'])
        self.assertIsNotNone(cmd)

        # Initially, Ball_35 should be active.
        self.assertTrue(self.ball35Prim.IsActive())

        ufeCmd.execute(cmd)
        self.assertFalse(self.ball35Prim.IsActive())

        cmds.undo()
        self.assertTrue(self.ball35Prim.IsActive())

        cmds.redo()
        self.assertFalse(self.ball35Prim.IsActive())

        # Mark / Unmark Prim as Instanceable
        cmd = self.contextOps.doOpCmd(['Toggle Instanceable State'])
        self.assertIsNotNone(cmd)

        # Initially, Ball_35 should be not instanceable.
        self.assertFalse(self.ball35Prim.IsInstanceable())

        ufeCmd.execute(cmd)
        self.assertTrue(self.ball35Prim.IsInstanceable())

        cmds.undo()
        self.assertFalse(self.ball35Prim.IsInstanceable())

        cmds.redo()
        self.assertTrue(self.ball35Prim.IsInstanceable())

        # Change variant in variant set.
        def shadingVariant():
            contextItems = self.contextOps.getItems(
                ['Variant Sets', 'shadingVariant'])

            for c in contextItems:
                if c.checked:
                    return c.item

        self.assertEqual(shadingVariant(), 'Ball_8')

        cmd = self.contextOps.doOpCmd(
            ['Variant Sets', 'shadingVariant', 'Cue'])
        self.assertIsNotNone(cmd)

        ufeCmd.execute(cmd)

        self.assertEqual(shadingVariant(), 'Cue')

        cmds.undo()

        self.assertEqual(shadingVariant(), 'Ball_8')

        cmds.redo()

        self.assertEqual(shadingVariant(), 'Cue')

        cmds.undo()

    def testAddNewPrim(self):
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create our UFE notification observer
        ufeObs = TestAddPrimObserver()
        ufe.Scene.addObserver(ufeObs)

        # Create a ContextOps interface for the proxy shape.
        proxyShapePath = ufe.Path([mayaUtils.createUfePathSegment(proxyShape)])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        # Add a new prim.
        cmd = contextOps.doOpCmd(['Add New Prim', 'Xform'])
        self.assertIsNotNone(cmd)
        ufeObs.reset()
        ufeCmd.execute(cmd)

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 1)
        self.assertEqual(ufeObs.nbDeleteNotif(), 0)

        # The proxy shape should now have a single UFE child item.
        proxyShapehier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertTrue(proxyShapehier.hasChildren())
        self.assertEqual(len(proxyShapehier.children()), 1)

        # Add a new prim to the prim we just added.
        cmds.pickWalk(d='down')

        # Get the scene item from the UFE selection.
        snIter = iter(ufe.GlobalSelection.get())
        xformItem = next(snIter)

        # Create a ContextOps interface for it.
        contextOps = ufe.ContextOps.contextOps(xformItem)

        # Add a new prim.
        cmd = contextOps.doOpCmd(['Add New Prim', 'Xform'])
        self.assertIsNotNone(cmd)
        ufeObs.reset()
        ufeCmd.execute(cmd)

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 1)
        self.assertEqual(ufeObs.nbDeleteNotif(), 0)

        # The xform prim should now have a single UFE child item.
        xformHier = ufe.Hierarchy.hierarchy(xformItem)
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 1)

        # Add another prim
        cmd = contextOps.doOpCmd(['Add New Prim', 'Capsule'])
        self.assertIsNotNone(cmd)
        ufeObs.reset()
        ufeCmd.execute(cmd)

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 1)
        self.assertEqual(ufeObs.nbDeleteNotif(), 0)

        # The xform prim should now have two UFE child items.
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 2)

        # Undo will remove the new prim, meaning one less child.
        ufeObs.reset()
        cmds.undo()
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 1)

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 0)
        self.assertEqual(ufeObs.nbDeleteNotif(), 1)

        # Undo again will remove the first added prim, meaning no children.
        cmds.undo()
        self.assertFalse(xformHier.hasChildren())

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 0)
        self.assertEqual(ufeObs.nbDeleteNotif(), 2)

        cmds.redo()
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 1)

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 1)
        self.assertEqual(ufeObs.nbDeleteNotif(), 2)

        cmds.redo()
        self.assertTrue(xformHier.hasChildren())
        self.assertEqual(len(xformHier.children()), 2)

        # Ensure we got the correct UFE notifs.
        self.assertEqual(ufeObs.nbAddNotif(), 2)
        self.assertEqual(ufeObs.nbDeleteNotif(), 2)

    @unittest.skipUnless(Usd.GetVersion() >= (0, 21, 8), 'Requires CanApplySchema from USD')
    def testMaterialBinding(self):
        """This test builds a material using only Ufe pre-4.10 capabilities."""
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create a ContextOps interface for the proxy shape.
        proxyPathSegment = mayaUtils.createUfePathSegment(proxyShape)
        proxyShapePath = ufe.Path([proxyPathSegment])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        cmd = contextOps.doOpCmd(['Add New Prim', 'Capsule'])
        ufeCmd.execute(cmd)
        cmd = contextOps.doOpCmd(['Add New Prim', 'Material'])
        ufeCmd.execute(cmd)

        rootHier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertTrue(rootHier.hasChildren())
        self.assertEqual(len(rootHier.children()), 2)

        materialItem = rootHier.children()[-1]
        contextOps = ufe.ContextOps.contextOps(materialItem)

        cmd = contextOps.doOpCmd(['Add New Prim', 'Shader'])
        ufeCmd.execute(cmd)

        materialHier = ufe.Hierarchy.hierarchy(materialItem)
        self.assertTrue(materialHier.hasChildren())
        self.assertEqual(len(materialHier.children()), 1)

        shaderItem = materialHier.children()[0]
        shaderAttrs = ufe.Attributes.attributes(shaderItem)

        self.assertTrue(shaderAttrs.hasAttribute("info:id"))
        shaderAttr = shaderAttrs.attribute("info:id")
        shaderAttr.set("ND_standard_surface_surfaceshader")
        
        # TODO: Jerry will change this code to a createNode call on the shader definition
        shaderAttrs = ufe.Attributes.attributes(shaderItem)

        # Set base_color to red
        # This should generate an AttrDefHandle version of an Attribute
        baseColorAttr = shaderAttrs.attribute("inputs:base_color")
        self.assertTrue(baseColorAttr != None)
        baseColor = baseColorAttr.get()
        self.assertAlmostEqual(baseColor.r(), 0.8)
        self.assertAlmostEqual(baseColor.g(), 0.8)
        self.assertAlmostEqual(baseColor.b(), 0.8)
        newBaseColorValue = ufe.Color3f(1.0, 0.0, 0.0)
        baseColorAttr.set(newBaseColorValue)
        baseColor2 = baseColorAttr.get()
        self.assertAlmostEqual(baseColor2.r(), 1.0)
        self.assertAlmostEqual(baseColor2.g(), 0.0)
        self.assertAlmostEqual(baseColor2.b(), 0.0)
        # This should use the previously stored AttrDefHandle
        baseColorAttr2 = shaderAttrs.attribute("inputs:base_color")
        self.assertTrue(baseColorAttr2 != None)
        baseColor3 = baseColorAttr2.get()
        self.assertAlmostEqual(baseColor3.r(), 1.0)
        self.assertAlmostEqual(baseColor3.g(), 0.0)
        self.assertAlmostEqual(baseColor3.b(), 0.0)
        newBaseColorValue2 = ufe.Color3f(0.0, 1.0, 0.0)
        baseColorAttr2.set(newBaseColorValue2)
        baseColor4 = baseColorAttr2.get()
        self.assertAlmostEqual(baseColor4.r(), 0.0)
        self.assertAlmostEqual(baseColor4.g(), 1.0)
        self.assertAlmostEqual(baseColor4.b(), 0.0)

        # Set float specular_IOR to 0.5
        iorFloatAttr = shaderAttrs.attribute("inputs:specular_IOR")
        self.assertTrue(iorFloatAttr != None)
        iorFloat = iorFloatAttr.get()
        self.assertAlmostEqual(iorFloat, 1.5)
        newIorFloat = 0.5
        iorFloatAttr.set(newIorFloat)
        iorFloat2 = iorFloatAttr.get()
        self.assertAlmostEqual(iorFloat2, newIorFloat)

        # Set vector3 coat_normal to (0.5, 0.5, 0.0)
        coatNormalVec3Attr = shaderAttrs.attribute("inputs:coat_normal")
        self.assertTrue(coatNormalVec3Attr != None)
        coatNormalVec3 = coatNormalVec3Attr.get()
        self.assertAlmostEqual(coatNormalVec3.x(), 0.0)
        self.assertAlmostEqual(coatNormalVec3.y(), 0.0)
        self.assertAlmostEqual(coatNormalVec3.z(), 0.0)
        newCoatNormalVec3 = ufe.Vector3f(0.0, 1.0, 1.0)
        coatNormalVec3Attr.set(newCoatNormalVec3)
        coatNormalVec3_2 = coatNormalVec3Attr.get()
        self.assertAlmostEqual(coatNormalVec3_2.x(), 0.0)
        self.assertAlmostEqual(coatNormalVec3_2.y(), 1.0)
        self.assertAlmostEqual(coatNormalVec3_2.z(), 1.0)

        # Set bool thin_walled to true
        thinWalledBoolAttr = shaderAttrs.attribute("inputs:thin_walled")
        self.assertTrue(thinWalledBoolAttr != None)
        thinWalledBool = thinWalledBoolAttr.get()
        self.assertEqual(thinWalledBool, False)
        newThinWalledBool = True
        thinWalledBoolAttr.set(newThinWalledBool)
        thinWalledBool2 = thinWalledBoolAttr.get()
        self.assertAlmostEqual(thinWalledBool2, newThinWalledBool)

        shaderAttrs = ufe.Attributes.attributes(shaderItem)
        self.assertTrue(shaderAttrs.hasAttribute("info:id"))
        shaderAttr = shaderAttrs.attribute("info:id")
        shaderAttr.set("ND_constant_integer")
        shaderAttrs = ufe.Attributes.attributes(shaderItem)
        valueIntAttr = shaderAttrs.attribute("inputs:value")
        self.assertTrue(valueIntAttr != None)
        valueInt = valueIntAttr.get()
        self.assertEqual(valueInt, 0)
        newValueInt = 2
        valueIntAttr.set(newValueInt)
        valueInt_2 = valueIntAttr.get()
        self.assertEqual(valueInt_2, newValueInt)

        import pdb; pdb.set_trace()

        shaderAttrs = ufe.Attributes.attributes(shaderItem)
        self.assertTrue(shaderAttrs.hasAttribute("info:id"))
        shaderAttr = shaderAttrs.attribute("info:id")
        shaderAttr.set("ND_constant_string")
        shaderAttrs = ufe.Attributes.attributes(shaderItem)
        valueStringAttr = shaderAttrs.attribute("inputs:value")
        self.assertTrue(valueStringAttr != None)
        valueString = valueStringAttr.get()
        self.assertEqual(valueString, "")
        newValueString = "test"
        valueStringAttr.set(newValueString)
        valueString_2 = valueStringAttr.get()
        self.assertEqual(valueString_2, newValueString)

        # TODO: Connect "/Material1.outputs:mtlx:surface" to "/Material1/Shader1.outputs:surface"

        # Now that we have a material, we can bind it on the capsule item even if incomplete
        capsuleItem = rootHier.children()[0]
        capsulePrim = usdUtils.getPrimFromSceneItem(capsuleItem)
        self.assertFalse(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))

        contextOps = ufe.ContextOps.contextOps(capsuleItem)
        cmd = contextOps.doOpCmd(['Bind Material', '/Material1'])
        self.assertTrue(cmd)
        ufeCmd.execute(cmd)
        self.assertTrue(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
        capsuleBindAPI = UsdShade.MaterialBindingAPI(capsulePrim)
        self.assertEqual(capsuleBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/Material1"))

        cmds.undo()
        self.assertFalse(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
        cmds.redo()
        self.assertTrue(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
        self.assertEqual(capsuleBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/Material1"))

        cmd = contextOps.doOpCmd(['Unbind Material'])
        self.assertTrue(cmd)
        ufeCmd.execute(cmd)

        self.assertTrue(capsuleBindAPI.GetDirectBinding().GetMaterialPath().isEmpty)
        cmds.undo()
        self.assertEqual(capsuleBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/Material1"))
        cmds.redo()
        self.assertTrue(capsuleBindAPI.GetDirectBinding().GetMaterialPath().isEmpty)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4010', 'Test only available in UFE preview version 0.4.10 and greater')
    @unittest.skipUnless(Usd.GetVersion() >= (0, 21, 8), 'Requires CanApplySchema from USD')
    def testMaterialBindingWithNodeDefHandler(self):
        """In this test we will go as far as possible towards creating and binding a working
           material using only Ufe and Maya commands (for full undo capabilities). It is locked
           at what Ufe 0.4.10 offers."""
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create a ContextOps interface for the proxy shape.
        proxyPathSegment = mayaUtils.createUfePathSegment(proxyShape)
        proxyShapePath = ufe.Path([proxyPathSegment])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        cmd = contextOps.doOpCmd(['Add New Prim', 'Capsule'])
        ufeCmd.execute(cmd)
        cmd = contextOps.doOpCmd(['Add New Prim', 'Material'])
        ufeCmd.execute(cmd)

        rootHier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertTrue(rootHier.hasChildren())
        self.assertEqual(len(rootHier.children()), 2)

        materialItem = rootHier.children()[-1]
        contextOps = ufe.ContextOps.contextOps(materialItem)

        shaderName = "ND_standard_surface_surfaceshader"
        surfDef = ufe.NodeDef.definition(materialItem.runTimeId(), shaderName)
        cmd = surfDef.createNodeCmd(materialItem, ufe.PathComponent("Red1"))
        ufeCmd.execute(cmd)
        shaderItem = cmd.insertedChild
        shaderAttrs = ufe.Attributes.attributes(shaderItem)

        self.assertTrue(shaderAttrs.hasAttribute("info:id"))
        self.assertEqual(shaderAttrs.attribute("info:id").get(), shaderName)
        self.assertEqual(ufe.PathString.string(shaderItem.path()), "|stage1|stageShape1,/Material1/Red11")
        materialHier = ufe.Hierarchy.hierarchy(materialItem)
        self.assertTrue(materialHier.hasChildren())

        cmds.undo()
        materialHier = ufe.Hierarchy.hierarchy(materialItem)
        self.assertFalse(materialHier.hasChildren())

        cmds.redo()
        materialHier = ufe.Hierarchy.hierarchy(materialItem)
        self.assertTrue(materialHier.hasChildren())
        shaderItem = cmd.insertedChild
        shaderAttrs = ufe.Attributes.attributes(shaderItem)

        # Now that we have a material, we can bind it on the capsule item even if incomplete
        capsuleItem = rootHier.children()[0]
        capsulePrim = usdUtils.getPrimFromSceneItem(capsuleItem)
        self.assertFalse(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))

        contextOps = ufe.ContextOps.contextOps(capsuleItem)
        cmd = contextOps.doOpCmd(['Bind Material', '/Material1'])
        self.assertTrue(cmd)
        ufeCmd.execute(cmd)
        self.assertTrue(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
        capsuleBindAPI = UsdShade.MaterialBindingAPI(capsulePrim)
        self.assertEqual(capsuleBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/Material1"))

        cmds.undo()
        self.assertFalse(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
        cmds.redo()
        self.assertTrue(capsulePrim.HasAPI(UsdShade.MaterialBindingAPI))
        self.assertEqual(capsuleBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/Material1"))

        cmd = contextOps.doOpCmd(['Unbind Material'])
        self.assertTrue(cmd)
        ufeCmd.execute(cmd)

        self.assertTrue(capsuleBindAPI.GetDirectBinding().GetMaterialPath().isEmpty)
        cmds.undo()
        self.assertEqual(capsuleBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/Material1"))
        cmds.redo()
        self.assertTrue(capsuleBindAPI.GetDirectBinding().GetMaterialPath().isEmpty)

    def testAddNewPrimWithDelete(self):
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create a ContextOps interface for the proxy shape.
        proxyShapePath = ufe.Path([mayaUtils.createUfePathSegment(proxyShape)])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        # Add a new Xform prim.
        cmd = contextOps.doOpCmd(['Add New Prim', 'Xform'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        # The proxy shape should now have a single UFE child item.
        proxyShapehier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertTrue(proxyShapehier.hasChildren())
        self.assertEqual(len(proxyShapehier.children()), 1)

        # Using UFE, delete this new prim (which doesn't actually delete it but
        # instead makes it inactive). In Maya 2023 and greater, delete really
        # deletes so deactivate instead to make the test act the same for all
        # versions.
        cmds.pickWalk(d='down')
        if mayaUtils.mayaMajorVersion() >= 2023:
            ufeItem = ufe.GlobalSelection.get().front()
            item = usdUtils.getPrimFromSceneItem(ufeItem)
            item.SetActive(False)
        else:
            cmds.delete()

        # The proxy shape should now have no UFE child items (since we skip inactive
        # when returning the children list) but hasChildren still reports true in
        # UFE version before 0.4.4 for inactive to allow the caller to do conditional
        # inactive filtering, so we test that hasChildren is true for those versions.
        if (os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') >= '4004'):
            self.assertFalse(proxyShapehier.hasChildren())
        else:
            self.assertTrue(proxyShapehier.hasChildren())
        self.assertEqual(len(proxyShapehier.children()), 0)

        # Add another Xform prim (which should get a unique name taking into
        # account the prim we just made inactive).
        cmd = contextOps.doOpCmd(['Add New Prim', 'Xform'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        # The proxy shape should now have a single UFE child item.
        self.assertTrue(proxyShapehier.hasChildren())
        self.assertEqual(len(proxyShapehier.children()), 1)

    def testLoadAndUnload(self):
        '''
        Tests the working set management contextOps "Load", "Load with
        Descendants", and "Unload".
        '''
        proxyShapePathSegment = mayaUtils.createUfePathSegment(
            '|transform1|proxyShape1')

        propsPath = ufe.Path([
            proxyShapePathSegment,
            usdUtils.createUfePathSegment('/Room_set/Props')])
        propsItem = ufe.Hierarchy.createItem(propsPath)
        ball1Path = ufe.Path([
            proxyShapePathSegment,
            usdUtils.createUfePathSegment('/Room_set/Props/Ball_1')])
        ball1Item = ufe.Hierarchy.createItem(ball1Path)
        ball15Path = ufe.Path([
            proxyShapePathSegment,
            usdUtils.createUfePathSegment('/Room_set/Props/Ball_15')])
        ball15Item = ufe.Hierarchy.createItem(ball15Path)

        def _validateLoadAndUnloadItems(hierItem, itemStrings):
            ALL_ITEM_STRINGS = set([
                'Load',
                'Load with Descendants',
                'Unload'
            ])

            expectedItemStrings = set(itemStrings or [])
            expectedAbsentItemStrings = ALL_ITEM_STRINGS - expectedItemStrings

            contextOps = ufe.ContextOps.contextOps(hierItem)
            contextItems = contextOps.getItems([])
            contextItemStrings = [c.item for c in contextItems]

            for itemString in expectedItemStrings:
                self.assertIn(itemString, contextItemStrings)

            for itemString in expectedAbsentItemStrings:
                self.assertNotIn(itemString, contextItemStrings)

        # The stage is fully loaded, so all items should have "Unload" items.
        _validateLoadAndUnloadItems(propsItem, ['Unload'])
        _validateLoadAndUnloadItems(ball1Item, ['Unload'])
        _validateLoadAndUnloadItems(ball15Item, ['Unload'])

        # The mesh prim path under each Ball asset prim has nothing loadable at
        # or below it, so it should not have any load or unload items.
        ball15meshPath = ufe.Path([
            proxyShapePathSegment,
            usdUtils.createUfePathSegment('/Room_set/Props/Ball_15/mesh')])
        ball15meshItem = ufe.Hierarchy.createItem(ball15meshPath)
        _validateLoadAndUnloadItems(ball15meshItem, [])

        # Unload Ball_1.
        contextOps = ufe.ContextOps.contextOps(ball1Item)
        cmd = contextOps.doOpCmd(['Unload'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        # Only Ball_1 should have been unloaded, and since it has a payload, it
        # should now have "Load" and "Load with Descendants" context items.
        # "Props" will now also have "Load with Descendants" because something
        # loadable below it is unloaded.
        _validateLoadAndUnloadItems(propsItem, ['Load with Descendants', 'Unload'])
        _validateLoadAndUnloadItems(ball1Item, ['Load', 'Load with Descendants'])
        _validateLoadAndUnloadItems(ball15Item, ['Unload'])

        cmds.undo()

        _validateLoadAndUnloadItems(propsItem, ['Unload'])
        _validateLoadAndUnloadItems(ball1Item, ['Unload'])
        _validateLoadAndUnloadItems(ball15Item, ['Unload'])

        # Unload Props.
        contextOps = ufe.ContextOps.contextOps(propsItem)
        cmd = contextOps.doOpCmd(['Unload'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        # The "Props" prim does not have a payload of its own, so it should
        # only have the "Load with Descendants" item. The Ball assets will also
        # have been unloaded.
        _validateLoadAndUnloadItems(propsItem, ['Load with Descendants'])
        _validateLoadAndUnloadItems(ball1Item, ['Load', 'Load with Descendants'])
        _validateLoadAndUnloadItems(ball15Item, ['Load', 'Load with Descendants'])

        cmds.undo()

        _validateLoadAndUnloadItems(propsItem, ['Unload'])
        _validateLoadAndUnloadItems(ball1Item, ['Unload'])
        _validateLoadAndUnloadItems(ball15Item, ['Unload'])

        cmds.redo()

        _validateLoadAndUnloadItems(propsItem, ['Load with Descendants'])
        _validateLoadAndUnloadItems(ball1Item, ['Load', 'Load with Descendants'])
        _validateLoadAndUnloadItems(ball15Item, ['Load', 'Load with Descendants'])

        # Load Props.
        contextOps = ufe.ContextOps.contextOps(propsItem)
        cmd = contextOps.doOpCmd(['Load with Descendants'])
        self.assertIsNotNone(cmd)
        ufeCmd.execute(cmd)

        _validateLoadAndUnloadItems(propsItem, ['Unload'])
        _validateLoadAndUnloadItems(ball1Item, ['Unload'])
        _validateLoadAndUnloadItems(ball15Item, ['Unload'])

        cmds.undo()

        _validateLoadAndUnloadItems(propsItem, ['Load with Descendants'])
        _validateLoadAndUnloadItems(ball1Item, ['Load', 'Load with Descendants'])
        _validateLoadAndUnloadItems(ball15Item, ['Load', 'Load with Descendants'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
