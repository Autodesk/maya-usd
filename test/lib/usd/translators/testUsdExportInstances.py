#!/usr/bin/env mayapy
#
# Copyright 2017 Pixar
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

from pxr import Kind
from pxr import Usd
from pxr import UsdGeom

import fixturesUtils

class testUsdExportInstances(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        filePath = os.path.join(inputPath, "UsdExportInstancesTest", "UsdExportInstancesTest.ma")
        cmds.file(filePath, force=True, open=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportInstances(self):
        usdFile = os.path.abspath('UsdExportInstances_instances.usda')
        cmds.usdExport(mergeTransformAndShape=True,
                       exportInstances=True,
                       stripNamespaces=True,
                       shadingMode='none',
                       file=usdFile)

        stage = Usd.Stage.Open(usdFile)

        s = UsdGeom.Scope.Get(stage, '/MayaExportedInstanceSources')
        self.assertTrue(s.GetPrim().IsValid())

        expectedMeshPath = [
            '/MayaExportedInstanceSources/pCube1_pCubeShape1/pCubeShape1',
            '/MayaExportedInstanceSources/pCube1_pCube2_pCubeShape2/pCubeShape2'
        ]

        for each in expectedMeshPath:
            m = UsdGeom.Mesh.Get(stage, each)
            p = m.GetPrim()
            self.assertTrue(p.IsValid())            

        expectedXForm = [
            ('/pCube1', None),
            ('/pCube1/pCubeShape1', '/MayaExportedInstanceSources/pCube1_pCubeShape1'),
            ('/pCube1/pCube2', '/MayaExportedInstanceSources/pCube1_pCube2'),
            ('/pCube1/pCube3', '/MayaExportedInstanceSources/pCube1_pCube3'),
            ('/pCube4', None),
            ('/pCube4/pCubeShape1', '/MayaExportedInstanceSources/pCube1_pCubeShape1'),
            ('/pCube4/pCube2', '/MayaExportedInstanceSources/pCube1_pCube2'),
            ('/pCube4/pCube3', '/MayaExportedInstanceSources/pCube1_pCube3'),
            ('/MayaExportedInstanceSources/pCube1_pCube2/pCubeShape2',
                    '/MayaExportedInstanceSources/pCube1_pCube2_pCubeShape2'),
            ('/MayaExportedInstanceSources/pCube1_pCube3/pCubeShape2',
                    '/MayaExportedInstanceSources/pCube1_pCube2_pCubeShape2'),
            # "ns:" should be stripped in usd result:
            ('/group1', None),
            ('/group2', None),
            ('/group1/pCube5', '/MayaExportedInstanceSources/group1_pCube5_pCubeShape5'),
            ('/group2/pCube6', '/MayaExportedInstanceSources/group1_pCube5_pCubeShape5'),
        ]

        layer = stage.GetLayerStack()[1]

        for pp, i in expectedXForm:
            x = UsdGeom.Xform.Get(stage, pp)
            p = x.GetPrim()
            self.assertTrue(p.IsValid())
            if i is not None:
                self.assertTrue(p.IsInstanceable())
                self.assertTrue(p.IsInstance())
                ps = layer.GetPrimAtPath(pp)

                ref = ps.referenceList.GetAddedOrExplicitItems()[0]
                self.assertEqual(ref.assetPath, "")
                self.assertEqual(ref.primPath, i)

        # Test that the MayaExportedInstanceSources prim is last in the layer's root prims.
        rootPrims = list(layer.rootPrims.keys())
        self.assertEqual(rootPrims[-1], "MayaExportedInstanceSources")

    def testExportInstances_WithExportRoots(self):
        cmds.file(new=True, force=True)
        sphere = cmds.sphere(r=10, name="instance")
        lod_group = cmds.group(name="my_lod")
        asset_group = cmds.group(name="my_asset")
        for i in range(5):
            inst = cmds.instance([sphere[0]], name=sphere[0] +  "_" + str(i))
            cmds.move(25 * (i + 1), 0, 0, inst[0])
            
        usdFile = os.path.abspath('UsdExportInstances_instances.usda')
        cmds.mayaUSDExport(
            asset_group, file=usdFile,
            exportDisplayColor=True,
            kind="group", shadingMode="none",
            exportInstances=True, exportCollectionBasedBindings=False,
            mergeTransformAndShape=True,
            defaultPrim=lod_group,
            exportRoots=[lod_group],
        )

        stage = Usd.Stage.Open(usdFile)
        self.assertTrue(stage)

        exportedPaths = [
            '/my_lod/instance',
            '/my_lod/instance_0',
            '/my_lod/instance_1',
            '/my_lod/instance_2',
            '/my_lod/instance_3',
            '/my_lod/instance_4',
            '/MayaExportedInstanceSources',
            '/MayaExportedInstanceSources/my__asset_my__lod_instance_instanceShape',
        ]

        for path in exportedPaths:
            prim = stage.GetPrimAtPath(path)
            self.assertTrue(prim.IsValid(), 'Failed to find path ' + path)

    def testExportInstances_ModelHierarchyValidation(self):
        """Tests that model-hierarchy validation works with instances."""
        usdFile = os.path.abspath('UsdExportInstances_kinds.usda')
        with self.assertRaises(RuntimeError):
            # Should fail because assembly |pCube1 contains gprims.
            cmds.usdExport(mergeTransformAndShape=True, exportInstances=True,
                shadingMode='none', kind='assembly', file=usdFile)

    def testExportInstances_NoKindForInstanceSources(self):
        """Tests that the -kind export flag doesn't affect MayaExportedInstanceSources."""
        usdFile = os.path.abspath('UsdExportInstances_instancesources.usda')
        cmds.usdExport(mergeTransformAndShape=True, exportInstances=True,
            shadingMode='none', kind='component', file=usdFile)

        stage = Usd.Stage.Open(usdFile)

        instanceSources = Usd.ModelAPI.Get(stage, '/MayaExportedInstanceSources')
        self.assertEqual(instanceSources.GetKind(), '')

        pCube1 = Usd.ModelAPI.Get(stage, '/pCube1')
        self.assertEqual(pCube1.GetKind(), Kind.Tokens.component)

if __name__ == '__main__':
    unittest.main(verbosity=2)
