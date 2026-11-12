#!/usr/bin/env python

#
# Copyright 2025 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License")
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import fixturesUtils
import mayaUtils
import testUtils
import ufeUtils
import usdUtils

import mayaUsd

from maya import standalone
from maya import cmds

import ufe

from pxr import Sdf, Usd, UsdShade

if usdUtils.hasMaterialXVersionSupport():
    from pxr import UsdMtlx

import os
import platform
import subprocess
import sys
import unittest

import MaterialX_1_38_to_1_39

_skipUnlessMtlxVersionAuthored = unittest.skipUnless(
    usdUtils.hasMaterialXVersionSupport() and ufeUtils.ufeFeatureSetVersion() >= 4,
    'MaterialX version is only authored with USD 25.02 or later, UsdMtlx and UFE v4')

def diff(filePath, expectedResultPath):
    if platform.system() == 'Windows':
        diff = 'fc.exe'
    else:
        diff = '/usr/bin/diff'
    cmd = [diff, filePath, expectedResultPath]

    # This will print any diffs to stdout which is a nice side-effect
    return subprocess.call(cmd) == 0

class MaterialXUpgradeTestCase(unittest.TestCase):
    '''Verify the MaterialX upgrade produces expected results. The source test
    file will exercise every branch of the upgrade code.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        cls._testDir = fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testPythonUpgrade(self):
        '''Test MaterialX upgrade using Python script. This script can be used
          at command line level to quickly process files.'''

        cmds.file(new=True, force=True)

        # Fetch the test file.
        testFile = testUtils.getTestScene("lookdevXUsd", "mx_updates_to_1_39.usda")
        resultsFile = testUtils.getTestScene("lookdevXUsd", "mx_updates_to_1_39_final.usda")
        outputFile = os.path.join(os.getcwd(),
                                  "mx_updates_to_1_39_upgraded_python.usda")

        # Perform the upgrade.
        stage = Usd.Stage.Open(testFile)
        for prim in [x for x in stage.Traverse() if x.IsA(UsdShade.Material)]:
            MaterialX_1_38_to_1_39.ConvertMaterialTo139(prim)        
        stage.GetRootLayer().Export(outputFile)

        # Compare with expected results:
        self.assertTrue(diff(outputFile, resultsFile))

    def testMayaUpgrade(self):
        '''Test MaterialX upgrade using MayaUSD code. This simulates a user
        opening a stage in MayaUSD and having the upgrade applied using a
        contextual operation. Please note that Python bindings for the
        MaterialHandler itself are not provided by LookdevXUfe.'''

        cmds.file(new=True, force=True)

        # Fetch the test file.
        testFile = testUtils.getTestScene("lookdevXUsd", "mx_updates_to_1_39.usda")
        resultsFile = testUtils.getTestScene("lookdevXUsd", "mx_updates_to_1_39_final.usda")
        outputFile = os.path.join(os.getcwd(),
                                  "mx_updates_to_1_39_upgraded_maya.usda")

        # Open the test file in Maya.
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)
        self.assertIsNotNone(shapeNode)
        self.assertIsNotNone(stage)

        layerMaterial = ufeUtils.createUfeSceneItem(shapeNode,
            "/mtl/layer_update_material")
        self.assertIsNotNone(layerMaterial)
        
        contextOps = ufe.ContextOps.contextOps(layerMaterial)
        self.assertIsNotNone(contextOps)
        items = contextOps.getItems([])
        self.assertIn("Upgrade Material", [item.item for item in items])

        cmd = contextOps.doOpCmd(["Upgrade Material",])
        self.assertIsNotNone(cmd)
        cmd.execute()

        # Should not require anymore upgrade:
        items = contextOps.getItems([])
        self.assertNotIn("Upgrade Material", [item.item for item in items])

        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        for matName in ["subsurface_update_material", "switch_update_material", "swizzle_update_material", "atan_update_material", "normalmap_update_material"]:
            matItem = ufeUtils.createUfeSceneItem(shapeNode, f"/mtl/{matName}")
            globalSelection.append(matItem)
        contextOps = ufe.ContextOps.contextOps(matItem)
        items = contextOps.getItems([])
        self.assertIn("Upgrade Material", [item.item for item in items])

        cmd = contextOps.doOpCmd(["Upgrade Material",])
        self.assertIsNotNone(cmd)
        cmd.execute()

        # Should not require anymore upgrade:
        items = contextOps.getItems([])
        self.assertNotIn("Upgrade Material", [item.item for item in items])

        stage.GetRootLayer().Export(outputFile)

        # Compare with expected results:
        self.assertTrue(diff(outputFile, resultsFile))

    def testMayaFullStageUpgrade(self):
        '''Test MaterialX upgrade using MayaUSD code. This simulates a user
        opening a stage in MayaUSD and having the upgrade applied using a
        contextual operation. Please note that Python bindings for the
        MaterialHandler itself are not provided by LookdevXUfe.'''

        cmds.file(new=True, force=True)

        # Fetch the test file.
        testFile = testUtils.getTestScene("lookdevXUsd", "mx_updates_to_1_39.usda")
        resultsFile = testUtils.getTestScene("lookdevXUsd", "mx_updates_to_1_39_final.usda")
        outputFile = os.path.join(os.getcwd(),
                                  "mx_updates_to_1_39_upgraded_maya_via_stage.usda")

        # Open the test file in Maya.
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)
        self.assertIsNotNone(shapeNode)
        self.assertIsNotNone(stage)

        # Get scene item for stage:
        stageUfePath = ufe.PathString.path(shapeNode)
        stageUfeSceneItem = ufe.Hierarchy.createItem(stageUfePath)
        self.assertIsNotNone(stageUfeSceneItem)

        contextOps = ufe.ContextOps.contextOps(stageUfeSceneItem)
        self.assertIsNotNone(contextOps)
        items = contextOps.getItems([])
        self.assertIn('UpgradeStageLegacyMaterials', [item.item for item in items])

        cmd = contextOps.doOpCmd(["UpgradeStageLegacyMaterials",])
        self.assertIsNotNone(cmd)
        cmd.execute()

        stage.GetRootLayer().Export(outputFile)

        # Compare with expected results:
        self.assertTrue(diff(outputFile, resultsFile))

    def _addShaderWithoutId(self, materialItem):
        '''Add a shader without info:id in the material and return its scene item.'''
        materialHier = ufe.Hierarchy.hierarchy(materialItem)
        childPathsBefore = {ufe.PathString.string(child.path()) for child in materialHier.children()}

        ufe.ContextOps.contextOps(materialItem).doOp(['Add New Prim', 'Shader'])

        newChildren = [child for child in materialHier.children()
                       if ufe.PathString.string(child.path()) not in childPathsBefore]
        self.assertEqual(len(newChildren), 1)
        return newChildren[0]

    def _addShader(self, materialItem, shaderId):
        '''Add a shader with the given info:id in the material and return its scene item.'''
        shaderItem = self._addShaderWithoutId(materialItem)
        ufe.Attributes.attributes(shaderItem).attribute("info:id").set(shaderId)
        return shaderItem

    def _createNodeFromDef(self, parentItem, shaderId, name):
        '''Create a shader from its node definition with the undoable UFE command. Return the
        executed command.'''
        nodeDef = ufe.NodeDef.definition(parentItem.runTimeId(), shaderId)
        self.assertIsNotNone(nodeDef)
        cmd = nodeDef.createNodeCmd(parentItem, ufe.PathComponent(name))
        self.assertIsNotNone(cmd)
        cmd.execute()
        self.assertIsNotNone(cmd.insertedChild)
        return cmd

    def _defineShaderWithUsd(self, parentPrim, name, shaderId):
        '''Define a shader with the USD API, bypassing UFE, and return its prim.'''
        shader = UsdShade.Shader.Define(parentPrim.GetStage(), parentPrim.GetPath().AppendChild(name))
        shader.CreateIdAttr(shaderId)
        return shader.GetPrim()

    def _authoredMaterialXVersion(self, materialPrim):
        '''Return the MaterialX version authored on the material, or None.'''
        if not materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI):
            return None
        return materialPrim.GetAttribute("config:mtlx:version").Get()

    def _addStandardSurfaceAndConnect(self, materialItem):
        '''Add a MaterialX standard surface in the material and connect it to outputs:surface
        with the undoable UFE connection command. Return the executed command.'''
        shaderItem = self._addShader(materialItem, "ND_standard_surface_surfaceshader")
        return self._connectSurface(shaderItem, materialItem)

    def _connectSurface(self, shaderItem, materialItem):
        '''Connect the shader to the material outputs:surface with the undoable UFE connection
        command. Return the executed command.'''
        # Refetch the attributes so the shader output reflects the new info:id.
        shaderOutput = ufe.Attributes.attributes(shaderItem).attribute("outputs:out")
        materialOutput = ufe.Attributes.attributes(materialItem).attribute("outputs:surface")
        handler = ufe.RunTimeMgr.instance().connectionHandler(materialItem.runTimeId())
        cmd = handler.createConnectionCmd(shaderOutput, materialOutput)
        cmd.execute()
        self.assertEqual(cmd.connection.dst.name, "outputs:mtlx:surface")
        return cmd

    def _upgradeOffered(self, materialItem):
        items = ufe.ContextOps.contextOps(materialItem).getItems([])
        return "Upgrade Material" in [item.item for item in items]

    def _stageUpgradeOffered(self, proxyShapeItem):
        items = ufe.ContextOps.contextOps(proxyShapeItem).getItems([])
        return "UpgradeStageLegacyMaterials" in [item.item for item in items]

    def _createProxyShapeItem(self):
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapeItem = ufe.Hierarchy.createItem(ufe.PathString.path(proxyShape))
        return proxyShape, proxyShapeItem

    def _createEmptyMaterial(self, proxyShapeItem=None):
        '''Add an empty material at the root of the stage, in a new stage if no proxy shape
        item is given.'''
        if proxyShapeItem is None:
            _, proxyShapeItem = self._createProxyShapeItem()
        ufe.ContextOps.contextOps(proxyShapeItem).doOp(['Add New Prim', 'Material'])
        materialItem = ufe.Hierarchy.hierarchy(proxyShapeItem).children()[-1]
        return materialItem, usdUtils.getPrimFromSceneItem(materialItem)

    def _createMaterialsScope(self, proxyShape):
        '''Define the default materials scope and return its scene item.'''
        mayaUsd.ufe.getStage(proxyShape).DefinePrim('/mtl', 'Scope')
        scopeItem = ufeUtils.createUfeSceneItem(proxyShape, '/mtl')
        self.assertIsNotNone(scopeItem)
        return scopeItem

    def _createStampedMaterial(self, proxyShapeItem):
        '''Add a material stamped with the current MaterialX version through a new MaterialX
        shader. Return the material item and prim, the shader item and the version.'''
        materialItem, materialPrim = self._createEmptyMaterial(proxyShapeItem)
        shaderItem = self._addShader(materialItem, "ND_standard_surface_surfaceshader")
        version = self._authoredMaterialXVersion(materialPrim)
        self.assertIsNotNone(version)
        return materialItem, materialPrim, shaderItem, version

    def _copy(self, items):
        '''Copy the items to the UFE clipboard and return the clipboard handler.'''
        handler = ufe.ClipboardHandler.clipboardHandler(items[0].runTimeId())
        ufe.ClipboardHandler.preCopy()
        handler.copyCmd_(ufe.Selection(items)).execute()
        self.assertTrue(handler.hasItemsToPaste_())
        return handler

    def _paste(self, handler, targetItem):
        '''Paste the clipboard into the target item and return the executed command.'''
        ufe.GlobalSelection.get().clear()
        cmd = handler.pasteCmd_(targetItem)
        self.assertIsNotNone(cmd)
        cmd.execute()
        return cmd

    def _loadLegacyMaterial(self):
        '''Open the legacy test file. Return the proxy shape, and the item and prim of the
        unstamped /mtl/layer_update_material, whose MaterialX surface is connected.'''
        testFile = testUtils.getTestScene("lookdevXUsd", "mx_updates_to_1_39.usda")
        shapeNode, _ = mayaUtils.createProxyFromFile(testFile)
        self.assertIsNotNone(shapeNode)

        materialItem = ufeUtils.createUfeSceneItem(shapeNode, "/mtl/layer_update_material")
        self.assertIsNotNone(materialItem)
        return shapeNode, materialItem, usdUtils.getPrimFromSceneItem(materialItem)

    def _loadDisconnectedLegacyMaterial(self):
        '''Open the legacy test file and disconnect the MaterialX surface of
        /mtl/layer_update_material, leaving its legacy nodes unconnected. Return the proxy
        shape, the material item and the material prim.'''
        shapeNode, materialItem, materialPrim = self._loadLegacyMaterial()
        legacySurfaceItem = ufeUtils.createUfeSceneItem(
            shapeNode, "/mtl/layer_update_material/surface1")
        self.assertIsNotNone(legacySurfaceItem)

        handler = ufe.RunTimeMgr.instance().connectionHandler(materialItem.runTimeId())
        handler.deleteConnectionCmd(
            ufe.Attributes.attributes(legacySurfaceItem).attribute("outputs:out"),
            ufe.Attributes.attributes(materialItem).attribute("outputs:mtlx:surface")).execute()
        # The disconnection also removes the now unused MaterialX surface output.
        surfaceOutput = UsdShade.Material(materialPrim).GetSurfaceOutput("mtlx")
        self.assertFalse(surfaceOutput and surfaceOutput.HasConnectedSource())
        return shapeNode, materialItem, materialPrim

    @_skipUnlessMtlxVersionAuthored
    def testNewEmptyMaterialIsNotLegacy(self):
        '''Test that adding and connecting a MaterialX shader in a new empty material does not
        make it legacy.'''
        cmds.file(new=True, force=True)

        materialItem, materialPrim = self._createEmptyMaterial()

        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))

        self._addStandardSurfaceAndConnect(materialItem)

        self.assertTrue(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertFalse(self._upgradeOffered(materialItem))

    @_skipUnlessMtlxVersionAuthored
    def testAuthoredVersionIsKeptOnConnection(self):
        '''Test that a version already authored on a material is not overwritten by new
        MaterialX nodes or connections, so an older version stays detectable as legacy.'''
        cmds.file(new=True, force=True)

        materialItem, materialPrim = self._createEmptyMaterial()

        UsdMtlx.MaterialXConfigAPI.Apply(materialPrim).CreateConfigMtlxVersionAttr("1.38")

        self._createNodeFromDef(materialItem, "ND_image_color3", "image1")
        self._addStandardSurfaceAndConnect(materialItem)

        self.assertEqual(self._authoredMaterialXVersion(materialPrim), "1.38")
        self.assertTrue(self._upgradeOffered(materialItem))

    @_skipUnlessMtlxVersionAuthored
    def testPreviewSurfaceMaterialIsNotLegacy(self):
        '''Test that connecting a MaterialX shader to a UsdPreviewSurface material does not
        make it legacy and keeps the universal surface connection.'''
        cmds.file(new=True, force=True)

        proxyShape, proxyShapeItem = self._createProxyShapeItem()
        ufe.ContextOps.contextOps(proxyShapeItem).doOp(['Add New Prim', 'Capsule'])
        capsuleItem = ufe.Hierarchy.hierarchy(proxyShapeItem).children()[-1]
        capsulePrim = usdUtils.getPrimFromSceneItem(capsuleItem)

        cmdPS = ufe.ContextOps.contextOps(capsuleItem).doOpCmd(
            ['Assign New Material', 'USD', 'UsdPreviewSurface'])
        self.assertIsNotNone(cmdPS)
        cmdPS.execute()

        material = UsdShade.MaterialBindingAPI(capsulePrim).ComputeBoundMaterial()[0]
        self.assertTrue(material)
        materialPrim = material.GetPrim()
        materialItem = ufeUtils.createUfeSceneItem(proxyShape, str(materialPrim.GetPath()))
        self.assertIsNotNone(materialItem)

        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))

        self._addStandardSurfaceAndConnect(materialItem)

        self.assertTrue(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertFalse(self._upgradeOffered(materialItem))
        self.assertTrue(material.GetSurfaceOutput().HasConnectedSource())

    @_skipUnlessMtlxVersionAuthored
    def testLegacyMaterialStaysLegacyOnConnection(self):
        '''Test that a legacy material, whose MaterialX surface is already connected, is not
        stamped with the current version by a new connection.'''
        cmds.file(new=True, force=True)

        _, materialItem, materialPrim = self._loadLegacyMaterial()

        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertTrue(self._upgradeOffered(materialItem))

        self._addStandardSurfaceAndConnect(materialItem)

        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertTrue(self._upgradeOffered(materialItem))

    @_skipUnlessMtlxVersionAuthored
    def testDisconnectedLegacyMaterialStaysLegacy(self):
        '''Test that a legacy material whose MaterialX surface was disconnected is not stamped
        with the current version when a new surface is connected, since its legacy nodes remain.'''
        cmds.file(new=True, force=True)

        _, materialItem, materialPrim = self._loadDisconnectedLegacyMaterial()

        self._addStandardSurfaceAndConnect(materialItem)

        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertTrue(self._upgradeOffered(materialItem))

    @_skipUnlessMtlxVersionAuthored
    def testUsdAuthoredShadersAreNotStampedOnConnection(self):
        '''Test that connecting MaterialX shaders authored with the USD API, bypassing UFE, does
        not stamp the material, since their version is unknown.'''
        cmds.file(new=True, force=True)

        materialItem, materialPrim = self._createEmptyMaterial()

        self._defineShaderWithUsd(materialPrim, "image1", "ND_image_color3")
        self._defineShaderWithUsd(materialPrim, "surface1", "ND_standard_surface_surfaceshader")
        imageItem = ufe.Hierarchy.createItem(materialItem.path() + "image1")
        surfaceItem = ufe.Hierarchy.createItem(materialItem.path() + "surface1")

        handler = ufe.RunTimeMgr.instance().connectionHandler(materialItem.runTimeId())
        handler.createConnectionCmd(
            ufe.Attributes.attributes(imageItem).attribute("outputs:out"),
            ufe.Attributes.attributes(surfaceItem).attribute("inputs:base_color")).execute()
        self._connectSurface(surfaceItem, materialItem)

        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertTrue(self._upgradeOffered(materialItem))

    @_skipUnlessMtlxVersionAuthored
    def testReconnectedLegacyMaterialStaysLegacy(self):
        '''Test that reconnecting the original surface of a disconnected legacy material does
        not stamp it, so the upgrade is still offered.'''
        cmds.file(new=True, force=True)

        shapeNode, materialItem, materialPrim = self._loadDisconnectedLegacyMaterial()
        surfaceItem = ufeUtils.createUfeSceneItem(shapeNode, "/mtl/layer_update_material/surface1")

        self._connectSurface(surfaceItem, materialItem)

        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertTrue(self._upgradeOffered(materialItem))

    @_skipUnlessMtlxVersionAuthored
    def testInfoIdChangeOnLegacyNodeStaysLegacy(self):
        '''Test that changing the info:id of the only MaterialX node of an unstamped material
        does not stamp it, since that node was already in the material.'''
        cmds.file(new=True, force=True)

        materialItem, materialPrim = self._createEmptyMaterial()
        self._defineShaderWithUsd(materialPrim, "image1", "ND_image_color3")
        imageItem = ufe.Hierarchy.createItem(materialItem.path() + "image1")

        ufe.Attributes.attributes(imageItem).attribute("info:id").set("ND_image_color4")

        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertTrue(self._upgradeOffered(materialItem))

    @_skipUnlessMtlxVersionAuthored
    def testUnregisteredNodeDefIsLegacy(self):
        '''Test that a node definition missing from the current MaterialX library counts as
        legacy MaterialX content, so a new MaterialX node does not stamp its material.'''
        cmds.file(new=True, force=True)

        materialItem, materialPrim = self._createEmptyMaterial()
        self._defineShaderWithUsd(materialPrim, "removed1", "ND_removed_node_color3")

        self.assertTrue(self._upgradeOffered(materialItem))

        self._createNodeFromDef(materialItem, "ND_image_color3", "image1")

        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertTrue(self._upgradeOffered(materialItem))

    @_skipUnlessMtlxVersionAuthored
    def testUnconnectedLegacyNodesAreDetected(self):
        '''Test that a legacy material whose MaterialX nodes are all unconnected is legacy.'''
        cmds.file(new=True, force=True)

        shapeNode, materialItem, _ = self._loadDisconnectedLegacyMaterial()

        self.assertTrue(self._upgradeOffered(materialItem))
        proxyShapeItem = ufe.Hierarchy.createItem(ufe.PathString.path(shapeNode))
        self.assertTrue(self._stageUpgradeOffered(proxyShapeItem))

    @_skipUnlessMtlxVersionAuthored
    def testLegacyNodeInCompoundIsDetected(self):
        '''Test that an unstamped material whose only MaterialX node sits unconnected inside a
        child NodeGraph is legacy.'''
        cmds.file(new=True, force=True)

        _, proxyShapeItem = self._createProxyShapeItem()
        materialItem, materialPrim = self._createEmptyMaterial(proxyShapeItem)
        nodeGraph = UsdShade.NodeGraph.Define(
            materialPrim.GetStage(), materialPrim.GetPath().AppendChild("compound1"))

        self.assertFalse(self._upgradeOffered(materialItem))
        self.assertFalse(self._stageUpgradeOffered(proxyShapeItem))

        self._defineShaderWithUsd(nodeGraph.GetPrim(), "image1", "ND_image_color3")

        self.assertTrue(self._upgradeOffered(materialItem))
        self.assertTrue(self._stageUpgradeOffered(proxyShapeItem))

    @_skipUnlessMtlxVersionAuthored
    def testNonMaterialXMaterialIsNotLegacy(self):
        '''Test that a material with only an unconnected UsdPreviewSurface shader is neither
        legacy nor stamped.'''
        cmds.file(new=True, force=True)

        materialItem, materialPrim = self._createEmptyMaterial()

        self._defineShaderWithUsd(materialPrim, "UsdPreviewSurface1", "UsdPreviewSurface")

        self.assertFalse(self._upgradeOffered(materialItem))
        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))

    @_skipUnlessMtlxVersionAuthored
    def testLegacyNodeOnArnoldTerminalIsDetected(self):
        '''Test that an unstamped material whose MaterialX surface feeds outputs:arnold:surface
        is legacy.'''
        cmds.file(new=True, force=True)

        materialItem, materialPrim = self._createEmptyMaterial()
        surfacePrim = self._defineShaderWithUsd(
            materialPrim, "surface1", "ND_standard_surface_surfaceshader")
        UsdShade.Material(materialPrim).CreateSurfaceOutput("arnold").ConnectToSource(
            UsdShade.Shader(surfacePrim).CreateOutput("out", Sdf.ValueTypeNames.Token))

        self.assertTrue(self._upgradeOffered(materialItem))

    @_skipUnlessMtlxVersionAuthored
    def testNewNodeOnArnoldTerminalIsNotLegacy(self):
        '''Test that a new MaterialX node connected to outputs:arnold:surface stamps its
        material, so it is not legacy.'''
        cmds.file(new=True, force=True)

        materialItem, materialPrim = self._createEmptyMaterial()
        surfaceItem = self._createNodeFromDef(
            materialItem, "ND_standard_surface_surfaceshader", "surface1").insertedChild
        surfacePrim = usdUtils.getPrimFromSceneItem(surfaceItem)
        UsdShade.Material(materialPrim).CreateSurfaceOutput("arnold").ConnectToSource(
            UsdShade.Shader(surfacePrim).CreateOutput("out", Sdf.ValueTypeNames.Token))

        self.assertIsNotNone(self._authoredMaterialXVersion(materialPrim))
        self.assertFalse(self._upgradeOffered(materialItem))

    @_skipUnlessMtlxVersionAuthored
    def testInfoIdStampsNewMaterial(self):
        '''Test that setting a MaterialX info:id on a new shader in an empty material stamps
        the material, and that the version follows undo and redo.'''
        cmds.file(new=True, force=True)

        materialItem, materialPrim = self._createEmptyMaterial()
        shaderItem = self._addShaderWithoutId(materialItem)
        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))

        cmd = ufe.Attributes.attributes(shaderItem).attribute("info:id").setCmd(
            "ND_standard_surface_surfaceshader")
        self.assertIsNotNone(cmd)
        cmd.execute()

        self.assertTrue(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertFalse(self._upgradeOffered(materialItem))

        cmd.undo()
        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))

        cmd.redo()
        self.assertTrue(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertFalse(self._upgradeOffered(materialItem))

    @_skipUnlessMtlxVersionAuthored
    def testNodeDefCreationStampsNewMaterial(self):
        '''Test that creating a MaterialX node from its node definition in an empty material
        stamps the material, and that the version follows undo and redo.'''
        cmds.file(new=True, force=True)

        materialItem, materialPrim = self._createEmptyMaterial()

        cmd = self._createNodeFromDef(materialItem, "ND_standard_surface_surfaceshader", "surface1")

        self.assertTrue(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertFalse(self._upgradeOffered(materialItem))

        cmd.undo()
        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))

        cmd.redo()
        self.assertTrue(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertFalse(self._upgradeOffered(materialItem))

    @_skipUnlessMtlxVersionAuthored
    def testNodeInCompoundStampsEnclosingMaterial(self):
        '''Test that creating a MaterialX node inside a NodeGraph of an empty material stamps
        the material and not the NodeGraph.'''
        cmds.file(new=True, force=True)

        materialItem, materialPrim = self._createEmptyMaterial()
        nodeGraph = UsdShade.NodeGraph.Define(
            materialPrim.GetStage(), materialPrim.GetPath().AppendChild("compound1"))
        nodeGraphItem = ufe.Hierarchy.createItem(materialItem.path() + "compound1")

        self._createNodeFromDef(nodeGraphItem, "ND_image_color3", "image1")

        self.assertTrue(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertFalse(nodeGraph.GetPrim().HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertFalse(self._upgradeOffered(materialItem))

    @_skipUnlessMtlxVersionAuthored
    def testNonMaterialXNodeDoesNotStamp(self):
        '''Test that creating, or setting the info:id of, a UsdPreviewSurface shader leaves the
        material unstamped.'''
        cmds.file(new=True, force=True)

        materialItem, materialPrim = self._createEmptyMaterial()

        self._createNodeFromDef(materialItem, "UsdPreviewSurface", "UsdPreviewSurface1")
        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))

        self._addShader(materialItem, "UsdPreviewSurface")
        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))

    @_skipUnlessMtlxVersionAuthored
    def testShaderOutsideMaterialDoesNotStamp(self):
        '''Test that a MaterialX shader outside of any material authors no version.'''
        cmds.file(new=True, force=True)

        proxyShape, _ = self._createProxyShapeItem()
        stage = mayaUsd.ufe.getStage(proxyShape)
        stage.DefinePrim('/Scope1', 'Scope')
        scopeItem = ufeUtils.createUfeSceneItem(proxyShape, '/Scope1')

        self._createNodeFromDef(scopeItem, "ND_standard_surface_surfaceshader", "surface1")
        shaderPrim = UsdShade.Shader.Define(stage, '/Scope1/image1').GetPrim()
        shaderItem = ufeUtils.createUfeSceneItem(proxyShape, '/Scope1/image1')
        ufe.Attributes.attributes(shaderItem).attribute("info:id").set("ND_image_color3")

        self.assertEqual(shaderPrim.GetAttribute("info:id").Get(), "ND_image_color3")
        self.assertFalse(any(prim.HasAPI(UsdMtlx.MaterialXConfigAPI) for prim in stage.Traverse()))

    @_skipUnlessMtlxVersionAuthored
    def testNewNodesInLegacyMaterialStayLegacy(self):
        '''Test that MaterialX nodes created and connected in a disconnected legacy material do
        not stamp it, so the upgrade is still offered.'''
        cmds.file(new=True, force=True)

        _, materialItem, materialPrim = self._loadDisconnectedLegacyMaterial()

        imageItem = self._createNodeFromDef(materialItem, "ND_image_color3", "image1").insertedChild
        surfaceItem = self._addShader(materialItem, "ND_standard_surface_surfaceshader")

        handler = ufe.RunTimeMgr.instance().connectionHandler(materialItem.runTimeId())
        handler.createConnectionCmd(
            ufe.Attributes.attributes(imageItem).attribute("outputs:out"),
            ufe.Attributes.attributes(surfaceItem).attribute("inputs:base_color")).execute()
        self._connectSurface(surfaceItem, materialItem)

        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertTrue(self._upgradeOffered(materialItem))

    @_skipUnlessMtlxVersionAuthored
    def testPasteFromStampedMaterialIntoScope(self):
        '''Test that shaders pasted into a materials scope create a material with the version
        of their source material.'''
        cmds.file(new=True, force=True)

        proxyShape, proxyShapeItem = self._createProxyShapeItem()
        _, _, shaderItem, version = self._createStampedMaterial(proxyShapeItem)
        scopeItem = self._createMaterialsScope(proxyShape)

        pasteCmd = self._paste(self._copy([shaderItem]), scopeItem)

        self.assertEqual(len(pasteCmd.targetItems()), 1)
        pastedMaterialItem = pasteCmd.targetItems()[0]
        pastedMaterialPrim = usdUtils.getPrimFromSceneItem(pastedMaterialItem)
        self.assertEqual(self._authoredMaterialXVersion(pastedMaterialPrim), version)
        self.assertFalse(self._upgradeOffered(pastedMaterialItem))

    @_skipUnlessMtlxVersionAuthored
    def testPasteFromLegacyMaterialIntoScope(self):
        '''Test that shaders from a legacy material pasted into a materials scope create an
        unstamped material that offers the upgrade.'''
        cmds.file(new=True, force=True)

        shapeNode, _, _ = self._loadLegacyMaterial()
        surfaceItem = ufeUtils.createUfeSceneItem(shapeNode, "/mtl/layer_update_material/surface1")
        self.assertIsNotNone(surfaceItem)
        scopeItem = ufeUtils.createUfeSceneItem(shapeNode, "/mtl")

        pasteCmd = self._paste(self._copy([surfaceItem]), scopeItem)

        self.assertEqual(len(pasteCmd.targetItems()), 1)
        pastedMaterialItem = pasteCmd.targetItems()[0]
        pastedMaterialPrim = usdUtils.getPrimFromSceneItem(pastedMaterialItem)
        self.assertFalse(pastedMaterialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertTrue(self._upgradeOffered(pastedMaterialItem))

    @_skipUnlessMtlxVersionAuthored
    def testPasteIntoEmptyMaterial(self):
        '''Test that shaders pasted into an empty material give it the version of their source
        material, and that the version follows undo and redo of the paste.'''
        cmds.file(new=True, force=True)

        _, proxyShapeItem = self._createProxyShapeItem()
        _, _, shaderItem, version = self._createStampedMaterial(proxyShapeItem)
        targetItem, targetPrim = self._createEmptyMaterial(proxyShapeItem)

        pasteCmd = self._paste(self._copy([shaderItem]), targetItem)

        self.assertEqual(self._authoredMaterialXVersion(targetPrim), version)
        self.assertFalse(self._upgradeOffered(targetItem))

        pasteCmd.undo()
        self.assertFalse(targetPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))

        pasteCmd.redo()
        self.assertEqual(self._authoredMaterialXVersion(targetPrim), version)

    @_skipUnlessMtlxVersionAuthored
    def testPasteIntoMaterialWithMaterialXContent(self):
        '''Test that shaders pasted into a material that already has MaterialX content leave
        its version, or its lack of one, unchanged.'''
        cmds.file(new=True, force=True)

        _, proxyShapeItem = self._createProxyShapeItem()
        _, _, shaderItem, version = self._createStampedMaterial(proxyShapeItem)
        self.assertNotEqual(version, "1.38")

        olderItem, olderPrim = self._createEmptyMaterial(proxyShapeItem)
        UsdMtlx.MaterialXConfigAPI.Apply(olderPrim).CreateConfigMtlxVersionAttr("1.38")
        self._defineShaderWithUsd(olderPrim, "surface1", "ND_standard_surface_surfaceshader")

        unstampedItem, unstampedPrim = self._createEmptyMaterial(proxyShapeItem)
        self._defineShaderWithUsd(unstampedPrim, "surface1", "ND_standard_surface_surfaceshader")

        handler = self._copy([shaderItem])
        self._paste(handler, olderItem)
        self._paste(handler, unstampedItem)

        self.assertEqual(self._authoredMaterialXVersion(olderPrim), "1.38")
        self.assertFalse(unstampedPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))

    @_skipUnlessMtlxVersionAuthored
    def testPasteMixedVersionsIntoEmptyMaterial(self):
        '''Test that shaders from materials with different versions pasted together into an
        empty material leave it unstamped.'''
        cmds.file(new=True, force=True)

        _, proxyShapeItem = self._createProxyShapeItem()
        olderItem, olderPrim = self._createEmptyMaterial(proxyShapeItem)
        UsdMtlx.MaterialXConfigAPI.Apply(olderPrim).CreateConfigMtlxVersionAttr("1.38")
        olderShaderItem = self._addShader(olderItem, "ND_standard_surface_surfaceshader")
        _, _, currentShaderItem, version = self._createStampedMaterial(proxyShapeItem)
        self.assertNotEqual(version, "1.38")
        targetItem, targetPrim = self._createEmptyMaterial(proxyShapeItem)

        self._paste(self._copy([olderShaderItem, currentShaderItem]), targetItem)

        self.assertFalse(targetPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertTrue(self._upgradeOffered(targetItem))

    @_skipUnlessMtlxVersionAuthored
    def testPasteWholeMaterialKeepsVersion(self):
        '''Test that a copied material keeps its version when pasted.'''
        cmds.file(new=True, force=True)

        proxyShape, proxyShapeItem = self._createProxyShapeItem()
        materialItem, _, _, version = self._createStampedMaterial(proxyShapeItem)
        scopeItem = self._createMaterialsScope(proxyShape)

        pasteCmd = self._paste(self._copy([materialItem]), scopeItem)

        self.assertEqual(len(pasteCmd.targetItems()), 1)
        pastedMaterialPrim = usdUtils.getPrimFromSceneItem(pasteCmd.targetItems()[0])
        self.assertEqual(self._authoredMaterialXVersion(pastedMaterialPrim), version)

    def _exportMaterialXStack(self, usdFileName, convertMaterialsTo):
        '''Export a LookdevX MaterialX stack assigned to a sphere and return the exported
        stage and material prim. The caller must hold the stage to keep the prim valid.'''
        try:
            cmds.loadPlugin("LookdevXMaya", quiet=True)
        except RuntimeError:
            self.skipTest('The LookdevXMaya plugin is not available')

        cmds.file(new=True, force=True)

        mtlxFile = os.path.join(self._testDir, '..', 'usd', 'translators',
            'UsdExportMaterialXSurfaceShader', 'MaterialXStackExport.mtlx')
        stackName = cmds.createNode('materialxStack')
        stackItem = ufe.Hierarchy.createItem(ufe.PathString.path(cmds.ls(stackName, long=True)[0]))
        ufe.ContextOps.contextOps(stackItem).doOp(['MxImportDocument', mtlxFile])
        documentItem = ufe.Hierarchy.hierarchy(stackItem).children()[-1]

        cmds.select(cmds.polySphere())
        surfaceItem = ufe.Hierarchy.createItem(ufe.PathString.path(
            ufe.PathString.string(documentItem.path()) + "%Standard_Surface1"))
        ufe.ContextOps.contextOps(surfaceItem).doOp(['Assign Material to Selection'])

        usdFilePath = os.path.join(os.getcwd(), usdFileName)
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo=convertMaterialsTo,
            materialsScopeName='Looks', defaultPrim='None')

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)
        materialPrim = stage.GetPrimAtPath("/Looks/Standard_Surface1SG")
        self.assertTrue(materialPrim)
        return stage, materialPrim

    def _assertCurrentMaterialXVersion(self, materialPrim):
        # The writer reads the stack document through MaterialX, which upgrades it to the
        # library version, so the material carries the major.minor library version.
        fullVersion = os.environ.get('MATERIALX_VERSION')
        if not fullVersion:
            self.skipTest('MATERIALX_VERSION is not set by the test harness')
        expectedVersion = ".".join(fullVersion.split(".")[:2])
        self.assertEqual(self._authoredMaterialXVersion(materialPrim), expectedVersion)

    @_skipUnlessMtlxVersionAuthored
    def testExportMaterialXStackAuthorsVersionOnMaterial(self):
        '''Test that exporting a MaterialX stack authors the MaterialX version on the material.'''
        _stage, materialPrim = self._exportMaterialXStack('MaterialXStackVersion.usda', ['MaterialX'])

        self._assertCurrentMaterialXVersion(materialPrim)

    @_skipUnlessMtlxVersionAuthored
    def testExportMaterialXStackMultiConversionVersionOnMaterial(self):
        '''Test that exporting a MaterialX stack with several material conversions authors the
        MaterialX version on the material and not on the per-conversion NodeGraph.'''
        _stage, materialPrim = self._exportMaterialXStack('MaterialXStackVersionMulti.usda',
            ['UsdPreviewSurface', 'MaterialX'])

        self._assertCurrentMaterialXVersion(materialPrim)
        nodeGraphPrim = materialPrim.GetChild("MaterialX")
        self.assertTrue(nodeGraphPrim)
        self.assertFalse(nodeGraphPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))


if __name__ == '__main__':
    unittest.main(verbosity=2)
