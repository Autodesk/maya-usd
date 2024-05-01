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

import unittest

import mayaUtils
import ufeUtils
import fixturesUtils
import testUtils

import mayaUsd.lib

from maya import cmds
from maya import standalone

from mayaUsdLibRegisterStrings import getMayaUsdLibString

from pxr import Usd

import ufe

class AttributeEditorTemplateTestCase(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def setUp(self):
        cmds.file(new=True, force=True)
        cmds.select(clear=True)

    def searchForMayaControl(self, controlOrLayout, mayaControlCmd, labelToFind):
        '''Helper function to search for a Maya control or layout with the label
        matching the input string <p labelToFind>. The type of the control being
        searched for is also given as input <p mayaControlCmd>.
        '''
        if mayaControlCmd(controlOrLayout, exists=True):
            if mayaControlCmd(controlOrLayout, q=True, label=True) == labelToFind:
                return controlOrLayout

        if controlOrLayout:
            childrenOfLayout = cmds.layout(controlOrLayout, q=True, ca=True)
            if childrenOfLayout:
                for child in childrenOfLayout:
                    child = controlOrLayout + '|' + child
                    if cmds.layout(child, exists=True):
                        foundControl = self.searchForMayaControl(child, mayaControlCmd, labelToFind)
                        if foundControl:
                            return foundControl
                    elif mayaControlCmd(child, exists=True):
                        if mayaControlCmd(child, q=True, label=True) == labelToFind:
                            return child
        return None

    def attrEdFormLayoutName(self, obj):
        # Ufe runtime name (USD) was added to formLayout name in 2022.2.
        attrEdLayout = 'AttrEdUSD%sFormLayout' if mayaUtils.mayaMajorMinorVersions() >= (2022, 2) else 'AttrEd%sFormLayout'
        formLayoutName = attrEdLayout % obj
        return formLayoutName

    def testAETemplate(self):
        '''Simple test to check the Attribute Editor template has no scripting errors
        which prevent it from being used. When that happens there is no layout in AE
        for USD prims. All attributes are just listed one after another.'''

        # Create a simple USD scene with a single prim.
        cmds.file(new=True, force=True)

        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path(proxyShape)
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)

        # Create a Capsule via contextOps menu. Not all versions of Maya automatically
        # select the prim from 'Add New Prim', so always select it here.
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'Capsule'])
        cmds.select(proxyShape + ",/Capsule1")

        # Enable the display of Array Attributes.
        cmds.optionVar(intValue=('mayaUSD_AEShowArrayAttributes', 1))

        # Make sure the AE is visible.
        import maya.mel
        maya.mel.eval('openAEWindow')

        # --------------------------------------------------------------------------------
        # Test the 'buildUI' method of template.
        # --------------------------------------------------------------------------------
        
        # In the AE there is a formLayout for each USD prim type. We start
        # by making sure we can find the one for capsule.
        capsuleFormLayout = self.attrEdFormLayoutName('Capsule')
        self.assertTrue(cmds.formLayout(capsuleFormLayout, exists=True))
        startLayout = cmds.formLayout(capsuleFormLayout, query=True, fullPathName=True)
        self.assertIsNotNone(startLayout, 'Could not get full path for Capsule formLayout')

        # We should have a frameLayout called 'Capsule' in the template.
        # If there is a scripting error in the template, this layout will be missing.
        frameLayout = self.searchForMayaControl(startLayout, cmds.frameLayout, 'Capsule')
        self.assertIsNotNone(frameLayout, 'Could not find Capsule frameLayout')

        # We should also have float slider controls for 'Height' & 'Radius'.
        # Note: During Maya 2024 the controls were switched from floatFieldGrp to floatSliderGrp.
        heightControl1 = self.searchForMayaControl(frameLayout, cmds.floatSliderGrp, 'Height')
        heightControl2 = self.searchForMayaControl(frameLayout, cmds.floatFieldGrp, 'Height')
        self.assertTrue(heightControl1 or heightControl2, 'Could not find Capsule Height control')
        radiusControl1 = self.searchForMayaControl(frameLayout, cmds.floatSliderGrp, 'Radius')
        radiusControl2 = self.searchForMayaControl(frameLayout, cmds.floatFieldGrp, 'Radius')
        self.assertIsNotNone(radiusControl1 or radiusControl2, 'Could not find Capsule Radius control')

        # Since we enabled array attributes we should have an 'Extent' attribute.
        extentControl = self.searchForMayaControl(frameLayout, cmds.text, 'Extent')
        self.assertIsNotNone(extentControl, 'Could not find Capsule Extent control')

        # --------------------------------------------------------------------------------
        # Test the 'createMetadataSection' method of template.
        # --------------------------------------------------------------------------------

        # We should have a frameLayout called 'Metadata' in the template.
        # If there is a scripting error in the template, this layout will be missing.
        frameLayout = self.searchForMayaControl(startLayout, cmds.frameLayout, getMayaUsdLibString('kLabelMetadata'))
        self.assertIsNotNone(frameLayout, 'Could not find Metadata frameLayout')

        # Create a SphereLight via contextOps menu.
        # This prim type has applied schemas.
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'SphereLight'])
        cmds.select(proxyShape + ",/SphereLight1")
        maya.mel.eval('openAEWindow')

        # Make sure we can find formLayout for the SphereLight.
        lightFormLayout = self.attrEdFormLayoutName('SphereLight')
        self.assertTrue(cmds.formLayout(lightFormLayout, exists=True))
        startLayout = cmds.formLayout(lightFormLayout, query=True, fullPathName=True)
        self.assertIsNotNone(startLayout, 'Could not get full path for SphereLight formLayout')

        # --------------------------------------------------------------------------------
        # Test the 'createAppliedSchemasSection' method of template.
        # Requires USD 0.21.2
        # --------------------------------------------------------------------------------
        if Usd.GetVersion() >= (0, 21, 2):
            # We should have a frameLayout called 'Applied Schemas' in the template.
            # If there is a scripting error in the template, this layout will be missing.
            frameLayout = self.searchForMayaControl(startLayout, cmds.frameLayout, getMayaUsdLibString('kLabelAppliedSchemas'))
            self.assertIsNotNone(frameLayout, 'Could not find Applied Schemas frameLayout')

        # --------------------------------------------------------------------------------
        # Test the 'createCustomExtraAttrs' method of template.
        # --------------------------------------------------------------------------------

        # Create a PhysicsScene via contextOps menu.
        # This prim type has extra attributes.
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'PhysicsScene'])
        cmds.select(proxyShape + ",/PhysicsScene1")
        maya.mel.eval('openAEWindow')

        # Make sure we can find formLayout for the PhysicsScene.
        lightFormLayout = self.attrEdFormLayoutName('PhysicsScene')
        self.assertTrue(cmds.formLayout(lightFormLayout, exists=True))
        startLayout = cmds.formLayout(lightFormLayout, query=True, fullPathName=True)
        self.assertIsNotNone(startLayout, 'Could not get full path for PhysicsScene formLayout')

        sectionName = maya.mel.eval("uiRes(\"s_TPStemplateStrings.rExtraAttributes\");")
        frameLayout = self.searchForMayaControl(startLayout, cmds.frameLayout, sectionName)
        self.assertIsNotNone(frameLayout, 'Could not find Extra Attributes frameLayout')

    def testAECustomMaterialControl(self):
        '''Simple test for the MaterialCustomControl in AE template.'''

        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene("MaterialX", "MayaSurfaces.usda")
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        # Select this prim which has a custom material control attribute.
        cmds.select('|stage|stageShape,/pCube1', r=True)

        # Make sure the AE is visible.
        import maya.mel
        maya.mel.eval('openAEWindow')

        # In the AE there is a formLayout for each USD prim type. We start
        # by making sure we can find the one for the mesh.
        meshFormLayout = self.attrEdFormLayoutName('Mesh')
        self.assertTrue(cmds.formLayout(meshFormLayout, exists=True))
        startLayout = cmds.formLayout(meshFormLayout, query=True, fullPathName=True)
        self.assertIsNotNone(startLayout, 'Could not get full path for Mesh formLayout')

        # In the AE there is a formLayout for each USD prim type. We start
        # by making sure we can find the one for the mesh.
        materialFormLayout = self.searchForMayaControl(startLayout, cmds.frameLayout, 'Material')
        self.assertIsNotNone(materialFormLayout, 'Could not find "Material" frameLayout')

        # We can now search for the control for the meterial.
        assignedMaterialControl = self.searchForMayaControl(materialFormLayout, cmds.text, 'Assigned Material')
        self.assertIsNotNone(assignedMaterialControl, 'Could not find the "Assigned Material" control')
        strengthControl = self.searchForMayaControl(materialFormLayout, cmds.optionMenuGrp, 'Strength')
        self.assertIsNotNone(strengthControl, 'Could not find the "Strength" control')

    def testAECustomImageControl(self):
        '''Simple test for the customImageControlCreator in AE template.'''
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene("MaterialX", "MtlxValueTypes.usda")
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        # Select this prim which has a custom image control attribute.
        cmds.select('|stage|stageShape,/TypeSampler/MaterialX/D_filename', r=True)

        # Make sure the AE is visible.
        import maya.mel
        maya.mel.eval('openAEWindow')

        # In the AE there is a formLayout for each USD prim type. We start
        # by making sure we can find the one for shader.
        shaderFormLayout = self.attrEdFormLayoutName('Shader')
        self.assertTrue(cmds.formLayout(shaderFormLayout, exists=True))
        startLayout = cmds.formLayout(shaderFormLayout, query=True, fullPathName=True)
        self.assertIsNotNone(startLayout, 'Could not get full path for Shader formLayout')

        if mayaUtils.mayaMajorVersion() > 2024:
            # We should have a frameLayout called 'Shader: Dot' in the template.
            # If there is a scripting error in the template, this layout will be missing.
            frameLayout = self.searchForMayaControl(startLayout, cmds.frameLayout, 'Shader: Dot')
            self.assertIsNotNone(frameLayout, 'Could not find "Shader: Dot" frameLayout')

            # We should also have custom image control for 'In'.
            InputsInControl = self.searchForMayaControl(frameLayout, cmds.text, 'In')
            self.assertIsNotNone(InputsInControl, 'Could not find D_filename "In" control')

    def testAECustomEnumControl(self):
        '''Simple test for the customEnumControlCreator in AE template.'''

        from ufe_ae.usd.nodes.usdschemabase.ae_template import AETemplate
        from ufe_ae.usd.nodes.usdschemabase.custom_enum_control import customEnumControlCreator
        if customEnumControlCreator not in AETemplate._controlCreators:
            self.skipTest('Test only available if AE template has customEnumControlCreator.')

        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene("MaterialX", "int_enum.usda")
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        # Select this prim which has a custom image control attribute.
        cmds.select('|stage|stageShape,/Material1/gltf_pbr1', r=True)

        # Make sure the AE is visible.
        import maya.mel
        maya.mel.eval('openAEWindow')

        # In the AE there is a formLayout for each USD prim type. We start
        # by making sure we can find the one for Shader.
        shaderFormLayout = self.attrEdFormLayoutName('Shader')
        self.assertTrue(cmds.formLayout(shaderFormLayout, exists=True))
        startLayout = cmds.formLayout(shaderFormLayout, query=True, fullPathName=True)
        self.assertIsNotNone(startLayout, 'Could not get full path for Shader formLayout')
        
        # We should have a frameLayout called 'Alpha' in the template.
        # If there is a scripting error in the template, this layout will be missing.
        frameLayout = self.searchForMayaControl(startLayout, cmds.frameLayout, 'Alpha')
        self.assertIsNotNone(frameLayout, 'Could not find "Alpha" frameLayout')
        
        # We should also have custom enum control for 'Inputs Alpha Mode'.
        InputsAlphaModeControl = self.searchForMayaControl(frameLayout, cmds.text, 'Alpha  Mode')
        self.assertIsNotNone(InputsAlphaModeControl, 'Could not find gltf_pbr1 "Alpha Mode" control')

    def testAEConnectionsCustomControl(self):
        '''Simple test for the connectionsCustomControlCreator in AE template.'''

        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('MaterialX', 'multiple_connections.usda')
        testPath,shapeStage = mayaUtils.createProxyFromFile(testFile)

        # Select this prim which has an attribute with a connection.
        cmds.select('|stage|stageShape,/Material1/fractal3d1', r=True)

        # Make sure the AE is visible.
        import maya.mel
        maya.mel.eval('openAEWindow')

        # In the AE there is a formLayout for each USD prim type. We start
        # by making sure we can find the one for Shader.
        shaderFormLayout = self.attrEdFormLayoutName('Shader')
        self.assertTrue(cmds.formLayout(shaderFormLayout, exists=True))
        startLayout = cmds.formLayout(shaderFormLayout, query=True, fullPathName=True)
        self.assertIsNotNone(startLayout, 'Could not get full path for Shader formLayout')
        
        if mayaUtils.mayaMajorVersion() > 2024:
            # We should have a frameLayout called 'Shader: Fractal3d' in the template.
            # If there is a scripting error in the template, this layout will be missing.
            frameLayout = self.searchForMayaControl(startLayout, cmds.frameLayout, 'Shader: Fractal3d')
            self.assertIsNotNone(frameLayout, 'Could not find "Shader: Fractal3d" frameLayout')
            
            # We should also have an attribute called 'Amplitude' which has a connection.
            AmplitudeControl = self.searchForMayaControl(frameLayout, cmds.text, 'Amplitude')
            self.assertIsNotNone(AmplitudeControl, 'Could not find fractal3d1 "Amplitude" control')

    def testAEConnectionsCustomControlWithComponents(self):
        '''Test that connectionsCustomControlCreator in AE template navigates component connections correctly.'''
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('MaterialX', 'multiple_connections.usda')
        testPath,shapeStage = mayaUtils.createProxyFromFile(testFile)

        # Select this prim which has an attribute with a connection.
        cmds.select('|stage|stageShape,/Material1/component_add', r=True)

        # Make sure the AE is visible.
        import maya.mel
        maya.mel.eval('openAEWindow')

        # In the AE there is a formLayout for each USD prim type. We start
        # by making sure we can find the one for Shader.
        shaderFormLayout = self.attrEdFormLayoutName('Shader')
        self.assertTrue(cmds.formLayout(shaderFormLayout, exists=True))
        startLayout = cmds.formLayout(shaderFormLayout, query=True, fullPathName=True)
        self.assertIsNotNone(startLayout, 'Could not get full path for Shader formLayout')
        
        if mayaUtils.mayaMajorVersion() > 2024:
            # We should have a frameLayout called 'Shader: Add' in the template.
            # If there is a scripting error in the template, this layout will be missing.
            frameLayout = self.searchForMayaControl(startLayout, cmds.frameLayout, 'Shader: Add')
            self.assertIsNotNone(frameLayout, 'Could not find "Shader: Add" frameLayout')
            
            # We should also have an attribute called 'In1' which has component connections.
            in1Label = self.searchForMayaControl(frameLayout, cmds.text, 'In1')
            self.assertIsNotNone(in1Label, 'Could not find component_add "In1" control')

            # We are interested in the popup menu which should lead to two constant nodes:
            in1Text = in1Label.removesuffix("|nameTxt") + "|attrTypeFld"
            popupMenu = in1Text + "|" + cmds.textField(in1Text, q=True, popupMenuArray=True)[0]
            self.assertEqual(2, cmds.popupMenu(popupMenu, q=True, numberOfItems=True))
            expectedLabels = ['constant1.outputs:out...', 'constant2.outputs:out...']
            for menuItem in cmds.popupMenu(popupMenu, q=True, itemArray=True):
                self.assertIn(cmds.menuItem(popupMenu + "|" + menuItem, q=True, l=True), expectedLabels)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
