#!/usr/bin/env mayapy
#
# Copyright 2026 Autodesk
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

from maya import cmds
from maya import standalone
from pxr import Sdf

import fixturesUtils

import os
import unittest


class testUtilsRelativePaths(unittest.TestCase):
    """
    Tests making file paths relative, either immediately or postponed until
    the layer containing them gets a file name.
    """

    _optionVarName = 'testUtilsRelativePaths_MakePathRelative'

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.optionVar(intValue=(self._optionVarName, 1))

    def tearDown(self):
        cmds.optionVar(remove=self._optionVarName)

    def _path(self, *parts):
        # The current directory is the test output directory.
        return os.path.join(os.getcwd(), *parts).replace('\\', '/')

    def _giveFileName(self, layer, fileName):
        # Setting the identifier gives the anonymous layer a real path
        # while keeping the same layer, as a save would.
        layer.identifier = self._path(fileName)

    def testRelativeAnchorIsRejected(self):
        # An existing directory, so path resolution can turn the relative anchor
        # into an absolute path under the current directory.
        os.makedirs(self._path('anchor'), exist_ok=True)
        filePath = self._path('layer.usda')
        self.assertEqual(mayaUsdLib.Util.getPathRelativeToDirectory(filePath, 'anchor'), filePath)

    def testNonNormalizedAttributePathBecomesRelative(self):
        layer = Sdf.Layer.CreateAnonymous()
        prim = Sdf.PrimSpec(layer, 'Root', Sdf.SpecifierDef)
        attr = Sdf.AttributeSpec(prim, 'file', Sdf.ValueTypeNames.Asset)
        nonNormalized = self._path('sub', '..', 'texture.png')
        attr.default = Sdf.AssetPath(nonNormalized)

        mayaUsdLib.Util.handleAssetPathThatMaybeRelativeToLayer(
            nonNormalized, 'file', layer, self._optionVarName)
        self._giveFileName(layer, 'attributeRoot.usda')
        mayaUsdLib.Util.updatePostponedRelativePaths(layer)

        self.assertEqual(attr.default.path, 'texture.png')

    def testPostponedPathsSurviveUpdateWithoutFileName(self):
        childPath = self._path('child.usda')
        child = Sdf.Layer.CreateNew(childPath)
        child.Save()
        layer = Sdf.Layer.CreateAnonymous()
        layer.subLayerPaths.append(childPath)

        mayaUsdLib.Util.handleAssetPathThatMaybeRelativeToLayer(
            childPath, 'unused', layer, self._optionVarName)
        mayaUsdLib.Util.updatePostponedRelativePaths(layer)
        self.assertEqual(list(layer.subLayerPaths), [childPath])

        self._giveFileName(layer, 'subLayerRoot.usda')
        mayaUsdLib.Util.updatePostponedRelativePaths(layer)
        self.assertEqual(list(layer.subLayerPaths), ['child.usda'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
