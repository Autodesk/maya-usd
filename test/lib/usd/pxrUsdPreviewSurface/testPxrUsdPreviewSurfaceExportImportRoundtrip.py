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

import os
import unittest

from maya import cmds
from maya import standalone


class testPxrUsdPreviewSurfaceExport(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        test_dir = os.path.join(os.path.abspath('.'),
                                "PxrUsdPreviewSurfaceExportTest")

        cmds.workspace(test_dir, o=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testPxrUsdPreviewSurfaceRoundtrip(self):
        """
        Tests that a pxrUsdPreviewSurface exports and imports correctly.
        """
        mayaUsdPluginName = "mayaUsdPlugin"
        if not cmds.pluginInfo(mayaUsdPluginName, query=True, loaded=True):
            cmds.loadPlugin(mayaUsdPluginName)

        cmds.file(f=True, new=True)

        sphere_xform = cmds.polySphere()[0]

        material_node = cmds.shadingNode("pxrUsdPreviewSurface", asShader=True)

        material_sg = cmds.sets(renderable=True, noSurfaceShader=True,
                                empty=True, name=material_node+"SG")
        cmds.connectAttr(material_node+".outColor",
                         material_sg+".surfaceShader", force=True)
        cmds.sets(sphere_xform, e=True, forceElement=material_sg)

        cmds.setAttr(material_node + ".roughness", 0.25)
        cmds.setAttr(material_node + ".specularColor", 0.125, 0.25, 0.75,
                     type="double3")

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

        txfile = "PxrUsdPreviewSurfaceExportTest/Brazilian_rosewood_pxr128.png"
        cmds.setAttr(file_node+".fileTextureName", txfile, type="string")
        cmds.setAttr(file_node + ".defaultColor", 0.5, 0.25, 0.125,
                     type="double3")

        default_ext_setting = cmds.file(q=True, defaultExtensions=True)
        cmds.file(defaultExtensions=False)
        cmds.setAttr(uv_node+".wrapU", 0)

        # Export to USD:
        usd_path = os.path.abspath('PxrUsdPreviewSurfaceRoundtripTest.usda')

        cmds.file(usd_path, force=True,
                  options="shadingMode=useRegistry;mergeTransformAndShape=1",
                  typ="USD Export", pr=True, ea=True)

        cmds.file(defaultExtensions=default_ext_setting)

        # Import back:
        cmds.file(usd_path, i=True, type="USD Import",
                  ignoreVersion=True, ra=True, mergeNamespacesOnClash=False,
                  namespace="Test", pr=True, importTimeRange="combine",
                  options=";shadingMode=useRegistry;primPath=/")

        # Check the new sphere is in the new shading group:
        self.assertTrue(cmds.sets(
            "pSphere1Shape",
            isMember="USD_Materials:pxrUsdPreviewSurface1SG"))

        # Check connections:
        self.assertEqual(
            cmds.connectionInfo("pxrUsdPreviewSurface2.outColor", dfs=True),
            ["USD_Materials:pxrUsdPreviewSurface1SG.surfaceShader"])
        self.assertEqual(
            cmds.connectionInfo("pxrUsdPreviewSurface2.diffuseColor", sfd=True),
            "file2.outColor")
        self.assertEqual(
            cmds.connectionInfo("file2.wrapU", sfd=True),
            "place2dTexture.wrapU")

        # Check values:
        self.assertAlmostEqual(cmds.getAttr("pxrUsdPreviewSurface2.roughness"),
                               0.25)
        self.assertEqual(cmds.getAttr("pxrUsdPreviewSurface2.specularColor"),
                         [(0.125, 0.25, 0.75)])
        self.assertEqual(cmds.getAttr("file2.defaultColor"),
                         [(0.5, 0.25, 0.125)])
        self.assertTrue(cmds.getAttr("file2.fileTextureName").endswith(txfile))
        self.assertEqual(cmds.getAttr("place2dTexture.wrapU"), 0)
        self.assertEqual(cmds.getAttr("place2dTexture.wrapV"), 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
