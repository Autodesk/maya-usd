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

import usdUtils, mayaUtils, ufeUtils
from testUtils import assertVectorAlmostEqual
import testTRSBase
import ufe

import unittest

from functools import partial

def transform3dScale(transform3d):
    matrix = om.MMatrix(transform3d.inclusiveMatrix().matrix)
    return om.MTransformationMatrix(matrix).scale(om.MSpace.kObject)

def combineScales(scale1, scale2):
    return [scale1[0]*scale2[0], scale1[1]*scale2[1], scale1[2]*scale2[2] ]
    
def multiSelectCombineScales(scaleList, scale2):
    return [combineScales(scale, scale2) for scale in scaleList]

class ScaleCmdTestCase(testTRSBase.TRSTestCaseBase):
    '''Verify the Transform3d UFE interface, for multiple runtimes.

    The Maya scale command is used to test setting object scale.  As of
    27-Mar-2018, only world space relative scales are supported by Maya code.
    Object scale is read using the Transform3d interface and the native
    run-time interface.
    
    UFE Feature : Transform3d
    Maya Feature : scale
    Action : Relative scale in world space.
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
        
        # Callables to get current object scale using the run-time and
        # UFE.
        self.runTimeScale = None
        self.ufeScale = None

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()
        
        # Create some extra Maya nodes
        cmds.polySphere()
        cmds.polyCube()
        
        # Clear selection to start off
        cmds.select(clear=True)

    def snapshotRunTimeUFE(self):
        '''Return a pair with scale read from the run-time and from UFE.

        Tests that the scale read from the run-time interface matches the
        UFE scale.
        '''
        runTimeVec = self.runTimeScale()
        ufeVec  = self.ufeScale()
        assertVectorAlmostEqual(self, runTimeVec, ufeVec, places=6)
        return (runTimeVec, ufeVec)

    def multiSelectSnapshotRunTimeUFE(self, items):
        '''Return a list of pairs with scale read from the run-time and from UFE.

        Tests that the scale read from the run-time interface matches the
        UFE scale.
        '''
        snapshot = []
        for item in items:
            runTimeVec = self.runTimeScale(item)
            ufeVec  = self.ufeScale(item)
            assertVectorAlmostEqual(self, runTimeVec, ufeVec, places=6)
            snapshot.append((runTimeVec, ufeVec))
        return snapshot

    def runTestScale(self, expected):
        '''Engine method to run scale test.'''

        # Save the initial position to the memento list.
        self.snapShotAndTest(expected)

        # Do some scale commands, and compare with expected. 
        for relativeScale in [[4, 5, 6], [0.5, 0.2, 0.1]]:
            cmds.scale(*relativeScale, relative=True)
            expected = combineScales(expected, relativeScale)
            self.snapShotAndTest(expected)

        # Test undo, redo.
        self.rewindMemento()
        self.fforwardMemento()

    def runMultiSelectTestScale(self, items, expected, places=7):
        '''Engine method to run multiple selection scale test.'''

        # Save the initial positions to the memento list.
        self.multiSelectSnapShotAndTest(items, expected)

        # Do some scale commands, and compare with expected. 
        for relativeScale in [[4, 5, 6], [0.5, 0.2, 0.1]]:
            cmds.scale(*relativeScale, relative=True)
            expected = multiSelectCombineScales(expected, relativeScale)
            self.multiSelectSnapShotAndTest(items, expected, places)

        # Test undo, redo.
        self.multiSelectRewindMemento(items)
        self.multiSelectFforwardMemento(items)

    def testScaleMaya(self):
        '''Scale Maya object, read through the Transform3d interface.'''

        # Give the sphere an initial position, and select it.
        expected = [1, 2, 3]
        sphereObj = om.MSelectionList().add('pSphere1').getDagPath(0).node()
        sphereFn = om.MFnTransform(sphereObj)
        sphereFn.setScale(expected)
        spherePath = ufe.Path(mayaUtils.createUfePathSegment("|pSphere1"))
        sphereItem = ufe.Hierarchy.createItem(spherePath)

        ufe.GlobalSelection.get().append(sphereItem)

        # Create a Transform3d interface for it.
        transform3d = ufe.Transform3d.transform3d(sphereItem)

        # Set up the callables that will retrieve the scale.
        self.runTimeScale = partial(
            sphereFn.scale)
        self.ufeScale = partial(transform3dScale, transform3d)

        self.runTestScale(expected)

    def testScaleUSD(self):
        '''Scale USD object, read through the Transform3d interface.'''

        # Select Ball_35 to scale it.
        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment("|world|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)

        ufe.GlobalSelection.get().append(ball35Item)

        # Create a Transform3d interface for it.
        transform3d = ufe.Transform3d.transform3d(ball35Item)

        # We compare the UFE scale with the USD run-time scale.  To
        # obtain the full scale of Ball_35, we need to add the USD
        # scale to the Maya proxy shape scale.
        proxyShapeXformObj = om.MSelectionList().add('transform1').getDagPath(0).node()
        proxyShapeXformFn = om.MFnTransform(proxyShapeXformObj)

        def ball35Scale():
            ball35Prim = usdUtils.getPrimFromSceneItem(ball35Item)
            if not ball35Prim.HasAttribute('xformOp:scale'):
                return proxyShapeXformFn.scale()
            else:
                return combineScales(proxyShapeXformFn.scale(), ball35Prim.GetAttribute('xformOp:scale').Get())
            
        # Set up the callables that will retrieve the scale.
        self.runTimeScale = ball35Scale
        self.ufeScale = partial(transform3dScale, transform3d)

        # Save the initial position to the memento list.
        expected = ball35Scale()

        self.runTestScale(expected)

    def testMultiSelectScaleUSD(self):
        '''Scale multiple USD objects, read through Transform3d interface.'''

        # Select multiple balls to scale them.
        proxyShapePathSegment = mayaUtils.createUfePathSegment(
            "|world|transform1|proxyShape1")

        # Test passes for a single item.
        # balls = ['Ball_33']
        balls = ['Ball_33', 'Ball_34']
        ballPaths = [
            ufe.Path([proxyShapePathSegment, 
                      usdUtils.createUfePathSegment('/Room_set/Props/'+ball)])
            for ball in balls]
        ballItems = [ufe.Hierarchy.createItem(ballPath) 
                     for ballPath in ballPaths]

        for ballItem in ballItems:
            ufe.GlobalSelection.get().append(ballItem)

        # We compare the UFE scale with the USD run-time scale.  To
        # obtain the full scale of USD scene items, we need to add the USD
        # scale to the Maya proxy shape scale.
        proxyShapeXformObj = om.MSelectionList().add('transform1').getDagPath(0).node()
        proxyShapeXformFn = om.MFnTransform(proxyShapeXformObj)

        def usdSceneItemScale(item):
            prim = usdUtils.getPrimFromSceneItem(item)
            if not prim.HasAttribute('xformOp:scale'):
                return proxyShapeXformFn.scale()
            else:
                return combineScales(proxyShapeXformFn.scale(), prim.GetAttribute('xformOp:scale').Get())

        def ufeSceneItemScale(item):
            return transform3dScale(ufe.Transform3d.transform3d(item))

        # Set up the callables that will retrieve the scale.
        self.runTimeScale = usdSceneItemScale
        self.ufeScale = ufeSceneItemScale

        # Give the tail item in the selection an initial scale that
        # is different, to catch bugs where the relative scale
        # incorrectly squashes any existing scale.
        backItem = ballItems[-1]
        backT3d = ufe.Transform3d.transform3d(backItem)
        initialScale = [1.1, 2.2, 3.3]
        backT3d.scale(*initialScale)
        assertVectorAlmostEqual(self, initialScale, usdSceneItemScale(backItem), places=6)

        # Save the initial positions to the memento list.
        expected = [usdSceneItemScale(ballItem) for ballItem in ballItems]

        self.runMultiSelectTestScale(ballItems, expected, places=6)
