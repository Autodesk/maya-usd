#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

from pxr import Usd
from pxr import UsdShade

from maya import cmds
from maya import standalone

import fixturesUtils

class testUsdExportShadingInstanced(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        # Stage with simple (non-nested) instancing.

        mayaFile = os.path.join(inputPath, "UsdExportShadingInstancedTest", "InstancedShading.ma")
        cmds.file(mayaFile, force=True, open=True)

        usdFilePath = os.path.abspath('InstancedShading.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFilePath,
                shadingMode='useRegistry', exportInstances=True,
                materialsScopeName='Materials',
                exportCollectionBasedBindings=True,
                exportMaterialCollections=True,
                materialCollectionsPath="/World")

        cls._simpleStage = Usd.Stage.Open(usdFilePath)

        # Stage with nested instancing.
        mayaFile = os.path.join(inputPath, "UsdExportShadingInstancedTest", "NestedInstancedShading.ma")
        cmds.file(mayaFile, force=True, open=True)

        usdFilePath = os.path.abspath('NestedInstancedShading.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFilePath,
                shadingMode='useRegistry', exportInstances=True,
                materialsScopeName='Materials',
                exportCollectionBasedBindings=True,
                exportMaterialCollections=True,
                materialCollectionsPath="/World")

        cls._nestedStage = Usd.Stage.Open(usdFilePath)


    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testInstancedGeom(self):
        """Tests that different shader bindings are correctly authored on
        instanced geometry."""
        worldPath = "/World" # Where collections are authored
        redMat = "/World/Materials/blinn1SG"
        redPaths = ["/World/redCube", "/World/redSphere"]
        blueMat = "/World/Materials/phong1SG"
        bluePaths = [
                "/World/blueCube", "/World/blueSphere", "/World/blueSphere2"]
        instanceMasters = [
                "/InstanceSources/World_redSphere_blueSphereMultiAssignShape",
                "/InstanceSources/World_blueCube_blueCubeShape"]

        for path in redPaths:
            prim = self._simpleStage.GetPrimAtPath(path)
            self.assertTrue(prim.IsInstance())
            bindingAPI = UsdShade.MaterialBindingAPI(prim)
            mat, rel = bindingAPI.ComputeBoundMaterial()
            self.assertEqual(mat.GetPath(), redMat)
            self.assertEqual(rel.GetPrim().GetPath(), worldPath)

        for path in bluePaths:
            prim = self._simpleStage.GetPrimAtPath(path)
            self.assertTrue(prim.IsInstance())
            bindingAPI = UsdShade.MaterialBindingAPI(prim)
            mat, rel = bindingAPI.ComputeBoundMaterial()
            self.assertEqual(mat.GetPath(), blueMat)
            self.assertEqual(rel.GetPrim().GetPath(), worldPath)

        for path in instanceMasters:
            prim = self._simpleStage.GetPrimAtPath(path)
            self.assertTrue(prim)
            self.assertFalse(
                    prim.HasRelationship(UsdShade.Tokens.materialBinding))

    def testInstancedGeom_Subsets(self):
        """Tests that instanced geom with materials assigned to subsets are
        automatically de-instanced."""
        multiAssignPrim = self._simpleStage.GetPrimAtPath(
                "/World/blueSphereMultiAssign")
        self.assertFalse(multiAssignPrim.IsInstanceable())

        shape = multiAssignPrim.GetChild("blueSphereMultiAssignShape")
        self.assertFalse(shape.IsInstance())

        subset1 = shape.GetChild("initialShadingGroup")
        self.assertTrue(subset1)
        mat, _ = UsdShade.MaterialBindingAPI(subset1).ComputeBoundMaterial()
        self.assertEqual(mat.GetPath(), "/World/Materials/initialShadingGroup")

        subset2 = shape.GetChild("blinn1SG")
        self.assertTrue(subset2)
        mat, _ = UsdShade.MaterialBindingAPI(subset2).ComputeBoundMaterial()
        self.assertEqual(mat.GetPath(), "/World/Materials/blinn1SG")

    def testUninstancedGeom(self):
        """Tests a basic case of non-instanced geometry with bindings."""
        worldPath = "/World" # Where collections are authored
        redMat = self._simpleStage.GetPrimAtPath("/World/Materials/blinn1SG")
        uninstancedPrim = self._simpleStage.GetPrimAtPath("/World/notInstanced")

        self.assertFalse(uninstancedPrim.IsInstance())
        bindingAPI = UsdShade.MaterialBindingAPI(uninstancedPrim)
        mat, rel = bindingAPI.ComputeBoundMaterial()
        self.assertEqual(mat.GetPrim(), redMat)
        self.assertEqual(rel.GetPrim().GetPath(), worldPath)

    def testNestedInstancedGeom(self):
        """Tests that different shader bindings are correctly authored on
        instanced geometry within nested instances."""
        worldPath = "/World" # Where collections are authored
        greenMat = "/World/Materials/blinn1SG"
        greenPaths = [
                "/World/SimpleInstance1/SimpleInstanceShape1",
                "/World/ComplexA/NestedA/Base1/BaseShape1",
                "/World/ComplexA/NestedB/Base1/BaseShape1",
                "/World/Extra/Base3/BaseShape1",
                "/World/ComplexB/NestedA/Base1/BaseShape1",
                "/World/ComplexB/NestedB/Base1/BaseShape1"]
        blueMat = "/World/Materials/blinn2SG"
        bluePaths = [
                "/World/SimpleInstance2/SimpleInstanceShape1",
                "/World/ComplexA/NestedA/Base2/BaseShape1",
                "/World/ComplexA/NestedB/Base2/BaseShape1",
                "/World/ComplexB/NestedA/Base2/BaseShape1",
                "/World/ComplexB/NestedB/Base2/BaseShape1"]
        instanceMasters = [
                "/InstanceSources/World_ComplexA_NestedA_Base1_BaseShape1" +
                    "/BaseShape1",
                "/InstanceSources/World_SimpleInstance1_SimpleInstanceShape1" +
                    "/SimpleInstanceShape1"]

        for path in greenPaths:
            prim = self._nestedStage.GetPrimAtPath(path)
            self.assertTrue(prim, msg=path)
            self.assertTrue(prim.IsInstanceProxy())
            bindingAPI = UsdShade.MaterialBindingAPI(prim)
            mat, rel = bindingAPI.ComputeBoundMaterial()
            self.assertEqual(mat.GetPath(), greenMat)
            self.assertEqual(rel.GetPrim().GetPath(), worldPath)

        for path in bluePaths:
            prim = self._nestedStage.GetPrimAtPath(path)
            self.assertTrue(prim, msg=path)
            self.assertTrue(prim.IsInstanceProxy())
            bindingAPI = UsdShade.MaterialBindingAPI(prim)
            mat, rel = bindingAPI.ComputeBoundMaterial()
            self.assertEqual(mat.GetPath(), blueMat)
            self.assertEqual(rel.GetPrim().GetPath(), worldPath)

        for path in instanceMasters:
            prim = self._nestedStage.GetPrimAtPath(path)
            self.assertTrue(prim)
            self.assertFalse(
                    prim.HasRelationship(UsdShade.Tokens.materialBinding))


if __name__ == '__main__':
    unittest.main(verbosity=2)
