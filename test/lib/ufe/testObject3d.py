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

import fixturesUtils
import mayaUtils
from testUtils import assertVectorAlmostEqual
from testUtils import assertVectorEqual
import usdUtils

import mayaUsd.ufe

from pxr import Usd
from pxr import UsdGeom

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as OpenMaya

import ufe

import os
import unittest


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
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

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

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '2034', 'testUndoVisibleCmd is only available in UFE preview version 0.2.34 and greater')
    def testUndoVisibleCmd(self):

        ''' Verify the token / attribute values for visibility after performing undo/redo '''

        cmds.file(new=True, force=True)

        # create a Capsule via contextOps menu
        import mayaUsd_createStageWithNewLayer
        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path('|stage1|stageShape1')
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'Capsule'])

        # create an Object3d interface.
        capsulePath = ufe.PathString.path('|stage1|stageShape1,/Capsule1')
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)
        capsulePrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(capsulePath))
        object3d = ufe.Object3d.object3d(capsuleItem)

        # stage / primSpec
        stage = mayaUsd.ufe.getStage(str(proxyShapePath))
        primSpec = stage.GetEditTarget().GetPrimSpecForScenePath('/Capsule1');

        # initially capsuleItem should be visible.
        self.assertTrue(object3d.visibility())

        # make it invisible.
        visibleCmd = object3d.setVisibleCmd(False)
        visibleCmd.execute()

        # get the visibility "attribute"
        visibleAttr = capsulePrim.GetAttribute('visibility')

        # expect the visibility attribute to be 'invisible'
        self.assertEqual(visibleAttr.Get(), 'invisible')

        # visibility "token" must exists now in the USD data model
        self.assertTrue(bool(primSpec and UsdGeom.Tokens.visibility in primSpec.attributes))

        # undo
        visibleCmd.undo()

        # expect the visibility attribute to be 'inherited'
        self.assertEqual(visibleAttr.Get(), 'inherited')

        # visibility token must not exists now in the USD data model after undo
        self.assertFalse(bool(primSpec and UsdGeom.Tokens.visibility in primSpec.attributes))

        # capsuleItem must be visible now
        self.assertTrue(object3d.visibility())

        # redo
        visibleCmd.redo()

        # expect the visibility attribute to be 'invisible'
        self.assertEqual(visibleAttr.Get(), 'invisible')

        # visibility token must exists now in the USD data model after redo
        self.assertTrue(bool(primSpec and UsdGeom.Tokens.visibility in primSpec.attributes))

        # capsuleItem must be invisible now
        self.assertFalse(object3d.visibility())

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '2034', 'testMayaHideAndShowHiddenUndoCommands is only available in UFE preview version 0.2.34 and greater')
    def testMayaHideAndShowHiddenUndoCommands(self):
        ''' Verify the token / attribute values for visibility via "hide", "showHidden" commands + Undo/Redo '''

        cmds.file(new=True, force=True)

        # create a Capsule and Cylinder via contextOps menu
        import mayaUsd_createStageWithNewLayer
        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path('|stage1|stageShape1')
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'Capsule'])
        proxyShapeContextOps.doOp(['Add New Prim', 'Cylinder'])

        # capsule
        capsulePath = ufe.PathString.path('|stage1|stageShape1,/Capsule1')
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)
        capsulePrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(capsulePath))

        # cylinder
        cylinderPath = ufe.PathString.path('|stage1|stageShape1,/Cylinder1')
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)
        cylinderPrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(cylinderPath))

        # stage / primSpec
        stage = mayaUsd.ufe.getStage(str(proxyShapePath))
        primSpecCapsule = stage.GetEditTarget().GetPrimSpecForScenePath('/Capsule1');
        primSpecCylinder = stage.GetEditTarget().GetPrimSpecForScenePath('/Cylinder1');

        # select capsule and cylinder prims
        ufe.GlobalSelection.get().append(capsuleItem)
        ufe.GlobalSelection.get().append(cylinderItem)

        # hide selected items
        cmds.hide(cs=True)

        # get the visibility "attribute"
        capsuleVisibleAttr = capsulePrim.GetAttribute('visibility')
        cylinderVisibleAttr = cylinderPrim.GetAttribute('visibility')

        # expect the visibility attribute to be 'invisible'
        self.assertEqual(capsuleVisibleAttr.Get(), 'invisible')
        self.assertEqual(cylinderVisibleAttr.Get(), 'invisible')

        # visibility "token" must exists now in the USD data model
        self.assertTrue(bool(primSpecCapsule and UsdGeom.Tokens.visibility in primSpecCapsule.attributes))
        self.assertTrue(bool(primSpecCylinder and UsdGeom.Tokens.visibility in primSpecCylinder.attributes))

        # undo
        cmds.undo()

        # expect the visibility attribute to be 'inherited'
        self.assertEqual(capsuleVisibleAttr.Get(), 'inherited')
        self.assertEqual(cylinderVisibleAttr.Get(), 'inherited')

        # visibility token must not exists now in the USD data model after undo
        self.assertFalse(bool(primSpecCapsule and UsdGeom.Tokens.visibility in primSpecCapsule.attributes))
        self.assertFalse(bool(primSpecCylinder and UsdGeom.Tokens.visibility in primSpecCylinder.attributes))

        # undo
        cmds.redo()

        # expect the visibility attribute to be 'invisible'
        self.assertEqual(capsuleVisibleAttr.Get(), 'invisible')
        self.assertEqual(cylinderVisibleAttr.Get(), 'invisible')

        # visibility "token" must exists now in the USD data model
        self.assertTrue(bool(primSpecCapsule and UsdGeom.Tokens.visibility in primSpecCapsule.attributes))
        self.assertTrue(bool(primSpecCylinder and UsdGeom.Tokens.visibility in primSpecCylinder.attributes))

        # hide selected items again
        cmds.hide(cs=True)

        # right after, call showHidden -all to make everything visible again
        cmds.showHidden( all=True )

        # expect the visibility attribute to be 'inherited'
        self.assertEqual(capsuleVisibleAttr.Get(), 'inherited')
        self.assertEqual(cylinderVisibleAttr.Get(), 'inherited')

        # This time, expect the visibility token to exists in the USD data model
        self.assertTrue(bool(primSpecCapsule and UsdGeom.Tokens.visibility in primSpecCapsule.attributes))
        self.assertTrue(bool(primSpecCylinder and UsdGeom.Tokens.visibility in primSpecCylinder.attributes))

        # undo the showHidden command
        cmds.undo()

        # expect the visibility attribute to be 'invisible'
        self.assertEqual(capsuleVisibleAttr.Get(), 'invisible')
        self.assertEqual(cylinderVisibleAttr.Get(), 'invisible')

        # visibility "token" must exists now in the USD data model
        self.assertTrue(bool(primSpecCapsule and UsdGeom.Tokens.visibility in primSpecCapsule.attributes))
        self.assertTrue(bool(primSpecCylinder and UsdGeom.Tokens.visibility in primSpecCylinder.attributes))

        # redo 
        cmds.redo()

        # expect the visibility attribute to be 'inherited'
        self.assertEqual(capsuleVisibleAttr.Get(), 'inherited')
        self.assertEqual(cylinderVisibleAttr.Get(), 'inherited')

        # visibility "token" must exists now in the USD data model
        self.assertTrue(bool(primSpecCapsule and UsdGeom.Tokens.visibility in primSpecCapsule.attributes))
        self.assertTrue(bool(primSpecCylinder and UsdGeom.Tokens.visibility in primSpecCylinder.attributes))

    def testMayaGeomExtentsRecomputation(self):
        ''' Verify the automatic extents computation in when geom attributes change '''

        cmds.file(new=True, force=True)

        # create a Capsule via contextOps menu
        import mayaUsd_createStageWithNewLayer
        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path('|stage1|stageShape1')
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'Capsule'])

        # capsule
        capsulePath = ufe.PathString.path('|stage1|stageShape1,/Capsule1')
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)
        capsulePrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(capsulePath))

        # get the height and extent "attributes"
        capsuleHeightAttr = capsulePrim.GetAttribute('height')
        capsuleExtentAttr = capsulePrim.GetAttribute('extent')

        self.assertAlmostEqual(capsuleHeightAttr.Get(), 1.0)
        capsuleExtent = capsuleExtentAttr.Get()
        self.assertAlmostEqual(capsuleExtent[0][2], -1.0)
        self.assertAlmostEqual(capsuleExtent[1][2], 1.0)

        capsuleHeightAttr.Set(10.0)

        self.assertAlmostEqual(capsuleHeightAttr.Get(), 10.0)
        # Extent will have been recomputed:
        capsuleExtent = capsuleExtentAttr.Get()
        self.assertAlmostEqual(capsuleExtent[0][2], -5.5)
        self.assertAlmostEqual(capsuleExtent[1][2], 5.5)

if __name__ == '__main__':
    unittest.main(verbosity=2)
