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

import fixturesUtils
import mayaUtils
import testUtils

from pxr import Usd

from maya import cmds
from maya import standalone

import unittest


class testMaterialCommands(unittest.TestCase):
    '''
    Verify the correctness of the data returned by the MaterialCommands. Currently the
    returned values are largely hard-coded, so this test will become more complex in the 
    future once we dynamically query the renderers for their materials.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        ''' Called initially to set up the maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

    def _StartTest(self, testName=None):
        cmds.file(force=True, new=True)
        if (testName):
            self._testName = testName
            testFile = testUtils.getTestScene("material", self._testName + ".usda")
            mayaUtils.createProxyFromFile(testFile)

    def testMayaUsdGetNewMaterials(self):
        """
        Checks that the list of new materials for different renderers matches the expected values.
        Arnold is excluded here as it cannot be assumed to be installed in the testing environment.
        """

        self._StartTest()

        expectedMaterials = ['USD/USD Preview Surface|UsdPreviewSurface',
                             'MaterialX/Standard Surface|ND_standard_surface_surfaceshader',
                             # Not available in earlier versions of USD
                             # 'MaterialX/glTF PBR|ND_gltf_pbr_surfaceshader',
                             'MaterialX/USD Preview Surface|ND_UsdPreviewSurface_surfaceshader'
                             ]

        materials = cmds.mayaUsdGetMaterialsFromRenderers()

        self.assertTrue(set(materials).issuperset(set(expectedMaterials)))


    def testMayaUsdGetExistingMaterials(self):
        """
        Checks that the list of materials found in the stage matches the expected values.
        """
        self._StartTest('multipleMaterials')
        
        expectedMaterials = ['/mtl/UsdPreviewSurface1', '/mtl/UsdPreviewSurface2']
        
        materialsInStage = cmds.mayaUsdGetMaterialsInStage("|stage|stageShape,/cube")
        self.assertEqual(materialsInStage, expectedMaterials)

    def testmayaUsdMaterialBindings(self):
        """
        Tests various material-binding attributes.
        """

        usdVersion = Usd.GetVersion()

        self._StartTest('materialAssignment')

        # Check for material bindings
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/CubeWithMaterial", hasMaterialBinding=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/CubeWithoutMaterial", hasMaterialBinding=True), False)

        # Geometry
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/BasisCurves1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Camera1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Capsule1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Cone1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Cube1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Cylinder1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/GeomSubset1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/HermiteCurves1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Imageable1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Mesh1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/NurbsCurves1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/NurbsPatch1", canAssignMaterialToNodeType=True), True)
        if usdVersion >= (0, 22, 8):
            self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Plane1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/PointInstancer1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Points1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Scope1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Sphere1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Xform1", canAssignMaterialToNodeType=True), True)

        # Volumes
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Field3DAsset1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/FieldAsset1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/FieldBase1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/OpenVDBAsset1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Volume1", canAssignMaterialToNodeType=True), True)

        # Lights
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/CylinderLight1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/DiskLight1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/DistantLight1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/DomeLight1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/GeometryLight1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/LightFilter1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/PluginLight1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/PluginLightFilter1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/PortalLight1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/RectLight1", canAssignMaterialToNodeType=True), True)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/SphereLight1", canAssignMaterialToNodeType=True), True)

        # Skeleton
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/SkelAnimation1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/SkelBindingAPI1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/SkelBlendShape1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/SkelRoot1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Skeleton1", canAssignMaterialToNodeType=True), False)

        # Audio
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/SpatialAudio1", canAssignMaterialToNodeType=True), False)

        # Physics
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/ArticulationRootAPI1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/CollisionAPI1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/CollisionGroup1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/DistanceJoint1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/DriveAPI1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/FilteredPairsAPI1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/FixedJoint1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Joint1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/LimitAPI1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/MassAPI1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/MaterialAPI1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/MeshCollisionAPI1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/PrismaticJoint1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/RevoluteJoint1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/RigidBodyAPI1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Scene1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/SphericalJoint1", canAssignMaterialToNodeType=True), False)

        # Material
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Material1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/MaterialBindingAPI1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/NodeDefAPI1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/NodeGraph1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Shader1", canAssignMaterialToNodeType=True), False)

        # Render Utilities
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/RenderDenoisePass1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/RenderPass1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/RenderProduct1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/RenderSettings1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/RenderSettingsBase1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/RenderVar1", canAssignMaterialToNodeType=True), False)

        # Procedurals
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/GenerativeProcedural1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/UsdHydraGenerativeProceduralAPI1", canAssignMaterialToNodeType=True), False)

        # UI
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Backdrop1", canAssignMaterialToNodeType=True), False)

        # References
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/ALMayaReference1", canAssignMaterialToNodeType=True), False)
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/MayaReference1", canAssignMaterialToNodeType=True), False)

        # Unknown object type
        self.assertEqual(cmds.mayaUsdMaterialBindings("|stage|stageShape,/Foo1", canAssignMaterialToNodeType=True), False)

        # Path to non-existent object
        self.assertRaises(RuntimeError, cmds.mayaUsdMaterialBindings, "|stage|stageShape,/Bar1")


if __name__ == '__main__':
    unittest.main(globals())
