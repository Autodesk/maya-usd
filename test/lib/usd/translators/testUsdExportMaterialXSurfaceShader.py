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

from pxr import Usd
from pxr import Sdf
from pxr import Sdr
from pxr import UsdShade

from maya import standalone
import maya.cmds as cmds
import maya.mel as mel
import ufe

import os
import unittest

import fixturesUtils

class testUsdExportMaterialXSurfaceShader(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # The materialxStack node is available in the LookdevX plugin.
        cmds.loadPlugin("LookdevXMaya", quiet=True)
        cls._inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportMaterialXStack(self):

        cmds.file(f=True, new=True)

        mtlxFile = os.path.join(self._inputPath, 'UsdExportMaterialXSurfaceShader',
            'MaterialXStackExport.mtlx')

        stackName = mel.eval("createNode materialxStack")
        stackPathString = mel.eval("ls -l " + stackName)[0]
        stackItem = ufe.Hierarchy.createItem(ufe.PathString.path(stackPathString))
        stackHierarchy = ufe.Hierarchy.hierarchy(stackItem)
        stackContextOps = ufe.ContextOps.contextOps(stackItem)
        stackContextOps.doOp(['MxImportDocument', mtlxFile])
        documentItem = stackHierarchy.children()[-1]
        sphere = cmds.polySphere()
        surfPathString = ufe.PathString.string(documentItem.path()) + "%Standard_Surface1"
        cmds.select(sphere)
        materialContextOps = ufe.ContextOps.contextOps(ufe.Hierarchy.createItem(ufe.PathString.path(surfPathString)))
        materialContextOps.doOp(['Assign Material to Selection']) 

        # Export to USD.
        usdFilePath = os.path.abspath('MaterialXStackExport.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo=['MaterialX'],
            defaultPrim='None')

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)
        prim = stage.GetPrimAtPath("/Looks/Standard_Surface1SG")
        self.assertTrue(prim)
        material = UsdShade.Material(prim)
        self.assertTrue(material)

        # Validate the displacement shader
        dispOutput = material.GetDisplacementOutput("mtlx")
        self.assertTrue(dispOutput)
        dispSource = dispOutput.GetConnectedSources()[0]
        dispPrim = dispSource[0].source.GetPrim()
        self.assertEqual(dispPrim.GetName(), "displacement1")
        id = dispPrim.GetAttribute('info:id')
        self.assertEqual(id.Get(), 'ND_displacement_float')
        dispShader = UsdShade.Shader(dispPrim)
        dispIn = dispShader.GetInput('displacement')
        # Validate the hexagon shader, connected to the displacement
        hexPrim = dispIn.GetConnectedSources()[0][0].source.GetPrim()
        self.assertEqual(hexPrim.GetName(), 'hexagon1')
        self.assertEqual(hexPrim.GetAttribute('info:id').Get(), 'ND_hexagon_float')
        hexShader = UsdShade.Shader(hexPrim)
        self.assertEqual(hexShader.GetInput('radius').Get(), 1)

        # Validate the Surface shader
        surfOutput = material.GetSurfaceOutput("mtlx")
        self.assertTrue(surfOutput)
        surfSource = surfOutput.GetConnectedSources()[0]
        surfPrim = surfSource[0].source.GetPrim()
        self.assertEqual(surfPrim.GetName(), "Standard_Surface1")
        id = surfPrim.GetAttribute('info:id')
        self.assertEqual(id.Get(), 'ND_standard_surface_surfaceshader')
        surfShader = UsdShade.Shader(surfPrim)
        colorIn = surfShader.GetInput('base_color')
        # Validate the Node Graph
        nodeGraphPrim = colorIn.GetConnectedSources()[0][0].source.GetPrim()
        self.assertEqual(nodeGraphPrim.GetName(), 'compound1')
        nodeGraph = UsdShade.NodeGraph(nodeGraphPrim)
        # Validate the Checkerboard Shader
        checkboardPrim = nodeGraph.GetOutput('out').GetConnectedSources()[0][0].source.GetPrim()
        self.assertEqual(checkboardPrim.GetName(), 'checkerboard1')
        self.assertEqual(checkboardPrim.GetAttribute('info:id').Get(), 'ND_checkerboard_color3')

    def testExportMultiOutput(self):

        cmds.file(f=True, new=True)

        mtlxFile = os.path.join(self._inputPath, 'UsdExportMaterialXSurfaceShader',
            'MultiOutput.mtlx')

        stackName = mel.eval("createNode materialxStack")
        stackPathString = mel.eval("ls -l " + stackName)[0]
        stackItem = ufe.Hierarchy.createItem(ufe.PathString.path(stackPathString))
        stackHierarchy = ufe.Hierarchy.hierarchy(stackItem)
        stackContextOps = ufe.ContextOps.contextOps(stackItem)
        stackContextOps.doOp(['MxImportDocument', mtlxFile])
        documentItem = stackHierarchy.children()[-1]
        sphere = cmds.polySphere()
        surfPathString = ufe.PathString.string(documentItem.path()) + "%test_multi_out_mat"
        cmds.select(sphere)
        materialContextOps = ufe.ContextOps.contextOps(ufe.Hierarchy.createItem(ufe.PathString.path(surfPathString)))
        materialContextOps.doOp(['Assign Material to Selection']) 

        # Export to USD.
        usdFilePath = os.path.abspath('Multioutput.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo=['MaterialX'],
            defaultPrim='None')

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        prim = stage.GetPrimAtPath("/Looks/test_multi_out_matSG")
        self.assertTrue(prim)
        material = UsdShade.Material(prim)
        self.assertTrue(material)

        # Validate that the artistic_ior output were properly exported and connected
        surfOutput = material.GetSurfaceOutput("mtlx")
        self.assertTrue(surfOutput)
        surfSource = surfOutput.GetConnectedSources()[0]
        surfPrim = surfSource[0].source.GetPrim()
        self.assertEqual(surfPrim.GetName(), "test_multi_out_mat")
        id = surfPrim.GetAttribute('info:id')
        self.assertEqual(id.Get(), 'ND_standard_surface_surfaceshader')
        surfShader = UsdShade.Shader(surfPrim)
        specular = surfShader.GetInput('specular')
        # Validate the Node Graph
        nodeGraphPrim = specular.GetConnectedSources()[0][0].source.GetPrim()
        self.assertEqual(nodeGraphPrim.GetName(), 'compound1')
        nodeGraph = UsdShade.NodeGraph(nodeGraphPrim)
        # Validate the ior output
        iorExtractPrim = nodeGraph.GetOutput('specular_ior_output').GetConnectedSources()[0][0].source.GetPrim()
        self.assertEqual(iorExtractPrim.GetName(), 'ior')
        self.assertEqual(iorExtractPrim.GetAttribute('info:id').Get(), 'ND_extract_color3')
        iorExtractShader = UsdShade.Shader(iorExtractPrim)
        artisticiorPrim = iorExtractShader.GetInput('in').GetConnectedSources()[0][0].source.GetPrim()
        self.assertEqual(artisticiorPrim.GetName(), 'artistic_ior')
        self.assertEqual(artisticiorPrim.GetAttribute('info:id').Get(), 'ND_artistic_ior')

        # Validate the extinction output
        specularExtractPrim = nodeGraph.GetOutput('specular_output').GetConnectedSources()[0][0].source.GetPrim()
        self.assertEqual(specularExtractPrim.GetName(), 'specular')
        self.assertEqual(specularExtractPrim.GetAttribute('info:id').Get(), 'ND_extract_color3')
        specularExtractShader = UsdShade.Shader(specularExtractPrim)
        artisticiorPrim = specularExtractShader.GetInput('in').GetConnectedSources()[0][0].source.GetPrim()
        self.assertEqual(artisticiorPrim.GetName(), 'artistic_ior')

        artisticiorShader = UsdShade.Shader(artisticiorPrim)
        outs = artisticiorShader.GetOutputs() 
        self.assertEqual(outs[0].GetBaseName(), 'extinction')
        self.assertEqual(outs[1].GetBaseName(), 'ior')
        
    def testExportImageWithoutGeomProp(self):
        cmds.file(f=True, new=True)

        mtlxFile = os.path.join(self._inputPath, 'UsdExportMaterialXSurfaceShader',
            'ImageWithoutGeomProp.mtlx')

        stackName = mel.eval("createNode materialxStack")
        stackPathString = mel.eval("ls -l " + stackName)[0]
        stackItem = ufe.Hierarchy.createItem(ufe.PathString.path(stackPathString))
        stackHierarchy = ufe.Hierarchy.hierarchy(stackItem)
        stackContextOps = ufe.ContextOps.contextOps(stackItem)
        stackContextOps.doOp(['MxImportDocument', mtlxFile])
        documentItem = stackHierarchy.children()[-1]
        sphere = cmds.polySphere()
        newUVSetName = "newUVSetName"
        cmds.polyUVSet(sphere, rename=True, uvSet="map1", newUVSet=newUVSetName)
        surfPathString = ufe.PathString.string(documentItem.path()) + "%ImageWithoutGeomProp"
        cmds.select(sphere)
        materialContextOps = ufe.ContextOps.contextOps(ufe.Hierarchy.createItem(ufe.PathString.path(surfPathString)))
        materialContextOps.doOp(['Assign Material to Selection']) 

        # Export to USD.
        # preserveUVSetNames=True to validate that the UV set name are properly assigned by the _UVMappingManager
        usdFilePath = os.path.abspath('ImageWithoutGeomProp.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo=['MaterialX'],
            defaultPrim='None', preserveUVSetNames=True)

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        prim = stage.GetPrimAtPath("/Looks/ImageWithoutGeomPropSG")
        self.assertTrue(prim)
        material = UsdShade.Material(prim)
        self.assertTrue(material)

        # Validate the inputs for image_color and image_roughness
        image_color_input = material.GetInput("image_color:varname")
        image_roughness_input = material.GetInput("image_roughness:varname")
        self.assertEqual(image_color_input.GetAttr().Get(), newUVSetName)
        self.assertEqual(image_roughness_input.GetAttr().Get(), newUVSetName)

        #Get the NodeGraph and validate the inputs for image_color and image_roughness
        nodeGraphPrim = stage.GetPrimAtPath("/Looks/ImageWithoutGeomPropSG/NG_ImageWithoutGeomProp")
        self.assertTrue(nodeGraphPrim)
        nodeGraph = UsdShade.NodeGraph(nodeGraphPrim)
        NG_image_color_input = nodeGraph.GetInput("image_color:varname")
        NG_image_roughness_input = nodeGraph.GetInput("image_roughness:varname")
        self.assertEqual(NG_image_color_input.GetConnectedSources()[0][0].source.GetPrim().GetName(), "ImageWithoutGeomPropSG")
        self.assertEqual(NG_image_color_input.GetConnectedSources()[0][0].sourceName, "image_color:varname")

        self.assertEqual(NG_image_roughness_input.GetConnectedSources()[0][0].source.GetPrim().GetName(), "ImageWithoutGeomPropSG")
        self.assertEqual(NG_image_roughness_input.GetConnectedSources()[0][0].sourceName, "image_roughness:varname")

        # Validate the geompropvalue shader that were created by the export
        geomPropColorPrim = stage.GetPrimAtPath("/Looks/ImageWithoutGeomPropSG/NG_ImageWithoutGeomProp/geompropvalue_image_color")
        self.assertTrue(geomPropColorPrim)
        geomPropColorShader = UsdShade.Shader(geomPropColorPrim)
        geomColorInput = geomPropColorShader.GetInput("geomprop")
        self.assertEqual(geomColorInput.GetConnectedSources()[0][0].source.GetPrim().GetName(), "NG_ImageWithoutGeomProp")
        self.assertEqual(geomColorInput.GetConnectedSources()[0][0].sourceName, "image_color:varname")

        geomPropRoughnesPrim = stage.GetPrimAtPath("/Looks/ImageWithoutGeomPropSG/NG_ImageWithoutGeomProp/geompropvalue_image_roughness")
        self.assertTrue(geomPropColorPrim)
        geomPropRoughnesShader = UsdShade.Shader(geomPropRoughnesPrim)
        geomRoughnessInput = geomPropRoughnesShader.GetInput("geomprop")
        self.assertEqual(geomRoughnessInput.GetConnectedSources()[0][0].source.GetPrim().GetName(), "NG_ImageWithoutGeomProp")
        self.assertEqual(geomRoughnessInput.GetConnectedSources()[0][0].sourceName, "image_roughness:varname")


    def testExportWithRelativePath(self):
        cmds.file(f=True, new=True)
        mtlxFile = os.path.join(self._inputPath, 'UsdExportMaterialXSurfaceShader',
            'RelativePath.mtlx')
        
        stackName = mel.eval("createNode materialxStack")
        stackPathString = mel.eval("ls -l " + stackName)[0]
        stackItem = ufe.Hierarchy.createItem(ufe.PathString.path(stackPathString))
        stackHierarchy = ufe.Hierarchy.hierarchy(stackItem)
        stackContextOps = ufe.ContextOps.contextOps(stackItem)
        stackContextOps.doOp(['MxImportDocument', mtlxFile])
        documentItem = stackHierarchy.children()[-1]
        sphere = cmds.polySphere()
        surfPathString = ufe.PathString.string(documentItem.path()) + "%RelPath"
        cmds.select(sphere)
        materialContextOps = ufe.ContextOps.contextOps(ufe.Hierarchy.createItem(ufe.PathString.path(surfPathString)))
        materialContextOps.doOp(['Assign Material to Selection']) 

        # Export to USD.
        usdFilePath = os.path.abspath('RelPath.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry', convertMaterialsTo=['MaterialX'],
            defaultPrim='None', exportRelativeTextures='relative')

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        prim = stage.GetPrimAtPath("/Looks/RelPathSG")
        self.assertTrue(prim)
        material = UsdShade.Material(prim)
        self.assertTrue(material)

        # Validate the texture path
        fileAttrName = 'inputs:file'
        prim = stage.GetPrimAtPath("/Looks/RelPathSG/NG_RelPath/image_color")
        fileAttr = prim.GetAttribute(fileAttrName)
        self.assertTrue(fileAttr, fileAttrName)
        exportedTexturePath = fileAttr.Get()
        self.assertTrue(os.path.exists(exportedTexturePath.resolvedPath),
                        'The exported texture %s is not resolved by USD' % exportedTexturePath)

if __name__ == '__main__':
    unittest.main(verbosity=2)
