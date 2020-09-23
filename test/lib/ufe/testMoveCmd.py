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

from ufeTestUtils import usdUtils, mayaUtils, ufeUtils
from ufeTestUtils.testUtils import assertVectorAlmostEqual
import testTRSBase
import ufe

import unittest

from functools import partial

def matrix4dTranslation(matrix4d):
    translation = matrix4d.matrix[-1]
    return translation[:-1]

def transform3dTranslation(transform3d):
    return matrix4dTranslation(transform3d.inclusiveMatrix())

def addVec(mayaVec, usdVec):
    return mayaVec + om.MVector(*usdVec)

def multiSelectAddTranslations(translationList, translation2):
    return [translation+translation2 for translation in translationList]

class MoveCmdTestCase(testTRSBase.TRSTestCaseBase):
    '''Verify the Transform3d UFE interface, for multiple runtimes.

    The Maya move command is used to test setting object translation.  As of
    27-Mar-2018, only world space relative moves are supported by Maya code.
    Object translation is read using the Transform3d interface and the native
    run-time interface.
    
    UFE Feature : Transform3d
    Maya Feature : move
    Action : Relative move in world space.
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
        
        # Callables to get current object translation using the run-time and
        # UFE.
        self.runTimeTranslation = None
        self.ufeTranslation = None

        # Open top_layer.ma scene in test-samples
        mayaUtils.openTopLayerScene()
        
        # Create some extra Maya nodes
        cmds.polySphere()
        cmds.polyCube()
        
        # Clear selection to start off
        cmds.select(clear=True)

    def snapshotRunTimeUFE(self):
        '''Return a pair with translation read from the run-time and from UFE.

        Tests that the translation read from the run-time interface matches the
        UFE translation.
        '''
        runTimeVec = self.runTimeTranslation()
        ufeVec  = self.ufeTranslation()
        assertVectorAlmostEqual(self, runTimeVec, ufeVec)
        return (runTimeVec, ufeVec)

    def multiSelectSnapshotRunTimeUFE(self, items):
        '''Return a list of pairs with translation read from the run-time and from UFE.

        Tests that the translation read from the run-time interface matches the
        UFE translation.
        '''
        snapshot = []
        for item in items:
            runTimeVec = self.runTimeTranslation(item)
            ufeVec  = self.ufeTranslation(item)
            assertVectorAlmostEqual(self, runTimeVec, ufeVec)
            snapshot.append((runTimeVec, ufeVec))
        return snapshot

    def runTestMove(self, expected):
        '''Engine method to run move test.'''

        # Save the initial position to the memento list.
        self.snapShotAndTest(expected)

        # Do some move commands, and compare with expected. 
        for relativeMove in [om.MVector(4, 5, 6), om.MVector(-3, -2, -1)]:
            cmds.move(*relativeMove, relative=True)
            expected += relativeMove
            self.snapShotAndTest(expected)

        # Test undo, redo.
        self.rewindMemento()
        self.fforwardMemento()

    def runMultiSelectTestMove(self, items, expected):
        '''Engine method to run multiple selection move test.'''

        # Save the initial positions to the memento list.
        self.multiSelectSnapShotAndTest(items, expected)

        # Do some move commands, and compare with expected. 
        for relativeMove in [om.MVector(4, 5, 6), om.MVector(-3, -2, -1)]:
            cmds.move(*relativeMove, relative=True)
            expected = multiSelectAddTranslations(expected, relativeMove)
            self.multiSelectSnapShotAndTest(items, expected)

        # Test undo, redo.
        self.multiSelectRewindMemento(items)
        self.multiSelectFforwardMemento(items)

    def testMoveMaya(self):
        '''Move Maya object, read through the Transform3d interface.'''

        # Give the sphere an initial position, and select it.
        expected = om.MVector(1, 2, 3)
        sphereObj = om.MSelectionList().add('pSphere1').getDagPath(0).node()
        sphereFn = om.MFnTransform(sphereObj)
        sphereFn.setTranslation(expected, om.MSpace.kTransform)
        spherePath = ufe.Path(mayaUtils.createUfePathSegment("|pSphere1"))
        sphereItem = ufe.Hierarchy.createItem(spherePath)

        ufe.GlobalSelection.get().append(sphereItem)

        # Create a Transform3d interface for it.
        transform3d = ufe.Transform3d.transform3d(sphereItem)

        # Set up the callables that will retrieve the translation.
        self.runTimeTranslation = partial(
            sphereFn.translation, om.MSpace.kTransform)
        self.ufeTranslation = partial(transform3dTranslation, transform3d)

        self.runTestMove(expected)

    def testMoveUSD(self):
        '''Move USD object, read through the Transform3d interface.'''

        # Select Ball_35 to move it.
        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment("|world|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)

        ufe.GlobalSelection.get().append(ball35Item)

        # Create a Transform3d interface for it.
        transform3d = ufe.Transform3d.transform3d(ball35Item)

        # We compare the UFE translation with the USD run-time translation.  To
        # obtain the full translation of Ball_35, we need to add the USD
        # translation to the Maya proxy shape translation.
        proxyShapeXformObj = om.MSelectionList().add('transform1').getDagPath(0).node()
        proxyShapeXformFn = om.MFnTransform(proxyShapeXformObj)

        def ball35Translation():
            ball35Prim = usdUtils.getPrimFromSceneItem(ball35Item)
            return addVec(
                proxyShapeXformFn.translation(om.MSpace.kTransform),
                ball35Prim.GetAttribute('xformOp:translate').Get())

        # Set up the callables that will retrieve the translation.
        self.runTimeTranslation = ball35Translation
        self.ufeTranslation = partial(transform3dTranslation, transform3d)

        # Save the initial position to the memento list.
        expected = ball35Translation()

        self.runTestMove(expected)

    def testMultiSelectMoveUSD(self):
        '''Move multiple USD objects, read through Transform3d interface.'''

        # Select multiple balls to move them.
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

        # We compare the UFE translation with the USD run-time translation.  To
        # obtain the full translation of USD scene items, we need to add the USD
        # translation to the Maya proxy shape translation.
        proxyShapeXformObj = om.MSelectionList().add('transform1').getDagPath(0).node()
        proxyShapeXformFn = om.MFnTransform(proxyShapeXformObj)

        def usdSceneItemTranslation(item):
            prim = usdUtils.getPrimFromSceneItem(item)
            if not prim.HasAttribute('xformOp:translate'):
                return proxyShapeXformFn.translation(om.MSpace.kTransform)
            else:
                return addVec(
                    proxyShapeXformFn.translation(om.MSpace.kTransform),
                    prim.GetAttribute('xformOp:translate').Get())

        def ufeSceneItemTranslation(item):
            return transform3dTranslation(ufe.Transform3d.transform3d(item))

        # Set up the callables that will retrieve the translation.
        self.runTimeTranslation = usdSceneItemTranslation
        self.ufeTranslation = ufeSceneItemTranslation

        # Give the tail item in the selection an initial translation that
        # is different, to catch bugs where the relative translation
        # incorrectly squashes any existing translation.
        backItem = ballItems[-1]
        backT3d = ufe.Transform3d.transform3d(backItem)
        initialTranslation = [-10, -20, -30]
        backT3d.translate(*initialTranslation)
        assertVectorAlmostEqual(self, ufeSceneItemTranslation(backItem), 
                                     usdSceneItemTranslation(backItem))

        # Save the initial positions to the memento list.
        expected = [usdSceneItemTranslation(ballItem) for ballItem in ballItems]

        self.runMultiSelectTestMove(ballItems, expected)
