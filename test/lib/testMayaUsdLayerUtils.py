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

import unittest

from maya import cmds
import mayaUsd_layerUtils

from pxr import Sdf, Usd


class MayaUsdLayerUtilsTestCase(unittest.TestCase):
    """Test the Python helpers used by the layer editor menus."""

    @classmethod
    def setUpClass(cls):
        cmds.loadPlugin('mayaUsdPlugin')

    def testSessionLayerIdsWithoutStage(self):
        self.assertEqual(set(), mayaUsd_layerUtils._getAllSessionLayerIds(None))

    def testSessionLayerIdsNestedHierarchy(self):
        """All the sub-layers under the session layer are session layers, not the root's."""
        stage = Usd.Stage.CreateInMemory()
        session = stage.GetSessionLayer()
        child = Sdf.Layer.CreateAnonymous('child')
        grandChild = Sdf.Layer.CreateAnonymous('grandChild')
        sibling = Sdf.Layer.CreateAnonymous('sibling')
        rootSubLayer = Sdf.Layer.CreateAnonymous('rootSubLayer')
        child.subLayerPaths.append(grandChild.identifier)
        session.subLayerPaths.append(child.identifier)
        session.subLayerPaths.append(sibling.identifier)
        stage.GetRootLayer().subLayerPaths.append(rootSubLayer.identifier)

        sessionLayerIds = mayaUsd_layerUtils._getAllSessionLayerIds(stage)

        self.assertEqual(
            {session.identifier, child.identifier, grandChild.identifier, sibling.identifier},
            sessionLayerIds)

    @unittest.skipUnless(hasattr(Sdf, 'VariableExpression'), 'Requires USD sub-layer expressions')
    def testSessionLayerIdsIncludeSubLayerExpression(self):
        """The active render layer is a session sub-layer set through an expression."""
        stage = Usd.Stage.CreateInMemory()
        session = stage.GetSessionLayer()
        renderLayer = Sdf.Layer.CreateAnonymous('RenderLayer_1')
        session.expressionVariables = {'adskActiveRenderLayer': renderLayer.identifier}
        session.subLayerPaths.insert(0, '`${adskActiveRenderLayer}`')
        self.assertTrue(stage.HasLocalLayer(renderLayer))

        sessionLayerIds = mayaUsd_layerUtils._getAllSessionLayerIds(stage)

        self.assertEqual({session.identifier, renderLayer.identifier}, sessionLayerIds)

    @unittest.skipUnless(hasattr(Sdf, 'VariableExpression'), 'Requires USD sub-layer expressions')
    def testSessionLayerIdsSkipUnresolvedSubLayers(self):
        """Sub-layers that cannot be resolved are skipped."""
        stage = Usd.Stage.CreateInMemory()
        session = stage.GetSessionLayer()
        session.subLayerPaths.append('`${undefinedVariable}`')
        session.subLayerPaths.append('no_such_file.usd')

        sessionLayerIds = mayaUsd_layerUtils._getAllSessionLayerIds(stage)

        self.assertEqual({session.identifier}, sessionLayerIds)


if __name__ == '__main__':
    unittest.main(verbosity=2)
