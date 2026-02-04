#!/usr/bin/env mayapy
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

from pxr import Tf
from pxr import Usd
from pxr import UsdShade, UsdGeom

import json
import os
import unittest

from maya import cmds
from maya import standalone

import fixturesUtils
try:
    import mayaUtils
except ImportError:
    pass

def connectUVNode(uv_node, file_node):
    for att_name in (".coverage", ".translateFrame", ".rotateFrame",
                    ".mirrorU", ".mirrorV", ".stagger", ".wrapU",
                    ".wrapV", ".repeatUV", ".offset", ".rotateUV",
                    ".noiseUV", ".vertexUvOne", ".vertexUvTwo",
                    ".vertexUvThree", ".vertexCameraOne"):
        cmds.connectAttr(uv_node + att_name, file_node + att_name, f=True)
    cmds.connectAttr(uv_node + ".outUV", file_node + ".uvCoord", f=True)
    cmds.connectAttr(uv_node + ".outUvFilterSize", file_node + ".uvFilterSize", f=True)

class testUsdExportImportRoundtripPreviewSurface(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

        test_dir = os.path.join(cls.inputPath,
                                "UsdExportImportRoundtripPreviewSurface")
        if not os.path.isdir(test_dir):
            os.mkdir(test_dir)
        cmds.workspace(test_dir, o=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testUsdPreviewSurfaceRoundtripSpecular(self):
        self.__testUsdPreviewSurfaceRoundtrip(metallic=False)

    def testUsdPreviewSurfaceRoundtripMetallic(self):
        self.__testUsdPreviewSurfaceRoundtrip(metallic=True)

    # Temporarily disabling since the import will be in a separate PR.
    @unittest.skipUnless("mayaUtils" in globals() and mayaUtils.mayaMajorVersion() >= 2023, 'Requires MaterialX support.')
    def testUsdPreviewSurfaceRoundtripMaterialX(self):
        self.__testUsdPreviewSurfaceRoundtrip(metallic=True,
                                              convertTo="MaterialX")

    def __testUsdPreviewSurfaceRoundtrip(self,
                                         metallic=True,
                                         convertTo="UsdPreviewSurface"):
        """
        Tests that a usdPreviewSurface exports and imports correctly.
        """
        mark = Tf.Error.Mark()
        mark.SetMark()
        self.assertTrue(mark.IsClean())

        self.assertTrue(mayaUtils.loadPlugin("mayaUsdPlugin"))

        cmds.file(f=True, new=True)

        sphere_xform = cmds.polySphere()[0]

        material_node = cmds.shadingNode("usdPreviewSurface", asShader=True,
                                         name="usdPreviewSurface42")

        material_sg = cmds.sets(renderable=True, noSurfaceShader=True,
                                empty=True, name=material_node+"SG")
        cmds.connectAttr(material_node+".outColor",
                         material_sg+".surfaceShader", force=True)
        cmds.sets(sphere_xform, e=True, forceElement=material_sg)

        cmds.setAttr(material_node + ".ior", 2)
        cmds.setAttr(material_node + ".roughness", 0.25)
        cmds.setAttr(material_node + ".specularColor", 0.125, 0.25, 0.75,
                     type="double3")
        cmds.setAttr(material_node + ".useSpecularWorkflow", not metallic)
        cmds.setAttr(material_node + ".opacityThreshold", 0.5)

        file_node = cmds.shadingNode("file", asTexture=True,
                                     isColorManaged=True)
        uv_node = cmds.shadingNode("place2dTexture", asUtility=True)

        connectUVNode(uv_node, file_node)

        cmds.connectAttr(file_node + ".outColor",
                         material_node + ".diffuseColor", f=True)

        txfile = os.path.join("..", "textures", "Brazilian_rosewood_pxr128.png")
        cmds.setAttr(file_node+".fileTextureName", txfile, type="string")
        cmds.setAttr(file_node+".colorSpace", "ACEScg", type="string")
        cmds.setAttr(file_node + ".defaultColor", 0.5, 0.25, 0.125,
                     type="double3")

        default_ext_setting = cmds.file(q=True, defaultExtensions=True)
        cmds.file(defaultExtensions=False)
        cmds.setAttr(uv_node+".wrapU", 0)
        original_path = cmds.getAttr(file_node+".fileTextureName")

        # Export to USD:
        file_suffix = "_{}_{}".format(convertTo, 'Metallic' if metallic else 'Specular')
        usd_path = os.path.abspath('UsdPreviewSurfaceRoundtripTest{}.usda'.format(file_suffix))

        export_options = [
            "shadingMode=useRegistry",
            "convertMaterialsTo=[{}]".format(convertTo),
            "mergeTransformAndShape=1",
            "legacyMaterialScope=0",
            "defaultPrim=None"
        ]

        cmds.file(usd_path, force=True,
                  options=";".join(export_options),
                  typ="USD Export", pr=True, ea=True)

        cmds.file(defaultExtensions=default_ext_setting)

        cmds.file(newFile=True, force=True)

        # Import back:
        import_options = ("shadingMode=[[useRegistry,{}]]".format(convertTo),
                          "preferredMaterial=none",
                          "primPath=/")
        cmds.file(usd_path, i=True, type="USD Import",
                  ignoreVersion=True, ra=True, mergeNamespacesOnClash=False,
                  namespace="Test", pr=True, importTimeRange="combine",
                  options=";".join(import_options))

        # Check the new sphere is in the new shading group:
        self.assertTrue(cmds.sets(
            "Test:pSphere1Shape",
            isMember="Test:"+material_sg))

        # MaterialX preserves the place2dTexture name:
        p2dName = "place2dTexture"
        if convertTo == "MaterialX":
            p2dName = uv_node

        # Check that we have no spurious "Looks" transform
        expectedTr = set(['front', 'persp', 'side', 'top', 'Test:pSphere1'])
        allTr = set(cmds.ls(tr=True))
        self.assertEqual(allTr, expectedTr)

        # Check connections:
        self.assertEqual(
            cmds.connectionInfo("Test:"+material_node+".outColor", dfs=True),
            ["Test:"+material_sg+".surfaceShader"])
        self.assertEqual(
            cmds.connectionInfo("Test:"+material_node+".diffuseColor", sfd=True),
            "Test:"+file_node+".outColor")
        self.assertEqual(
            cmds.connectionInfo("Test:"+file_node+".wrapU", sfd=True),
            "Test:%s.wrapU"%p2dName)

        # Check values:
        self.assertAlmostEqual(cmds.getAttr("Test:"+material_node+".ior"), 2)
        self.assertAlmostEqual(cmds.getAttr("Test:"+material_node+".roughness"),
                               0.25)
        self.assertAlmostEqual(cmds.getAttr("Test:"+material_node+".opacityThreshold"),
                               0.5)
        self.assertEqual(cmds.getAttr("Test:"+material_node+".specularColor"),
                         [(0.125, 0.25, 0.75)])

        self.assertEqual(cmds.getAttr("Test:"+material_node+".useSpecularWorkflow"), int(not metallic))

        self.assertEqual(cmds.getAttr("Test:"+file_node+".defaultColor"),
                         [(0.5, 0.25, 0.125)])
        self.assertEqual(cmds.getAttr("Test:"+file_node+".colorSpace"), "ACEScg")
        self.assertEqual(cmds.getAttr("Test:"+file_node+".colorSpace"), "ACEScg")
        imported_path = cmds.getAttr("Test:"+file_node+".fileTextureName")
        # imported path will be absolute:
        self.assertFalse(imported_path.startswith(".."))
        self.assertEqual(os.path.normpath(imported_path.lower()),
                         os.path.normpath(original_path.lower()))
        self.assertEqual(cmds.getAttr("Test:%s.wrapU"%p2dName), 0)
        self.assertEqual(cmds.getAttr("Test:%s.wrapV"%p2dName), 1)

        # Make sure paths are relative in the USD file. Joining the directory
        # that the USD file lives in with the texture path should point us at
        # a file that exists.
        file_template = "/Looks/{}/{}"
        if convertTo == "MaterialX":
            file_template = "/Looks/{0}/MayaNG_{0}/{1}"
        stage = Usd.Stage.Open(usd_path)
        texture_prim = stage.GetPrimAtPath(
            file_template.format(material_sg, file_node))
        rel_texture_path = texture_prim.GetAttribute('inputs:file').Get().path

        usd_dir = os.path.dirname(usd_path)
        full_texture_path = os.path.join(usd_dir, rel_texture_path)
        self.assertTrue(os.path.isfile(full_texture_path))

        self.assertTrue(mark.IsClean())

    def testShadingRoundtrip(self):
        """
        Test that shading group and surface node names will survive a roundtrip
        """
        mark = Tf.Error.Mark()
        mark.SetMark()
        self.assertTrue(mark.IsClean())

        self.assertTrue(mayaUtils.loadPlugin("mayaUsdPlugin"))

        testPatterns = [
            # Each test has 5 elements:
            #   - Shader type
            #   - Initial shader name
            #   - Initial shading group name
            #   - Roundtrip shader name
            #   - Roundtrip shading group name

            # Fully modified names will survive a roundtrip:
            ("lambert", "bob", "bobSG", "bob", "bobSG"),
            # Modified sg name survive (and we do not touch surface name)
            ("blinn", "blinn42", "blueSG", "blinn42", "blueSG"),
            # Default surface names will survive even if mismatched:
            ("phong", "phong12", "blinn27SG", "phong12", "blinn27SG"),

            # WARNING: Meaninful surface names win over boring shading group
            # names, so this combination does not roundtrip. The final shading
            # group name will be modified to be consistent with the surface
            # shader name:
            ("blinn", "myGold", "blinn12SG",
                      "myGold", "myGoldSG"),
            ("usdPreviewSurface", "jersey12", "blinn27SG",
                                  "jersey12", "jersey12SG"),

            # This will make the UsdMaterial and UsdGeomSubset names more
            # meaningful.
        ]

        for sh_type, init_surf, init_sg, final_surf, final_sg in testPatterns:

            cmds.file(f=True, new=True)

            sphere_xform = cmds.polySphere()[0]

            material_node = cmds.shadingNode(sh_type, asShader=True,
                                             name=init_surf)
            material_sg = cmds.sets(renderable=True, noSurfaceShader=True,
                                    empty=True, name=init_sg)
            cmds.connectAttr(material_node+".outColor",
                             material_sg+".surfaceShader", force=True)
            cmds.sets(sphere_xform, e=True, forceElement=material_sg)

            default_ext_setting = cmds.file(q=True, defaultExtensions=True)
            cmds.file(defaultExtensions=False)

            # Export to USD:
            usd_path = os.path.abspath('%sRoundtripTest.usda' % init_surf)

            cmds.file(usd_path, force=True,
                    options="shadingMode=useRegistry;convertMaterialsTo=[UsdPreviewSurface];mergeTransformAndShape=1",
                    typ="USD Export", pr=True, ea=True)

            cmds.file(defaultExtensions=default_ext_setting)

            cmds.file(newFile=True, force=True)

            # Import back:
            import_options = ("shadingMode=[[useRegistry,UsdPreviewSurface]]",
                            "preferredMaterial=none",
                            "primPath=/")
            cmds.file(usd_path, i=True, type="USD Import",
                    ignoreVersion=True, ra=True, mergeNamespacesOnClash=False,
                    namespace="Test", pr=True, importTimeRange="combine",
                    options=";".join(import_options))

            # Check shading group name:
            self.assertTrue(cmds.sets("Test:pSphere1Shape", isMember="Test:"+final_sg))

            # Check surface name:
            self.assertEqual(
                cmds.connectionInfo("Test:"+final_surf+".outColor", dfs=True),
                ["Test:"+final_sg+".surfaceShader"])

        self.assertTrue(mark.IsClean())

    def testDisplayColorLossyRoundtrip(self):
        """
        Test that shading group names created for display color import are in
        sync with their surface shaders.
        """
        mark = Tf.Error.Mark()
        mark.SetMark()
        self.assertTrue(mark.IsClean())

        self.assertTrue(mayaUtils.loadPlugin("mayaUsdPlugin"))

        for i in range(1,4):
            sphere_xform = cmds.polySphere()[0]
            init_surf = "test%i" % i
            init_sg = init_surf + "SG"
            material_node = cmds.shadingNode("lambert", asShader=True,
                                             name=init_surf)
            material_sg = cmds.sets(renderable=True, noSurfaceShader=True,
                                    empty=True, name=init_sg)
            cmds.connectAttr(material_node+".outColor",
                             material_sg+".surfaceShader", force=True)
            cmds.sets(sphere_xform, e=True, forceElement=material_sg)

        # Export to USD:
        usd_path = os.path.abspath('DisplayColorRoundtripTest.usda')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usd_path,
            shadingMode='none',
            exportDisplayColor=True)


        for preferred in ("blinn", "phong"):
            cmds.file(newFile=True, force=True)

            import_options = ("shadingMode=[[displayColor,default]]",
                              "preferredMaterial=%s" % preferred,
                              "primPath=/")
            cmds.file(usd_path, i=True, type="USD Import",
                    ignoreVersion=True, ra=True, mergeNamespacesOnClash=False,
                    namespace="Test", pr=True, importTimeRange="combine",
                    options=";".join(import_options))

            for i in ("", "1", "2"):
                # We expect blinn, blinn1, blinn2
                final_surf = "Test:%s%s" % (preferred, i)
                # We expect blinnSG, blinn1SG, blinn2SG
                final_sg = final_surf + "SG"

                # Check surface name:
                self.assertEqual(
                    cmds.connectionInfo(final_surf+".outColor", dfs=True),
                    [final_sg+".surfaceShader"])

        self.assertTrue(mark.IsClean())

    def testUVReaderMerging(self):
        """
        Test that we produce a minimal number of UV readers
        """
        cmds.file(f=True, new=True)

        sphere_xform = cmds.polySphere()[0]

        material_node = cmds.shadingNode("usdPreviewSurface", asShader=True,
                                            name="ss01")
        material_sg = cmds.sets(renderable=True, noSurfaceShader=True,
                                empty=True, name="ss01SG")
        cmds.connectAttr(material_node+".outColor",
                            material_sg+".surfaceShader", force=True)
        cmds.sets(sphere_xform, e=True, forceElement=material_sg)

        # One file with UVs connected to diffuse:
        file_node = cmds.shadingNode("file", asTexture=True,
                                     isColorManaged=True)
        uv_node = cmds.shadingNode("place2dTexture", asUtility=True)
        cmds.setAttr(uv_node + ".offsetU", 0.125)
        cmds.setAttr(uv_node + ".offsetV", 0.5)
        connectUVNode(uv_node, file_node)
        cmds.connectAttr(file_node + ".outColor",
                         material_node + ".diffuseColor", f=True)

        # Another file, same UVs, connected to emissiveColor
        file_node = cmds.shadingNode("file", asTexture=True,
                                     isColorManaged=True)
        connectUVNode(uv_node, file_node)
        cmds.connectAttr(file_node + ".outColor",
                         material_node + ".emissiveColor", f=True)

        # Another file, no UVs, connected to metallic
        file_node = cmds.shadingNode("file", asTexture=True,
                                     isColorManaged=True)
        cmds.connectAttr(file_node + ".outColorR",
                         material_node + ".metallic", f=True)

        # Another file, no UVs, connected to roughness
        file_node = cmds.shadingNode("file", asTexture=True,
                                     isColorManaged=True)
        cmds.connectAttr(file_node + ".outColorR",
                         material_node + ".roughness", f=True)
        cmds.setAttr(file_node + ".offsetU", 0.25)
        cmds.setAttr(file_node + ".offsetV", 0.75)

        # Export to USD:
        usd_path = os.path.abspath('MinimalUVReader.usda')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usd_path,
            shadingMode='useRegistry',
            convertMaterialsTo=['UsdPreviewSurface'],
            exportDisplayColor=True,
            legacyMaterialScope=False,
            defaultPrim="None")

        # We expect 2 primvar readers, and 2 st transforms:
        stage = Usd.Stage.Open(usd_path)
        mat_path = "/Looks/ss01SG/"

        # Here are the expected connections in the produced USD file:
        connections = [
            # Source node, input, destination node:
            ("ss01", "diffuseColor", "file1"),
            ("file1", "st", "place2dTexture1_UsdTransform2d"),
            ("place2dTexture1_UsdTransform2d", "in", "place2dTexture1"),

            ("ss01", "emissiveColor", "file2"),
            ("file2", "st", "place2dTexture1_UsdTransform2d"), # re-used
            # Note that the transform name is derived from place2DTexture name.

            ("ss01", "metallic", "file3"),
            ("file3", "st", "shared_TexCoordReader"), # no UV in Maya.

            ("ss01", "roughness", "file4"),
            ("file4", "st", "file4_UsdTransform2d"), # xform on file node
            ("file4_UsdTransform2d", "in", "shared_TexCoordReader")
            # Note that the transform name is derived from file node name.
        ]
        for src_name, input_name, dst_name in connections:
            src_prim = stage.GetPrimAtPath(mat_path + src_name)
            self.assertTrue(src_prim)
            src_shade = UsdShade.Shader(src_prim)
            self.assertTrue(src_shade)
            src_input = src_shade.GetInput(input_name)
            self.assertTrue(src_input.HasConnectedSource())
            (connect_api, out_name, _) = src_input.GetConnectedSource()
            self.assertEqual(connect_api.GetPath(), mat_path + dst_name)

    def CommonGeomSubsetRoundtrip(self, useCollections):
        """Common test for geom subset export"""
        cmds.file(f=True, new=True)

        # Take a cube since it already has 6 subsets defined:
        cube_xform = cmds.polyCube()[0]

        # Apply a global material and two face materials:
        def createMaterial(name, R, G, B, subset):
            material_node = cmds.shadingNode("usdPreviewSurface", asShader=True, name=name)
            material_sg = cmds.sets(renderable=True, noSurfaceShader=True, empty=True, name=name+"SG")
            cmds.connectAttr(material_node+".outColor", material_sg+".surfaceShader", force=True)
            cmds.setAttr(material_node+".diffuseColor", R, G, B, type="double3")
            cmds.sets(cube_xform + subset, e=True, forceElement=material_sg)
        createMaterial("greyCube", .15, .15, .15, "")
        createMaterial("redFace", .95, .0, .0, ".f[0]")
        createMaterial("blueFace", .0, .0, .95, ".f[4]")

        # Export:
        if useCollections:
            usd_path = os.path.abspath('CubeWithAssignedFaces_CB.usda')
            cmds.usdExport(mergeTransformAndShape=True,
                file=usd_path,
                shadingMode='useRegistry',
                convertMaterialsTo=['UsdPreviewSurface'],
                exportDisplayColor=False,
                exportCollectionBasedBindings=True,
                exportComponentTags=True,
                legacyMaterialScope=False,
                defaultPrim='pCube1')
        else:
            usd_path = os.path.abspath('CubeWithAssignedFaces.usda')
            cmds.usdExport(mergeTransformAndShape=True,
                file=usd_path,
                shadingMode='useRegistry',
                convertMaterialsTo=['UsdPreviewSurface'],
                exportDisplayColor=False,
                exportComponentTags=True,
                legacyMaterialScope=False,
                defaultPrim='pCube1')

        stage = Usd.Stage.Open(usd_path)

        cube = stage.GetPrimAtPath("/pCube1")
        numGeomSubsets = 0
        for children in cube.GetAllChildren():
            if children.IsA(UsdGeom.Subset):
                numGeomSubsets += 1
        self.assertEqual(numGeomSubsets, 9)

        # These are the subsets as initially exported from the cube:
        allSubsets = {
            # The shading GeomSubsets should be marked as Maya generated
            "greyCubeSG": ([1, 2, 3, 5], "materialBind", "greyCubeSG", True),
            "redFaceSG": ([0,], "materialBind", "redFaceSG", True),
            "blueFaceSG": ([4,], "materialBind", "blueFaceSG", True),
            # The cube subsets are all standard:
            "top": ([1,], "componentTag", None, False),
            "bottom": ([3,], "componentTag", None, False),
            "left": ([5,], "componentTag", None, False),
            "right": ([4,], "componentTag", None, False),
            "front": ([0,], "componentTag", None, False),
            "back": ([2,], "componentTag", None, False),
        }
        
        def ValidateUSDStage(self, stage, allSubsets):
            cube = stage.GetPrimAtPath("/pCube1")
            numGeomSubsets = 0
            for children in cube.GetAllChildren():
                if children.IsA(UsdGeom.Subset):
                    numGeomSubsets += 1
            self.assertEqual(numGeomSubsets, len(allSubsets))

            # Validate the subsets are as expected:
            for geomSubsetName, info in allSubsets.items():
                faces, familyName, materialName, isMayaGenerated = info
                geomSubset = stage.GetPrimAtPath("/pCube1/" + geomSubsetName)
                self.assertEqual(geomSubset.GetAttribute("familyName").Get(), familyName)
                self.assertEqual(geomSubset.GetAttribute("elementType").Get(), "face")
                self.assertEqual(geomSubset.GetAttribute("indices").Get(), faces)
                if isMayaGenerated:
                    customData = geomSubset.GetCustomDataByKey("Maya")
                    self.assertIsNotNone(customData)
                    self.assertIn("generated", customData)
                else:
                    self.assertFalse(geomSubset.HasCustomDataKey("Maya"))
                if materialName:
                    bindAPI = UsdShade.MaterialBindingAPI(geomSubset)
                    material = bindAPI.ComputeBoundMaterial()[0]
                    self.assertEqual(material.GetPath().name, materialName)

        ValidateUSDStage(self, stage, allSubsets)

        cmds.file(f=True, new=True)

        # Re-import. We expect to still have 6 componentTags on the mesh and
        # two assigned faces:
        cmds.usdImport(file=usd_path, shadingMode=[["useRegistry", "UsdPreviewSurface"], ])
        
        expectedFaces = {
            "top": "f[1]",
            "bottom": "f[3]",
            "left": "f[5]",
            "right": "f[4]",
            "front": "f[0]",
            "back": "f[2]",
        }
        self.assertEqual(set(cmds.geometryAttrInfo('pCube1.outMesh', cnm=True)), set(expectedFaces.keys()))
        for tagName, components in expectedFaces.items():
            self.assertEqual(cmds.geometryAttrInfo("pCube1.outMesh", cmp=True, cex=tagName), [components,])

        cmds.select("greyCubeSG")
        self.assertEqual(set(cmds.ls(selection=True)), set(['pCube1.f[1:3]', 'pCube1.f[5]']))
        cmds.select("redFaceSG")
        self.assertEqual(cmds.ls(selection=True), ['pCube1.f[0]',])
        cmds.select("blueFaceSG")
        self.assertEqual(cmds.ls(selection=True), ['pCube1.f[4]',])

        expectedMayaSets = set(["defaultLightSet", "defaultObjectSet", "initialParticleSE", "initialShadingGroup",
                                "greyCubeSG", "redFaceSG", "blueFaceSG"])
        self.assertEqual(set(cmds.ls(sets=True)), expectedMayaSets)

        # We now edit the USD data to create a few interesting differences
        # in component tag family names and shader assignments:
        geomSubset = stage.GetPrimAtPath("/pCube1/top")
        geomSubset.GetAttribute("familyName").Set("topFamily")

        geomSubset = stage.GetPrimAtPath("/pCube1/left")
        geomSubset.GetAttribute("familyName").Set("materialBind")
        subsetBindAPI = UsdShade.MaterialBindingAPI.Apply(geomSubset.GetPrim())
        subsetBindAPI.Bind(UsdShade.Material(stage.GetPrimAtPath("/pCube1/Looks/blueFaceSG")))

        meshBindAPI = UsdShade.MaterialBindingAPI(stage.GetPrimAtPath("/pCube1"))
        geomSubset = meshBindAPI.CreateMaterialBindSubset("newKidOnTheBlock", [1, 2], UsdGeom.Tokens.face)
        subsetBindAPI = UsdShade.MaterialBindingAPI.Apply(geomSubset.GetPrim())
        subsetBindAPI.Bind(UsdShade.Material(stage.GetPrimAtPath("/pCube1/Looks/redFaceSG")))

        # The "unassigned faced" were previously [1, 2, 3, 5]
        # We have assigned faces 1, 2, and 5 (left), so the only one remaining is 3:
        geomSubset = UsdGeom.Subset(stage.GetPrimAtPath("/pCube1/greyCubeSG"))
        geomSubset.GetIndicesAttr().Set([3,])

        # Save the modified stage under a new name:
        if useCollections:
            modified_usd_path = os.path.abspath('CubeWithModifiedFaces_CB.usda')
        else:
            modified_usd_path = os.path.abspath('CubeWithModifiedFaces.usda')
        stage.GetRootLayer().Export(modified_usd_path)

        # Update our expected results:
        allSubsets["top"] = ([1,], "topFamily", None, False)
        allSubsets["left"] = ([5,], "materialBind", "blueFaceSG", False)
        allSubsets["newKidOnTheBlock"] = ([1, 2], "materialBind", "redFaceSG", False)
        allSubsets["greyCubeSG"] = ([3,], "materialBind", "greyCubeSG", True)

        # Test that the stage we are about to import corresponds to our expectations:        
        ValidateUSDStage(self, stage, allSubsets)

        cmds.file(f=True, new=True)

        # Re-import. We expect now to have 7 componentTags on the mesh and
        # a more complex face assignment:
        cmds.usdImport(file=modified_usd_path, shadingMode=[["useRegistry", "UsdPreviewSurface"], ])

        expectedFaces["newKidOnTheBlock"] = "f[1:2]"

        self.assertEqual(set(cmds.geometryAttrInfo('pCube1.outMesh', cnm=True)), set(expectedFaces.keys()))
        for tagName, components in expectedFaces.items():
            self.assertEqual(cmds.geometryAttrInfo("pCube1.outMesh", cmp=True, cex=tagName), [components,])

        cmds.select("greyCubeSG")
        self.assertEqual(cmds.ls(selection=True), ['pCube1.f[3]',])
        cmds.select("redFaceSG")
        self.assertEqual(cmds.ls(selection=True), ['pCube1.f[0:2]',])
        cmds.select("blueFaceSG")
        self.assertEqual(cmds.ls(selection=True), ['pCube1.f[4:5]',])

        self.assertEqual(set(cmds.ls(sets=True)), expectedMayaSets)

        # We should also have some roundtripping info on the mesh:
        roundtripInfo = json.loads(cmds.getAttr("pCube1.USD_GeomSubsetInfo"))
        cmds.select("redFaceSG", noExpand=True)
        redUUID = cmds.ls(selection=True, uuid=True)[0]
        cmds.select("blueFaceSG", noExpand=True)
        blueUUID = cmds.ls(selection=True, uuid=True)[0]
        expectedInfo = {
            "left": {
                "familyName": "materialBind",
                "materialUUID": blueUUID
            },
            "newKidOnTheBlock": {
                "familyName": "materialBind",
                "materialUUID": redUUID},
            "top": {
                "familyName": "topFamily"
            }
        }
        self.assertEqual(roundtripInfo, expectedInfo)

        # We export without changes to make sure we get back the modified cube without
        # any extra subsets.
        if useCollections:
            reexported_usd_path = os.path.abspath('CubeWithAssignedFaces_reexported_CB.usda')
            cmds.usdExport(mergeTransformAndShape=True,
                file=reexported_usd_path,
                shadingMode='useRegistry',
                exportDisplayColor=False,
                exportCollectionBasedBindings=True,
                exportComponentTags=True)
        else:
            reexported_usd_path = os.path.abspath('CubeWithAssignedFaces_reexported.usda')
            cmds.usdExport(mergeTransformAndShape=True,
                file=reexported_usd_path,
                shadingMode='useRegistry',
                exportDisplayColor=False,
                exportComponentTags=True)
        
        stage = Usd.Stage.Open(reexported_usd_path)

        # Test that the re-exported stage has the exact same GeomsSubsets as the one we imported:
        ValidateUSDStage(self, stage, allSubsets)

    def testGeomSubsetRoundtrip(self):
        """Test that GeomSubsets numbers stay under control when rountripping
           with direct material assignment"""
        self.CommonGeomSubsetRoundtrip(False)

    def testGeomSubsetRoundtripCollectionBased(self):
        """Test that GeomSubsets numbers stay under control when rountripping
           with collection based material assignemnt"""
        self.CommonGeomSubsetRoundtrip(True)

    def testOpacityRoundtrip(self):
        """
        Test that opacity roundtrips as expected.
        """
        filePath = os.path.join(self.inputPath,
                                "UsdExportImportRoundtripPreviewSurfaceTest",
                                "OpacityTests.ma")
        cmds.file(filePath, force=True, open=True)

        usd_path = os.path.abspath('OpacityRoundtripTest.usda')
        cmds.usdExport(mergeTransformAndShape=True,
                       file=usd_path,
                       shadingMode='useRegistry',
                       convertMaterialsTo=['UsdPreviewSurface'],
                       legacyMaterialScope=False,
                       defaultPrim='None')

        stage = Usd.Stage.Open(usd_path)

        # We have 7 materials that are named:
        #    /Looks/standardSurface7SG/standardSurface7
        surf_base = "/Looks/standardSurface{1}SG/standardSurface{1}"
        # results for opacity are mostly connections to:
        #    /Looks/standardSurface7SG/file6.outputs:a
        cnx_base = "/Looks/standardSurface{1}SG/file{0}"
        # so we only need to expect a channel name:
        expected = ["r", "a", "r", "a", "g", "a", 0.4453652]

        for i, val in enumerate(expected):
            surf_path = surf_base.format(i+1, i+2)
            surf_prim = stage.GetPrimAtPath(surf_path)
            self.assertTrue(surf_prim)
            surf_shade = UsdShade.Shader(surf_prim)
            self.assertTrue(surf_shade)
            # useSpecularWorkflow is not exported anymore:
            use_specular_workflow = surf_shade.GetInput("useSpecularWorkflow")
            self.assertFalse(use_specular_workflow)
            opacity = surf_shade.GetInput("opacity")
            self.assertTrue(opacity)
            if (isinstance(val, float)):
                self.assertAlmostEqual(opacity.Get(), val)
            else:
                (connect_api, out_name, _) = opacity.GetConnectedSource()
                self.assertEqual(out_name, val)
                cnx_string = cnx_base.format(i+1, i+2)
                self.assertEqual(connect_api.GetPath(), cnx_string)

        cmds.file(f=True, new=True)

        # Re-import for a full roundtrip:
        # Import back:
        import_options = ("shadingMode=[[useRegistry,UsdPreviewSurface]]",
                          "preferredMaterial=standardSurface",
                          "primPath=/")
        cmds.file(usd_path, i=True, type="USD Import",
                  ignoreVersion=True, ra=True, mergeNamespacesOnClash=False,
                  namespace="Test", pr=True, importTimeRange="combine",
                  options=";".join(import_options))

        # The roundtrip is not perfect. We use explicit connections everywhere
        # on import:
        #
        # We expect the following connections:
        #      {'standardSurface2.opacityR': 'file1.outColorR',
        #       'standardSurface2.opacityG': 'file1.outColorR',
        #       'standardSurface2.opacityB': 'file1.outColorR'}
        #
        port_names = {"r": "outColorR", "g": "outColorG",
                      "b": "outColorB", "a": "outAlpha"}
        opacity_names = ["opacityR", "opacityG", "opacityB"]
        for i, val in enumerate(expected):
            if (isinstance(val, float)):
                for v in cmds.getAttr("Test:standardSurface8.opacity")[0]:
                    self.assertAlmostEqual(v, val)
            else:
                cnx = cmds.listConnections("Test:standardSurface{}".format(i+2),
                                           d=False, c=True, p=True)
                self.assertEqual(len(cnx), 6)
                for j in range(int(len(cnx)/2)):
                    k = cnx[2*j].split(".")
                    v = cnx[2*j+1].split(".")
                    self.assertEqual(len(k), 2)
                    self.assertEqual(len(v), 2)
                    self.assertTrue(k[1] in opacity_names)
                    self.assertEqual(v[0], "Test:file{}".format(i+1))
                    self.assertEqual(v[1], port_names[val])

        cmds.file(f=True, new=True)


if __name__ == '__main__':
    unittest.main(verbosity=2)
