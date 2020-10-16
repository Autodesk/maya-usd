#!/pxrpythonsubst
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

        cmds.workspace(test_dir, o=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testUsdPreviewSurfaceRoundtrip(self):
        """
        Tests that a usdPreviewSurface exports and imports correctly.
        """
        mayaUsdPluginName = "mayaUsdPlugin"
        if not cmds.pluginInfo(mayaUsdPluginName, query=True, loaded=True):
            cmds.loadPlugin(mayaUsdPluginName)

        cmds.file(f=True, new=True)

        sphere_xform = cmds.polySphere()[0]

        material_node = cmds.shadingNode("usdPreviewSurface", asShader=True)

        material_sg = cmds.sets(renderable=True, noSurfaceShader=True,
                                empty=True, name=material_node+"SG")
        cmds.connectAttr(material_node+".outColor",
                         material_sg+".surfaceShader", force=True)
        cmds.sets(sphere_xform, e=True, forceElement=material_sg)

        cmds.setAttr(material_node + ".roughness", 0.25)
        cmds.setAttr(material_node + ".specularColor", 0.125, 0.25, 0.75,
                     type="double3")
        cmds.setAttr(material_node + ".useSpecularWorkflow", True)

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

        txfile = os.path.join("UsdExportImportRoundtripPreviewSurface",
                              "Brazilian_rosewood_pxr128.png")
        cmds.setAttr(file_node+".fileTextureName", txfile, type="string")
        cmds.setAttr(file_node + ".defaultColor", 0.5, 0.25, 0.125,
                     type="double3")

        default_ext_setting = cmds.file(q=True, defaultExtensions=True)
        cmds.file(defaultExtensions=False)
        cmds.setAttr(uv_node+".wrapU", 0)

        # Export to USD:
        usd_path = os.path.abspath('UsdPreviewSurfaceRoundtripTest.usda')

        cmds.file(usd_path, force=True,
                  options="shadingMode=useRegistry;mergeTransformAndShape=1",
                  typ="USD Export", pr=True, ea=True)

        cmds.file(defaultExtensions=default_ext_setting)

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
            isMember="USD_Materials:usdPreviewSurface1SG"))

        # Check connections:
        self.assertEqual(
            cmds.connectionInfo("usdPreviewSurface2.outColor", dfs=True),
            ["USD_Materials:usdPreviewSurface1SG.surfaceShader"])
        self.assertEqual(
            cmds.connectionInfo("usdPreviewSurface2.diffuseColor", sfd=True),
            "file2.outColor")
        self.assertEqual(
            cmds.connectionInfo("file2.wrapU", sfd=True),
            "place2dTexture.wrapU")

        # Check values:
        self.assertAlmostEqual(cmds.getAttr("usdPreviewSurface2.roughness"),
                               0.25)
        self.assertEqual(cmds.getAttr("usdPreviewSurface2.specularColor"),
                         [(0.125, 0.25, 0.75)])
        self.assertTrue(cmds.getAttr("usdPreviewSurface2.useSpecularWorkflow"))
        self.assertEqual(cmds.getAttr("file2.defaultColor"),
                         [(0.5, 0.25, 0.125)])
        original_path = cmds.getAttr(file_node+".fileTextureName")
        imported_path = cmds.getAttr("file2.fileTextureName")
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
            "/pSphere1/Looks/usdPreviewSurface1SG/file1")
        rel_texture_path = texture_prim.GetAttribute('inputs:file').Get().path

        usd_dir = os.path.dirname(usd_path)
        full_texture_path = os.path.join(usd_dir, rel_texture_path)
        self.assertTrue(os.path.isfile(full_texture_path))


if __name__ == '__main__':
    unittest.main(verbosity=2)
