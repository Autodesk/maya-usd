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

from pxr import Usd

from maya import cmds
from maya import standalone

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

    def testExportSelectedStage(self):
        '''
        Test that we can export the selected stages and find it as a USD reference.
        '''
        self._prepareScene()
        self._export(exportStagesAsRef=True, selection=['stage1'])
        self._verifyReference('/stage1/Top')


if __name__ == '__main__':
    unittest.main(verbosity=2)
