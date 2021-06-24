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

from pxr import Gf
from pxr import Sdf
from pxr import Usd
from pxr import UsdShade
from pxr import UsdUtils

import os
import unittest

from maya import cmds
from maya import standalone

import fixturesUtils

class testUsdExportImportRoundtripPreviewSurface(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        test_dir = os.path.join(inputPath,
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

    def __testUsdPreviewSurfaceRoundtrip(self, metallic=True):
        """
        Tests that a usdPreviewSurface exports and imports correctly.
        """
        mayaUsdPluginName = "mayaUsdPlugin"
        if not cmds.pluginInfo(mayaUsdPluginName, query=True, loaded=True):
            cmds.loadPlugin(mayaUsdPluginName)

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

        for att_name in (".coverage", ".translateFrame", ".rotateFrame",
                         ".mirrorU", ".mirrorV", ".stagger", ".wrapU",
                         ".wrapV", ".repeatUV", ".offset", ".rotateUV",
                         ".noiseUV", ".vertexUvOne", ".vertexUvTwo",
                         ".vertexUvThree", ".vertexCameraOne"):
            cmds.connectAttr(uv_node + att_name, file_node + att_name, f=True)

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
        usd_path = os.path.abspath('UsdPreviewSurfaceRoundtripTest{}.usda'.format('Metallic' if metallic else 'Specular'))

        cmds.file(usd_path, force=True,
                  options="shadingMode=useRegistry;mergeTransformAndShape=1",
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

        # Check the new sphere is in the new shading group:
        self.assertTrue(cmds.sets(
            "pSphere1Shape",
            isMember=material_sg))

        # Check that we have no spurious "Looks" transform
        expectedTr = set(['front', 'persp', 'side', 'top', 'pSphere1'])
        allTr = set(cmds.ls(tr=True))
        self.assertEqual(allTr, expectedTr)

        # Check connections:
        self.assertEqual(
            cmds.connectionInfo(material_node+".outColor", dfs=True),
            [material_sg+".surfaceShader"])
        self.assertEqual(
            cmds.connectionInfo(material_node+".diffuseColor", sfd=True),
            file_node+".outColor")
        self.assertEqual(
            cmds.connectionInfo(file_node+".wrapU", sfd=True),
            "place2dTexture.wrapU")

        # Check values:
        self.assertAlmostEqual(cmds.getAttr(material_node+".ior"), 2)
        self.assertAlmostEqual(cmds.getAttr(material_node+".roughness"),
                               0.25)
        self.assertAlmostEqual(cmds.getAttr(material_node+".opacityThreshold"),
                               0.5)
        self.assertEqual(cmds.getAttr(material_node+".specularColor"),
                         [(0.125, 0.25, 0.75)])

        self.assertEqual(cmds.getAttr(material_node+".useSpecularWorkflow"), int(not metallic))

        self.assertEqual(cmds.getAttr(file_node+".defaultColor"),
                         [(0.5, 0.25, 0.125)])
        self.assertEqual(cmds.getAttr(file_node+".colorSpace"), "ACEScg")
        self.assertEqual(cmds.getAttr(file_node+".colorSpace"), "ACEScg")
        imported_path = cmds.getAttr(file_node+".fileTextureName")
        # imported path will be absolute:
        self.assertFalse(imported_path.startswith(".."))
        self.assertEqual(imported_path.lower(), original_path.lower())
        self.assertEqual(cmds.getAttr("place2dTexture.wrapU"), 0)
        self.assertEqual(cmds.getAttr("place2dTexture.wrapV"), 1)

        # Make sure paths are relative in the USD file. Joining the directory
        # that the USD file lives in with the texture path should point us at
        # a file that exists.
        stage = Usd.Stage.Open(usd_path)
        texture_prim = stage.GetPrimAtPath(
            "/%s/Looks/%s/%s" % (sphere_xform, material_sg, file_node))
        rel_texture_path = texture_prim.GetAttribute('inputs:file').Get().path

        usd_dir = os.path.dirname(usd_path)
        full_texture_path = os.path.join(usd_dir, rel_texture_path)
        self.assertTrue(os.path.isfile(full_texture_path))

    def testShadingRoundtrip(self):
        """
        Test that shading group and surface node names will survive a roundtrip
        """
        mayaUsdPluginName = "mayaUsdPlugin"
        if not cmds.pluginInfo(mayaUsdPluginName, query=True, loaded=True):
            cmds.loadPlugin(mayaUsdPluginName)

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
                    options="shadingMode=useRegistry;mergeTransformAndShape=1",
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
            self.assertTrue(cmds.sets("pSphere1Shape", isMember=final_sg))

            # Check surface name:
            self.assertEqual(
                cmds.connectionInfo(final_surf+".outColor", dfs=True),
                [final_sg+".surfaceShader"])

    def testDisplayColorLossyRoundtrip(self):
        """
        Test that shading group names created for display color import are in
        sync with their surface shaders.
        """
        mayaUsdPluginName = "mayaUsdPlugin"
        if not cmds.pluginInfo(mayaUsdPluginName, query=True, loaded=True):
            cmds.loadPlugin(mayaUsdPluginName)

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
                final_surf = "%s%s" % (preferred, i)
                # We expect blinnSG, blinn1SG, blinn2SG
                final_sg = final_surf + "SG"

                # Check surface name:
                self.assertEqual(
                    cmds.connectionInfo(final_surf+".outColor", dfs=True),
                    [final_sg+".surfaceShader"])



if __name__ == '__main__':
    unittest.main(verbosity=2)
