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

import os
import fixturesUtils
import mayaUtils
import testTRSBase
from testUtils import assertVectorAlmostEqual
import ufeUtils
import usdUtils

import mayaUsd_createStageWithNewLayer
import mayaUsd.ufe

from pxr import UsdGeom, Vt, Gf

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
        
        # Set up memento, a list of snapshots.
        self.memento = []
        
        # Callables to get current object translation using the run-time and
        # UFE.
        self.runTimeTranslation = None
        self.ufeTranslation = None

        # Open top_layer.ma scene in testSamples
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
            "|transform1|proxyShape1")

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

    def runTestOpUndo(self, createTransformOp, attrSpec):
        '''Engine method for op undo testing.'''
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        
        proxyShapePath = ufe.PathString.path(proxyShape)
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'Sphere'])

        spherePath = ufe.PathString.path('%s,/Sphere1' % proxyShape)
        sphereItem = ufe.Hierarchy.createItem(spherePath)

        spherePrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(spherePath))
        sphereXformable = UsdGeom.Xformable(spherePrim)
        createTransformOp(self, sphereXformable)

        # Set the edit target to the session layer, then move.
        stage = mayaUsd.ufe.getStage(proxyShape)
        sessionLayer = stage.GetLayerStack()[0]
        stage.SetEditTarget(sessionLayer)

        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(sphereItem)

        cmds.move(0, 10, 0, relative=True, os=True, wd=True)

        sphereT3d = ufe.Transform3d.transform3d(sphereItem)
        assertVectorAlmostEqual(self, sphereT3d.translation().vector, [0, 10, 0])

        primSpec = stage.GetEditTarget().GetPrimSpecForScenePath('/Sphere1')

        # Writing to the session layer has created a primSpec in that layer,
        # with the xformOp:transform attrSpec.
        self.assertIsNotNone(primSpec)
        self.assertIn(attrSpec, primSpec.attributes)

        # Undo: primSpec in session layer should be gone.
        cmds.undo()

        assertVectorAlmostEqual(self, sphereT3d.translation().vector, [0, 0, 0])

        primSpec = stage.GetEditTarget().GetPrimSpecForScenePath('/Sphere1')
        self.assertIsNone(primSpec)

        cmds.redo()

        assertVectorAlmostEqual(self, sphereT3d.translation().vector, [0, 10, 0])

        # Undo after redo must also remove primSpec.
        cmds.undo()

        assertVectorAlmostEqual(self, sphereT3d.translation().vector, [0, 0, 0])

        primSpec = stage.GetEditTarget().GetPrimSpecForScenePath('/Sphere1')
        self.assertIsNone(primSpec)

        # Performing change in main layer should also work.
        layer = stage.GetLayerStack()[1]
        stage.SetEditTarget(layer)

        cmds.move(0, 10, 0, relative=True, os=True, wd=True)

        def checkTransform(expectedTranslation):
            # Check through both USD and UFE interfaces
            assertVectorAlmostEqual(
                self, sphereT3d.translation().vector, expectedTranslation)
            expected = Gf.Matrix4d(1.0)
            expected.SetTranslate(expectedTranslation)
            actual = sphereXformable.GetLocalTransformation()
            self.assertTrue(Gf.IsClose(actual, expected, 1e-5))
            
        checkTransform([0, 10, 0])

        cmds.undo()

        checkTransform([0, 0, 0])

        cmds.redo()

        checkTransform([0, 10, 0])

    def testMatrixOpUndo(self):
        '''Undo of matrix op move must completely remove attr spec.'''

        def createMatrixTransformOp(testCase, sphereXformable):
            transformOp = sphereXformable.AddTransformOp()
            xform = Gf.Matrix4d(1.0)
            transformOp.Set(xform)
    
            testCase.assertEqual(
                sphereXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray([
                    "xformOp:transform"]))

        self.runTestOpUndo(createMatrixTransformOp, 'xformOp:transform')

    def testCommonAPIUndo(self):
        '''Undo of move with common API must completely remove attr spec.'''

        def createCommonAPI(testCase, sphereXformable):
            sphereXformable.AddTranslateOp(
                UsdGeom.XformOp.PrecisionFloat, "pivot")
            sphereXformable.AddTranslateOp(
                UsdGeom.XformOp.PrecisionFloat, "pivot", True)
    
            self.assertEqual(
                sphereXformable.GetXformOpOrderAttr().Get(), 
                Vt.TokenArray(("xformOp:translate:pivot",
                               "!invert!xformOp:translate:pivot")))
            self.assertTrue(UsdGeom.XformCommonAPI(sphereXformable))

        self.runTestOpUndo(createCommonAPI, 'xformOp:translate')

    @unittest.skipUnless(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '3014', 'Creating a SceneItem was fixed in UFE v3.14')
    def testBadSceneItem(self):
        '''Improperly constructed scene item should not crash Maya.'''

        # MAYA-112601 / GitHub #1169: improperly constructed Python scene item
        # should not cause a crash.
        cmds.file(new=True, force=True)

        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        proxyShapePath = ufe.PathString.path(proxyShape)
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'Sphere'])

        spherePath = ufe.PathString.path('%s,/Sphere1' % proxyShape)
        # The proper way to create a scene item is the following:
        #
        # sphereItem = ufe.Hierarchy.createItem(spherePath)
        #
        # A naive user can create a scene item as the following.  The resulting
        # scene item is not a USD scene item: it is a Python base class scene
        # item, which has a path but nothing else.  This should not a crash
        # when using the move command.
        sphereItem = ufe.SceneItem(spherePath)

        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(sphereItem)

        cmds.move(0, 10, 0, relative=True, os=True, wd=True)

    @unittest.skipUnless(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') >= '3014', 'Creating a SceneItem works in UFE v3.14')
    def testFactorySceneItem(self):
        '''A scene item can be constructed directly and will work.'''

        # Was:
        # MAYA-112601 / GitHub #1169: improperly constructed Python scene item
        # should not cause a crash.
        # Now:
        # MAYA-113409 / GutHub PR #222: Use factory CTOR for directly constructed
        # SceneItems.
        cmds.file(new=True, force=True)

        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        proxyShapePath = ufe.PathString.path(proxyShape)
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'Sphere'])

        spherePath = ufe.PathString.path('%s,/Sphere1' % proxyShape)
        sphereItem = ufe.SceneItem(spherePath)

        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(sphereItem)

        cmds.move(0, 10, 0, relative=True, os=True, wd=True)

        # The USD prim will have a transform since the move command worked:
        sphereAttrs = ufe.Attributes.attributes(sphereItem)
        self.assertIsNotNone(sphereAttrs)
        self.assertTrue("xformOp:translate" in sphereAttrs.attributeNames)

if __name__ == '__main__':
    unittest.main(verbosity=2)
