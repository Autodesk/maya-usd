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

import os
import unittest

from maya import cmds
from maya import standalone

from pxr import Usd
from pxr import Sdf

import fixturesUtils

class testUsdExportFileFormat(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)
        cmds.polySphere()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def GetUnderlyingUsdFileFormat(self, layer):
        if Sdf.FileFormat.FindById('usda').CanRead(layer.identifier):
            return 'usda'
        elif Sdf.FileFormat.FindById('usdc').CanRead(layer.identifier):
            return 'usdc'
        return ''
        
    def testUsdExportFileFormat_cmd_default(self):
        name = 'default.usd'
        usdFile = os.path.abspath(name)

        args = ()
        kwargs = {
            'file': usdFile,
        }
        cmds.mayaUSDExport(*args, **kwargs)

        layer = Sdf.Layer.FindOrOpen(usdFile)
        format = self.GetUnderlyingUsdFileFormat(layer)
        self.assertEqual(format, 'usdc')

    def testUsdExportFileFormat_cmd_usdc(self):
        name = 'crate.usd'
        usdFile = os.path.abspath(name)

        args = ()
        kwargs = {
            'file': usdFile,
            'defaultUSDFormat': 'usdc'
        }
        cmds.mayaUSDExport(*args, **kwargs)

        layer = Sdf.Layer.FindOrOpen(usdFile)
        format = self.GetUnderlyingUsdFileFormat(layer)
        self.assertEqual(format, 'usdc')

    def testUsdExportFileFormat_cmd_usda(self):
        name = 'ascii.usd'
        usdFile = os.path.abspath(name)

        args = ()
        kwargs = {
            'file': usdFile,
            'defaultUSDFormat': 'usda'
        }
        cmds.mayaUSDExport(*args, **kwargs)

        layer = Sdf.Layer.FindOrOpen(usdFile)
        format = self.GetUnderlyingUsdFileFormat(layer)
        self.assertEqual(format, 'usda')

    def testUsdExportFileFormat_file_cmd_default(self):
        name = 'default.usd'
        usdFile = os.path.abspath(name)

        args = (usdFile,)
        kwargs = {
            'force': True,
            'ea': True,
            'typ': 'USD Export',
            'options': ';defaultUSDFormat=usdc;'
        }
        cmds.file(*args, **kwargs)

        layer = Sdf.Layer.FindOrOpen(usdFile)
        format = self.GetUnderlyingUsdFileFormat(layer)
        self.assertEqual(format, 'usdc')

    def testUsdExportFileFormat_file_cmd_usdc(self):
        name = 'crate.usd'
        usdFile = os.path.abspath(name)

        args = (usdFile,)
        kwargs = {
            'force': True,
            'ea': True,
            'typ': 'USD Export',
            'options': ';defaultUSDFormat=usdc;'
        }
        cmds.file(*args, **kwargs)

        layer = Sdf.Layer.FindOrOpen(usdFile)
        format = self.GetUnderlyingUsdFileFormat(layer)
        self.assertEqual(format, 'usdc')

    def testUsdExportFileFormat_file_cmd_usda(self):
        name = 'ascii.usd'
        usdFile = os.path.abspath(name)

        args = (usdFile,)
        kwargs = {
            'force': True,
            'ea': True,
            'typ': 'USD Export',
            'options': ';defaultUSDFormat=usda;'
        }
        cmds.file(*args, **kwargs)

        layer = Sdf.Layer.FindOrOpen(usdFile)
        format = self.GetUnderlyingUsdFileFormat(layer)
        self.assertEqual(format, 'usda')

if __name__ == '__main__':
    unittest.main(verbosity=2)
