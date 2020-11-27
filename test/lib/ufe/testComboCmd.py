#!/usr/bin/env python

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

import maya.api.OpenMaya as om
import maya.cmds as cmds
from math import radians, degrees

import usdUtils, mayaUtils, ufeUtils
from testUtils import assertVectorAlmostEqual
import testTRSBase
import ufe

import unittest

from functools import partial

def transform3dScale(transform3d):
    matrix = om.MMatrix(transform3d.inclusiveMatrix().matrix)
    return om.MTransformationMatrix(matrix).scale(om.MSpace.kObject)

def transform3dRotation(transform3d):
    matrix = om.MMatrix(transform3d.inclusiveMatrix().matrix)
    return om.MTransformationMatrix(matrix).rotation()
    
def matrix4dTranslation(matrix4d):
    translation = matrix4d.matrix[-1]
    return translation[:-1]

def transform3dTranslation(transform3d):
    return matrix4dTranslation(transform3d.inclusiveMatrix())

def addVec(mayaVec, usdVec):
    return mayaVec + om.MVector(*usdVec)

def combineScales(scale1, scale2):
    return [scale1[0]*scale2[0], scale1[1]*scale2[1], scale1[2]*scale2[2] ]
    
class ComboCmdTestCase(testTRSBase.TRSTestCaseBase):
    '''Verify the Transform3d UFE interface, for multiple runtimes.

    The Maya move, rotate, and scale commands is used to test setting object
    translation, rotation, and scale.
    As of 05-May-2020, object space relative moves, rotates, and scales are
    supported by Maya code, and move and rotate are supported in world space
    as well, although scale is not supported in world space.
    Object translation, rotation, and scale is read using the Transform3d
    interface and the native run-time interface.

    This test performs a sequence of the possible types of operations, and
    verifies that the position, rotation, and scale of the object has been
    modified according to how such operations should cumulate.

    The expected value consists of the translate, rotate, and scale vectors
    (in world space). It is computed by:
        - initializing the translate, rotate, and scale vectors
        - calling updateTRS after each operation; this method will reassemble
          the transformation matrix from the three vectors, apply the
          appropriate matrix transformation for the given operation, in the
          given space, and extract the translate, rotate, and scale vector,
          once again in world space.

    When a snapshot is taken for comparison purposes, the value extracted from
    the runtime objects is extracted for each component, and assembled into a
    vector that can be compared to the computed expected value vector.
    
    UFE Feature : Transform3d
    Maya Feature : move, rotate, scale
    Action : Relative move, rotate, and scale in object space; move, rotate in
    object space.
    Applied On Selection :
        - No selection - Given node as param
        - Single Selection [Maya, Non-Maya]
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
        
        # Callables to get current object translation and rotation using the
        # run-time and UFE.
        self.move = 0
        self.rotate = 1
        self.scale = 2
        self.ops = [self.move, self.rotate, self.scale]
        self.runTimes = [None,None,None]
        self.ufes = [None,None,None]

        self.noSpace = None
        self.spaces = [om.MSpace.kObject, om.MSpace.kWorld]

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()
        
        # Create some extra Maya nodes
        cmds.polySphere()
        cmds.polyCube()
        
        # Clear selection to start off
        cmds.select(clear=True)

    def updateTRS(self, expectedTRS, op, v, space=om.MSpace.kWorld):
        '''Update the expected vector based on given operation, vector and space
        The expectedTRS vector has 9 entries:
        * 0-2 The world position
        * 3-5 The world rotation (in degrees)
        * 6-8 The world scale
        The possible operations are move, rotate, and scale.
        The possible spaces are kObject and kWorld (default)
        '''
        if( expectedTRS is None ):
            expectedTRS = [None] * 9
        # trs starts as the identity matrix
        #
        trs = om.MTransformationMatrix()

        # Add translation, rotation, and scale, in world space, to recreate
        # the last transformation matrix.
        #
        if( expectedTRS[0] is not None ):
            trs.setTranslation( om.MVector( expectedTRS[0], expectedTRS[1], expectedTRS[2]), om.MSpace.kWorld )
        if( expectedTRS[3] is not None ):
            trs.setRotation( om.MEulerRotation( radians(expectedTRS[3]), radians(expectedTRS[4]), radians(expectedTRS[5])) )
        if( expectedTRS[6] is not None ):
            trs.setScale( om.MVector( expectedTRS[6], expectedTRS[7], expectedTRS[8]), om.MSpace.kWorld )
        # Apply the requested operation. If the space is kObject, and we had a
        # scale factor, we must counteract it to get the right matrix, by
        # dividing the translation vector by it (otherwise it ends up being
        # scaled twice, and the expected value is incorrect).
        #
        if op == self.move:
            if( space == om.MSpace.kObject and expectedTRS[6] is not None):
                trs.translateBy( om.MVector( v[0]/expectedTRS[6], v[1]/expectedTRS[7], v[2]/expectedTRS[8] ), space )
            else:
                trs.translateBy( om.MVector( v[0], v[1], v[2] ), space )
        elif op == self.rotate:
            trs.rotateBy( om.MEulerRotation( radians(v[0]), radians(v[1]), radians(v[2])), space)
        elif op == self.scale:
            trs.scaleBy( om.MVector( v[0], v[1], v[2] ), space )
        # Recover the world space translate, rotate, and scale, and updated
        # the expected vector
        #
        expectedTRS[0:3] = trs.translation(om.MSpace.kWorld)
        r = trs.rotation().asVector();
        expectedTRS[3] = degrees(r[0])
        expectedTRS[4] = degrees(r[1])
        expectedTRS[5] = degrees(r[2])
        expectedTRS[6:9] = trs.scale(om.MSpace.kWorld)
        return expectedTRS

    def extractTRS(self, expectedTRS, op):
        '''Extract the move, rotate, or scale component
        '''
        if op == self.move:
            # Translation (x, y, z)
            #
            return expectedTRS[0:3]
        elif op == self.rotate:
            # Rotation vector in degrees (x, y, z)
            #
            return expectedTRS[3:6]
        elif op == self.scale:
            # Scale (x, y, z)
            #
            return expectedTRS[6:9]

    def snapshotRunTimeUFE(self):
        '''Return a pair with an op read from the run-time and from UFE.

        Tests that the op read from the run-time interface matches the
        UFE op.
        '''
        # Get translation
        #
        rtAll = None
        ufeAll = None
        offset = 0
        for op in self.ops:
            runTimeVec = self.runTimes[op]()
            ufeVec  = self.ufes[op]()
            if op == self.rotate:
                # The runtimes for rotate return an MEulerRotation, which we
                # must convert to a vector in degrees, since updateTRS expects
                # it in that format.
                #
                r = runTimeVec.asVector()
                rtAll = self.updateTRS( rtAll, op, [degrees(r[0]), degrees(r[1]), degrees(r[2])] )
                r = ufeVec.asVector()
                ufeAll = self.updateTRS( ufeAll, op, [degrees(r[0]), degrees(r[1]), degrees(r[2])] )
            else:
                rtAll = self.updateTRS( rtAll, op, runTimeVec )
                ufeAll = self.updateTRS( ufeAll, op, ufeVec )
            assertVectorAlmostEqual(self, runTimeVec, ufeVec)

        assertVectorAlmostEqual(self, rtAll, ufeAll)

        return (rtAll, ufeAll)

    def runTestCombo(self, expectedTRS):
        '''Engine method to run move, rotate, and scale test.'''

        # Save the initial values to the memento list.
        self.snapShotAndTest(expectedTRS, 6)

        # Do a combination of commands, and compare with expected.
        # Note: scale not supported in kObject space, hence, no test
        #       rotate values are in degrees
        #
        ops = [ [self.rotate,[10,20,30], om.MSpace.kObject]
              , [self.move,[4,5,6], om.MSpace.kWorld]
              , [self.move,[4,5,6], om.MSpace.kObject]
              , [self.scale,[.1,10,100], om.MSpace.kObject]
              , [self.rotate,[-10,-20,-30], om.MSpace.kWorld]
              , [self.move,[-3,-2,-1], om.MSpace.kWorld]
              , [self.scale,[1000,.01,.1], om.MSpace.kObject]
              , [self.move,[-3,-2,-1], om.MSpace.kObject]
              ]
        for item in ops:
            op = item[0]
            if( op not in self.ops ):
                continue
            v = item[1]
            space = item[2]
            if( op == self.move ):
                if( space == om.MSpace.kObject ):
                    cmds.move(v[0], v[1], v[2], relative=True, os=True, wd=True)
                else:
                    cmds.move(v[0], v[1], v[2], relative=True)
            elif( op == self.rotate ):
                if( space == om.MSpace.kObject ):
                    cmds.rotate(v[0], v[1], v[2], relative=True, os=True, forceOrderXYZ=True)
                else:
                    cmds.rotate(v[0], v[1], v[2], relative=True, ws=True, forceOrderXYZ=True)
            elif( op == self.scale ):
                if( space == om.MSpace.kObject ):
                    cmds.scale(v[0], v[1], v[2], relative=True)
                else:
                    # scale is only supported in object space; if it is
                    # eventually supported in world space, this would be the
                    # command to emit:
                    #cmds.scale(v[0], v[1], v[2], relative=True, ws=True)
                    # Fail if we attempt to test this type of operation
                    self.assertEqual( space, om.MSpace.kObject, 'scale only supported in object space' )
                    continue
            expectedTRS = self.updateTRS( expectedTRS, op, v, space )

            self.snapShotAndTest(expectedTRS, 6)

        # Test undo, redo.
        self.rewindMemento()
        self.fforwardMemento()

    def testComboMaya(self):
        '''Move, rotate, and scale Maya object, read through the Transform3d interface.''' 
        # Give the sphere an initial position, rotation, scale, and select it.
        sphereObj = om.MSelectionList().add('pSphere1').getDagPath(0).node()
        sphereFn = om.MFnTransform(sphereObj)

        expectedTRS = None
        if( self.move in self.ops ):
            expectedTRS = self.updateTRS( expectedTRS, self.move, [1,2,3] )
            t = self.extractTRS(expectedTRS,self.move)
            sphereFn.setTranslation(om.MVector( t[0], t[1], t[2] ), om.MSpace.kTransform)
        if( self.rotate in self.ops ):
            expectedTRS = self.updateTRS( expectedTRS, self.rotate, [30,60,90] )
            r = self.extractTRS(expectedTRS,self.rotate)
            sphereFn.setRotation(om.MEulerRotation(radians(r[0]),radians(r[1]),radians(r[2])), om.MSpace.kTransform)

        if( self.scale in self.ops ):
            expectedTRS = self.updateTRS( expectedTRS, self.scale, [1,2,3] )
            s = self.extractTRS(expectedTRS,self.scale)
            sphereFn.setScale(om.MVector( s[0], s[1], s[2] ) )

        spherePath = ufe.Path(mayaUtils.createUfePathSegment("|pSphere1"))
        sphereItem = ufe.Hierarchy.createItem(spherePath)

        ufe.GlobalSelection.get().append(sphereItem)

        # Create a Transform3d interface for it.
        transform3d = ufe.Transform3d.transform3d(sphereItem)

        # Set up the callables that will retrieve the translation.
        self.runTimes[self.move] = partial(
            sphereFn.translation, om.MSpace.kTransform)
        self.ufes[self.move] = partial(transform3dTranslation, transform3d)

        # Set up the callables that will retrieve the rotation.
        self.runTimes[self.rotate] = partial(
            sphereFn.rotation, om.MSpace.kTransform)
        self.ufes[self.rotate] = partial(transform3dRotation, transform3d)

        # Set up the callables that will retrieve the scale.
        self.runTimes[self.scale] = partial(
            sphereFn.scale)
        self.ufes[self.scale] = partial(transform3dScale, transform3d)

        self.runTestCombo(expectedTRS)

    def testComboUSD(self):
        '''Move, rotate, and scale USD object, read through the Transform3d interface.'''

        # Select Ball_35 to move, rotate, and scale it.
        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment("|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)

        ufe.GlobalSelection.get().append(ball35Item)

        # Create a Transform3d interface for it.
        transform3d = ufe.Transform3d.transform3d(ball35Item)

        # We compare the UFE ops with the USD run-time ops.  To
        # obtain the full ops of Ball_35, we need to add the USD
        # ops to the Maya proxy shape ops.
        proxyShapeXformObj = om.MSelectionList().add('transform1').getDagPath(0).node()
        proxyShapeXformFn = om.MFnTransform(proxyShapeXformObj)

        def ball35Translation():
            ball35Prim = usdUtils.getPrimFromSceneItem(ball35Item)
            return addVec(
                proxyShapeXformFn.translation(om.MSpace.kTransform),
                ball35Prim.GetAttribute('xformOp:translate').Get())

        def ball35Rotation():
            ball35Prim = usdUtils.getPrimFromSceneItem(ball35Item)
            if not ball35Prim.HasAttribute('xformOp:rotateXYZ'):
                return proxyShapeXformFn.rotation(om.MSpace.kTransform)
            else:
                x,y,z = ball35Prim.GetAttribute('xformOp:rotateXYZ').Get()
                return proxyShapeXformFn.rotation(om.MSpace.kTransform) + om.MEulerRotation(radians(x), radians(y), radians(z))

        def ball35Scale():
            ball35Prim = usdUtils.getPrimFromSceneItem(ball35Item)
            if not ball35Prim.HasAttribute('xformOp:scale'):
                return proxyShapeXformFn.scale()
            else:
                return combineScales(proxyShapeXformFn.scale(), ball35Prim.GetAttribute('xformOp:scale').Get())
            
        # Set up the callables that will retrieve the translation.
        self.runTimes[self.move] = ball35Translation
        self.ufes[self.move] = partial(transform3dTranslation, transform3d)

        # Set up the callables that will retrieve the rotation.
        self.runTimes[self.rotate] = ball35Rotation
        self.ufes[self.rotate] = partial(transform3dRotation, transform3d)

        # Set up the callables that will retrieve the scale.
        self.runTimes[self.scale] = ball35Scale
        self.ufes[self.scale] = partial(transform3dScale, transform3d)

        # Save the initial position to the memento list.
        expectedTRS = None
        if( self.move in self.ops ):
            v = ball35Translation()
            expectedTRS = self.updateTRS( expectedTRS, self.move, [v[0], v[1], v[2]] )
        if( self.rotate in self.ops ):
            r = ball35Rotation().asVector()
            expectedTRS = self.updateTRS( expectedTRS, self.rotate, [degrees(r[0]), degrees(r[1]), degrees(r[2])] )

        if( self.scale in self.ops ):
            s = ball35Scale()
            expectedTRS = self.updateTRS( expectedTRS, self.scale, [s[0], s[1], s[2]] )

        self.runTestCombo(expectedTRS)

