#!/usr/bin/env python

#
# Copyright 2026 Autodesk
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
import usdUfe

from pxr import Usd

from maya import cmds
from maya import standalone

import unittest


class TestMaterialUtils(unittest.TestCase):
    '''Verify UsdUfe MaterialUtils Python bindings.'''

    pluginsLoaded = False
    _stagePathPrefix = '|stage|stageShape,'

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        self.assertTrue(self.pluginsLoaded)

    def _ufePath(self, primPath):
        return self._stagePathPrefix + primPath

    def _startTest(self, testName=None):
        cmds.file(force=True, new=True)
        if testName:
            testFile = testUtils.getTestScene('material', testName + '.usda')
            mayaUtils.createProxyFromFile(testFile)

    def testGetMaterialsFromRenderers(self):
        """Checks creatable surface shader menu entries from renderers."""
        self._startTest()

        expectedMaterials = [
            ('USD', 'USD Preview Surface', 'UsdPreviewSurface'),
            ('MaterialX', 'Standard Surface', 'ND_standard_surface_surfaceshader'),
            ('MaterialX', 'USD Preview Surface', 'ND_UsdPreviewSurface_surfaceshader'),
        ]

        materials = usdUfe.getMaterialsFromRenderers()
        self.assertTrue(set(materials).issuperset(set(expectedMaterials)))

    def testGetMaterialsInStage_multipleMaterials(self):
        self._startTest('multipleMaterials')

        expectedMaterials = ['/mtl/UsdPreviewSurface1', '/mtl/UsdPreviewSurface2']
        materialsInStage = usdUfe.getMaterialsInStage(self._ufePath('/cube'))
        self.assertEqual(materialsInStage, expectedMaterials)

    def testGetMaterialsInStage_singleMaterial(self):
        self._startTest('singleMaterial')

        expectedMaterials = ['/mtl/UsdPreviewSurface1']
        materialsInStage = usdUfe.getMaterialsInStage(self._ufePath('/cube'))
        self.assertEqual(materialsInStage, expectedMaterials)

    def testGetMaterialsInStage_noMaterials(self):
        self._startTest('noMaterial')

        materialsInStage = usdUfe.getMaterialsInStage(self._ufePath('/cube'))
        self.assertEqual(materialsInStage, [])

    def testGetMaterialsInStage_invalidPath(self):
        self._startTest()

        materialsInStage = usdUfe.getMaterialsInStage(self._ufePath('/doesNotExist'))
        self.assertEqual(materialsInStage, [])

    def testCanAssignMaterialToNodeType_nullSceneItem(self):
        self.assertFalse(usdUfe.canAssignMaterialToNodeType(''))

    def testCanAssignMaterialToNodeType_missingSceneItem(self):
        self._startTest('materialAssignment')
        self.assertFalse(usdUfe.canAssignMaterialToNodeType(self._ufePath('/Bar1')))

    def testCanAssignMaterialToNodeType(self):
        """Tests node-type allow/reject lists used by material assignment menus."""
        usdVersion = Usd.GetVersion()
        self._startTest('materialAssignment')

        assignablePrims = [
            '/BasisCurves1',
            '/Capsule1',
            '/Cone1',
            '/Cube1',
            '/Cylinder1',
            '/GeomSubset1',
            '/HermiteCurves1',
            '/Imageable1',
            '/Mesh1',
            '/NurbsCurves1',
            '/NurbsPatch1',
            '/PointInstancer1',
            '/Points1',
            '/Scope1',
            '/Sphere1',
            '/Xform1',
            '/Volume1',
            '/CylinderLight1',
            '/DiskLight1',
            '/DistantLight1',
            '/DomeLight1',
            '/GeometryLight1',
            '/LightFilter1',
            '/PluginLight1',
            '/PluginLightFilter1',
            '/PortalLight1',
            '/RectLight1',
            '/SphereLight1',
        ]

        if usdVersion >= (0, 22, 8):
            assignablePrims.append('/Plane1')

        nonAssignablePrims = [
            '/Camera1',
            '/Field3DAsset1',
            '/FieldAsset1',
            '/FieldBase1',
            '/OpenVDBAsset1',
            '/SkelAnimation1',
            '/SkelBindingAPI1',
            '/SkelBlendShape1',
            '/SkelRoot1',
            '/Skeleton1',
            '/SpatialAudio1',
            '/ArticulationRootAPI1',
            '/CollisionAPI1',
            '/CollisionGroup1',
            '/DistanceJoint1',
            '/DriveAPI1',
            '/FilteredPairsAPI1',
            '/FixedJoint1',
            '/Joint1',
            '/LimitAPI1',
            '/MassAPI1',
            '/MaterialAPI1',
            '/MeshCollisionAPI1',
            '/PrismaticJoint1',
            '/RevoluteJoint1',
            '/RigidBodyAPI1',
            '/Scene1',
            '/SphericalJoint1',
            '/Material1',
            '/MaterialBindingAPI1',
            '/NodeDefAPI1',
            '/NodeGraph1',
            '/Shader1',
            '/RenderDenoisePass1',
            '/RenderPass1',
            '/RenderProduct1',
            '/RenderSettings1',
            '/RenderSettingsBase1',
            '/RenderVar1',
            '/GenerativeProcedural1',
            '/UsdHydraGenerativeProceduralAPI1',
            '/Backdrop1',
            '/ALMayaReference1',
            '/MayaReference1',
            '/Foo1',
        ]

        if usdVersion <= (0, 23, 8):
            nonAssignablePrims.append('/SkelPackedJointAnimation1')

        for primPath in assignablePrims:
            self.assertTrue(
                usdUfe.canAssignMaterialToNodeType(self._ufePath(primPath)),
                primPath)

        for primPath in nonAssignablePrims:
            self.assertFalse(
                usdUfe.canAssignMaterialToNodeType(self._ufePath(primPath)),
                primPath)


if __name__ == '__main__':
    unittest.main(globals())
