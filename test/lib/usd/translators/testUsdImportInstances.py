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

import maya.api.OpenMaya as om
import os
import unittest

from maya import cmds
from maya import standalone

from pxr import Gf

import fixturesUtils

class testUsdImportInstances(unittest.TestCase):
    """Test for importing instances. The test cases used here were designed
    by pmolodo as future improvements on the import/export of instances. We
    expect them to load without errors, but not necessarily to provide perfect
    results at this point in time."""
    @classmethod
    def setUpClass(cls):
        cls._path = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        """Clear the scene"""
        cmds.file(f=True, new=True)

    def sortedPathsTo(self, node):
        """Get a sorted list of paths going to the specified node"""
        try:
            sel = om.MGlobal.getSelectionListByName(node)
        except Exception:
            self.assertTrue(False, 'Could not find node %s' % node)
        self.assertEqual(sel.length(), 1, 'Could not find node %s' % node)
        val = [str(i) for i in om.MDagPath.getAllPathsTo(sel.getDependNode(0))]
        val.sort()
        return tuple(val)

    def testImportSimpleUnoptimized(self):
        """
        Test importing the following non-optimized USD simple instancing
        scenario:

        pCube1 [Xform]
            pCubeShape1 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCubeShape1)
        pCube2 [Xform]
            pCubeShape1 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCubeShape1)
        pCube3 [Xform]
            pCubeShape1 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCubeShape1)
        MayaExportedInstanceSources
            pCube1_pCubeShape1 [Scope]
                pCubeShape1 [Mesh]

        The goal would be for Maya to skip all [Scope] items and end up with:

        pCube1 [transform]
            pCubeShape1 [mesh] (instance "source")
        pCube2 [transform]
            pCubeShape1 [mesh] (instance pCube1|pCubeShape1)
        pCube3 [transform]
            pCubeShape1 [mesh] (instance pCube1|pCubeShape1)

        But that will happen in a second phase. We only make sure this imports
        without coding errors here.
        """
        usd_file = os.path.join(self._path, "UsdImportInstancesTest",
                                "SimpleScenario_unoptimized.usda")

        cmds.mayaUSDImport(file=usd_file,
                           primPath="/")

        # Format is sorted all paths of an instance clique:
        expected_cliques = [
            # pCube1 is not an instance:
            ('pCube1',),
            # Correctly shared via instancing, but with 2 extra transforms:
            ('pCube1|pCubeShape1',),

            ('pCube1|pCubeShape1|pCubeShape1',
             'pCube2|pCubeShape1|pCubeShape1',
             'pCube3|pCubeShape1|pCubeShape1'),
        ]

        for clique in expected_cliques:
            self.assertEqual(clique, self.sortedPathsTo(clique[0]))

    def testImportSimpleOptimized(self):
        """
        Test importing the following optimized USD simple instancing scenario:

        pCube1 [Xform] (instance /MayaExportedInstanceSources/pCube1_instanceParent)
        pCube2 [Xform] (instance /MayaExportedInstanceSources/pCube1_instanceParent)
        pCube3 [Xform] (instance /MayaExportedInstanceSources/pCube1_instanceParent)
        MayaExportedInstanceSources
            pCube1_instanceParent [Xform]
                pCubeShape1 [Mesh]

        The goal would be for Maya to end up with this:

        pCube1 [transform]
            pCubeShape1 [mesh] (instance "source")
        pCube2 [transform]
            pCubeShape1 [mesh] (instance pCube1|pCubeShape1)
        pCube3 [transform]
            pCubeShape1 [mesh] (instance pCube1|pCubeShape1)

        This would require finding out that pCube1_instanceParent was not
        merged with pCubeShape1 and do not require adding an extra unmerging
        transform when reading the Gprim.

        But that will happen in a second phase. We only make sure this imports
        without coding errors here.
        """
        usd_file = os.path.join(self._path, "UsdImportInstancesTest",
                                "SimpleScenario_optimized.usda")

        cmds.mayaUSDImport(file=usd_file,
                           primPath="/")

        # Format is sorted all paths of an instance clique:
        expected_cliques = [
            # pCube1 is not an instance:
            ('pCube1',),
            # Correctly shared via instancing, but with 1 extra transforms:
            ('pCube1|pCubeShape1',
             'pCube2|pCubeShape1',
             'pCube3|pCubeShape1'),
        ]

        for clique in expected_cliques:
            self.assertEqual(clique, self.sortedPathsTo(clique[0]))

    def testImportGeomWithTransform(self):
        """
        Test importing the following optimized USD instancing scenario:

        Top [Xform]
            Cup_1 [Xform] (instance GeomWithTrf.usda)
            Cup_2 [Xform] (instance GeomWithTrf.usda)
            Cup_3 [Xform] (instance GeomWithTrf.usda)

        The referenced GeomWithTrf prototype contains:

        Cup [Xform] (will be overlaid over what is referencing it: Cup_1, Cup2, Cup3)
            Geom [Xform]
                Cup [Mesh] (with transforms on it)

        The goal would be for Maya to end up with this:

        Top [transform]
            Cup_1 [transform]
                Geom [transform]  (instance "source")
                    Cup [transform]
                        CupShape [mesh]
            Cup_2 [transform]
                Geom [transform] (instance Cup_1|Geom)
            Cup_3 [transform]
                Geom [transform] (instance Cup_1|Geom)
        """
        usd_file = os.path.join(self._path, "UsdImportInstancesTest",
                                "UsdImportInstancesTest_GeomWithTrf.usda")

        cmds.mayaUSDImport(file=usd_file,
                           primPath="/")

        # Format is sorted all paths of an instance clique:
        expected_cliques = [
            # Cup_1, Cup_2, Cup_3 are instances:
            ('Cup_1',),
            ('Cup_2',),
            ('Cup_3',),

            # Correctly shared via instancing:
            ('Cup_1|Geom',
             'Cup_2|Geom',
             'Cup_3|Geom'),
             
            ('Cup_1|Geom|Cup',
             'Cup_2|Geom|Cup',
             'Cup_3|Geom|Cup'),
        ]

        for clique in expected_cliques:
            self.assertEqual(clique, self.sortedPathsTo(clique[0]))

        # Verify the position, which are affected by the transform on the geom.
        def _GetMayaTransform(transformName):
            selectionList = om.MSelectionList()
            selectionList.add(transformName)
            mObj = selectionList.getDependNode(0)

            return om.MFnTransform(mObj)

        cup_and_pos = [
            ('Cup_1', [11., 11., 0.]),
            ('Cup_2', [-11., 11., 0.]),
            ('Cup_3', [11., -11., 0.]),
            ('Cup_1|Geom', [0., 0., 0.]),
            ('Cup_2|Geom', [0., 0., 0.]),
            ('Cup_3|Geom', [0., 0., 0.]),
            # Note: these used to be lost before the EMSUSD-986 fix.
            ('Cup_1|Geom|Cup', [-205.55, 128.622, -80.4]),
            ('Cup_2|Geom|Cup', [-205.55, 128.622, -80.4]),
            ('Cup_3|Geom|Cup', [-205.55, 128.622, -80.4]),
        ]

        EPSILON = 1e-3

        for name, expectedTranslation in cup_and_pos:
            mayaTransform = _GetMayaTransform(name)
            transformationMatrix = mayaTransform.transformation()

            actualTranslation = list(
                transformationMatrix.translation(om.MSpace.kTransform))
            
            self.assertTrue(
                Gf.IsClose(expectedTranslation, actualTranslation, EPSILON))

    def testImportExportInstancesTest(self):
        """
        Test importing the USD currently generated by testExportInstances.

        The goal would be for Maya to end up with the same node hierarchy as
        the original scene:

        pCube1 [transform]
            pCubeShape1 [mesh] (instance "source")
            pCube2 [transform] (instance "source")
                pCubeShape2 [mesh] (instance "source")
            pCube3 [transform] (instance "source")
                pCubeShape2 (instance pCube1|pCube2|pCubeShape2)
        pCube4 [transform]
            pCubeShape1 [mesh] (instance pCube1|pCubeShape1)
            pCube2 [transform] (instance pCube1|pCube2)
            pCube3 [transform] (instance pCube1|pCube3)

        There is some work to do to achieve that.
        """
        usd_file = os.path.join(self._path, "UsdImportInstancesTest",
                                "UsdExportInstancesTest_original.usda")

        cmds.mayaUSDImport(file=usd_file,
                           primPath="/")

        # Format is sorted all paths of an instance clique:
        expected_cliques = [
            # Correctly shared via instancing, but with 1 extra transforms:
            ('pCube1|pCubeShape1|pCubeShape1',
             'pCube4|pCubeShape1|pCubeShape1'),

            ('pCube1|pCube2|pCubeShape2|pCubeShape2',
             'pCube1|pCube3|pCubeShape2|pCubeShape2',
             'pCube4|pCube2|pCubeShape2|pCubeShape2',
             'pCube4|pCube3|pCubeShape2|pCubeShape2'),
            # Incorrect: Would expect all pCube2 to be shared:
            ('pCube1|pCube2',),

            ('pCube4|pCube2',),
            # Incorrect: Would expect all pCube3 to be shared:
            ('pCube1|pCube3',),

            ('pCube4|pCube3',),
        ]

        for clique in expected_cliques:
            self.assertEqual(clique, self.sortedPathsTo(clique[0]))

    def testImportExportReworkUnoptimizedTest(self):
        """
        Testing another way testExportInstances could export to USD that would
        be easier to re-read. We would export unoptimized to:

        pCube1 [Xform]
            pCubeShape1 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCubeShape1)
            pCube2 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube2)
            pCube3 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube3)
        pCube4 [Xform]
            pCubeShape1 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCubeShape1)
            pCube2 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube2)
            pCube3 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube3)
        pCube5 [Xform]
            pCubeShape1 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCubeShape1)
            pCube2 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube2)
            pCube3 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube3)
        MayaExportedInstanceSources
            pCube1_pCubeShape1 [Scope]
                pCubeShape1 [Mesh]
            pCube1_pCube2 [Scope]
                pCube2 [Xform]
                    pCubeShape2 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube2_pCubeShape2)
            pCube1_pCube3 [Scope]
                pCube3 [Xform]
                    pCubeShape2 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube2_pCubeShape2)
            pCube1_pCube2_pCubeShape2 [Scope]
                pCubeShape2 [Mesh]

        The goal would be for Maya to end up with the same node hierarchy as
        the original scene by skipping all [Scope] primitives:

        pCube1 [transform]
            pCubeShape1 [mesh] (instance "source")
            pCube2 [transform] (instance "source")
                pCubeShape2 [mesh] (instance "source")
            pCube3 [transform] (instance "source")
                pCubeShape2 (instance pCube1|pCube2|pCubeShape2)
        pCube4 [transform]
            pCubeShape1 [mesh] (instance pCube1|pCubeShape1)
            pCube2 [transform] (instance pCube1|pCube2)
            pCube3 [transform] (instance pCube1|pCube3)
        pCube5 [transform]
            pCubeShape1 [mesh] (instance pCube1|pCubeShape1)
            pCube2 [transform] (instance pCube1|pCube2)
            pCube3 [transform] (instance pCube1|pCube3)

        There is some work to do to achieve that.
        """
        usd_file = os.path.join(self._path, "UsdImportInstancesTest",
                                "UsdExportInstancesTest_unoptimized.usda")

        cmds.mayaUSDImport(file=usd_file,
                           primPath="/")

        # Format is sorted all paths of an instance clique:
        expected_cliques = [
            # Correctly shared via instancing, but with 1 extra transforms:
            ('pCube1|pCubeShape1|pCubeShape1',
             'pCube4|pCubeShape1|pCubeShape1',
             'pCube5|pCubeShape1|pCubeShape1'),
            # Correctly shared, but with 2 extra transforms:
            ('pCube1|pCube2|pCube2|pCubeShape2|pCubeShape2',
             'pCube1|pCube3|pCube3|pCubeShape2|pCubeShape2',
             'pCube4|pCube2|pCube2|pCubeShape2|pCubeShape2',
             'pCube4|pCube3|pCube3|pCubeShape2|pCubeShape2',
             'pCube5|pCube2|pCube2|pCubeShape2|pCubeShape2',
             'pCube5|pCube3|pCube3|pCubeShape2|pCubeShape2'),
            # Incorrect: Would expect all pCube2 to be shared:
            ('pCube1|pCube2',),

            ('pCube4|pCube2',),

            ('pCube5|pCube2',),
            # Incorrect: Would expect all pCube3 to be shared:
            ('pCube1|pCube3',),

            ('pCube4|pCube3',),

            ('pCube5|pCube3',),
            # But at the pCube2|pCube2 level, we get correct sharing:
            ('pCube1|pCube2|pCube2',
             'pCube4|pCube2|pCube2',
             'pCube5|pCube2|pCube2'),
            # Same with pCube3|pCube3:
            ('pCube1|pCube3|pCube3',
             'pCube4|pCube3|pCube3',
             'pCube5|pCube3|pCube3'),
        ]

        for clique in expected_cliques:
            self.assertEqual(clique, self.sortedPathsTo(clique[0]))

    def testImportExportReworkOptimizedTest(self):
        """
        Testing another way testExportInstances could export to USD that would
        be easier to re-read. We would export optimized to:

        pCube1 [Xform] (instance /MayaExportedInstanceSources/pCube1_instanceParent)
        pCube4 [Xform] (instance /MayaExportedInstanceSources/pCube1_instanceParent)
        pCube5 [Xform] (instance /MayaExportedInstanceSources/pCube1_instanceParent)
        MayaExportedInstanceSources
            pCube1_instanceParent [Xform]
                pCubeShape1 [Mesh]
                pCube2 [Xform] (instance /MayaExportedInstanceSources/pCube1_pCube2_instancedParent)
                pCube3 [Xform] (instance /MayaExportedInstanceSources/pCube1_pCube2_instancedParent)
            pCube1_pCube2_instancedParent [Xform]
                pCubeShape2 [Mesh]

        The optimization is detecting that all children of pCube1 are instances
        allowing a complex master to be created.

        The goal would be for Maya to end up with the same node hierarchy as
        the original scene:

        pCube1 [transform]
            pCubeShape1 [mesh] (instance "source")
            pCube2 [transform] (instance "source")
                pCubeShape2 [mesh] (instance "source")
            pCube3 [transform] (instance "source")
                pCubeShape2 (instance pCube1|pCube2|pCubeShape2)
        pCube4 [transform]
            pCubeShape1 [mesh] (instance pCube1|pCubeShape1)
            pCube2 [transform] (instance pCube1|pCube2)
            pCube3 [transform] (instance pCube1|pCube3)
        pCube5 [transform]
            pCubeShape1 [mesh] (instance pCube1|pCubeShape1)
            pCube2 [transform] (instance pCube1|pCube2)
            pCube3 [transform] (instance pCube1|pCube3)

        There is some work to do to achieve that.
        """
        usd_file = os.path.join(self._path, "UsdImportInstancesTest",
                                "UsdExportInstancesTest_optimized.usda")

        cmds.mayaUSDImport(file=usd_file,
                           primPath="/")

        # Format is sorted all paths of an instance clique:
        expected_cliques = [
            # Correctly shared via instancing:
            ('pCube1|pCubeShape1',
             'pCube4|pCubeShape1',
             'pCube5|pCubeShape1'),
            # Correctly shared:
            ('pCube1|pCube2|pCubeShape2',
             'pCube1|pCube3|pCubeShape2',
             'pCube4|pCube2|pCubeShape2',
             'pCube4|pCube3|pCubeShape2',
             'pCube5|pCube2|pCubeShape2',
             'pCube5|pCube3|pCubeShape2'),
            # All pCube2 are shared:
            ('pCube1|pCube2', 'pCube4|pCube2', 'pCube5|pCube2',),
            # All pCube3 are shared:
            ('pCube1|pCube3', 'pCube4|pCube3', 'pCube5|pCube3',)
        ]

        for clique in expected_cliques:
            self.assertEqual(clique, self.sortedPathsTo(clique[0]))

    def testImportComplexUnoptimizedTest(self):
        """
        Testing a complex scenario, where one of the instances has been
        modified:

        Maya scene:

        pCube1 [transform]
            pCubeShape1 [mesh] (instance "source")
            pCube2 [transform] (instance "source")
                pCubeShape2 [mesh] (instance "source")
            pCube3 [transform] (instance "source")
                pCubeShape2 (instance pCube1|pCube2|pCubeShape2)
        pCube4 [transform]
            pCubeShape1 [mesh] (instance pCube1|pCubeShape1)
            pCube2 [transform] (instance pCube1|pCube2)
            pCube3 [transform] (instance pCube1|pCube3)
        pCube5 [transform]
            pCubeShape1 [mesh] (instance pCube1|pCubeShape1)
            pCube2 [transform] (instance pCube1|pCube2)
            pCube3 [transform] (instance pCube1|pCube3)
            someUniqueChildOf_pCube5 [transform]

        We would expect the following unoptimized USD export to reload
        correctly:

        pCube1 [Xform]
            pCubeShape1 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCubeShape1)
            pCube2 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube2)
            pCube3 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube3)
        pCube4 [Xform]
            pCubeShape1 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCubeShape1)
            pCube2 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube2)
            pCube3 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube3)
        pCube5 [Xform]
            pCubeShape1 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCubeShape1)
            pCube2 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube2)
            pCube3 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube3)
            someUniqueChildOf_pCube5 [Mesh]
        MayaExportedInstanceSources
            pCube1_pCubeShape1 [Scope]
                pCubeShape1 [Mesh]
            pCube1_pCube2 [Scope]
                pCube2 [Xform]
                    pCubeShape2 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube2_pCubeShape2)
            pCube1_pCube3 [Scope]
                pCube3 [Xform]
                    pCubeShape2 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube2_pCubeShape2)
            pCube1_pCube2_pCubeShape2 [Scope]
                pCubeShape2 [Mesh]

        There is some work to do to achieve that.
        """
        usd_file = os.path.join(self._path, "UsdImportInstancesTest",
                                "ComplexScenario_unoptimized.usda")

        cmds.mayaUSDImport(file=usd_file,
                           primPath="/")

        # Format is sorted all paths of an instance clique:
        expected_cliques = [
            # Correctly shared via instancing, but with 1 extra transforms:
            ('pCube1|pCubeShape1|pCubeShape1',
             'pCube4|pCubeShape1|pCubeShape1',
             'pCube5|pCubeShape1|pCubeShape1'),
            # Correctly shared, but with 2 extra transforms:
            ('pCube1|pCube2|pCube2|pCubeShape2|pCubeShape2',
             'pCube1|pCube3|pCube3|pCubeShape2|pCubeShape2',
             'pCube4|pCube2|pCube2|pCubeShape2|pCubeShape2',
             'pCube4|pCube3|pCube3|pCubeShape2|pCubeShape2',
             'pCube5|pCube2|pCube2|pCubeShape2|pCubeShape2',
             'pCube5|pCube3|pCube3|pCubeShape2|pCubeShape2'),
            # Incorrect: Would expect all pCube2 to be shared:
            ('pCube1|pCube2',),

            ('pCube4|pCube2',),

            ('pCube5|pCube2',),
            # Incorrect: Would expect all pCube3 to be shared:
            ('pCube1|pCube3',),

            ('pCube4|pCube3',),

            ('pCube5|pCube3',),
            # But at the pCube2|pCube2 level, we get correct sharing:
            ('pCube1|pCube2|pCube2',
             'pCube4|pCube2|pCube2',
             'pCube5|pCube2|pCube2'),
            # Same with pCube3|pCube3:
            ('pCube1|pCube3|pCube3',
             'pCube4|pCube3|pCube3',
             'pCube5|pCube3|pCube3'),
        ]

        for clique in expected_cliques:
            self.assertEqual(clique, self.sortedPathsTo(clique[0]))

    def testImportComplexOptimizedTest(self):
        """
        Testing a complex scenario, where one of the instances has been
        modified:

        Maya scene:

        pCube1 [transform]
            pCubeShape1 [mesh] (instance "source")
            pCube2 [transform] (instance "source")
                pCubeShape2 [mesh] (instance "source")
            pCube3 [transform] (instance "source")
                pCubeShape2 (instance pCube1|pCube2|pCubeShape2)
        pCube4 [transform]
            pCubeShape1 [mesh] (instance pCube1|pCubeShape1)
            pCube2 [transform] (instance pCube1|pCube2)
            pCube3 [transform] (instance pCube1|pCube3)
        pCube5 [transform]
            pCubeShape1 [mesh] (instance pCube1|pCubeShape1)
            pCube2 [transform] (instance pCube1|pCube2)
            pCube3 [transform] (instance pCube1|pCube3)
            someUniqueChildOf_pCube5 [transform]

        We would expect the following unoptimized USD export to reload
        correctly:

        pCube1 [Xform] (instance /MayaExportedInstanceSources/pCube1_instanceParent)
        pCube4 [Xform] (instance /MayaExportedInstanceSources/pCube1_instanceParent)
        pCube5 [Xform]
            pCubeShape1 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCubeShape1)
            pCube2 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube2)
            pCube3 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube3)
            someUniqueChildOf_pCube5 [Xform]
        MayaExportedInstanceSources
            pCube1_pCubeShape1 [Scope]
                pCubeShape1 [Mesh]
            pCube1_pCube2 [Scope]
                pCube2 [Xform] (instance /MayaExportedInstanceSources/pCube1_pCube2_instancedParent)
            pCube1_pCube3 [Scope]
                pCube3 [Xform] (instance /MayaExportedInstanceSources/pCube1_pCube2_instancedParent)
            pCube1_instanceParent [Xform]
                pCubeShape1 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCubeShape1)
                pCube2 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube2)
                pCube3 [Scope] (instance /MayaExportedInstanceSources/pCube1_pCube3)
            pCube1_pCube2_instancedParent [Xform]
                pCubeShape2 [Mesh]

        Here we have created a simplified instance for children of pCube1 and
        pCube4, but kept unoptimized masters for pCube5. The instance
        pCube1_instanceParent is sharing the unoptimized instances.
        """
        usd_file = os.path.join(self._path, "UsdImportInstancesTest",
                                "ComplexScenario_optimized.usda")

        cmds.mayaUSDImport(file=usd_file,
                           primPath="/")

        # Format is sorted all paths of an instance clique:
        expected_cliques = [
            # Correctly shared via instancing, but with 1 extra transforms:
            ('pCube1|pCubeShape1|pCubeShape1',
             'pCube4|pCubeShape1|pCubeShape1',
             'pCube5|pCubeShape1|pCubeShape1'),
            # The modified pCube5 does not share at the pCubeShape1 level:
            ('pCube1|pCubeShape1', 'pCube4|pCubeShape1'),

            ('pCube5|pCubeShape1',),
            # Correctly shared, with 1 extra transform:
            ('pCube1|pCube2|pCube2|pCubeShape2',
             'pCube1|pCube3|pCube3|pCubeShape2',
             'pCube4|pCube2|pCube2|pCubeShape2',
             'pCube4|pCube3|pCube3|pCubeShape2',
             'pCube5|pCube2|pCube2|pCubeShape2',
             'pCube5|pCube3|pCube3|pCubeShape2'),
            # All pCube2|pCube2 and pCube3|pCube3 are shared:
            ('pCube1|pCube2|pCube2', 'pCube4|pCube2|pCube2', 'pCube5|pCube2|pCube2',),

            ('pCube1|pCube3|pCube3', 'pCube4|pCube3|pCube3', 'pCube5|pCube3|pCube3',),
            # But at the pCube2 and pCube3 level the modified pCube5 is unshared:
            ('pCube1|pCube2', 'pCube4|pCube2'),

            ('pCube5|pCube2',),

            ('pCube1|pCube3', 'pCube4|pCube3'),

            ('pCube5|pCube3',),
        ]

        for clique in expected_cliques:
            self.assertEqual(clique, self.sortedPathsTo(clique[0]))

if __name__ == '__main__':
    unittest.main(verbosity=2)
