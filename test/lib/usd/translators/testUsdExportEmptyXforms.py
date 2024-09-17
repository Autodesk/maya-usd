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

import os
import unittest

from pxr import Usd

from maya import cmds
from maya import standalone
import mayaUsd.lib

import fixturesUtils

class AddPayloadOrRefChaser(mayaUsd.lib.ExportChaser):
    '''
    Export chaser used to add a payload or reference to a prim so that
    we can test that a prim with a payload or ref is not considered empty.

    For this class to be found, the PXR_OVERRIDE_PLUGINPATH_NAME env var
    must point to the location of the `plugInfo.json` file specific to
    this chaser, which is in `ExportEmptyConfig` and setup by Cmake in
    `test/lib/usd/plugin/plugInfoExportEmptyConfig.json` by the cmake
    file in that folder.
    '''

    addPayloadOn = None
    addRefOn = None

    def __init__(self, factoryContext, *args, **kwargs):
        super(AddPayloadOrRefChaser, self).__init__(factoryContext, *args, **kwargs)
        self.stage = factoryContext.GetStage()

    def PostExport(self):
        '''
        Called by the MayaUSD USD export process at the end of the export.
        '''
        self._addPayload(self.stage, self.addPayloadOn)
        self._addRef(self.stage, self.addRefOn)

        # Clear stage so that we don't accidentally retain the USD stage.
        self.stage = None

        # We must return true to keep the export going.
        return True

    @staticmethod
    def _addPayload(stage, path):
        '''
        Add a payload to a prim on the stage.
        '''
        if not stage:
            return
        
        if not path:
            return
        
        prim = stage.GetPrimAtPath(path)
        if not prim:
            return
        
        payloadFile = '/dummy/file/does/not/matter.usd'
        prim.GetPayloads().AddPayload(path)

    @staticmethod
    def _addRef(stage, path):
        '''
        Add a USD reference to a prim on the stage.
        '''
        if not stage:
            return
        
        if not path:
            return
        
        prim = stage.GetPrimAtPath(path)
        if not prim:
            return
        
        refFile = '/dummy/file/does/not/matter.usd'
        prim.GetReferences().AddReference(refFile)


class testUsdExportEmptyXforms(unittest.TestCase):

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

    def _setupTest(self):
        AddPayloadOrRefChaser.addPayloadOn = None
        AddPayloadOrRefChaser.addRefOn = None
        mayaUsd.lib.ExportChaser.Register(AddPayloadOrRefChaser, "AddPayloadOrRefChaser")

    def _createNestedEmpties(self):
        '''
        Create a series of nested empty groups.
            null1
                null2
                    null3
                    null4
        '''
        cmds.file(new=True, force=True)
        cmds.CreateEmptyGroup()
        cmds.CreateEmptyGroup()
        cmds.CreateEmptyGroup()
        cmds.CreateEmptyGroup()
        cmds.parent('null2', 'null1')
        cmds.parent('null3', 'null2')
        cmds.parent('null4', 'null2')

        createdPrims = [
            '/null1',
            '/null1/null2',
            '/null1/null2/null3',
            '/null1/null2/null4'
        ]
        return createdPrims

    def _exportToUSD(self, filename, includeEmpties, defaultPrimPath=None):
        '''
        Export to USD, either including or excluding empty Xforms.
        '''
        cmds.usdExport(file=filename, includeEmptyTransforms=includeEmpties, defaultPrim=defaultPrimPath)

    def _verifyPrims(self, stage, present = [], absent = []):
        '''
        Verify if the Xforms are present or not, as expected.
        '''
        for path in present:
            prim = stage.GetPrimAtPath(path)
            self.assertTrue(bool(prim), 'Expected path %s to be present' % path)

        for path in absent:
            prim = stage.GetPrimAtPath(path)
            self.assertFalse(bool(prim), 'Expected path %s to be absent' % path)
            
    def testExportIncludeEmpty(self):
        self._setupTest()
        includeEmpties = True

        createdPrims = self._createNestedEmpties()

        usdFilePath = os.path.abspath('UsdExportIncludeEmptyTest.usda')
        cmds.select('null1')
        self._exportToUSD(usdFilePath, includeEmpties)
        
        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        self._verifyPrims(stage, present=createdPrims)

    def testExportExcludeEmpty(self):
        self._setupTest()
        includeEmpties = False

        createdPrims = self._createNestedEmpties()

        usdFilePath = os.path.abspath('UsdExportExcludeEmptyTest.usda')
        cmds.select('null1')
        self._exportToUSD(usdFilePath, includeEmpties, defaultPrimPath="None")
        
        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        self._verifyPrims(stage, absent=createdPrims)

    def testExportExcludeEmptyButKeepPrims(self):
        self._setupTest()
        includeEmpties = False

        createdPrims = self._createNestedEmpties()

        cmds.sphere()[0]

        usdFilePath = os.path.abspath('UsdExportExcludeEmptyTest.usda')
        cmds.select('null1')
        self._exportToUSD(usdFilePath, includeEmpties, defaultPrimPath="None")
        
        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        present = ["/nurbsSphere1", "/mtl", "/mtl/initialShadingGroup"]
        self._verifyPrims(stage, absent=createdPrims, present=present)

    def testExportExcludeEmptyButDefaultPrim(self):
        self._setupTest()
        includeEmpties = False

        createdPrims = self._createNestedEmpties()
        defaultPrimPath = '/null1/null2/null3'

        usdFilePath = os.path.abspath('UsdExportExcludeEmptyTest.usda')
        cmds.select('null1')
        self._exportToUSD(usdFilePath, includeEmpties, defaultPrimPath=defaultPrimPath)
        
        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        present = createdPrims[:]
        present.remove('/null1/null2/null4')
        absent = ['/null1/null2/null4']
        self._verifyPrims(stage, absent=absent, present=present)

    def testExportExcludeEmptyWithPayload(self):
        self._setupTest()
        includeEmpties = False
        AddPayloadOrRefChaser.addPayloadOn = '/null1/null2/null3'

        createdPrims = self._createNestedEmpties()

        usdFilePath = os.path.abspath('UsdExportExcludeEmptyTest.usda')
        cmds.select('null1')
        self._exportToUSD(usdFilePath, includeEmpties)
        
        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        present = createdPrims[:]
        present.remove('/null1/null2/null4')
        absent = ['/null1/null2/null4']
        self._verifyPrims(stage, present=present, absent=absent)

    def testExportExcludeEmptyWithReference(self):
        self._setupTest()
        includeEmpties = False
        AddPayloadOrRefChaser.addRefOn = '/null1/null2'

        self._createNestedEmpties()

        usdFilePath = os.path.abspath('UsdExportExcludeEmptyTest.usda')
        cmds.select('null1')
        self._exportToUSD(usdFilePath, includeEmpties)
        
        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        present = ['/null1', '/null1/null2']
        absent = ['/null1/null2/null3', '/null1/null2/null4']
        self._verifyPrims(stage, present=present, absent=absent)


if __name__ == '__main__':
    unittest.main(verbosity=2)
