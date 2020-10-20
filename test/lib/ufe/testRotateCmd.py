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

import maya.api.OpenMaya as om
import maya.cmds as cmds
from math import radians

import usdUtils, mayaUtils, ufeUtils
from testUtils import assertVectorAlmostEqual
import testTRSBase
import ufe

import unittest

from functools import partial

def transform3dRotation(transform3d):
    matrix = om.MMatrix(transform3d.inclusiveMatrix().matrix)
    return om.MTransformationMatrix(matrix).rotation()
    
def addRotations(eulerRotation1, eulerRotation2):
    src = om.MTransformationMatrix(eulerRotation1.asMatrix())
    return src.rotateBy(eulerRotation2, om.MSpace.kObject).rotation()

def multiSelectAddRotations(eulerRotationList, eulerRotation2):
    return [addRotations(eulerRotation, eulerRotation2)
            for eulerRotation in eulerRotationList]

class RotateCmdTestCase(testTRSBase.TRSTestCaseBase):
    '''Verify the Transform3d UFE interface, for multiple runtimes.

    The Maya rotate command is used to test setting object rotation.  As of
    23-May-2018, only world space relative rotates are supported by Maya code.
    Object rotation is read using the Transform3d interface and the native
    run-time interface.
    
    UFE Feature : Transform3d
    Maya Feature : rotate
    Action : Relative rotate in world space.
    Applied On Selection :
        - No selection - Given node as param
        - Single Selection [Maya, Non-Maya]
        - Multiple Selection [Mixed, Non-Maya].  Maya-only selection tested by
          Maya.
    Undo/Redo Test : Yes
    Expect Results To Test :
        - Maya Dag object world space position.
        - USD object world space position.
    Edge Cases :
        - None.
    '''

    pluginsLoaded = False
    
    @classmethod
    def setUpClass(cls):
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()
    
    def setUp(self):
        ''' Called initially to set up the maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)
        
        # Set up memento, a list of snapshots.
        self.memento = []
        
        # Callables to get current object rotation using the run-time and
        # UFE.
        self.runTimeRotation = None
        self.ufeRotation = None

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()
        
        # Create some extra Maya nodes
        cmds.polySphere()
        cmds.polyCube()
        
        # Clear selection to start off
        cmds.select(clear=True)

    def snapshotRunTimeUFE(self):
        '''Return a pair with rotation read from the run-time and from UFE.

        Tests that the rotation read from the run-time interface matches the
        UFE rotation.
        '''
        runTimeVec = self.runTimeRotation()
        ufeVec  = self.ufeRotation()
        assertVectorAlmostEqual(self, runTimeVec, ufeVec)
        return (runTimeVec, ufeVec)

    def multiSelectSnapshotRunTimeUFE(self, items):
        '''Return a list of pairs with rotation read from the run-time and from UFE.

        Tests that the rotation read from the run-time interface matches the
        UFE rotation.
        '''
        snapshot = []
        for item in items:
            runTimeVec = self.runTimeRotation(item)
            ufeVec  = self.ufeRotation(item)
            assertVectorAlmostEqual(self, runTimeVec, ufeVec)
            snapshot.append((runTimeVec, ufeVec))
        return snapshot

    def runTestRotate(self, expected):
        '''Engine method to run rotate test.'''

        # Save the initial position to the memento list.
        self.snapShotAndTest(expected)

        # Do some rotate commands, and compare with expected. 
        for x, y, z in [[10, 20, 30], [-30, -20, -10]]:
            cmds.rotate(x, y, z, relative=True, objectSpace=True, forceOrderXYZ=True) # Just as in default rotations by manipulator
            expected = addRotations(expected, om.MEulerRotation(radians(x), radians(y), radians(z)))
            self.snapShotAndTest(expected)

        # Test undo, redo.
        self.rewindMemento()
        self.fforwardMemento()

    def runMultiSelectTestRotate(self, items, expected):
        '''Engine method to run multiple selection rotate test.'''

        # Save the initial positions to the memento list.
        self.multiSelectSnapShotAndTest(items, expected)

        # Do some rotate commands, and compare with expected. 
        for x, y, z in [[10, 20, 30], [-30, -20, -10]]:
            cmds.rotate(x, y, z, relative=True, objectSpace=True, forceOrderXYZ=True) # Just as in default rotations by manipulator
            expected = multiSelectAddRotations(expected, om.MEulerRotation(radians(x), radians(y), radians(z)))
            self.multiSelectSnapShotAndTest(items, expected)

        # Test undo, redo.
        self.multiSelectRewindMemento(items)
        self.multiSelectFforwardMemento(items)

    def testRotateMaya(self):
        '''Rotate Maya object, read through the Transform3d interface.'''

        # Give the sphere an initial position, and select it.
        rotation = om.MEulerRotation(radians(30), radians(60), radians(90))
        sphereObj = om.MSelectionList().add('pSphere1').getDagPath(0).node()
        sphereFn = om.MFnTransform(sphereObj)
        sphereFn.setRotation(rotation, om.MSpace.kTransform)
        
        # Select pSphere1
        spherePath = ufe.Path(mayaUtils.createUfePathSegment("|pSphere1"))
        sphereItem = ufe.Hierarchy.createItem(spherePath)
        ufe.GlobalSelection.get().append(sphereItem)

        # Create a Transform3d interface for it.
        transform3d = ufe.Transform3d.transform3d(sphereItem)

        # Set up the callables that will retrieve the rotation.
        self.runTimeRotation = partial(
            sphereFn.rotation, om.MSpace.kTransform)
        self.ufeRotation = partial(transform3dRotation, transform3d)

        self.runTestRotate(rotation)

    def testRotateUSD(self):
        '''Rotate USD object, read through the Transform3d interface.'''

        # Select Ball_35 to rotate it.
        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment("|world|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)

        ufe.GlobalSelection.get().append(ball35Item)

        # Create a Transform3d interface for it.
        transform3d = ufe.Transform3d.transform3d(ball35Item)

        # We compare the UFE rotation with the USD run-time rotation.  To
        # obtain the full rotation of Ball_35, we need to add the USD
        # rotation to the Maya proxy shape rotation.
        proxyShapeXformObj = om.MSelectionList().add('transform1').getDagPath(0).node()
        proxyShapeXformFn = om.MFnTransform(proxyShapeXformObj)

        def ball35Rotation():
            ball35Prim = usdUtils.getPrimFromSceneItem(ball35Item)
            if not ball35Prim.HasAttribute('xformOp:rotateXYZ'):
                return proxyShapeXformFn.rotation(om.MSpace.kTransform)
            else:
                x,y,z = ball35Prim.GetAttribute('xformOp:rotateXYZ').Get()
                return  proxyShapeXformFn.rotation(om.MSpace.kTransform) + om.MEulerRotation(radians(x), radians(y), radians(z))

        # Set up the callables that will retrieve the rotation.
        self.runTimeRotation = ball35Rotation
        self.ufeRotation = partial(transform3dRotation, transform3d)

        # Save the initial position to the memento list.
        expected = ball35Rotation()

        self.runTestRotate(expected)

    def testMultiSelectRotateUSD(self):
        '''Rotate multiple USD objects, read through Transform3d interface.'''

        # Select multiple balls to rotate them.
        proxyShapePathSegment = mayaUtils.createUfePathSegment(
            "|world|transform1|proxyShape1")

        balls = ['Ball_33', 'Ball_34']
        ballPaths = [
            ufe.Path([proxyShapePathSegment, 
                      usdUtils.createUfePathSegment('/Room_set/Props/'+ball)])
            for ball in balls]
        ballItems = [ufe.Hierarchy.createItem(ballPath) 
                     for ballPath in ballPaths]

        for ballItem in ballItems:
            ufe.GlobalSelection.get().append(ballItem)

        # We compare the UFE rotation with the USD run-time rotation.  To
        # obtain the full rotation of USD scene items, we need to add the USD
        # rotation to the Maya proxy shape rotation.
        proxyShapeXformObj = om.MSelectionList().add('transform1').getDagPath(0).node()
        proxyShapeXformFn = om.MFnTransform(proxyShapeXformObj)

        def usdSceneItemRotation(item):
            prim = usdUtils.getPrimFromSceneItem(item)
            if not prim.HasAttribute('xformOp:rotateXYZ'):
                return proxyShapeXformFn.rotation(om.MSpace.kTransform)
            else:
                x,y,z = prim.GetAttribute('xformOp:rotateXYZ').Get()
                return  proxyShapeXformFn.rotation(om.MSpace.kTransform) + om.MEulerRotation(radians(x), radians(y), radians(z))

        def ufeSceneItemRotation(item):
            return transform3dRotation(ufe.Transform3d.transform3d(item))

        # Set up the callables that will retrieve the rotation.
        self.runTimeRotation = usdSceneItemRotation
        self.ufeRotation = ufeSceneItemRotation

        # Give the tail item in the selection an initial rotation that
        # is different, to catch bugs where the relative rotation
        # incorrectly squashes any existing rotation.
        backItem = ballItems[-1]
        backT3d = ufe.Transform3d.transform3d(backItem)
        initialRot = [-10, -20, -30]
        backT3d.rotate(*initialRot)
        assertVectorAlmostEqual(self, [radians(a) for a in initialRot], 
                                     usdSceneItemRotation(backItem))

        # Save the initial positions to the memento list.
        expected = [usdSceneItemRotation(ballItem) for ballItem in ballItems]

        self.runMultiSelectTestRotate(ballItems, expected)
        
