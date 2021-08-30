#!/usr/bin/env mayapy
#
# Copyright 2021 Autodesk
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

from pxr import Tf

import fixturesUtils

class testUsdExportSchemaApi(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportSchemaApi(self):
        """Testing that a custom Schema API exporter is created and called"""
        mark = Tf.Error.Mark()
        mark.SetMark()
        self.assertTrue(mark.IsClean())

        cmds.polySphere(r=1)
        usdFilePath = os.path.abspath('UsdExportSchemaApiTest.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath, extraContext=["NullAPI",])

        self.assertFalse(mark.IsClean())

        errors = mark.GetErrors()
        messages = set()
        for e in errors:
            messages.add(e.commentary)
        expected = set([
            "Missing implementation for NullAPIChaser::ExportDefault",
            "Missing implementation for TestSchemaExporter::Write",
            "Missing implementation for TestSchemaExporter::PostExport"])
        self.assertEqual(messages, expected)


if __name__ == '__main__':
    unittest.main(verbosity=2)
