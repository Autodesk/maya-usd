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
import ufeUtils
import usdUtils

from pxr import Gf

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as om

import ufe

from functools import partial
import unittest


def matrix4dTranslation(matrix4d):
    translation = matrix4d.matrix[-1]
    return translation[:-1]

def transform3dTranslation(transform3d):
    return matrix4dTranslation(transform3d.inclusiveMatrix())

def addVec(mayaVec, usdVec):
    return mayaVec + om.MVector(*usdVec)

class TestObserver(ufe.Observer):
    def __init__(self):
        super(TestObserver, self).__init__()
        self.changed = 0

    def __call__(self, notification):
        if isinstance(notification, ufe.Transform3dChanged):
            self.changed += 1

    def notifications(self):
        return self.changed

class Transform3dTranslateTestCase(unittest.TestCase):
    '''Verify the Transform3d UFE translate interface, for multiple runtimes.

    The run-time native interface is used to test setting object translation.
    As of 27-Mar-2018, only world space relative moves are supported by Maya
    code.  Object translation is read using the Transform3d interface and the
    native run-time interface.
    
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
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        ''' Called initially to set up the maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)
        
        # Callables to get current object translation using the run-time and
        # UFE interfaces.
        self.getRunTimeTranslation = None
        self.getUfeTranslation = None

        # Callable to set current object translation using the run-time
        # interface.
        self.setRunTimeTranslation = None

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()
        
        # Create some extra Maya nodes
        cmds.polySphere()
        cmds.polyCube()
        
        # Clear selection to start off
        cmds.select(clear=True)

    def assertRunTimeUFEAlmostEqual(self):
        '''Tests that the translation read from the run-time interface matches the UFE translation.

        Returns a pair with translation read from the run-time and from UFE.
        '''
        runTimeVec = self.getRunTimeTranslation()
        ufeVec  = self.getUfeTranslation()
        assertVectorAlmostEqual(self, runTimeVec, ufeVec)

    def runTestMove(self):
        '''Engine method to run move test.'''

        # Compare the initial positions.
        self.assertRunTimeUFEAlmostEqual()

        # Do some move commands, and compare.
        for xlation in [om.MVector(4, 5, 6), om.MVector(-3, -2, -1)]:
            self.setRunTimeTranslation(xlation)
            self.assertRunTimeUFEAlmostEqual()

    def _testMoveMaya(self):
        '''Move Maya object, read through the Transform3d interface.'''

        # Give the sphere an initial position, and select it.
        expected = om.MVector(1, 2, 3)
        sphereObj = om.MSelectionList().add('pSphere1').getDagPath(0).node()
        sphereFn = om.MFnTransform(sphereObj)
        def setMayaTranslation(xlation):
            sphereFn.setTranslation(xlation, om.MSpace.kTransform)
        setMayaTranslation(expected)
        spherePath = ufe.Path(mayaUtils.createUfePathSegment("|pSphere1"))
        sphereItem = ufe.Hierarchy.createItem(spherePath)

        ufe.GlobalSelection.get().append(sphereItem)

        # Create a Transform3d interface for it.
        transform3d = ufe.Transform3d.transform3d(sphereItem)

        # Set up the callables that will set and get the translation.
        self.setRunTimeTranslation = setMayaTranslation
        self.getRunTimeTranslation = partial(
            sphereFn.translation, om.MSpace.kTransform)
        self.getUfeTranslation = partial(transform3dTranslation, transform3d)

        self.runTestMove()

    def _testMoveUSD(self):
        '''Move USD object, read through the Transform3d interface.'''

        # Select Ball_35 to move it.
        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment("|transform1|proxyShape1"), 
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

        def ball35SetTranslation(xlation):
            ball35Prim = usdUtils.getPrimFromSceneItem(ball35Item)
            ball35Prim.GetAttribute('xformOp:translate').Set(
                Gf.Vec3d(*xlation))

        # Set up the callables that will set and get the translation.
        self.setRunTimeTranslation = ball35SetTranslation
        self.getRunTimeTranslation = ball35Translation
        self.getUfeTranslation = partial(transform3dTranslation, transform3d)

        self.runTestMove()

    def testObservation(self):
        '''Test Transform3d observation interface.

        As of 11-Apr-2018 only implemented for USD objects.
        '''

        # Select Ball_35 to move it.
        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment("|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)

        # Create a Transform3d interface for it.
        transform3d = ufe.Transform3d.transform3d(ball35Item)

        t3dObs = TestObserver()

        # We start off with no observers for Ball_35.
        self.assertFalse(ufe.Transform3d.hasObservers(ball35Path))
        self.assertFalse(ufe.Transform3d.hasObserver(ball35Item, t3dObs))
        self.assertEqual(ufe.Transform3d.nbObservers(ball35Item), 0)

        # Set the observer to observe Ball_35.
        ufe.Transform3d.addObserver(ball35Item, t3dObs)

        self.assertTrue(ufe.Transform3d.hasObservers(ball35Path))
        self.assertTrue(ufe.Transform3d.hasObserver(ball35Item, t3dObs))
        self.assertEqual(ufe.Transform3d.nbObservers(ball35Item), 1)

        # No notifications yet.
        self.assertEqual(t3dObs.notifications(), 0)

        # We only select the ball AFTER doing our initial tests because
        # the MayaUSD plugin creates a transform3d observer on selection
        # change to update the bounding box.
        ufe.GlobalSelection.get().append(ball35Item)

        # Move the prim.
        ball35Prim = usdUtils.getPrimFromSceneItem(ball35Item)
        ball35Prim.GetAttribute('xformOp:translate').Set(
            Gf.Vec3d(10, 20, 30))

        # Notified.
        self.assertEqual(t3dObs.notifications(), 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
