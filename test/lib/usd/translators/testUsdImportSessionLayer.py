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
import unittest

from pxr import Usd

from maya import cmds
from maya import standalone

import fixturesUtils

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

        # Open the asset USD file and make sure the default variant is what we
        # expect it to be.
        stage = Usd.Stage.Open(usdFile)
        self.assertTrue(stage)

        modelPrimPath = '/Cubes'
        modelPrim = stage.GetPrimAtPath(modelPrimPath)
        self.assertTrue(modelPrim)

        variantSet = modelPrim.GetVariantSet('modelingVariant')
        variantSelection = variantSet.GetVariantSelection()

        # This is the default variant.
        self.assertEqual(variantSelection, 'OneCube')

        # Now do a usdImport of a different variant in a clean Maya scene.
        cmds.file(new=True, force=True)

        variants = [('modelingVariant', 'ThreeCubes')]
        cmds.usdImport(file=usdFile, primPath=modelPrimPath, variant=variants)

        expectedMayaCubeNodesSet = set([
            '|Cubes|Geom|CubeOne',
            '|Cubes|Geom|CubeTwo',
            '|Cubes|Geom|CubeThree'])
        mayaCubeNodesSet = set(cmds.ls('|Cubes|Geom|Cube*', long=True))
        self.assertEqual(expectedMayaCubeNodesSet, mayaCubeNodesSet)

        # The import should have made the variant selections in a session layer,
        # so make sure the selection in our open USD stage was not changed.
        variantSet = modelPrim.GetVariantSet('modelingVariant')
        variantSelection = variantSet.GetVariantSelection()
        self.assertEqual(variantSelection, 'OneCube')


if __name__ == '__main__':
    unittest.main(verbosity=2)
