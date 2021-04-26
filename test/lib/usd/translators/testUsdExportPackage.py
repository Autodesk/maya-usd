#!/usr/bin/env mayapy
#
# Copyright 2020 Apple Inc. All rights reserved.
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

import fixturesUtils
from maya import cmds
from maya import standalone
from pxr import Usd
from pxr import UsdShade
import zipfile



class testUsdExportPackage(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.temp_dir = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()


    def testExportSelfContainedPackage(self):
        """
        Tests that the exported usdz file is self contained, such that it's valid for AR QuickLook on iOS
        """

        maya_file = os.path.join(self.temp_dir, "UsdExportPackage", "asset.ma")
        cmds.file(maya_file, force=True, open=True)

        # Write the file out
        path = os.path.join(self.temp_dir, 'testExportSelfContainedPackage.usdz')
        cmds.mayaUSDExport(f=path, compatibility='appleArKit')

        # Check with USD what the path to the texture is
        stage = Usd.Stage.Open(path)
        prim = stage.GetPrimAtPath("/AssetGroup/Looks/AssetMatSG/file1")
        shader = UsdShade.Shader(prim)
        tex = shader.GetInput("file").Get().path

        # Check to see if the zipfile contains the texture
        with zipfile.ZipFile(path) as zf:
            for zfile in zf.filelist:
                if zfile.filename == tex:
                    break
            else:
                self.fail("Could not find texture inside zip file")


if __name__ == '__main__':
    unittest.main(verbosity=2)
