#!/usr/bin/env mayapy
#
# Copyright 2016 Pixar
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
import platform
import unittest

from pxr import Usd

from maya import cmds
from maya import standalone

import fixturesUtils
import mayaUsd

class testUsdImportSessionLayer(unittest.TestCase):

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.readOnlySetUpClass(__file__)

    def testUsdImport(self):
        """
        This tests that executing a usdImport with variants specified causes
        those variant selections to be made in a session layer and not affect
        other open stages.
        """
        usdFile = os.path.join(self.inputPath, "UsdImportSessionLayerTest", "Cubes.usda")

        # Note: due to a quirk deep inside USD handling of stage caches and layer identifiers,
        #       the file path on windows must be in lower-case. Otherwise, the stage cache
        #       can fail to find the stage depending onthe exact USD API used to create the
        #       stage. The problem is only with the drive letter sometimes being forced to
        #       lower-case by USD, but as path are case-insensitive on Window, we lower the
        #       while path, just in case.
        if platform.system() == 'Windows':
            usdFile = usdFile.lower()

        # Open the asset USD file and make sure the default variant is what we
        # expect it to be.
        stage = Usd.Stage.Open(usdFile)
        self.assertTrue(stage)

        modelPrimPath = '/Cubes'
        modelPrim = stage.GetPrimAtPath(modelPrimPath)
        self.assertTrue(modelPrim)

        cubesVariantSet = modelPrim.GetVariantSet('modelingVariant')
        cubesVariantSelection = cubesVariantSet.GetVariantSelection()

        cubeOnePrimPath = '/Cubes/Geom/CubeOne'
        cubeOnePrim = stage.GetPrimAtPath(cubeOnePrimPath)
        self.assertTrue(cubeOnePrim)

        cubeOneVariantSet = cubeOnePrim.GetVariantSet('displacement')
        cubeOneVariantSelection = cubeOneVariantSet.GetVariantSelection()

        # This is the default variant.
        self.assertEqual(cubesVariantSelection, 'OneCube')
        self.assertEqual(cubeOneVariantSelection, 'none')

        # Now do a usdImport of a different variant in a clean Maya scene.
        cmds.file(new=True, force=True)

        variants = [('modelingVariant', 'ThreeCubes')]
        primVariants = [('/Cubes/Geom/CubeOne', 'displacement', 'moved')]
        cmds.usdImport(file=usdFile, primPath=modelPrimPath, variant=variants, primVariant=primVariants)

        expectedMayaCubeNodesSet = set([
            '|Cubes|Geom|CubeOne',
            '|Cubes|Geom|CubeTwo',
            '|Cubes|Geom|CubeThree'])
        mayaCubeNodesSet = set(cmds.ls('|Cubes|Geom|Cube*', long=True))
        self.assertEqual(expectedMayaCubeNodesSet, mayaCubeNodesSet)

        cubeOneTranslation = cmds.getAttr('|Cubes|Geom|CubeOne.translate')
        self.assertEqual([(20., 10., 50.)], cubeOneTranslation)

        # The import should have made the variant selections in a session layer,
        # so make sure the selection in our open USD stage was not changed.
        cubesVariantSet = modelPrim.GetVariantSet('modelingVariant')
        cubesVariantSelection = cubesVariantSet.GetVariantSelection()
        self.assertEqual(cubesVariantSelection, 'OneCube')

        cubeOneVariantSet = cubeOnePrim.GetVariantSet('displacement')
        cubeOneVariantSelection = cubeOneVariantSet.GetVariantSelection()
        self.assertEqual(cubeOneVariantSelection, 'none')
        cubeOneTranslation = cubeOnePrim.GetAttribute('xformOp:translate').Get()
        self.assertEqual([-2, 0, 0.5], cubeOneTranslation)

        # Open the asset USD file using the MayaUSD proxy shape so that it uses
        # the MayaUSD stage cache, and make sure the default variant is still
        # what we expect it to be.
        shapeNode = cmds.createNode('mayaUsdProxyShape', skipSelect=True, name='stageShape')
        cmds.setAttr(shapeNode + '.filePath', usdFile, type='string')
        cmds.connectAttr('time1.outTime', shapeNode+'.time')
        psPathStr = cmds.ls(shapeNode, long=True)[0]
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        self.assertTrue(stage)

        modelPrimPath = '/Cubes'
        modelPrim = stage.GetPrimAtPath(modelPrimPath)
        self.assertTrue(modelPrim)

        cubesVariantSet = modelPrim.GetVariantSet('modelingVariant')
        cubesVariantSelection = cubesVariantSet.GetVariantSelection()

        cubeOnePrimPath = '/Cubes/Geom/CubeOne'
        cubeOnePrim = stage.GetPrimAtPath(cubeOnePrimPath)
        self.assertTrue(cubeOnePrim)

        cubeOneVariantSet = cubeOnePrim.GetVariantSet('displacement')
        cubeOneVariantSelection = cubeOneVariantSet.GetVariantSelection()

        # This is the default variant.
        self.assertEqual(cubesVariantSelection, 'OneCube')
        self.assertEqual(cubeOneVariantSelection, 'none')


if __name__ == '__main__':
    unittest.main(verbosity=2)
