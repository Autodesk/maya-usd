#!/usr/bin/env mayapy
#
# Copyright 2024 Autodesk
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

from pxr import Sdf, Usd, UsdShade

from maya import cmds
from maya import standalone
import mayaUsd.lib

import os
import unittest

import fixturesUtils
import mayaUtils


class testUsdExportStagesAsRefs(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _prepareScene(self):
        mayaFileName  = 'sceneWithStage.ma'
        self._mayaFile = os.path.join(self.inputPath, 'UsdExportStagesAsRefsTest', mayaFileName)
        cmds.file(self._mayaFile, force=True, open=True)

    def _prepareMultiRootScene(self):
        '''Point the scene stage to a USD file containing multiple root prims.'''
        self._prepareScene()
        usdFile = os.path.join(self.inputPath, 'UsdExportStagesAsRefsTest', 'multiRootStage.usda')
        cmds.setAttr('|stage1|stageShape1.filePath', usdFile, type='string')

    def _removeCube1FromSceneStage(self):
        '''Modify the stage contained in the Maya scene to make it dirty.'''
        proxyShapePath = '|stage1|stageShape1'
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
        self.assertTrue(stage)
        primPath = '/Top/Cube1'
        self.assertTrue(stage.GetPrimAtPath(primPath))
        stage.RemovePrim(primPath)
        self.assertFalse(stage.GetPrimAtPath(primPath))

    def _export(self, exportStagesAsRef=True, selection=None, mergeTransform=True):
        '''
        Export the Maya scene while converting USD stages to USD references.
        Then open the exported USD file.
        '''
        if selection:
            cmds.select(selection)

        # Export to USD.
        self._usdFilePath = os.path.abspath('ExportStagesAsRefsTest.usda')
        cmds.mayaUSDExport(
            file=self._usdFilePath,
            mergeTransformAndShape=mergeTransform,
            shadingMode='useRegistry', 
            defaultPrim='stage1',
            selection=bool(selection),
            exportStagesAsRefs=exportStagesAsRef)

        self._stage = Usd.Stage.Open(self._usdFilePath)

    def _verifyReference(self, expectedPath, expectPresent=True):
        '''
        Verify that the given USD reference is present or not.
        '''
        expectation = 'Expected to find' if expectPresent else 'Unexpected'
        prim = self._stage.GetPrimAtPath(expectedPath)
        self.assertEqual(bool(prim), expectPresent,
                            '%s %s when exporting %s to %s' % (expectation, expectedPath, self._mayaFile, self._usdFilePath))
        if not expectPresent:
            return
        self.assertTrue(prim.HasAuthoredReferences())
        cylPrim = self._stage.GetPrimAtPath(expectedPath + '/Cylinder1')
        self.assertTrue(cylPrim)

    def testStageOpens(self):
        '''
        Tests that the export can be done and that the USD stage can be opened successfully.
        '''
        self._prepareScene()
        self._export()
        self.assertTrue(self._stage)

    def testExportStage(self):
        '''
        Test that we can export stages and find it as a USD reference.
        '''
        self._prepareScene()
        self._export(exportStagesAsRef=True)
        self._verifyReference('/stage1/Top')

    def testExportStageDontMergeShape(self):
        '''
        Test that we can export stages without merging transform and shape and find it as a USD reference.
        '''
        self._prepareScene()
        self._export(exportStagesAsRef=True, mergeTransform=False)
        self._verifyReference('/stage1/stageShape1/Top')

    def testExportAnonymousStage(self):
        '''
        Test that anonymous stages don't get exported.
        '''
        self._prepareScene()
        
        # Create the anonymous stage. (A stage with an anonymous root layer.)
        shapeNode, shapeStage = mayaUtils.createProxyAndStage()

        self._export(exportStagesAsRef=True)
        self._verifyReference('/stage1/Top')
        self._verifyReference('/stage', expectPresent=False)

    def testDontExportStage(self):
        '''
        Test that we can turn off export stages and not find it as a USD reference.
        '''
        self._prepareScene()
        self._export(exportStagesAsRef=False)
        self._verifyReference('/stage1', expectPresent=False)

    def testExportNodeUnderStage(self):
        '''
        Test that we can turn off export stages and not find it as a USD reference
        but any Maya node that was added under the stage are kept.
        '''
        self._prepareScene()

        # Add a Maya node under the stage.
        cmds.sphere(name='pSphere1', polygon=1, radius=10)
        cmds.parent('pSphere1', 'stage1')

        self._export(exportStagesAsRef=False)
        self._verifyReference('/stage1/Top', expectPresent=False)
        self.assertTrue(self._stage.GetPrimAtPath('/stage1/pSphere1'))

    def testExportDirtyStage(self):
        '''
        Test that we can export a dirty stage and find it as a USD reference.
        '''
        self._prepareScene()
        self._removeCube1FromSceneStage()
        self._export(exportStagesAsRef=True)
        self._verifyReference('/stage1/Top')
        cubePrim = self._stage.GetPrimAtPath('/stage1/Top/Cube1')
        self.assertFalse(cubePrim)

    def testExportSelectedStage(self):
        '''
        Test that we can export the selected stages and find it as a USD reference.
        '''
        self._prepareScene()
        self._export(exportStagesAsRef=True, selection=['stage1'])
        self._verifyReference('/stage1/Top')

    def _verifyRootPrimReference(self, exportedPath, referencedPrimPath):
        prim = self._stage.GetPrimAtPath(exportedPath)
        self.assertTrue(prim, 'Expected to find %s in %s' % (exportedPath, self._usdFilePath))
        references = prim.GetMetadata('references').GetAddedOrExplicitItems()
        self.assertEqual([ref.primPath for ref in references], [Sdf.Path(referencedPrimPath)])

    def testExportMultipleRootPrims(self):
        '''
        Test that each root prim of the stage is exported as its own USD reference,
        even when the stage has a default prim.
        '''
        self._prepareMultiRootScene()
        self._export(exportStagesAsRef=True)
        self._verifyRootPrimReference('/stage1/Geo', '/Geo')
        self._verifyRootPrimReference('/stage1/Materials', '/Materials')
        self.assertTrue(self._stage.GetPrimAtPath('/stage1/Geo/CrossRootCube'))
        self.assertTrue(self._stage.GetPrimAtPath('/stage1/Materials/SharedMaterial'))

    def testExportMultipleRootPrimsDontMergeShape(self):
        '''
        Test that multiple root prims are exported under the shape prim when
        transform and shape are not merged.
        '''
        self._prepareMultiRootScene()
        self._export(exportStagesAsRef=True, mergeTransform=False)
        self._verifyRootPrimReference('/stage1/stageShape1/Geo', '/Geo')
        self._verifyRootPrimReference('/stage1/stageShape1/Materials', '/Materials')

    def testExportCrossRootRelationships(self):
        '''
        Test that relationships targeting a prim under another root prim still
        resolve after export, and that relationships within a single root prim
        are left to the reference.
        '''
        self._prepareMultiRootScene()
        self._export(exportStagesAsRef=True)

        crossRootPrim = self._stage.GetPrimAtPath('/stage1/Geo/CrossRootCube')
        crossRootBinding = UsdShade.MaterialBindingAPI(crossRootPrim).GetDirectBindingRel()
        self.assertEqual(crossRootBinding.GetTargets(),
                         [Sdf.Path('/stage1/Materials/SharedMaterial')])

        localPrim = self._stage.GetPrimAtPath('/stage1/Geo/LocalCube')
        localBinding = UsdShade.MaterialBindingAPI(localPrim).GetDirectBindingRel()
        self.assertEqual(localBinding.GetTargets(),
                         [Sdf.Path('/stage1/Geo/LocalMaterial')])
        rootLayer = self._stage.GetRootLayer()
        self.assertFalse(rootLayer.GetRelationshipAtPath(
            '/stage1/Geo/LocalCube.material:binding'))

    def testExportCrossRootInternalReference(self):
        '''
        Test that internal references targeting a prim under another root prim
        resolve within the referenced layer, without any override on the exported stage.
        '''
        self._prepareMultiRootScene()
        self._export(exportStagesAsRef=True)

        self.assertTrue(self._stage.GetPrimAtPath('/stage1/Geo/ProtoInstance/ProtoCube'))
        rootLayer = self._stage.GetRootLayer()
        self.assertFalse(rootLayer.GetPrimAtPath('/stage1/Geo/ProtoInstance'))


if __name__ == '__main__':
    unittest.main(verbosity=2)
