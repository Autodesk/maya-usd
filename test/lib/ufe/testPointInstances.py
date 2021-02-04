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

import fixturesUtils
import mayaUtils
import usdUtils

from mayaUsd import ufe as mayaUsdUfe

from pxr import UsdGeom

from maya import cmds
from maya import standalone

import ufe

import unittest


class PointInstancesTestCase(unittest.TestCase):
    '''
    Tests that the UFE path and scene item interfaces work as expected when
    working with USD point instances.
    '''

    _pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls._pluginsLoaded:
            cls._pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    def setUp(self):
        self.assertTrue(self._pluginsLoaded)

        mayaUtils.openPointInstancesGrid14Scene()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testPointInstances(self):
        # Create a UFE path to a PointInstancer prim with an instanceIndex on
        # the end. This path uniquely identifies a specific point instance.
        ufePath = ufe.Path([
            mayaUtils.createUfePathSegment('|UsdProxy|UsdProxyShape'),
            usdUtils.createUfePathSegment('/PointInstancerGrid/PointInstancer/7')])
        ufeItem = ufe.Hierarchy.createItem(ufePath)

        # Verify that we turned the path into a scene item correctly and that
        # we can access the PointInstancer prim.
        self.assertEqual(ufeItem.nodeType(), 'PointInstancer')

        ufeAttrs = ufe.Attributes.attributes(ufeItem)
        self.assertTrue(ufeAttrs.hasAttribute(UsdGeom.Tokens.positions))

        # XXX: Note that UsdSceneItem currently has no wrapping to Python, so
        # we are not able to verify that we can query the scene item directly
        # for its instance index. We can exercise the Utils functions though.

        ufePathString = ufe.PathString.string(ufePath)

        prim = mayaUsdUfe.ufePathToPrim(ufePathString)
        self.assertTrue(prim)
        self.assertTrue(prim.IsA(UsdGeom.PointInstancer))

        instanceIndex = mayaUsdUfe.ufePathToInstanceIndex(ufePathString)
        self.assertEqual(instanceIndex, 7)

        # Tests for the usdPathToUfePathSegment() utility function.
        ufePathSegmentString = mayaUsdUfe.usdPathToUfePathSegment(prim.GetPrimPath())
        self.assertEqual(ufePathSegmentString, '/PointInstancerGrid/PointInstancer')

        ufePathSegmentString = mayaUsdUfe.usdPathToUfePathSegment(prim.GetPrimPath(), 7)
        self.assertEqual(ufePathSegmentString, '/PointInstancerGrid/PointInstancer/7')

        ufePrimPathString = mayaUsdUfe.stripInstanceIndexFromUfePath(ufePathString)
        self.assertEqual(
            ufePrimPathString,
            '|UsdProxy|UsdProxyShape,/PointInstancerGrid/PointInstancer')

        # Create a path to the PointInstancer itself and verify that the
        # instanceIndex is ALL_INSTANCES.
        # XXX: Ideally we would use a named constant provided by
        # UsdImagingDelegate for ALL_INSTANCES, but currently that library does
        # not have any Python wrapping. We *could* get it indirectly as
        # UsdImagingGL.ALL_INSTANCES, but that would bring unnecessary baggage
        # with it, so we just re-define it here.
        ALL_INSTANCES = -1

        ufePath = ufe.Path([
            mayaUtils.createUfePathSegment('|UsdProxy|UsdProxyShape'),
            usdUtils.createUfePathSegment('/PointInstancerGrid/PointInstancer')])
        ufePathString = ufe.PathString.string(ufePath)

        instanceIndex = mayaUsdUfe.ufePathToInstanceIndex(ufePathString)
        self.assertEqual(instanceIndex, ALL_INSTANCES)

        # Create a path to a non-PointInstancer prim but append an instance
        # index to it. This isn't really a valid path, but the utility function
        # should recognize this case and return an instanceIndex of
        # ALL_INSTANCES.
        ufePath = ufe.Path([
            mayaUtils.createUfePathSegment('|UsdProxy|UsdProxyShape'),
            usdUtils.createUfePathSegment('/PointInstancerGrid/7')])
        ufePathString = ufe.PathString.string(ufePath)

        instanceIndex = mayaUsdUfe.ufePathToInstanceIndex(ufePathString)
        self.assertEqual(instanceIndex, ALL_INSTANCES)

    def testPickWalking(self):
        '''
        Tests pick-walking from a point instance up to the PointInstancer that
        generated it.
        '''
        ufePath = ufe.Path([
            mayaUtils.createUfePathSegment('|UsdProxy|UsdProxyShape'),
            usdUtils.createUfePathSegment('/PointInstancerGrid/PointInstancer/7')])
        ufeItem = ufe.Hierarchy.createItem(ufePath)

        globalSelection = ufe.GlobalSelection.get()
        globalSelection.append(ufeItem)

        # Pick-walk up from the point instance.
        cmds.pickWalk(direction='up')

        # The selection should have been replaced with the PointInstancer
        ufeItem = globalSelection.front()
        ufePathString = ufe.PathString.string(ufeItem.path())
        self.assertEqual(
            ufePathString,
            '|UsdProxy|UsdProxyShape,/PointInstancerGrid/PointInstancer')


if __name__ == '__main__':
    unittest.main(verbosity=2)
