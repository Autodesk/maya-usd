#!/usr/bin/env python

#
# Copyright 2019 Autodesk
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

import mayaUtils, usdUtils
from testUtils import assertVectorAlmostEqual, assertVectorEqual

import ufe

from pxr import Usd, UsdGeom

import maya.cmds as cmds
import maya.api.OpenMaya as OpenMaya

import unittest
import os

def nameToPlug(nodeName):
    selection = OpenMaya.MSelectionList()
    selection.add(nodeName)
    return selection.getPlug(0)

class TestObserver(ufe.Observer):
    def __init__(self):
        super(TestObserver, self).__init__()
        self.changed = 0

    def __call__(self, notification):
        if isinstance(notification, ufe.VisibilityChanged):
            self.changed += 1

    def notifications(self):
        return self.changed

class Object3dTestCase(unittest.TestCase):
    '''Verify the Object3d UFE interface, for the USD runtime.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    def setUp(self):
        ''' Called initially to set up the Maya test environment '''
        self.assertTrue(self.pluginsLoaded)

    def testBoundingBox(self):
        '''Test the Object3d bounding box interface.'''

        # Create a simple USD scene.  Default sphere radius is 1, so extents
        # are from (-1, -1, -1) to (1, 1, 1).
        usdFilePath = cmds.internalVar(utd=1) + '/testObject3d.usda'
        stage = Usd.Stage.CreateNew(usdFilePath)
        xform = stage.DefinePrim('/parent', 'Xform')
        sphere = stage.DefinePrim('/parent/sphere', 'Sphere')
        extentAttr = sphere.GetAttribute('extent')
        extent = extentAttr.Get()
        
        assertVectorAlmostEqual(self, extent[0], [-1]*3)
        assertVectorAlmostEqual(self, extent[1], [1]*3)

        # Move the sphere.  Its UFE bounding box should not be affected by
        # transformation hierarchy.
        UsdGeom.XformCommonAPI(xform).SetTranslate((7, 8, 9))

        # Save out the file, and bring it back into Maya under a proxy shape.
        stage.GetRootLayer().Save()

        proxyShape = cmds.createNode('mayaUsdProxyShape')
        cmds.setAttr('mayaUsdProxyShape1.filePath', usdFilePath, type='string')

        # MAYA-101766: loading a stage is done by the proxy shape compute.
        # Because we're a script, no redraw is done, so need to pull explicitly
        # on the outStageData (and simply discard the returned data), to get
        # the proxy shape compute to run, and thus the stage to load.  This
        # should be improved.  Without a loaded stage, UFE item creation
        # fails.  PPT, 31-10-2019.
        outStageData = nameToPlug('mayaUsdProxyShape1.outStageData')
        outStageData.asMDataHandle()

        proxyShapeMayaPath = cmds.ls(proxyShape, long=True)[0]
        proxyShapePathSegment = mayaUtils.createUfePathSegment(
            proxyShapeMayaPath)
        
        #######
        # Create a UFE scene item from the sphere prim.
        spherePathSegment = usdUtils.createUfePathSegment('/parent/sphere')
        spherePath = ufe.Path([proxyShapePathSegment, spherePathSegment])
        sphereItem = ufe.Hierarchy.createItem(spherePath)

        # Get its Object3d interface.
        object3d = ufe.Object3d.object3d(sphereItem)

        # Get its bounding box.
        ufeBBox = object3d.boundingBox()

        # Compare it to known extents.
        assertVectorAlmostEqual(self, ufeBBox.min.vector, [-1]*3)
        assertVectorAlmostEqual(self, ufeBBox.max.vector, [1]*3)

        #######
        # Create a UFE scene item from the parent Xform of the sphere prim.
        parentPathSegment = usdUtils.createUfePathSegment('/parent')
        parentPath = ufe.Path([proxyShapePathSegment, parentPathSegment])
        parentItem = ufe.Hierarchy.createItem(parentPath)

        # Get its Object3d interface.
        parentObject3d = ufe.Object3d.object3d(parentItem)

        # Get its bounding box.
        parentUFEBBox = parentObject3d.boundingBox()

        # Compare it to sphere's extents.
        assertVectorEqual(self, ufeBBox.min.vector, parentUFEBBox.min.vector)
        assertVectorEqual(self, ufeBBox.max.vector, parentUFEBBox.max.vector)


        #######
        # Remove the test file.
        os.remove(usdFilePath)

    def testAnimatedBoundingBox(self):
        '''Test the Object3d bounding box interface for animated geometry.'''

        # Open sphereAnimatedRadiusProxyShape.ma scene in testSamples
        mayaUtils.openSphereAnimatedRadiusScene()

        # The extents of the sphere are copied from the .usda file.
        expected = [
            [(-1.0000002, -1, -1.0000005), (1, 1, 1.0000001)],
            [(-1.3086424, -1.308642, -1.3086426), (1.308642, 1.308642, 1.3086421)],
            [(-2.135803, -2.1358025, -2.1358035), (2.1358025, 2.1358025, 2.1358027)],
            [(-3.333334, -3.3333333, -3.333335), (3.3333333, 3.3333333, 3.3333337)],
            [(-4.7530875, -4.7530866, -4.753089), (4.7530866, 4.7530866, 4.753087)],
            [(-6.246915, -6.2469134, -6.2469163), (6.2469134, 6.2469134, 6.2469144)],
            [(-7.6666684, -7.6666665, -7.6666703), (7.6666665, 7.6666665, 7.6666675)],
            [(-8.8642, -8.864198, -8.864202), (8.864198, 8.864198, 8.864199)],
            [(-9.6913595, -9.691358, -9.691362), (9.691358, 9.691358, 9.691359)],
            [(-10.000002, -10, -10.000005), (10, 10, 10.000001)]]

        # Create an Object3d interface for USD sphere.
        mayaPathSegment = mayaUtils.createUfePathSegment(
            '|transform1|proxyShape1')
        usdPathSegment = usdUtils.createUfePathSegment('/pSphere1')

        spherePath = ufe.Path([mayaPathSegment, usdPathSegment])
        sphereItem = ufe.Hierarchy.createItem(spherePath)

        object3d = ufe.Object3d.object3d(sphereItem)

        # Loop over frames 1 to 10, and compare the values returned to the
        # expected values.
        for frame in range(1,11):
            cmds.currentTime(frame)

            ufeBBox = object3d.boundingBox()

            # Compare it to known extents.
            assertVectorAlmostEqual(self, ufeBBox.min.vector,
                                    expected[frame-1][0], places=6)
            assertVectorAlmostEqual(self, ufeBBox.max.vector,
                                    expected[frame-1][1], places=6)

    def testVisibility(self):
        '''Test the Object3d visibility methods.'''

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

        # Get a scene item for Ball_35.
        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment("|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)

        # Create an Object3d interface for it.
        object3d = ufe.Object3d.object3d(ball35Item)

        visObs = TestObserver()

        # We start off with no visibility observers.
        self.assertFalse(ufe.Object3d.hasObserver(visObs))
        self.assertEqual(ufe.Object3d.nbObservers(), 0)

        # Set the observer for visibility changes.
        ufe.Object3d.addObserver(visObs)
        self.assertTrue(ufe.Object3d.hasObserver(visObs))
        self.assertEqual(ufe.Object3d.nbObservers(), 1)

        # No notifications yet.
        self.assertEqual(visObs.notifications(), 0)

        # Initially it should be visible.
        self.assertTrue(object3d.visibility())

        # Make it invisible.
        object3d.setVisibility(False)
        self.assertFalse(object3d.visibility())

        # We should have got 'one' notification.
        self.assertEqual(visObs.notifications(), 1)

        # Make it visible.
        object3d.setVisibility(True)
        self.assertTrue(object3d.visibility())

        # We should have got one more notification.
        self.assertEqual(visObs.notifications(), 2)

        # Remove the observer.
        ufe.Object3d.removeObserver(visObs)
        self.assertFalse(ufe.Object3d.hasObserver(visObs))
        self.assertEqual(ufe.Object3d.nbObservers(), 0)
