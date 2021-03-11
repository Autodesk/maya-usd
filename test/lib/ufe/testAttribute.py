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
import ufeUtils
import testUtils
import usdUtils

from pxr import UsdGeom

from maya import cmds
from maya import standalone
from maya.internal.ufeSupport import ufeCmdWrapper as ufeCmd

import ufe

import os
import random
import unittest


class TestObserver(ufe.Observer):
    def __init__(self):
        super(TestObserver, self).__init__()
        self._notifications = 0

    def __call__(self, notification):
        if (ufeUtils.ufeFeatureSetVersion() >= 2):
            if isinstance(notification, ufe.AttributeValueChanged):
                self._notifications += 1
        else:
            if isinstance(notification, ufe.AttributeChanged):
                self._notifications += 1

    @property
    def notifications(self):
        return self._notifications

class AttributeTestCase(unittest.TestCase):
    '''Verify the Attribute UFE interface, for multiple runtimes.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

        random.seed()

    @classmethod
    def tearDownClass(cls):
        # See comments in MayaUFEPickWalkTesting.tearDownClass
        cmds.file(new=True, force=True)

        standalone.uninitialize()

    def setUp(self):
        '''Called initially to set up the maya test environment'''
        self.assertTrue(self.pluginsLoaded)

    def assertVectorAlmostEqual(self, ufeVector, usdVector):
        testUtils.assertVectorAlmostEqual(
            self, ufeVector.vector, usdVector)

    def assertColorAlmostEqual(self, ufeColor, usdColor):
        for va, vb in zip(ufeColor.color, usdColor):
            self.assertAlmostEqual(va, vb, places=6)

    def runUndoRedo(self, attr, newVal, decimalPlaces=None):
        oldVal = attr.get()
        assert oldVal != newVal, "Undo / redo testing requires setting a value different from the current value"

        ufeCmd.execute(attr.setCmd(newVal))

        if decimalPlaces is not None:
            self.assertAlmostEqual(attr.get(), newVal, decimalPlaces)
            newVal = attr.get()
        else:
            self.assertEqual(attr.get(), newVal)

        cmds.undo()
        self.assertEqual(attr.get(), oldVal)
        cmds.redo()
        self.assertEqual(attr.get(), newVal)

    def runTestAttribute(self, path, attrName, ufeAttrClass, ufeAttrType):
        '''Engine method to run attribute test.'''

        # Create the UFE/USD attribute for this test from the input path.

        # Get a UFE scene item the input path in the scene.
        itemPath = ufe.Path([
            mayaUtils.createUfePathSegment("|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment(path)])
        ufeItem = ufe.Hierarchy.createItem(itemPath)

        # Get the USD prim for this item.
        usdPrim = usdUtils.getPrimFromSceneItem(ufeItem)

        # Create the attributes interface for the item.
        ufeAttrs = ufe.Attributes.attributes(ufeItem)
        self.assertIsNotNone(ufeAttrs)

        # Get the USDAttribute for the input attribute name so we can use it to
        # compare to UFE.
        usdAttr = usdPrim.GetAttribute(attrName)
        self.assertIsNotNone(usdAttr)

        # Get the attribute that matches the input name and make sure it matches
        # the class type of UFE attribute class passed in.
        self.assertTrue(ufeAttrs.hasAttribute(attrName))
        ufeAttr = ufeAttrs.attribute(attrName)
        self.assertIsInstance(ufeAttr, ufeAttrClass)

        # Verify that the attribute type matches the input UFE type.
        self.assertEqual(ufeAttr.type, ufeAttrType)

        # Verify that the scene item the attribute was created with matches
        # what is stored in the UFE attribute.
        self.assertEqual(ufeAttr.sceneItem(), ufeItem)

        # Verify that this attribute has a value. Note: all the attributes that
        # are tested by this method are assumed to have a value.
        self.assertTrue(ufeAttr.hasValue())

        # Verify that the name matched what we created the attribute from.
        self.assertEqual(ufeAttr.name, attrName)

        # Test that the string representation of the value is not empty.
        self.assertTrue(str(ufeAttr))

        return ufeAttr, usdAttr

    def testAttributeGeneric(self):
        '''Test the Generic attribute type.'''

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the xformOpOrder attribute which is
        # an unsupported USD type, so it will be a UFE Generic attribute.
        ufeAttr, usdAttr = attrDict = self.runTestAttribute(
            path='/Room_set/Props/Ball_35',
            attrName=UsdGeom.Tokens.xformOpOrder,
            ufeAttrClass=ufe.AttributeGeneric,
            ufeAttrType=ufe.Attribute.kGeneric)

        # Now we test the Generic specific methods.
        self.assertEqual(ufeAttr.nativeType(), usdAttr.GetTypeName().type.typeName)

    def testAttributeEnumString(self):
        '''Test the EnumString attribute type.'''

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the visibility attribute which is
        # an EnumString type.
        ufeAttr, usdAttr = attrDict = self.runTestAttribute(
            path='/Room_set/Props/Ball_35',
            attrName=UsdGeom.Tokens.visibility,
            ufeAttrClass=ufe.AttributeEnumString,
            ufeAttrType=ufe.Attribute.kEnumString)

        # Now we test the EnumString specific methods.

        # Compare the initial UFE value to that directly from USD.
        self.assertEqual(ufeAttr.get(), usdAttr.Get())

        # Make sure 'inherited' is in the list of allowed tokens.
        visEnumValues = ufeAttr.getEnumValues()
        self.assertIn(UsdGeom.Tokens.inherited, visEnumValues)

        # Change to 'invisible' and verify the return in UFE.
        ufeAttr.set(UsdGeom.Tokens.invisible)
        self.assertEqual(ufeAttr.get(), UsdGeom.Tokens.invisible)

        # Verify that the new UFE value matches what is directly in USD.
        self.assertEqual(ufeAttr.get(), usdAttr.Get())

        # Change back to 'inherited' using a command.
        self.runUndoRedo(ufeAttr, UsdGeom.Tokens.inherited)

    def testAttributeBool(self):
        '''Test the Bool attribute type.'''

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the visibility attribute which is
        # an bool type.
        ufeAttr, usdAttr = attrDict = self.runTestAttribute(
            path='/Room_set/Props/Ball_35/mesh',
            attrName='doubleSided',
            ufeAttrClass=ufe.AttributeBool,
            ufeAttrType=ufe.Attribute.kBool)

        # Now we test the Bool specific methods.

        # Compare the initial UFE value to that directly from USD.
        self.assertEqual(ufeAttr.get(), usdAttr.Get())

        # Set the attribute in UFE with the opposite boolean value.
        ufeAttr.set(not ufeAttr.get())

        # Then make sure that new UFE value matches what it in USD.
        self.assertEqual(ufeAttr.get(), usdAttr.Get())

        self.runUndoRedo(ufeAttr, not ufeAttr.get())

    def testAttributeInt(self):
        '''Test the Int attribute type.'''

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the visibility attribute which is
        # an integer type.
        ufeAttr, usdAttr = attrDict = self.runTestAttribute(
            path='/Room_set/Props/Ball_35/Looks/BallLook/Base',
            attrName='inputAOV',
            ufeAttrClass=ufe.AttributeInt,
            ufeAttrType=ufe.Attribute.kInt)

        # Now we test the Int specific methods.

        # Compare the initial UFE value to that directly from USD.
        self.assertEqual(ufeAttr.get(), usdAttr.Get())

        # Set the attribute in UFE with a different int value.
        ufeAttr.set(ufeAttr.get() + random.randint(1,5))

        # Then make sure that new UFE value matches what it in USD.
        self.assertEqual(ufeAttr.get(), usdAttr.Get())

        self.runUndoRedo(ufeAttr, ufeAttr.get()+1)

    def testAttributeFloat(self):
        '''Test the Float attribute type.'''

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the visibility attribute which is
        # an float type.
        ufeAttr, usdAttr = attrDict = self.runTestAttribute(
            path='/Room_set/Props/Ball_35/Looks/BallLook/Base',
            attrName='anisotropic',
            ufeAttrClass=ufe.AttributeFloat,
            ufeAttrType=ufe.Attribute.kFloat)

        # Now we test the Float specific methods.

        # Compare the initial UFE value to that directly from USD.
        self.assertEqual(ufeAttr.get(), usdAttr.Get())

        # Set the attribute in UFE with a different float value.
        ufeAttr.set(random.random())

        # Then make sure that new UFE value matches what it in USD.
        self.assertEqual(ufeAttr.get(), usdAttr.Get())

        # Python floating-point numbers are doubles.  If stored in a float
        # attribute, the resulting precision will be less than the original
        # Python value.
        self.runUndoRedo(ufeAttr, ufeAttr.get() + 1.0, decimalPlaces=6)

    def _testAttributeDouble(self):
        '''Test the Double attribute type.'''

        # I could not find an double attribute to test with
        pass

    def testAttributeStringString(self):
        '''Test the String (String) attribute type.'''

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the visibility attribute which is
        # an string type.
        ufeAttr, usdAttr = attrDict = self.runTestAttribute(
            path='/Room_set/Props/Ball_35/Looks/BallLook/BallTexture',
            attrName='filename',
            ufeAttrClass=ufe.AttributeString,
            ufeAttrType=ufe.Attribute.kString)

        # Now we test the String specific methods.

        # Compare the initial UFE value to that directly from USD.
        self.assertEqual(ufeAttr.get(), usdAttr.Get())

        # Set the attribute in UFE with a different string value.
        # Note: this ball uses the ball8.tex
        ufeAttr.set('./tex/ball7.tex')

        # Then make sure that new UFE value matches what it in USD.
        self.assertEqual(ufeAttr.get(), usdAttr.Get())

        self.runUndoRedo(ufeAttr, 'potato')

    def testAttributeStringToken(self):
        '''Test the String (Token) attribute type.'''

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the visibility attribute which is
        # an string type.
        ufeAttr, usdAttr = attrDict = self.runTestAttribute(
            path='/Room_set/Props/Ball_35/Looks/BallLook/BallTexture',
            attrName='filter',
            ufeAttrClass=ufe.AttributeString,
            ufeAttrType=ufe.Attribute.kString)

        # Now we test the String specific methods.

        # Compare the initial UFE value to that directly from USD.
        self.assertEqual(ufeAttr.get(), usdAttr.Get())

        # Set the attribute in UFE with a different string value.
        # Note: this attribute is initially set to token 'Box'
        ufeAttr.set('Sphere')

        # Then make sure that new UFE value matches what it in USD.
        self.assertEqual(ufeAttr.get(), usdAttr.Get())

        self.runUndoRedo(ufeAttr, 'Box')

    def testAttributeColorFloat3(self):
        '''Test the ColorFloat3 attribute type.'''

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the visibility attribute which is
        # an ColorFloat3 type.
        ufeAttr, usdAttr = attrDict = self.runTestAttribute(
            path='/Room_set/Props/Ball_35/Looks/BallLook/Base',
            attrName='emitColor',
            ufeAttrClass=ufe.AttributeColorFloat3,
            ufeAttrType=ufe.Attribute.kColorFloat3)

        # Now we test the ColorFloat3 specific methods.

        # Compare the initial UFE value to that directly from USD.
        self.assertColorAlmostEqual(ufeAttr.get(), usdAttr.Get())

        # Set the attribute in UFE with some random color values.
        vec = ufe.Color3f(random.random(), random.random(), random.random())
        ufeAttr.set(vec)

        # Then make sure that new UFE value matches what it in USD.
        self.assertColorAlmostEqual(ufeAttr.get(), usdAttr.Get())

        # The following causes a segmentation fault on CentOS 7.
        # self.runUndoRedo(ufeAttr,
        #                  ufe.Color3f(vec.r()+1.0, vec.g()+2.0, vec.b()+3.0))
        # Entered as MAYA-102168.
        newVec = ufe.Color3f(vec.color[0]+1.0, vec.color[1]+2.0, vec.color[2]+3.0)
        self.runUndoRedo(ufeAttr, newVec)

    def _testAttributeInt3(self):
        '''Test the Int3 attribute type.'''

        # I could not find an int3 attribute to test with.
        pass

    def testAttributeFloat3(self):
        '''Test the Float3 attribute type.'''

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the visibility attribute which is
        # an Float3 type.
        ufeAttr, usdAttr = attrDict = self.runTestAttribute(
            path='/Room_set/Props/Ball_35/Looks/BallLook/Base',
            attrName='bumpNormal',
            ufeAttrClass=ufe.AttributeFloat3,
            ufeAttrType=ufe.Attribute.kFloat3)

        # Now we test the Float3 specific methods.

        # Compare the initial UFE value to that directly from USD.
        self.assertVectorAlmostEqual(ufeAttr.get(), usdAttr.Get())

        # Set the attribute in UFE with some random values.
        vec = ufe.Vector3f(random.random(), random.random(), random.random())
        ufeAttr.set(vec)

        # Then make sure that new UFE value matches what it in USD.
        self.assertVectorAlmostEqual(ufeAttr.get(), usdAttr.Get())

        self.runUndoRedo(ufeAttr,
                         ufe.Vector3f(vec.x()+1.0, vec.y()+2.0, vec.z()+3.0))

    def testAttributeDouble3(self):
        '''Test the Double3 attribute type.'''

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the visibility attribute which is
        # an Double3 type.
        ufeAttr, usdAttr = attrDict = self.runTestAttribute(
            path='/Room_set/Props/Ball_35',
            attrName='xformOp:translate',
            ufeAttrClass=ufe.AttributeDouble3,
            ufeAttrType=ufe.Attribute.kDouble3)

        # Now we test the Double3 specific methods.

        # Compare the initial UFE value to that directly from USD.
        self.assertVectorAlmostEqual(ufeAttr.get(), usdAttr.Get())

        # Set the attribute in UFE with some random values.
        vec = ufe.Vector3d(random.uniform(-100, 100), random.uniform(-100, 100), random.uniform(-100, 100))
        ufeAttr.set(vec)

        # Then make sure that new UFE value matches what it in USD.
        self.assertVectorAlmostEqual(ufeAttr.get(), usdAttr.Get())

        self.runUndoRedo(ufeAttr,
                         ufe.Vector3d(vec.x()-1.0, vec.y()-2.0, vec.z()-3.0))

    def testObservation(self):
        '''Test Attributes observation interface.

        Test both global attribute observation and per-node attribute
        observation.
        '''

        # Create three observers, one for global attribute observation, and two
        # on different UFE items.
        proxyShapePathSegment = mayaUtils.createUfePathSegment(
            "|transform1|proxyShape1")
        path = ufe.Path([
            proxyShapePathSegment, 
            usdUtils.createUfePathSegment('/Room_set/Props/Ball_34')])
        ball34 = ufe.Hierarchy.createItem(path)
        path = ufe.Path([
            proxyShapePathSegment, 
            usdUtils.createUfePathSegment('/Room_set/Props/Ball_35')])
        ball35 = ufe.Hierarchy.createItem(path)
        
        (ball34Obs, ball35Obs, globalObs) = [TestObserver() for i in range(3)]

        # Maya registers a single global observer on startup.
        self.assertEqual(ufe.Attributes.nbObservers(), 1)

        # No item-specific observers.
        self.assertFalse(ufe.Attributes.hasObservers(ball34.path()))
        self.assertFalse(ufe.Attributes.hasObservers(ball35.path()))
        self.assertEqual(ufe.Attributes.nbObservers(ball34), 0)
        self.assertEqual(ufe.Attributes.nbObservers(ball35), 0)
        self.assertFalse(ufe.Attributes.hasObserver(ball34, ball34Obs))
        self.assertFalse(ufe.Attributes.hasObserver(ball35, ball35Obs))

        # No notifications yet.
        self.assertEqual(ball34Obs.notifications, 0)
        self.assertEqual(ball35Obs.notifications, 0)
        self.assertEqual(globalObs.notifications, 0)

        # Add a global observer.
        ufe.Attributes.addObserver(globalObs)

        self.assertEqual(ufe.Attributes.nbObservers(), 2)
        self.assertFalse(ufe.Attributes.hasObservers(ball34.path()))
        self.assertFalse(ufe.Attributes.hasObservers(ball35.path()))
        self.assertEqual(ufe.Attributes.nbObservers(ball34), 0)
        self.assertEqual(ufe.Attributes.nbObservers(ball35), 0)
        self.assertFalse(ufe.Attributes.hasObserver(ball34, ball34Obs))
        self.assertFalse(ufe.Attributes.hasObserver(ball35, ball35Obs))

        # Add item-specific observers.
        ufe.Attributes.addObserver(ball34, ball34Obs)

        self.assertEqual(ufe.Attributes.nbObservers(), 2)
        self.assertTrue(ufe.Attributes.hasObservers(ball34.path()))
        self.assertFalse(ufe.Attributes.hasObservers(ball35.path()))
        self.assertEqual(ufe.Attributes.nbObservers(ball34), 1)
        self.assertEqual(ufe.Attributes.nbObservers(ball35), 0)
        self.assertTrue(ufe.Attributes.hasObserver(ball34, ball34Obs))
        self.assertFalse(ufe.Attributes.hasObserver(ball34, ball35Obs))
        self.assertFalse(ufe.Attributes.hasObserver(ball35, ball35Obs))

        ufe.Attributes.addObserver(ball35, ball35Obs)

        self.assertTrue(ufe.Attributes.hasObservers(ball35.path()))
        self.assertEqual(ufe.Attributes.nbObservers(ball34), 1)
        self.assertEqual(ufe.Attributes.nbObservers(ball35), 1)
        self.assertTrue(ufe.Attributes.hasObserver(ball35, ball35Obs))
        self.assertFalse(ufe.Attributes.hasObserver(ball35, ball34Obs))

        # Make a change to ball34, global and ball34 observers change.
        ball34Attrs = ufe.Attributes.attributes(ball34)
        ball34XlateAttr = ball34Attrs.attribute('xformOp:translate')

        self.assertEqual(ball34Obs.notifications, 0)

        ufeCmd.execute(ball34XlateAttr.setCmd(ufe.Vector3d(1, 2, 3)))

        self.assertEqual(ball34Obs.notifications, 1)
        self.assertEqual(ball35Obs.notifications, 0)
        self.assertEqual(globalObs.notifications, 1)

        # Undo, redo
        cmds.undo()

        self.assertEqual(ball34Obs.notifications, 2)
        self.assertEqual(ball35Obs.notifications, 0)
        self.assertEqual(globalObs.notifications, 2)

        cmds.redo()

        self.assertEqual(ball34Obs.notifications, 3)
        self.assertEqual(ball35Obs.notifications, 0)
        self.assertEqual(globalObs.notifications, 3)

        # Make a change to ball35, global and ball35 observers change.
        ball35Attrs = ufe.Attributes.attributes(ball35)
        ball35XlateAttr = ball35Attrs.attribute('xformOp:translate')

        ufeCmd.execute(ball35XlateAttr.setCmd(ufe.Vector3d(1, 2, 3)))

        self.assertEqual(ball34Obs.notifications, 3)
        self.assertEqual(ball35Obs.notifications, 1)
        self.assertEqual(globalObs.notifications, 4)

        # Undo, redo
        cmds.undo()

        self.assertEqual(ball34Obs.notifications, 3)
        self.assertEqual(ball35Obs.notifications, 2)
        self.assertEqual(globalObs.notifications, 5)

        cmds.redo()

        self.assertEqual(ball34Obs.notifications, 3)
        self.assertEqual(ball35Obs.notifications, 3)
        self.assertEqual(globalObs.notifications, 6)

        # Test removeObserver.
        ufe.Attributes.removeObserver(ball34, ball34Obs)

        self.assertFalse(ufe.Attributes.hasObservers(ball34.path()))
        self.assertTrue(ufe.Attributes.hasObservers(ball35.path()))
        self.assertEqual(ufe.Attributes.nbObservers(ball34), 0)
        self.assertEqual(ufe.Attributes.nbObservers(ball35), 1)
        self.assertFalse(ufe.Attributes.hasObserver(ball34, ball34Obs))

        ufeCmd.execute(ball34XlateAttr.setCmd(ufe.Vector3d(4, 5, 6)))

        self.assertEqual(ball34Obs.notifications, 3)
        self.assertEqual(ball35Obs.notifications, 3)
        self.assertEqual(globalObs.notifications, 7)

        ufe.Attributes.removeObserver(globalObs)

        self.assertEqual(ufe.Attributes.nbObservers(), 1)

        ufeCmd.execute(ball34XlateAttr.setCmd(ufe.Vector3d(7, 8, 9)))

        self.assertEqual(ball34Obs.notifications, 3)
        self.assertEqual(ball35Obs.notifications, 3)
        self.assertEqual(globalObs.notifications, 7)

    # Run last to avoid file new disturbing other tests.
    def testZAttrChangeRedoAfterPrimCreateRedo(self):
        '''Redo attribute change after redo of prim creation.'''
        cmds.file(new=True, force=True)

        # Create a capsule, change one of its attributes.
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path(proxyShape)
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        cmd = proxyShapeContextOps.doOpCmd(['Add New Prim', 'Capsule'])
        ufeCmd.execute(cmd)

        capsulePath = ufe.PathString.path('%s,/Capsule1' % proxyShape)
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)

        # Create the attributes interface for the item.
        attrs = ufe.Attributes.attributes(capsuleItem)
        self.assertIsNotNone(attrs)
        self.assertTrue(attrs.hasAttribute('radius'))
        radiusAttr = attrs.attribute('radius')

        oldRadius = radiusAttr.get()

        ufeCmd.execute(radiusAttr.setCmd(2))
        
        newRadius = radiusAttr.get()

        self.assertEqual(newRadius, 2)
        self.assertNotEqual(oldRadius, newRadius)

        # Undo 2x: undo attr change and prim creation.
        cmds.undo()
        cmds.undo()

        # Redo 2x: prim creation, attr change.
        cmds.redo()
        cmds.redo()

        # Re-create item, as its underlying prim was re-created.
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)
        attrs = ufe.Attributes.attributes(capsuleItem)
        radiusAttr = attrs.attribute('radius')
        
        self.assertEqual(radiusAttr.get(), newRadius)


if __name__ == '__main__':
    unittest.main(verbosity=2)
