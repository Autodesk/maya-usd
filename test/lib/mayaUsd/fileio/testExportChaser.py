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

import mayaUsd.lib as mayaUsdLib

from pxr import Usd

from maya import cmds
from maya import standalone

import fixturesUtils, os

import unittest

class exportChaserTest(mayaUsdLib.ExportChaser):
    ExportDefaultCalled = False
    ExportFrameCalled = False
    PostExportCalled = False
    NotCalled = False
    ChaserNames = set()
    ChaserArgs = {}
    ExportSelected = False
    ShadingMode = None
    ReferenceObjectMode = None
    ExportRelativeTextures = None
    Compatibility = None
    DefaultMeshScheme = None
    DefaultUSDFormat = None
    ExportSkels = None
    ExportSkin = None
    MaterialsScopeName = None
    RenderLayerMode = None
    RootKind = None
    GeomSidedness = None

    def __init__(self, factoryContext, *args, **kwargs):
        super(exportChaserTest, self).__init__(factoryContext, *args, **kwargs)
        exportChaserTest.ChaserNames = factoryContext.GetJobArgs().chaserNames
        exportChaserTest.ChaserArgs = factoryContext.GetJobArgs().allChaserArgs['test']
        exportChaserTest.ExportSelected = factoryContext.GetJobArgs().exportSelected
        exportChaserTest.DefaultPrim = factoryContext.GetJobArgs().defaultPrim
        exportChaserTest.RootPrim = factoryContext.GetJobArgs().rootPrim
        exportChaserTest.RootPrimType = factoryContext.GetJobArgs().rootPrimType
        exportChaserTest.ShadingMode = factoryContext.GetJobArgs().shadingMode
        exportChaserTest.ReferenceObjectMode = factoryContext.GetJobArgs().referenceObjectMode
        exportChaserTest.ExportRelativeTextures = factoryContext.GetJobArgs().exportRelativeTextures
        exportChaserTest.Compatibility = factoryContext.GetJobArgs().compatibility
        exportChaserTest.DefaultMeshScheme = factoryContext.GetJobArgs().defaultMeshScheme
        exportChaserTest.DefaultUSDFormat = factoryContext.GetJobArgs().defaultUSDFormat
        exportChaserTest.ExportSkels = factoryContext.GetJobArgs().exportSkels
        exportChaserTest.ExportSkin = factoryContext.GetJobArgs().exportSkin
        exportChaserTest.MaterialsScopeName = factoryContext.GetJobArgs().materialsScopeName
        exportChaserTest.RenderLayerMode = factoryContext.GetJobArgs().renderLayerMode
        exportChaserTest.RootKind = factoryContext.GetJobArgs().rootKind
        exportChaserTest.GeomSidedness = factoryContext.GetJobArgs().geomSidedness

    def ExportDefault(self):
        exportChaserTest.ExportDefaultCalled = True
        return self.ExportFrame(Usd.TimeCode.Default())

    def ExportFrame(self, frame):
        exportChaserTest.ExportFrameCalled = True
        return True

    def PostExport(self):
        exportChaserTest.PostExportCalled = True
        return True

class testExportChaser(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testSimpleExportChaser(self):
        mayaUsdLib.ExportChaser.Register(exportChaserTest, "test")
        cmds.polySphere(r = 3.5, name='apple')

        usdFilePath = os.path.join(self.temp_dir,'testExportChaser.usda')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            chaser=['test'],
            chaserArgs=[
                ('test', 'foo', 'tball'),
                ('test', 'bar', 'ometer'),
            ],
            shadingMode='none')

        self.assertTrue(exportChaserTest.ExportDefaultCalled)
        self.assertTrue(exportChaserTest.ExportFrameCalled)
        self.assertTrue(exportChaserTest.PostExportCalled)
        self.assertFalse(exportChaserTest.NotCalled)
        self.assertTrue('test' in exportChaserTest.ChaserNames)
        self.assertEqual(exportChaserTest.ChaserArgs,{'bar': 'ometer', 'foo': 'tball'})

        # test exportSelected
        self.assertFalse(exportChaserTest.ExportSelected)
        selection = ['apple']
        cmds.select(selection)

        cmds.usdExport(mergeTransformAndShape=True,  selection=True,
            file=usdFilePath,
            chaser=['test'],
            chaserArgs=[
                ('test', 'foo', 'tball'),
                ('test', 'bar', 'ometer'),
            ],
            shadingMode='none')
        self.assertTrue(exportChaserTest.ExportSelected)

        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            rootPrim = 'testRootPrim',
            rootPrimType = 'xform',
            defaultPrim = 'testRootPrim',
            chaser=['test'],
            chaserArgs=[
                ('test', 'foo', 'tball'),
                ('test', 'bar', 'ometer'),
            ],
            shadingMode='none')
        self.assertEqual(exportChaserTest.DefaultPrim,"testRootPrim")
        self.assertEqual(exportChaserTest.RootPrim, '/testRootPrim')
        self.assertEqual(exportChaserTest.RootPrimType, 'xform')

        # test 'compatibility' argument explicitly, all other arguments
        # remain default
        usdzFilePath = os.path.join(self.temp_dir,'testExportChaser.usdz')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdzFilePath,
            compatibility='appleArKit',  # Require to export as .usdz
            chaser=['test'],
            chaserArgs=[
                ('test', 'foo', 'tball'),
                ('test', 'bar', 'ometer'),
            ])
        self.assertEqual(exportChaserTest.Compatibility, 'appleArKit')
        # Verify all other TfToken arguments can be accessed and are default values
        self.assertEqual(exportChaserTest.ShadingMode, 'useRegistry')
        self.assertEqual(exportChaserTest.ReferenceObjectMode, 'none')
        self.assertEqual(exportChaserTest.ExportRelativeTextures, 'automatic')
        self.assertEqual(exportChaserTest.DefaultMeshScheme, 'catmullClark')
        self.assertEqual(exportChaserTest.DefaultUSDFormat, 'usdc')
        self.assertEqual(exportChaserTest.ExportSkels, 'none')
        self.assertEqual(exportChaserTest.ExportSkin, 'none')
        self.assertEqual(exportChaserTest.MaterialsScopeName, 'Looks')
        self.assertEqual(exportChaserTest.RenderLayerMode, 'defaultLayer')
        self.assertEqual(exportChaserTest.RootKind, '')
        self.assertEqual(exportChaserTest.GeomSidedness, 'derived')


if __name__ == '__main__':
    unittest.main(verbosity=2)
