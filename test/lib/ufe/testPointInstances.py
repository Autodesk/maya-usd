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

from pxr import Gf
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

    EPSILON = 1e-3

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

    def testManipulatePointInstancePosition(self):
        # Create a UFE path to a PointInstancer prim with an instanceIndex on
        # the end. This path uniquely identifies a specific point instance.
        # We also pick one with a non-zero initial position.
        instanceIndex = 7

        ufePath = ufe.Path([
            mayaUtils.createUfePathSegment('|UsdProxy|UsdProxyShape'),
            usdUtils.createUfePathSegment(
                '/PointInstancerGrid/PointInstancer/%d' % instanceIndex)])
        ufeItem = ufe.Hierarchy.createItem(ufePath)

        # Select the point instance scene item.
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.append(ufeItem)

        # Get the PointInstancer prim for validating the values in USD.
        ufePathString = ufe.PathString.string(ufePath)
        prim = mayaUsdUfe.ufePathToPrim(ufePathString)
        pointInstancer = UsdGeom.PointInstancer(prim)
        self.assertTrue(pointInstancer)

        # The PointInstancer should have 14 authored positions initially.
        positionsAttr = pointInstancer.GetPositionsAttr()
        positions = positionsAttr.Get()
        self.assertEqual(len(positions), 14)

        # Validate the initial position before manipulating
        position = positions[instanceIndex]
        self.assertTrue(
            Gf.IsClose(position, Gf.Vec3f(-4.5, 1.5, 0.0), self.EPSILON))

        # Perfom a translate manipulation via the move command.
        cmds.move(1.0, 2.0, 3.0, objectSpace=True, relative=True)

        # Re-fetch the USD positions and check for the update.
        position = positionsAttr.Get()[instanceIndex]
        self.assertTrue(
            Gf.IsClose(position, Gf.Vec3f(-3.5, 3.5, 3.0), self.EPSILON))

        # Try another move.
        cmds.move(6.0, 5.0, 4.0, objectSpace=True, relative=True)

        # Re-fetch the USD positions and check for the update.
        position = positionsAttr.Get()[instanceIndex]
        self.assertTrue(
            Gf.IsClose(position, Gf.Vec3f(2.5, 8.5, 7.0), self.EPSILON))

        # Now undo, and re-check.
        cmds.undo()
        position = positionsAttr.Get()[instanceIndex]
        self.assertTrue(
            Gf.IsClose(position, Gf.Vec3f(-3.5, 3.5, 3.0), self.EPSILON))

        # And once more.
        cmds.undo()
        position = positionsAttr.Get()[instanceIndex]
        self.assertTrue(
            Gf.IsClose(position, Gf.Vec3f(-4.5, 1.5, 0.0), self.EPSILON))

    def testManipulatePointInstanceOrientation(self):
        # Create a UFE path to a PointInstancer prim with an instanceIndex on
        # the end. This path uniquely identifies a specific point instance.
        instanceIndex = 7

        ufePath = ufe.Path([
            mayaUtils.createUfePathSegment('|UsdProxy|UsdProxyShape'),
            usdUtils.createUfePathSegment(
                '/PointInstancerGrid/PointInstancer/%d' % instanceIndex)])
        ufeItem = ufe.Hierarchy.createItem(ufePath)

        # Select the point instance scene item.
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.append(ufeItem)

        # Get the PointInstancer prim for validating the values in USD.
        ufePathString = ufe.PathString.string(ufePath)
        prim = mayaUsdUfe.ufePathToPrim(ufePathString)
        pointInstancer = UsdGeom.PointInstancer(prim)
        self.assertTrue(pointInstancer)

        # The PointInstancer should not have any authored orientations
        # initially.
        orientationsAttr = pointInstancer.GetOrientationsAttr()
        self.assertFalse(orientationsAttr.HasAuthoredValue())

        # Perfom an orientation manipulation via the rotate command.
        cmds.rotate(15.0, 30.0, 45.0, objectSpace=True, relative=True)

        # The initial rotate should have filled out the orientations attribute.
        orientations = orientationsAttr.Get()
        self.assertEqual(len(orientations), 14)

        # Validate the rotated item.
        orientation = orientations[instanceIndex]
        self.assertTrue(
            Gf.IsClose(orientation.real, 0.897461, self.EPSILON))
        self.assertTrue(
            Gf.IsClose(orientation.imaginary, Gf.Vec3h(0.01828, 0.2854, 0.335205), self.EPSILON))

        # The non-rotated items should all have identity orientations.
        for i in (range(instanceIndex) + range(instanceIndex + 1, 14)):
            orientation = orientations[i]
            self.assertTrue(
                Gf.IsClose(orientation.real, Gf.Quath.GetIdentity().real, self.EPSILON))
            self.assertTrue(
                Gf.IsClose(orientation.imaginary, Gf.Quath.GetIdentity().imaginary, self.EPSILON))

        # Perfom another rotate.
        cmds.rotate(25.0, 50.0, 75.0, objectSpace=True, relative=True)

        # Re-fetch the USD orientation and check for the update.
        orientation = orientationsAttr.Get()[instanceIndex]
        self.assertTrue(
            Gf.IsClose(orientation.real, 0.397949, self.EPSILON))
        self.assertTrue(
            Gf.IsClose(orientation.imaginary, Gf.Vec3h(-0.0886841, 0.57666, 0.708008), self.EPSILON))

        # Now undo, and re-check.
        cmds.undo()
        orientation = orientationsAttr.Get()[instanceIndex]
        self.assertTrue(
            Gf.IsClose(orientation.real, 0.897461, self.EPSILON))
        self.assertTrue(
            Gf.IsClose(orientation.imaginary, Gf.Vec3h(0.01828, 0.2854, 0.335205), self.EPSILON))

        # And once more.
        # Note that with more complete undo support, this would ideally clear
        # the authored orientations attribute, returning it to its true
        # original state. For now, we just validate that the orientation value
        # is back to its default of identity.
        cmds.undo()
        orientation = orientationsAttr.Get()[instanceIndex]
        self.assertTrue(
            Gf.IsClose(orientation.real, Gf.Quath.GetIdentity().real, self.EPSILON))
        self.assertTrue(
            Gf.IsClose(orientation.imaginary, Gf.Quath.GetIdentity().imaginary, self.EPSILON))

    def testManipulatePointInstanceScale(self):
        # Create a UFE path to a PointInstancer prim with an instanceIndex on
        # the end. This path uniquely identifies a specific point instance.
        instanceIndex = 7

        ufePath = ufe.Path([
            mayaUtils.createUfePathSegment('|UsdProxy|UsdProxyShape'),
            usdUtils.createUfePathSegment(
                '/PointInstancerGrid/PointInstancer/%d' % instanceIndex)])
        ufeItem = ufe.Hierarchy.createItem(ufePath)

        # Select the point instance scene item.
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.append(ufeItem)

        # Get the PointInstancer prim for validating the values in USD.
        ufePathString = ufe.PathString.string(ufePath)
        prim = mayaUsdUfe.ufePathToPrim(ufePathString)
        pointInstancer = UsdGeom.PointInstancer(prim)
        self.assertTrue(pointInstancer)

        # The PointInstancer should not have any authored scales initially.
        scalesAttr = pointInstancer.GetScalesAttr()
        self.assertFalse(scalesAttr.HasAuthoredValue())

        # Perfom a scale manipulation via the scale command.
        cmds.scale(1.0, 2.0, 3.0, objectSpace=True, relative=True)

        # The initial scale should have filled out the scales attribute.
        scales = scalesAttr.Get()
        self.assertEqual(len(scales), 14)

        # Validate the scaled item.
        scale = scales[instanceIndex]
        self.assertTrue(
            Gf.IsClose(scale, Gf.Vec3f(1.0, 2.0, 3.0), self.EPSILON))

        # The non-scaled items should all have identity scales.
        for i in (range(instanceIndex) + range(instanceIndex + 1, 14)):
            scale = scales[i]
            self.assertTrue(
                Gf.IsClose(scale, Gf.Vec3f(1.0, 1.0, 1.0), self.EPSILON))

        # Perfom another scale.
        cmds.scale(4.0, 5.0, 6.0, objectSpace=True, relative=True)

        # Re-fetch the USD scale and check for the update.
        scale = scalesAttr.Get()[instanceIndex]
        self.assertTrue(
            Gf.IsClose(scale, Gf.Vec3f(4.0, 10.0, 18.0), self.EPSILON))

        # Now undo, and re-check.
        cmds.undo()
        scale = scalesAttr.Get()[instanceIndex]
        self.assertTrue(
            Gf.IsClose(scale, Gf.Vec3f(1.0, 2.0, 3.0), self.EPSILON))

        # And once more.
        # Note that with more complete undo support, this would ideally clear
        # the authored scales attribute, returning it to its true original
        # state. For now, we just validate that the scale value is back to its
        # default of identity.
        cmds.undo()
        scale = scalesAttr.Get()[instanceIndex]
        self.assertTrue(
            Gf.IsClose(scale, Gf.Vec3f(1.0, 1.0, 1.0), self.EPSILON))


if __name__ == '__main__':
    unittest.main(verbosity=2)
