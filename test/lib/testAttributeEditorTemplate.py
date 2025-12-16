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

from __future__ import print_function
import unittest

import mayaUtils
import fixturesUtils
import testUtils

from maya import cmds
from maya import mel
import maya.internal.common.ufe_ae.template as ufeAeTemplate

from mayaUsdLibRegisterStrings import getMayaUsdLibString
import mayaUsd_createStageWithNewLayer
import mayaUsd.lib

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
    
    def findAllFrameLayoutLabels(self, startLayout):
        '''
        Find all child frame layout and return a list of their labels.
        '''
        found = []
        
        if not startLayout:
            return found
            
        childrenOfLayout = cmds.layout(startLayout, q=True, ca=True)
        if not childrenOfLayout:
            return found
            
        for child in childrenOfLayout:
            child = startLayout + '|' + child
            if cmds.frameLayout(child, exists=True):
                found.append(cmds.frameLayout(child, q=True, label=True))
            elif cmds.layout(child, exists=True):
                found += self.findAllFrameLayoutLabels(child)
                    
        return found

    def findExpandedFrameLayout(self, startLayout, labelToFind, primPath):
        '''
        Find and expand the given frame layout and return the new updated layout.
        We unfortunately need to explicitly update the AE for this to work.
        '''
        frameLayout = self.searchForMayaControl(startLayout, cmds.frameLayout, labelToFind)
        self.assertIsNotNone(frameLayout, 'Could not find "%s" frameLayout' % labelToFind)
        self.expandFrameLayout(frameLayout, primPath)
        return self.searchForMayaControl(startLayout, cmds.frameLayout, labelToFind)
    
    def expandFrameLayout(self, frameLayout, primPath):
        '''
        Expand the given frame layout and refresh the AE.
        You will need to find the layout again after that since it might have changed name.
        '''
        cmds.frameLayout(frameLayout, edit=True, collapse=False)
        mel.eval('''updateAE "%s"''' % primPath)

    def findAllNonLayout(self, startLayout):
        '''
        Find all child non-layout controls and return them as a list.
        '''
        found = []
        
        if not startLayout:
            return found
            
        childrenOfLayout = cmds.layout(startLayout, q=True, ca=True)
        if not childrenOfLayout:
            return found
            
        for child in childrenOfLayout:
            child = startLayout + '|' + child
            if cmds.layout(child, exists=True):
                found += self.findAllNonLayout(child)
            else:
                found.append(child)
                    
        return found

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

        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path(proxyShape)
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)

        # Create a Capsule via contextOps menu. Not all versions of Maya automatically
        # select the prim from 'Add New Prim', so always select it here.
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'Capsule'])
        fullPrimPath = proxyShape + ",/Capsule1"
        cmds.select(fullPrimPath)

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
        frameLayout = self.findExpandedFrameLayout(startLayout, 'Capsule', fullPrimPath)
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
        if mayaUtils.mayaMajorVersion() <= 2022:
            # In maya 2022, Python 2, the Extent attribute is now in a Boundable section.
            #
            # Note: in Maya 2022, in preflight, the layout cannot be found for some reaon,
            #       even though it works when run locally.
            pass

            # boundableLayout = self.findExpandedFrameLayout(startLayout, 'Boundable', fullPrimPath)
            # self.assertIsNotNone(boundableLayout, 'Could not find "Boundable" frameLayout')

            # self.expandFrameLayout(boundableLayout, fullPrimPath)
            # capsuleFormLayout = self.attrEdFormLayoutName('Capsule')
            # self.assertTrue(cmds.formLayout(capsuleFormLayout, exists=True))
            # startLayout = cmds.formLayout(capsuleFormLayout, query=True, fullPathName=True)
            # self.assertIsNotNone(startLayout, 'Could not get full path for Capsule formLayout')
            # boundableLayout = self.findExpandedFrameLayout(startLayout, 'Boundable', fullPrimPath)
            # self.assertIsNotNone(boundableLayout, 'Could not find "Boundable" frameLayout')

            # extentControl = self.searchForMayaControl(boundableLayout, cmds.text, 'Extent')
            # self.assertIsNotNone(extentControl, 'Could not find Capsule Extent control')
        else:
            extentControl = self.searchForMayaControl(frameLayout, cmds.text, 'Extent')
            self.assertIsNotNone(extentControl, 'Could not find Capsule Extent control')

        # --------------------------------------------------------------------------------
        # Test the 'createMetadataSection' method of template.
        # --------------------------------------------------------------------------------

        # We should have a frameLayout called 'Metadata' in the template.
        # If there is a scripting error in the template, this layout will be missing.
        frameLayout = self.findExpandedFrameLayout(startLayout, getMayaUsdLibString('kLabelMetadata'), fullPrimPath)
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

    def testCreateDisplaySection(self):
        '''Simple test for the createDisplaySection in AE template.'''

        # Create a simple USD scene with a single prim.
        cmds.file(new=True, force=True)

        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path(proxyShape)
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)

        # Create a Capsule via contextOps menu. Not all versions of Maya automatically
        # select the prim from 'Add New Prim', so always select it here.
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'Capsule'])
        fullPrimPath = proxyShape + ",/Capsule1"
        cmds.select(fullPrimPath)

        # Make sure the AE is visible.
        import maya.mel
        maya.mel.eval('openAEWindow')

        def findDisplayLayout():
            capsuleFormLayout = self.attrEdFormLayoutName('Capsule')
            startLayout = cmds.formLayout(capsuleFormLayout, query=True, fullPathName=True)
            kDisplay = maya.mel.eval('uiRes(\"m_AEdagNodeTemplate.kDisplay\");')
            return self.findExpandedFrameLayout(startLayout, kDisplay, fullPrimPath)


        # --------------------------------------------------------------------------------
        # Test the 'createDisplaySection' method of template.
        # --------------------------------------------------------------------------------
        displayLayout = findDisplayLayout()
        self.assertIsNotNone(displayLayout, 'Could not find Display frameLayout')

        # Maya 2022 doesn't remember and restore the expand/collapse state,
        # so skip this part of the test.
        if mayaUtils.mayaMajorVersion() > 2022:
            # We should have 'Visibility' optionMenuGrp (which is a label (text) and optionMenu).
            visControl = self.searchForMayaControl(displayLayout, cmds.text, 'Visibility')
            self.assertIsNotNone(visControl)

            # We should have a 'Use Outliner Color' checkBoxGrp (which is a label(text) and checkBox).
            useOutlinerColorControl = self.searchForMayaControl(displayLayout, cmds.checkBox, 'Use Outliner Color')
            self.assertIsNotNone(useOutlinerColorControl)

    def testAECustomMaterialControl(self):
        '''Simple test for the MaterialCustomControl in AE template.'''

        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene("MaterialX", "MayaSurfaces.usda")
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        # Select this prim which has a custom material control attribute.
        fullPrimPath = shapeNode + ',/pCube1'
        cmds.select(fullPrimPath, r=True)

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
        materialFormLayout = self.findExpandedFrameLayout(startLayout, 'Material', fullPrimPath)
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
        fullPrimPath = shapeNode + ',/TypeSampler/MaterialX/D_filename'
        cmds.select(fullPrimPath, r=True)

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
            frameLayout = self.findExpandedFrameLayout(startLayout, 'Shader: Dot', fullPrimPath)
            self.assertIsNotNone(frameLayout, 'Could not find "Shader: Dot" frameLayout')

            # We should also have custom image control for 'In'.
            InputsInControl = self.searchForMayaControl(frameLayout, cmds.text, 'In')
            self.assertIsNotNone(InputsInControl, 'Could not find D_filename "In" control')

    def testAECustomEnumControl(self):
        '''Simple test for the EnumCustomControl.creator in AE template.'''

        from ufe_ae.usd.nodes.usdschemabase.ae_template import AETemplate
        from ufe_ae.usd.nodes.usdschemabase.enumCustomControl import EnumCustomControl
        if EnumCustomControl.creator not in AETemplate._controlCreators:
            self.skipTest('Test only available if AE template has EnumCustomControl.creator.')

        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene("MaterialX", "int_enum.usda")
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        # Select this prim which has a custom image control attribute.
        fullPrimPath = shapeNode + ',/Material1/gltf_pbr1'
        cmds.select(fullPrimPath, r=True)

        # Make sure the AE is visible.
        import maya.mel
        maya.mel.eval('openAEWindow')

        # In the AE there is a formLayout for each USD prim type. We start
        # by making sure we can find the one for Shader.
        shaderFormLayout = self.attrEdFormLayoutName('Shader')
        self.assertTrue(cmds.formLayout(shaderFormLayout, exists=True))
        startLayout = cmds.formLayout(shaderFormLayout, query=True, fullPathName=True)
        self.assertIsNotNone(startLayout, 'Could not get full path for Shader formLayout')

        frameLayout = self.searchForMayaControl(startLayout, cmds.frameLayout, 'Shader: glTF PBR')
        self.assertIsNotNone(frameLayout, 'Could not find "Shader: glTF PBR" frameLayout')

        self.expandFrameLayout(frameLayout, fullPrimPath)

        shaderFormLayout = self.attrEdFormLayoutName('Shader')
        self.assertTrue(cmds.formLayout(shaderFormLayout, exists=True))
        startLayout = cmds.formLayout(shaderFormLayout, query=True, fullPathName=True)
        self.assertIsNotNone(startLayout, 'Could not get full path for Shader formLayout')
        frameLayout = self.searchForMayaControl(startLayout, cmds.frameLayout, 'Shader: glTF PBR')
        self.assertIsNotNone(frameLayout, 'Could not find "Shader: glTF PBR" frameLayout')

        # We should have a frameLayout called 'Alpha' in the template.
        # If there is a scripting error in the template, this layout will be missing.
        frameLayout = self.searchForMayaControl(startLayout, cmds.frameLayout, 'Alpha')
        self.assertIsNotNone(frameLayout, 'Could not find "Alpha" frameLayout')

        self.expandFrameLayout(frameLayout, fullPrimPath)

        shaderFormLayout = self.attrEdFormLayoutName('Shader')
        self.assertTrue(cmds.formLayout(shaderFormLayout, exists=True))
        startLayout = cmds.formLayout(shaderFormLayout, query=True, fullPathName=True)
        self.assertIsNotNone(startLayout, 'Could not get full path for Shader formLayout')
        frameLayout = self.searchForMayaControl(startLayout, cmds.frameLayout, 'Shader: glTF PBR')
        self.assertIsNotNone(frameLayout, 'Could not find "Shader: glTF PBR" frameLayout')
        frameLayout = self.searchForMayaControl(startLayout, cmds.frameLayout, 'Alpha')
        self.assertIsNotNone(frameLayout, 'Could not find "Alpha" frameLayout')

        # We should also have custom enum control for 'Inputs Alpha Mode'.
        InputsAlphaModeControl = self.searchForMayaControl(frameLayout, cmds.text, 'Alpha Mode')
        self.assertIsNotNone(InputsAlphaModeControl, 'Could not find gltf_pbr1 "Alpha Mode" control')

    def testAEConnectionsCustomControl(self):
        '''Simple test for the ConnectionsCustomControl.creator in AE template.'''

        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('MaterialX', 'multiple_connections.usda')
        testPath,shapeStage = mayaUtils.createProxyFromFile(testFile)

        # Select this prim which has an attribute with a connection.
        fullPrimPath = testPath + ',/Material1/fractal3d1'
        cmds.select(fullPrimPath, r=True)

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
            # We should have a frameLayout called 'Shader: 3D Fractal Noise' in the template.
            # If there is a scripting error in the template, this layout will be missing.
            frameLayout = self.findExpandedFrameLayout(startLayout, 'Shader: 3D Fractal Noise', fullPrimPath)
            self.assertIsNotNone(frameLayout, 'Could not find "Shader: 3D Fractal Noise" frameLayout')
            
            # We should also have an attribute called 'Amplitude' which has a connection.
            AmplitudeControl = self.searchForMayaControl(frameLayout, cmds.text, 'Amplitude')
            self.assertIsNotNone(AmplitudeControl, 'Could not find fractal3d1 "Amplitude" control')

    def testAEConnectionsCustomControlWithComponents(self):
        '''Test that ConnectionsCustomControl.creator in AE template navigates component connections correctly.'''
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('MaterialX', 'multiple_connections.usda')
        testPath,shapeStage = mayaUtils.createProxyFromFile(testFile)

        # Select this prim which has an attribute with a connection.
        fullPrimPath = testPath + ',/Material1/component_add'
        cmds.select(fullPrimPath, r=True)

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
            frameLayout = self.findExpandedFrameLayout(startLayout, 'Shader: Add', fullPrimPath)
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

    def testAEForLight(self):
        '''Test that the expected sections are created for lights.'''
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path(proxyShape)
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)

        # Create a cylinder light via contextOps menu. Not all versions of Maya automatically
        # select the prim from 'Add New Prim', so always select it here.
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'All Registered', 'Lighting', 'CylinderLight'])
        primName = 'CylinderLight1'
        fullPrimPath = proxyShape + ',/%s' % primName
        cmds.select(fullPrimPath)

        # Make sure the AE is visible.
        import maya.mel
        maya.mel.eval('openAEWindow')

        primFormLayout = self.attrEdFormLayoutName('CylinderLight')
        self.assertTrue(cmds.formLayout(primFormLayout, exists=True), 'Layout for %s was not found\n' % primName)
            
        startLayout = cmds.formLayout(primFormLayout, query=True, fullPathName=True)
        self.assertIsNotNone(startLayout, 'Could not get full path for %s formLayout' % primName)

        # Augment the maximum diff size to get better error message when comparing the lists.
        self.maxDiff = 2000
        
        actualSectionLabels = self.findAllFrameLayoutLabels(startLayout)

        # Note: different version of USD can have different schemas,
        #       so we only compare the ones we are interested in verifying.
        expectedInitialSectionLabels = [
            'Light ',
            'Cylinder Light',
            'Light Link Collection ',
            'Shadow Link Collection ']
        self.assertListEqual(
            actualSectionLabels[0:len(expectedInitialSectionLabels)],
            expectedInitialSectionLabels)

        # Note: the extra attributes sometimes show up, especially
        #       in older versions of Maya, so we don't compare an exact list.
        self.assertIn('Transforms', actualSectionLabels[-4:])
        self.assertIn('Display', actualSectionLabels[-4:])
        self.assertIn('Metadata', actualSectionLabels[-4:])

    def testAEForDefWithSchema(self):
        '''Test that the expected sections are created for def with added schema.'''
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path(proxyShape)
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)

        # Create a Def prim via contextOps menu. Not all versions of Maya automatically
        # select the prim from 'Add New Prim', so always select it here.
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'Def'])
        primName = 'Def1'
        fullPrimPath = proxyShape + ',/%s' % primName
        cmds.select(fullPrimPath)

        # Add Light schema to the Def prim.
        cmds.mayaUsdSchema(fullPrimPath, schema='LightAPI')

        # Make sure the AE is visible.
        import maya.mel
        maya.mel.eval('openAEWindow')

        # Note: for Def, the nodeType command returns ''.
        primFormLayout = self.attrEdFormLayoutName('')
        self.assertTrue(cmds.formLayout(primFormLayout, exists=True), 'Layout for %s was not found\n' % primName)
            
        startLayout = cmds.formLayout(primFormLayout, query=True, fullPathName=True)
        self.assertIsNotNone(startLayout, 'Could not get full path for %s formLayout' % primName)

        # Augment the maximum diff size to get better error message when comparing the lists.
        self.maxDiff = 2000
        
        actualSectionLabels = self.findAllFrameLayoutLabels(startLayout)

        # Note: different version of USD can have different schemas,
        #       so we only compare the ones we are interested in verifying.
        expectedInitialSectionLabels = [
            'Light ',
            'Light Link Collection ',
            'Shadow Link Collection ']
        self.assertListEqual(
            actualSectionLabels[0:len(expectedInitialSectionLabels)],
            expectedInitialSectionLabels)
        self.assertIn('Metadata', actualSectionLabels)

    def testAECustomAttributeCallback(self):
        '''Test that the custm atribute callbacks work.'''

        # AE template UI callback function.
        self.templateUseCount = 0
        customSectionName ='UnlikelySectionName'

        # Callback function which receives two params as input: callback context and
        # callback data (empty).
        def onBuildAETemplateCallback(context, data):
            # In the callback context you can retrieve the ufe path string.
            ufe_path_string = context.get('ufe_path_string')
            ufePath = ufe.PathString.path(ufe_path_string)
            ufeItem = ufe.Hierarchy.createItem(ufePath)
            self.assertTrue(ufeItem)

            # The callback data is empty.

            # Note: do not import this globally, because at unit test startup time
            #       the Python search path is not setup to find it. Once Maya runs,
            #       we can import it.
            from ufe_ae.usd.nodes.usdschemabase.ae_template import AETemplate as mayaUsd_AETemplate

            mayaUsdAETemplate = mayaUsd_AETemplate.getAETemplateForCustomCallback()
            self.assertTrue(mayaUsdAETemplate)
            if mayaUsdAETemplate:
                # Create a new section and add attributes to it.
                with ufeAeTemplate.Layout(mayaUsdAETemplate, customSectionName, collapse=False):
                    pass
            
            self.templateUseCount = self.templateUseCount + 1

        # Register your callback so it will be called during the MayaUsd AE
        # template UI building code. This would probably done during plugin loading.
        # The first param string 'onBuildAETemplate' is the callback operation and the
        # second is the name of your callback function.
        mayaUsd.lib.registerUICallback('onBuildAETemplate', onBuildAETemplateCallback)

        try:
            proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
            proxyShapePath = ufe.PathString.path(proxyShape)
            proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)

            # Create a capsule prim.
            proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
            proxyShapeContextOps.doOp(['Add New Prim', 'Capsule'])
            fullPrimPath = proxyShape + ",/Capsule1"
            cmds.select(fullPrimPath)

            # Make sure the AE is visible.
            import maya.mel
            maya.mel.eval('openAEWindow')

            capsuleFormLayout = self.attrEdFormLayoutName('Capsule')
            self.assertTrue(cmds.formLayout(capsuleFormLayout, exists=True))
            startLayout = cmds.formLayout(capsuleFormLayout, query=True, fullPathName=True)
            self.assertIsNotNone(startLayout, 'Could not get full path for Capsule formLayout')

            # Augment the maixmum diff size to get better error message when comparing the lists.
            self.maxDiff = 2000
            
            actualSectionLabels = self.findAllFrameLayoutLabels(startLayout)

            # Verify we got the custom section.
            self.assertIn(customSectionName, actualSectionLabels)
            self.assertGreater(self.templateUseCount, 0)
        except:
            # During your plugin unload you should unregister the callback (and any attribute
            # nice naming function you also added).
            mayaUsd.lib.unregisterUICallback('onBuildAETemplate', onBuildAETemplateCallback)
            raise
        else:
            mayaUsd.lib.unregisterUICallback('onBuildAETemplate', onBuildAETemplateCallback)

        
    def testAERelationshipAttribute(self):
        '''Test that the relationship attribute is displayed in the AE.'''
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path(proxyShape)
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)

        # Create a capsule prim.
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'Capsule'])
        fullPrimPath = proxyShape + ",/Capsule1"

        # check for a proxyPrim relationship property in the AE,
        # for now these will appear in the Extra attributes section

        # Make sure the AE is visible.
        import maya.mel
        maya.mel.eval('openAEWindow')

        # Make sure we can find formLayout for the PhysicsScene.
        capsuleFormLayout = self.attrEdFormLayoutName('Capsule')
        self.assertTrue(cmds.formLayout(capsuleFormLayout, exists=True))
        startLayout = cmds.formLayout(capsuleFormLayout, query=True, fullPathName=True)
        self.assertIsNotNone(startLayout, 'Could not get full path for Capsule formLayout')

        sectionName = maya.mel.eval("uiRes(\"s_TPStemplateStrings.rExtraAttributes\");")
        frameLayout = self.findExpandedFrameLayout(startLayout, sectionName, fullPrimPath)
        self.assertIsNotNone(frameLayout, 'Could not find Extra Attributes frameLayout')

        # check for the proxyPrim relationship property in the Extra attributes section
        proxyPrimControl = self.searchForMayaControl(frameLayout, cmds.text, 'Proxy Prim')
        self.assertIsNotNone(proxyPrimControl, 'Could not find Proxy Prim relationship property in Extra attributes section')

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
