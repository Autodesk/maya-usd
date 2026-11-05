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

from maya import standalone
from maya import cmds

import ufe

from pxr import Usd, UsdShade

import os
import platform
import subprocess
import sys
import unittest

import MaterialX_1_38_to_1_39

try:
    from pxr import UsdMtlx
    USD_HAS_MATERIALX_PLUGIN = True
except ImportError:
    USD_HAS_MATERIALX_PLUGIN = False

def _expectsMtlxVersion():
    return USD_HAS_MATERIALX_PLUGIN and Usd.GetVersion() >= (0, 25, 2)

_skipUnlessMtlxVersionAuthored = unittest.skipUnless(
    _expectsMtlxVersion() and ufeUtils.ufeFeatureSetVersion() >= 4,
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

    def _addShader(self, materialItem, shaderId):
        '''Add a shader with the given info:id in the material and return its scene item.'''
        materialHier = ufe.Hierarchy.hierarchy(materialItem)
        childPathsBefore = {ufe.PathString.string(child.path()) for child in materialHier.children()}

        ufe.ContextOps.contextOps(materialItem).doOp(['Add New Prim', 'Shader'])

        newChildren = [child for child in materialHier.children()
                       if ufe.PathString.string(child.path()) not in childPathsBefore]
        self.assertEqual(len(newChildren), 1)
        shaderItem = newChildren[0]

        ufe.Attributes.attributes(shaderItem).attribute("info:id").set(shaderId)
        return shaderItem

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

    def _createProxyShapeItem(self):
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapeItem = ufe.Hierarchy.createItem(ufe.PathString.path(proxyShape))
        return proxyShape, proxyShapeItem

    def _createEmptyMaterial(self):
        _, proxyShapeItem = self._createProxyShapeItem()
        ufe.ContextOps.contextOps(proxyShapeItem).doOp(['Add New Prim', 'Material'])
        materialItem = ufe.Hierarchy.hierarchy(proxyShapeItem).children()[-1]
        return materialItem, usdUtils.getPrimFromSceneItem(materialItem)

    @_skipUnlessMtlxVersionAuthored
    def testNewEmptyMaterialIsNotLegacy(self):
        '''Test that connecting a MaterialX shader to a new empty material does not make it
        legacy, and that the version follows undo and redo of the connection.'''
        cmds.file(new=True, force=True)

        materialItem, materialPrim = self._createEmptyMaterial()

        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))

        cmd = self._addStandardSurfaceAndConnect(materialItem)

        self.assertTrue(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertFalse(self._upgradeOffered(materialItem))

        cmd.undo()
        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))

        cmd.redo()
        self.assertTrue(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertFalse(self._upgradeOffered(materialItem))

    @_skipUnlessMtlxVersionAuthored
    def testAuthoredVersionIsKeptOnConnection(self):
        '''Test that a version already authored on a material is not overwritten by a new
        connection, so an older version stays detectable as legacy.'''
        cmds.file(new=True, force=True)

        materialItem, materialPrim = self._createEmptyMaterial()

        UsdMtlx.MaterialXConfigAPI.Apply(materialPrim).CreateConfigMtlxVersionAttr("1.38")

        self._addStandardSurfaceAndConnect(materialItem)

        self.assertTrue(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertEqual(materialPrim.GetAttribute("config:mtlx:version").Get(), "1.38")
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

        testFile = testUtils.getTestScene("lookdevXUsd", "mx_updates_to_1_39.usda")
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)
        self.assertIsNotNone(shapeNode)

        materialItem = ufeUtils.createUfeSceneItem(shapeNode, "/mtl/layer_update_material")
        self.assertIsNotNone(materialItem)
        materialPrim = usdUtils.getPrimFromSceneItem(materialItem)

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

        testFile = testUtils.getTestScene("lookdevXUsd", "mx_updates_to_1_39.usda")
        shapeNode, _ = mayaUtils.createProxyFromFile(testFile)
        self.assertIsNotNone(shapeNode)

        materialPath = "/mtl/layer_update_material"
        materialItem = ufeUtils.createUfeSceneItem(shapeNode, materialPath)
        self.assertIsNotNone(materialItem)
        legacySurfaceItem = ufeUtils.createUfeSceneItem(shapeNode, materialPath + "/surface1")
        self.assertIsNotNone(legacySurfaceItem)

        handler = ufe.RunTimeMgr.instance().connectionHandler(materialItem.runTimeId())
        handler.deleteConnectionCmd(
            ufe.Attributes.attributes(legacySurfaceItem).attribute("outputs:out"),
            ufe.Attributes.attributes(materialItem).attribute("outputs:mtlx:surface")).execute()
        materialPrim = usdUtils.getPrimFromSceneItem(materialItem)
        # The disconnection also removes the now unused MaterialX surface output.
        surfaceOutput = UsdShade.Material(materialPrim).GetSurfaceOutput("mtlx")
        self.assertFalse(surfaceOutput and surfaceOutput.HasConnectedSource())

        self._addStandardSurfaceAndConnect(materialItem)

        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertTrue(self._upgradeOffered(materialItem))

    @_skipUnlessMtlxVersionAuthored
    def testNewMaterialWithTextureIsNotLegacy(self):
        '''Test that a new material whose surface is fed by a texture node is stamped with the
        current version, since the texture is upstream of the connected surface.'''
        cmds.file(new=True, force=True)

        materialItem, materialPrim = self._createEmptyMaterial()

        imageItem = self._addShader(materialItem, "ND_image_color3")
        surfaceItem = self._addShader(materialItem, "ND_standard_surface_surfaceshader")

        handler = ufe.RunTimeMgr.instance().connectionHandler(materialItem.runTimeId())
        handler.createConnectionCmd(
            ufe.Attributes.attributes(imageItem).attribute("outputs:out"),
            ufe.Attributes.attributes(surfaceItem).attribute("inputs:base_color")).execute()
        self.assertFalse(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))

        self._connectSurface(surfaceItem, materialItem)

        self.assertTrue(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertFalse(self._upgradeOffered(materialItem))

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
        self.assertTrue(materialPrim.HasAPI(UsdMtlx.MaterialXConfigAPI))
        self.assertEqual(materialPrim.GetAttribute("config:mtlx:version").Get(), expectedVersion)

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
