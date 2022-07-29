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

from pxr import Usd, UsdGeom, Vt, Gf

from maya import cmds
from maya import standalone
from maya.internal.ufeSupport import ufeCmdWrapper as ufeCmd

import mayaUsd
from mayaUsd import ufe as mayaUsdUfe
from mayaUsd import lib as mayaUsdLib

import ufe

import ast
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

class TestObserver_4_24(ufe.Observer):
    """This advanced observer listens to notifications that appeared in UFE 0.4.24"""
    def __init__(self):
        super(TestObserver_4_24, self).__init__()
        self._addedNotifications = 0
        self._removedNotifications = 0
        self._valueChangedNotifications = 0
        self._connectionChangedNotifications = 0
        self._unknownNotifications = 0

    def __call__(self, notification):
        if isinstance(notification, ufe.AttributeAdded):
            self._addedNotifications += 1
        elif isinstance(notification, ufe.AttributeRemoved):
            self._removedNotifications += 1
        elif isinstance(notification, ufe.AttributeValueChanged):
            self._valueChangedNotifications += 1
        elif isinstance(notification, ufe.AttributeConnectionChanged):
            self._connectionChangedNotifications += 1
        else:
            self._unknownNotifications += 1

    def assertNotificationCount(self, test, **counters):
        test.assertEqual(self._addedNotifications, counters.get("numAdded", 0))
        test.assertEqual(self._removedNotifications, counters.get("numRemoved", 0))
        test.assertEqual(self._valueChangedNotifications, counters.get("numValue", 0))
        test.assertEqual(self._connectionChangedNotifications, counters.get("numConnection", 0))
        test.assertEqual(self._unknownNotifications, 0)

class AttributeTestCase(unittest.TestCase):
    '''Verify the Attribute UFE interface, for multiple runtimes.
    '''

    pluginsLoaded = False

    # Note: these variables are computed in the setUpClass method. Therefore
    #       you cannot use it in a test function decorator (as its value
    #       has not been computed yet).
    _getAttrSupportsUfe = False
    _setAttrSupportsUfe = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

        random.seed()

        # Test if Maya's getAttr command supports ufe.
        if cls.pluginsLoaded:
            import mayaUsd_createStageWithNewLayer
            psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
            stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
            stage.DefinePrim('/A', 'Xform')
            try:
                cmds.getAttr('|stage1|stageShape1,/A.visibility')
                cls._getAttrSupportsUfe = True
            except:
                _getAttrSupportsUfe = False

            # Maya's setAttr command was only made Ufe aware in Maya PR129 and Maya 2022.3
            # But we'll test it the same way as getAttr that way the condition never needs to updated.
            try:
                cmds.setAttr('|stage1|stageShape1,/A.visibility', UsdGeom.Tokens.invisible)
                cls._setAttrSupportsUfe = True
            except:
                _setAttrSupportsUfe = False

            # Cleanup for all tests.
            cmds.file(new=True, force=True)

    @classmethod
    def tearDownClass(cls):
        # See comments in MayaUFEPickWalkTesting.tearDownClass
        cmds.file(new=True, force=True)

        standalone.uninitialize()

    def setUp(self):
        '''Called initially to set up the maya test environment'''
        self.assertTrue(self.pluginsLoaded)

    @classmethod
    def openMaterialXSamplerStage(cls):
        '''Open MtlxValueTypes stage in testSamples. This stage can be freely loaded even if
           the underlying USD does not support MaterialX since we are not rendering that scene.
           We also rename the stage to match the hierarchy of the top_layer scene which is
           hardcoded in some tests.'''
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene("MaterialX", "MtlxValueTypes.usda")
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)
        cmds.rename(shapeNode, "proxyShape1")
        cmds.rename("|"+shapeNode.split("|")[1], "transform1")

    def assertMatrixAlmostEqual(self, ufeMatrix, usdMatrix):
        testUtils.assertMatrixAlmostEqual(
            self, ufeMatrix.matrix, usdMatrix)

    def assertVectorAlmostEqual(self, ufeVector, usdVector):
        testUtils.assertVectorAlmostEqual(
            self, ufeVector.vector, usdVector)

    def assertColorAlmostEqual(self, ufeColor, usdColor):
        for va, vb in zip(ufeColor.color, usdColor):
            self.assertAlmostEqual(va, vb, places=6)

    def getMayaAttrStr(self, ufeAttr):
        # Get the string we need for using the Ufe attribute with Maya command:
        # "<ufe_path_string>.<ufe_attribute_name>"
        return '%s.%s' % (ufe.PathString.string(ufeAttr.sceneItem().path()), ufeAttr.name)

    @property
    def getAttrSupportsUfe(self):
        # Variable is set during setUpClass().
        return self._getAttrSupportsUfe

    @property
    def setAttrSupportsUfe(self):
        # Variable is set during setUpClass().
        return self._setAttrSupportsUfe

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

    def runUndoRedoUsingMayaSetAttr(self, attr, newVal, decimalPlaces=None):
        if not self.setAttrSupportsUfe:
            return

        oldVal = attr.get()
        assert oldVal != newVal, "Undo / redo testing requires setting a value different from the current value"

        setAttrPath = self.getMayaAttrStr(attr)
        if hasattr(newVal, "vector"):
            cmds.setAttr(setAttrPath, *newVal.vector)
        elif hasattr(newVal, "color"):
            cmds.setAttr(setAttrPath, *newVal.color)
        elif hasattr(newVal, "matrix"):
            # Flatten the matrix for Maya:
            cmds.setAttr(setAttrPath, *[i for row in newVal.matrix for i in row])
        else:
            cmds.setAttr(setAttrPath, newVal)

        if decimalPlaces is not None:
            self.assertAlmostEqual(attr.get(), newVal, decimalPlaces)
            newVal = attr.get()
        else:
            self.assertEqual(attr.get(), newVal)

        cmds.undo()
        self.assertEqual(attr.get(), oldVal)
        cmds.redo()
        self.assertEqual(attr.get(), newVal)

    def runMayaGetAttrTest(self, ufeAttr, decimalPlaces=None):
        # Not all versions of Maya support getAttr with Ufe attributes.
        if not self.getAttrSupportsUfe:
            return

        getAttrPath = self.getMayaAttrStr(ufeAttr)

        ufeAttrType = ufeAttr.type
        cmds.select(getAttrPath.partition('.')[0])
        self.assertTrue(cmds.getAttr(getAttrPath, settable=True))
        self.assertFalse(cmds.getAttr(getAttrPath, lock=True))
        self.assertEqual(ufeAttrType, cmds.getAttr(getAttrPath, type=True))

        ufeVectorTypes = {ufe.Attribute.kColorFloat3 : ufe.Color3f,
                          ufe.Attribute.kInt3 : ufe.Vector3i,
                          ufe.Attribute.kFloat3 : ufe.Vector3f,
                          ufe.Attribute.kDouble3 : ufe.Vector3d
                          }

        if hasattr(ufe.Attribute, "kColorFloat4"):
            ufeVectorTypes.update({
                ufe.Attribute.kColorFloat4 : ufe.Color4f,
                ufe.Attribute.kFloat2 : ufe.Vector2f,
                ufe.Attribute.kFloat4 : ufe.Vector4f,
                ufe.Attribute.kMatrix3d : ufe.Matrix3d,
                ufe.Attribute.kMatrix4d : ufe.Matrix4d
            })

        if ufeAttrType == ufe.Attribute.kGeneric:
            self.assertEqual(cmds.getAttr(getAttrPath), str(ufeAttr))
        elif ufeAttrType in ufeVectorTypes:
            getAttrValue = cmds.getAttr(getAttrPath)
            # Pre Ufe 0.4.15: Maya might return the result as a string for colors. Fixed to always
            # return a vector post 0.4.15.
            if isinstance(getAttrValue, str):
                getAttrValue = ast.literal_eval(getAttrValue)
            if hasattr(ufe.Attribute, "kColorFloat4"):
                # Ufe post 0.4.15 can init vector/matrix types with sequences directly:
                self.assertEqual(ufeVectorTypes[ufeAttrType](getAttrValue), ufeAttr.get())
            else:
                # Before 0.4.15 we need to splat the value:
                self.assertEqual(ufeVectorTypes[ufeAttrType](*getAttrValue), ufeAttr.get())
        else:
            if decimalPlaces is not None:
                self.assertAlmostEqual(cmds.getAttr(getAttrPath), ufeAttr.get(), decimalPlaces)
            else:
                self.assertEqual(cmds.getAttr(getAttrPath), ufeAttr.get())

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

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the xformOpOrder attribute which is
        # an unsupported USD type, so it will be a UFE Generic attribute.
        ufeAttr, usdAttr = self.runTestAttribute(
            path='/Room_set/Props/Ball_35',
            attrName=UsdGeom.Tokens.xformOpOrder,
            ufeAttrClass=ufe.AttributeGeneric,
            ufeAttrType=ufe.Attribute.kGeneric)

        # Now we test the Generic specific methods.
        self.assertEqual(ufeAttr.nativeType(), usdAttr.GetTypeName().type.typeName)

        # Run test using Maya's getAttr command.
        self.runMayaGetAttrTest(ufeAttr)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4015', 'Test only available in UFE preview version 0.4.15 and greater')
    def testAttributeFilename(self):
        '''Test the Filename attribute type.'''

        AttributeTestCase.openMaterialXSamplerStage()

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the emitColor attribute which is
        # an ColorFloat3 type.
        ufeAttr, usdAttr = self.runTestAttribute(
            path='/TypeSampler/MaterialX/D_filename',
            attrName='inputs:in',
            ufeAttrClass=ufe.AttributeFilename,
            ufeAttrType=ufe.Attribute.kFilename)

        # Now we test the Filename specific methods.

        # Compare the initial UFE value to that directly from USD.
        self.assertEqual(ufeAttr.get(), usdAttr.Get())

        # Change to 'blue.png' and verify the return in UFE.
        ufeAttr.set("blue.png")
        self.assertEqual(ufeAttr.get(), "blue.png")

        # Verify that the new UFE value matches what is directly in USD.
        self.assertEqual(ufeAttr.get(), usdAttr.Get())

        # Change back to 'red.png' using a command.
        self.runUndoRedo(ufeAttr, "red.png")

        # Run test using Maya's setAttr command.
        self.runUndoRedoUsingMayaSetAttr(ufeAttr, "green.png")

        # Run test using Maya's getAttr command.
        self.runMayaGetAttrTest(ufeAttr)

    def testAttributeEnumString(self):
        '''Test the EnumString attribute type.'''

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the visibility attribute which is
        # an EnumString type.
        ufeAttr, usdAttr = self.runTestAttribute(
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

        # Run test using Maya's setAttr command.
        self.runUndoRedoUsingMayaSetAttr(ufeAttr, UsdGeom.Tokens.invisible)

        # Run test using Maya's getAttr command.
        self.runMayaGetAttrTest(ufeAttr)

    def testAttributeBool(self):
        '''Test the Bool attribute type.'''

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the doubleSided attribute which is
        # an bool type.
        ufeAttr, usdAttr = self.runTestAttribute(
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

        # Run test using Maya's setAttr command.
        self.runUndoRedoUsingMayaSetAttr(ufeAttr, not ufeAttr.get())

        # Run test using Maya's getAttr command.
        self.runMayaGetAttrTest(ufeAttr)

    def testAttributeInt(self):
        '''Test the Int attribute type.'''

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the inputAOV attribute which is
        # an integer type.
        ufeAttr, usdAttr = self.runTestAttribute(
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

        # Run test using Maya's setAttr command.
        self.runUndoRedoUsingMayaSetAttr(ufeAttr, ufeAttr.get()+1)

        # Run test using Maya's getAttr command.
        self.runMayaGetAttrTest(ufeAttr)

    def testAttributeFloat(self):
        '''Test the Float attribute type.'''

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the anisotropic attribute which is
        # an float type.
        ufeAttr, usdAttr = self.runTestAttribute(
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

        # Run test using Maya's setAttr command.
        self.runUndoRedoUsingMayaSetAttr(ufeAttr, ufeAttr.get()+1.0, decimalPlaces=6)

        # Run test using Maya's getAttr command.
        self.runMayaGetAttrTest(ufeAttr, decimalPlaces=6)

    def _testAttributeDouble(self):
        '''Test the Double attribute type.'''

        # I could not find an double attribute to test with
        pass

    def testAttributeStringString(self):
        '''Test the String (String) attribute type.'''

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the filename attribute which is
        # an string type.
        ufeAttr, usdAttr = self.runTestAttribute(
            path='/Room_set/Props/Ball_35/Looks/BallLook/BallTexture',
            attrName='filename',
            ufeAttrClass=ufe.AttributeString,
            ufeAttrType=ufe.Attribute.kString)

        # Now we test the String specific methods.

        # Compare the initial UFE value to that directly from USD.
        self.assertEqual(ufeAttr.get(), usdAttr.Get())

        # check to see if the attribute edit is allowed

        # Set the attribute in UFE with a different string value.
        # Note: this ball uses the ball8.tex
        ufeAttr.set('./tex/ball7.tex')

        # Then make sure that new UFE value matches what it in USD.
        self.assertEqual(ufeAttr.get(), usdAttr.Get())

        self.runUndoRedo(ufeAttr, 'potato')

        # Note: Maya's setAttr command does not support Ufe string attributes.

        # Run test using Maya's getAttr command.
        self.runMayaGetAttrTest(ufeAttr)

    def testAttributeStringToken(self):
        '''Test the String (Token) attribute type.'''

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the filter attribute which is
        # an string type.
        ufeAttr, usdAttr = self.runTestAttribute(
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

        # Note: Maya's setAttr command does not support Ufe string attributes.

        # Run test using Maya's getAttr command.
        self.runMayaGetAttrTest(ufeAttr)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') >= '4015', 'Test not available in UFE preview version 0.4.15 and greater')
    def testAttributeColorFloat3(self):
        '''Test the ColorFloat3 attribute type.'''

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the emitColor attribute which is
        # an ColorFloat3 type.
        ufeAttr, usdAttr = self.runTestAttribute(
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

        # Note: Maya's setAttr command does not support Ufe Color3 attributes.

        # Run test using Maya's getAttr command.
        self.runMayaGetAttrTest(ufeAttr)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4015', 'Test only available in UFE preview version 0.4.15 and greater')
    def testAttributeColorFloat3(self):
        '''Test the ColorFloat3 attribute type.'''

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the emitColor attribute which is
        # an ColorFloat3 type.
        ufeAttr, usdAttr = self.runTestAttribute(
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
        self.runUndoRedo(ufeAttr,
                         ufe.Color3f(vec.r()+1.0, vec.g()+2.0, vec.b()+3.0))

        # Run test using Maya's setAttr command.
        vec = ufeAttr.get()
        self.runUndoRedoUsingMayaSetAttr(ufeAttr, 
                         ufe.Color3f(vec.r()+1.0, vec.g()+2.0, vec.b()+3.0))

        # Run test using Maya's getAttr command.
        self.runMayaGetAttrTest(ufeAttr)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4015', 'Test only available in UFE preview version 0.4.15 and greater')
    def testAttributeColorFloat4(self):
        '''Test the ColorFloat4 attribute type.'''

        AttributeTestCase.openMaterialXSamplerStage()

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the emitColor attribute which is
        # an ColorFloat3 type.
        ufeAttr, usdAttr = self.runTestAttribute(
            path='/TypeSampler/MaterialX/TXT',
            attrName='inputs:default',
            ufeAttrClass=ufe.AttributeColorFloat4,
            ufeAttrType=ufe.Attribute.kColorFloat4)

        # Now we test the ColorFloat4 specific methods.

        # Compare the initial UFE value to that directly from USD.
        self.assertColorAlmostEqual(ufeAttr.get(), usdAttr.Get())

        # Set the attribute in UFE with some random color values.
        vec = ufe.Color4f(random.random(), random.random(), random.random(), random.random())
        ufeAttr.set(vec)

        # Then make sure that new UFE value matches what it in USD.
        self.assertColorAlmostEqual(ufeAttr.get(), usdAttr.Get())
        self.runUndoRedo(ufeAttr,
                         ufe.Color4f(vec.r()+1.0, vec.g()+2.0, vec.b()+3.0, vec.a()+4.0))

        # Run test using Maya's setAttr command.
        vec = ufeAttr.get()
        self.runUndoRedoUsingMayaSetAttr(ufeAttr, 
                         ufe.Color4f(vec.r()+1.0, vec.g()+2.0, vec.b()+3.0, vec.a()+4.0))

        # Run test using Maya's getAttr command.
        self.runMayaGetAttrTest(ufeAttr)

    def _testAttributeInt3(self):
        '''Test the Int3 attribute type.'''

        # I could not find an int3 attribute to test with.
        pass

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4015', 'Test only available in UFE preview version 0.4.15 and greater')
    def testAttributeFloat2(self):
        '''Test the Float2 attribute type.'''

        AttributeTestCase.openMaterialXSamplerStage()

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the bumpNormal attribute which is
        # an Float2 type.
        ufeAttr, usdAttr = self.runTestAttribute(
            path='/TypeSampler/MaterialX/D_vector2',
            attrName='inputs:in',
            ufeAttrClass=ufe.AttributeFloat2,
            ufeAttrType=ufe.Attribute.kFloat2)

        # Now we test the Float2 specific methods.

        # Compare the initial UFE value to that directly from USD.
        self.assertVectorAlmostEqual(ufeAttr.get(), usdAttr.Get())

        # Set the attribute in UFE with some random values.
        vec = ufe.Vector2f(random.random(), random.random())
        ufeAttr.set(vec)

        # Then make sure that new UFE value matches what it in USD.
        self.assertVectorAlmostEqual(ufeAttr.get(), usdAttr.Get())
        self.runUndoRedo(ufeAttr,
                         ufe.Vector2f(vec.x()+1.0, vec.y()+2.0))

        # Run test using Maya's setAttr command.
        vec = ufeAttr.get()
        self.runUndoRedoUsingMayaSetAttr(ufeAttr, 
                         ufe.Vector2f(vec.x()+1.0, vec.y()+2.0))

        # Run test using Maya's getAttr command.
        self.runMayaGetAttrTest(ufeAttr)

    def testAttributeFloat3(self):
        '''Test the Float3 attribute type.'''

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the bumpNormal attribute which is
        # an Float3 type.
        ufeAttr, usdAttr = self.runTestAttribute(
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

        # Run test using Maya's setAttr command.
        vec = ufeAttr.get()
        self.runUndoRedoUsingMayaSetAttr(ufeAttr, 
                         ufe.Vector3f(vec.x()+1.0, vec.y()+2.0, vec.z()+3.0))

        # Run test using Maya's getAttr command.
        self.runMayaGetAttrTest(ufeAttr)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4015', 'Test only available in UFE preview version 0.4.15 and greater')
    def testAttributeFloat4(self):
        '''Test the Float4 attribute type.'''

        AttributeTestCase.openMaterialXSamplerStage()

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the bumpNormal attribute which is
        # an Float4 type.
        ufeAttr, usdAttr = self.runTestAttribute(
            path='/TypeSampler/MaterialX/D_vector4',
            attrName='inputs:in',
            ufeAttrClass=ufe.AttributeFloat4,
            ufeAttrType=ufe.Attribute.kFloat4)

        # Now we test the Float4 specific methods.

        # Compare the initial UFE value to that directly from USD.
        self.assertVectorAlmostEqual(ufeAttr.get(), usdAttr.Get())

        # Set the attribute in UFE with some random values.
        vec = ufe.Vector4f(random.random(), random.random(),
                           random.random(), random.random())
        ufeAttr.set(vec)

        # Then make sure that new UFE value matches what it in USD.
        self.assertVectorAlmostEqual(ufeAttr.get(), usdAttr.Get())
        self.runUndoRedo(ufeAttr,
                         ufe.Vector4f(vec.x()+1.0, vec.y()+2.0,
                                      vec.z()+3.0, vec.w()+4.0))

        # Run test using Maya's setAttr command.
        vec = ufeAttr.get()
        self.runUndoRedoUsingMayaSetAttr(ufeAttr, 
                         ufe.Vector4f(vec.x()+1.0, vec.y()+2.0,
                                      vec.z()+3.0, vec.w()+4.0))

        # Run test using Maya's getAttr command.
        self.runMayaGetAttrTest(ufeAttr)

    def testAttributeDouble3(self):
        '''Test the Double3 attribute type.'''

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the translate attribute which is
        # an Double3 type.
        ufeAttr, usdAttr = self.runTestAttribute(
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

        # Run test using Maya's setAttr command.
        self.runUndoRedoUsingMayaSetAttr(ufeAttr, ufe.Vector3d(5.5, 6.6, 7.7))

        # Run test using Maya's getAttr command.
        self.runMayaGetAttrTest(ufeAttr)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4015', 'Test only available in UFE preview version 0.4.15 and greater')
    def testAttributeMatrix3d(self):
        '''Test the Matrix3d attribute type.'''

        AttributeTestCase.openMaterialXSamplerStage()

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the bumpNormal attribute which is
        # an Matrix3d type.
        ufeAttr, usdAttr = self.runTestAttribute(
            path='/TypeSampler/MaterialX/D_matrix33',
            attrName='inputs:in',
            ufeAttrClass=ufe.AttributeMatrix3d,
            ufeAttrType=ufe.Attribute.kMatrix3d)

        # Now we test the Matrix3d specific methods.

        # Compare the initial UFE value to that directly from USD.
        self.assertMatrixAlmostEqual(ufeAttr.get(), usdAttr.Get())

        # Set the attribute in UFE with some random values.
        mat = [[random.random() for i in range(3)] for j in range(3)]
        ufeAttr.set(ufe.Matrix3d(mat))

        # Then make sure that new UFE value matches what it in USD.
        self.assertMatrixAlmostEqual(ufeAttr.get(), usdAttr.Get())
        self.runUndoRedo(ufeAttr,
            ufe.Matrix3d([[mat[i][j] + mat[i][j] for j in range(3)] for i in range(3)]))

        # Run test using Maya's setAttr command.
        mat = ufeAttr.get().matrix
        self.runUndoRedoUsingMayaSetAttr(ufeAttr, 
            ufe.Matrix3d([[mat[i][j] + mat[i][j] for j in range(3)] for i in range(3)]))

        # Run test using Maya's getAttr command.
        self.runMayaGetAttrTest(ufeAttr)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4015', 'Test only available in UFE preview version 0.4.15 and greater')
    def testAttributeMatrix4d(self):
        '''Test the Matrix4d attribute type.'''

        AttributeTestCase.openMaterialXSamplerStage()

        # Use our engine method to run the bulk of the test (all the stuff from
        # the Attribute base class). We use the bumpNormal attribute which is
        # an Matrix4d type.
        ufeAttr, usdAttr = self.runTestAttribute(
            path='/TypeSampler/MaterialX/D_matrix44',
            attrName='inputs:in',
            ufeAttrClass=ufe.AttributeMatrix4d,
            ufeAttrType=ufe.Attribute.kMatrix4d)

        # Now we test the Matrix4d specific methods.

        # Compare the initial UFE value to that directly from USD.
        self.assertMatrixAlmostEqual(ufeAttr.get(), usdAttr.Get())

        # Set the attribute in UFE with some random values.
        mat = [[random.random() for i in range(4)] for j in range(4)]
        ufeAttr.set(ufe.Matrix4d(mat))

        # Then make sure that new UFE value matches what it in USD.
        self.assertMatrixAlmostEqual(ufeAttr.get(), usdAttr.Get())
        self.runUndoRedo(ufeAttr,
            ufe.Matrix4d([[mat[i][j] + mat[i][j] for j in range(4)] for i in range(4)]))

        # Run test using Maya's setAttr command.
        mat = ufeAttr.get().matrix
        self.runUndoRedoUsingMayaSetAttr(ufeAttr, 
            ufe.Matrix4d([[mat[i][j] + mat[i][j] for j in range(4)] for i in range(4)]))

        # Run test using Maya's getAttr command.
        self.runMayaGetAttrTest(ufeAttr)

    @unittest.skipUnless(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4024', 'Test for UFE preview version 0.4.23 and earlier')
    def testObservation(self):
        '''
        Test Attributes observation interface.

        Test both global attribute observation and per-node attribute
        observation.
        '''

        # Start we a clean scene so we can get a consistent number of notifications
        mayaUtils.openTopLayerScene()

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
        # Maya-Usd lib registers a single global observer when it is initialized.
        kNbGlobalObs = 2
        self.assertEqual(ufe.Attributes.nbObservers(), kNbGlobalObs)

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

        self.assertEqual(ufe.Attributes.nbObservers(), kNbGlobalObs+1)
        self.assertFalse(ufe.Attributes.hasObservers(ball34.path()))
        self.assertFalse(ufe.Attributes.hasObservers(ball35.path()))
        self.assertEqual(ufe.Attributes.nbObservers(ball34), 0)
        self.assertEqual(ufe.Attributes.nbObservers(ball35), 0)
        self.assertFalse(ufe.Attributes.hasObserver(ball34, ball34Obs))
        self.assertFalse(ufe.Attributes.hasObserver(ball35, ball35Obs))

        # Add item-specific observers.
        ufe.Attributes.addObserver(ball34, ball34Obs)

        self.assertEqual(ufe.Attributes.nbObservers(), kNbGlobalObs+1)
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

        # The first modification adds a new spec to ball_34 & its ancestors
        # "Props" and "Room_set". Ufe should be filtering out those notifications
        # so the global observer should still only see one notification.
        ufeCmd.execute(ball34XlateAttr.setCmd(ufe.Vector3d(4, 4, 15)))
        self.assertEqual(ball34Obs.notifications, 1)
        self.assertEqual(ball35Obs.notifications, 0)
        self.assertEqual(globalObs.notifications, 1)

        # The second modification only sends one USD notification for "xformOps:translate"
        # because all the spec's already exist. Ufe should also see one notification.
        ufeCmd.execute(ball34XlateAttr.setCmd(ufe.Vector3d(4, 4, 20)))
        self.assertEqual(ball34Obs.notifications, 2)
        self.assertEqual(ball35Obs.notifications, 0)
        self.assertEqual(globalObs.notifications, 2)

        # Undo, redo
        cmds.undo()
        self.assertEqual(ball34Obs.notifications, 3)
        self.assertEqual(ball35Obs.notifications, 0)
        self.assertEqual(globalObs.notifications, 3)

        cmds.redo()
        self.assertEqual(ball34Obs.notifications, 4)
        self.assertEqual(ball35Obs.notifications, 0)
        self.assertEqual(globalObs.notifications, 4)

        # get ready to undo the first modification
        cmds.undo()
        self.assertEqual(ball34Obs.notifications, 5)
        self.assertEqual(ball35Obs.notifications, 0)
        self.assertEqual(globalObs.notifications, 5)

        # Undo-ing the modification which created the USD specs is a little
        # different in USD, but from Ufe we should just still see one notification.
        cmds.undo()
        self.assertEqual(ball34Obs.notifications, 6)
        self.assertEqual(ball35Obs.notifications, 0)
        self.assertEqual(globalObs.notifications, 6)

        cmds.redo()
        self.assertEqual(ball34Obs.notifications, 7)
        self.assertEqual(ball35Obs.notifications, 0)
        self.assertEqual(globalObs.notifications, 7)

        # Make a change to ball35, global and ball35 observers change.
        ball35Attrs = ufe.Attributes.attributes(ball35)
        ball35XlateAttr = ball35Attrs.attribute('xformOp:translate')

        # "xformOp:translate"
        ufeCmd.execute(ball35XlateAttr.setCmd(ufe.Vector3d(4, 8, 15)))
        self.assertEqual(ball34Obs.notifications, 7)
        self.assertEqual(ball35Obs.notifications, 1)
        self.assertEqual(globalObs.notifications, 8)

        # Undo, redo
        cmds.undo()
        self.assertEqual(ball34Obs.notifications, 7)
        self.assertEqual(ball35Obs.notifications, 2)
        self.assertEqual(globalObs.notifications, 9)

        cmds.redo()
        self.assertEqual(ball34Obs.notifications, 7)
        self.assertEqual(ball35Obs.notifications, 3)
        self.assertEqual(globalObs.notifications, 10)

        # Test removeObserver.
        ufe.Attributes.removeObserver(ball34, ball34Obs)

        self.assertFalse(ufe.Attributes.hasObservers(ball34.path()))
        self.assertTrue(ufe.Attributes.hasObservers(ball35.path()))
        self.assertEqual(ufe.Attributes.nbObservers(ball34), 0)
        self.assertEqual(ufe.Attributes.nbObservers(ball35), 1)
        self.assertFalse(ufe.Attributes.hasObserver(ball34, ball34Obs))

        ufeCmd.execute(ball34XlateAttr.setCmd(ufe.Vector3d(4, 4, 25)))

        self.assertEqual(ball34Obs.notifications, 7)
        self.assertEqual(ball35Obs.notifications, 3)
        self.assertEqual(globalObs.notifications, 11)

        ufe.Attributes.removeObserver(globalObs)

        self.assertEqual(ufe.Attributes.nbObservers(), kNbGlobalObs)

        ufeCmd.execute(ball34XlateAttr.setCmd(ufe.Vector3d(7, 8, 9)))

        self.assertEqual(ball34Obs.notifications, 7)
        self.assertEqual(ball35Obs.notifications, 3)
        self.assertEqual(globalObs.notifications, 11)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4024', 'Test for UFE preview version 0.4.24 and later')
    def testObservation(self):
        '''
        Test Attributes observation interface.

        Test both global attribute observation and per-node attribute
        observation.
        '''

        # Start we a clean scene so we can get a consistent number of notifications
        mayaUtils.openTopLayerScene()

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
        
        (ball34Obs, ball35Obs, globalObs) = [TestObserver_4_24() for i in range(3)]

        # Maya registers a single global observer on startup.
        # Maya-Usd lib registers a single global observer when it is initialized.
        kNbGlobalObs = 2
        self.assertEqual(ufe.Attributes.nbObservers(), kNbGlobalObs)

        # No item-specific observers.
        self.assertFalse(ufe.Attributes.hasObservers(ball34.path()))
        self.assertFalse(ufe.Attributes.hasObservers(ball35.path()))
        self.assertEqual(ufe.Attributes.nbObservers(ball34), 0)
        self.assertEqual(ufe.Attributes.nbObservers(ball35), 0)
        self.assertFalse(ufe.Attributes.hasObserver(ball34, ball34Obs))
        self.assertFalse(ufe.Attributes.hasObserver(ball35, ball35Obs))

        # No notifications yet.
        ball34Obs.assertNotificationCount(self)
        ball35Obs.assertNotificationCount(self)
        globalObs.assertNotificationCount(self)

        # Add a global observer.
        ufe.Attributes.addObserver(globalObs)

        self.assertEqual(ufe.Attributes.nbObservers(), kNbGlobalObs+1)
        self.assertFalse(ufe.Attributes.hasObservers(ball34.path()))
        self.assertFalse(ufe.Attributes.hasObservers(ball35.path()))
        self.assertEqual(ufe.Attributes.nbObservers(ball34), 0)
        self.assertEqual(ufe.Attributes.nbObservers(ball35), 0)
        self.assertFalse(ufe.Attributes.hasObserver(ball34, ball34Obs))
        self.assertFalse(ufe.Attributes.hasObserver(ball35, ball35Obs))

        # Add item-specific observers.
        ufe.Attributes.addObserver(ball34, ball34Obs)

        self.assertEqual(ufe.Attributes.nbObservers(), kNbGlobalObs+1)
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

        ball34Obs.assertNotificationCount(self)

        # The first modification adds a new spec to ball_34 & its ancestors
        # "Props" and "Room_set". Ufe should be filtering out those notifications
        # so the global observer should still only see one notification.
        ufeCmd.execute(ball34XlateAttr.setCmd(ufe.Vector3d(4, 4, 15)))
        ball34Obs.assertNotificationCount(self, numAdded = 1, numValue = 1)
        ball35Obs.assertNotificationCount(self)
        globalObs.assertNotificationCount(self, numAdded = 1, numValue = 1)

        # The second modification only sends one USD notification for "xformOps:translate"
        # because all the spec's already exist. Ufe should also see one notification.
        ufeCmd.execute(ball34XlateAttr.setCmd(ufe.Vector3d(4, 4, 20)))
        ball34Obs.assertNotificationCount(self, numAdded = 1, numValue = 2)
        ball35Obs.assertNotificationCount(self)
        globalObs.assertNotificationCount(self, numAdded = 1, numValue = 2)

        # Undo, redo
        cmds.undo()
        ball34Obs.assertNotificationCount(self, numAdded = 1, numValue = 3)
        ball35Obs.assertNotificationCount(self)
        globalObs.assertNotificationCount(self, numAdded = 1, numValue = 3)

        cmds.redo()
        ball34Obs.assertNotificationCount(self, numAdded = 1, numValue = 4)
        ball35Obs.assertNotificationCount(self)
        globalObs.assertNotificationCount(self, numAdded = 1, numValue = 4)

        # get ready to undo the first modification
        cmds.undo()
        ball34Obs.assertNotificationCount(self, numAdded = 1, numValue = 5)
        ball35Obs.assertNotificationCount(self)
        globalObs.assertNotificationCount(self, numAdded = 1, numValue = 5)

        # Undo-ing the modification which created the USD specs is a little
        # different in USD, but from Ufe we should just still see one notification.
        cmds.undo()
        ball34Obs.assertNotificationCount(self, numAdded = 1, numRemoved = 1, numValue = 5)
        ball35Obs.assertNotificationCount(self)
        globalObs.assertNotificationCount(self, numAdded = 1, numRemoved = 1, numValue = 5)

        cmds.redo()
        # Note that UsdUndoHelper add an attribute with its value in one shot, which results in a
        # single AttributeAdded notification:
        ball34Obs.assertNotificationCount(self, numAdded = 2, numRemoved = 1, numValue = 5)
        ball35Obs.assertNotificationCount(self)
        globalObs.assertNotificationCount(self, numAdded = 2, numRemoved = 1, numValue = 5)

        # Make a change to ball35, global and ball35 observers change.
        ball35Attrs = ufe.Attributes.attributes(ball35)
        ball35XlateAttr = ball35Attrs.attribute('xformOp:translate')

        # "xformOp:translate"
        ufeCmd.execute(ball35XlateAttr.setCmd(ufe.Vector3d(4, 8, 15)))
        ball34Obs.assertNotificationCount(self, numAdded = 2, numRemoved = 1, numValue = 5)
        ball35Obs.assertNotificationCount(self, numAdded = 1, numValue = 1)
        globalObs.assertNotificationCount(self, numAdded = 3, numRemoved = 1, numValue = 6)

        # Undo, redo
        cmds.undo()
        ball34Obs.assertNotificationCount(self, numAdded = 2, numRemoved = 1, numValue = 5)
        ball35Obs.assertNotificationCount(self, numAdded = 1, numRemoved = 1, numValue = 1)
        globalObs.assertNotificationCount(self, numAdded = 3, numRemoved = 2, numValue = 6)

        cmds.redo()
        ball34Obs.assertNotificationCount(self, numAdded = 2, numRemoved = 1, numValue = 5)
        ball35Obs.assertNotificationCount(self, numAdded = 2, numRemoved = 1, numValue = 1)
        globalObs.assertNotificationCount(self, numAdded = 4, numRemoved = 2, numValue = 6)

        # Test removeObserver.
        ufe.Attributes.removeObserver(ball34, ball34Obs)

        self.assertFalse(ufe.Attributes.hasObservers(ball34.path()))
        self.assertTrue(ufe.Attributes.hasObservers(ball35.path()))
        self.assertEqual(ufe.Attributes.nbObservers(ball34), 0)
        self.assertEqual(ufe.Attributes.nbObservers(ball35), 1)
        self.assertFalse(ufe.Attributes.hasObserver(ball34, ball34Obs))

        ufeCmd.execute(ball34XlateAttr.setCmd(ufe.Vector3d(4, 4, 25)))

        ball34Obs.assertNotificationCount(self, numAdded = 2, numRemoved = 1, numValue = 5)
        ball35Obs.assertNotificationCount(self, numAdded = 2, numRemoved = 1, numValue = 1)
        globalObs.assertNotificationCount(self, numAdded = 4, numRemoved = 2, numValue = 7)

        ufe.Attributes.removeObserver(globalObs)

        self.assertEqual(ufe.Attributes.nbObservers(), kNbGlobalObs)

        ufeCmd.execute(ball34XlateAttr.setCmd(ufe.Vector3d(7, 8, 9)))

        ball34Obs.assertNotificationCount(self, numAdded = 2, numRemoved = 1, numValue = 5)
        ball35Obs.assertNotificationCount(self, numAdded = 2, numRemoved = 1, numValue = 1)
        globalObs.assertNotificationCount(self, numAdded = 4, numRemoved = 2, numValue = 7)

    def testAttrChangeRedoAfterPrimCreateRedo(self):
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

    def testSingleAttributeBlocking(self):
        ''' Authoring attribute(s) in weaker layer(s) are not permitted if there exist opinion(s) in stronger layer(s).'''

        # create new stage
        cmds.file(new=True, force=True)
        import mayaUsd_createStageWithNewLayer
        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        proxyShapePath = proxyShapes[0]
        proxyShapeItem = ufe.Hierarchy.createItem(ufe.PathString.path(proxyShapePath))
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()

        # create a prim. This creates the primSpec in the root layer.
        proxyShapeContextOps.doOp(['Add New Prim', 'Capsule'])

        # create a USD prim
        capsulePath = ufe.PathString.path('|stage1|stageShape1,/Capsule1')
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)
        capsulePrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(capsulePath))

        # author a new radius value
        self.assertTrue(capsulePrim.HasAttribute('radius'))
        radiusAttrUsd = capsulePrim.GetAttribute('radius')

        # authoring new attribute edit is expected to be allowed.
        self.assertTrue(mayaUsdUfe.isAttributeEditAllowed(radiusAttrUsd))
        radiusAttrUsd.Set(10)

        # author a new axis value
        self.assertTrue(capsulePrim.HasAttribute('axis'))
        axisAttrUsd = capsulePrim.GetAttribute('axis')

        # authoring new attribute edit is expected to be allowed.
        self.assertTrue(mayaUsdUfe.isAttributeEditAllowed(axisAttrUsd))
        axisAttrUsd.Set('Y')

        # create a sub-layer.
        rootLayer = stage.GetRootLayer()
        subLayerA = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="LayerA")[0]

        # check to see the if the sublayers was added
        addedLayers = [subLayerA]
        self.assertEqual(rootLayer.subLayerPaths, addedLayers)

        # set the edit target to LayerA.
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerA)

        # radiusAttrUsd is not allowed to change since there is an opinion in a stronger layer
        self.assertFalse(mayaUsdUfe.isAttributeEditAllowed(radiusAttrUsd))

        # axisAttrUsd is not allowed to change since there is an opinion in a stronger layer
        self.assertFalse(mayaUsdUfe.isAttributeEditAllowed(axisAttrUsd))

    def testTransformationAttributeBlocking(self):
        '''Authoring transformation attribute(s) in weaker layer(s) are not permitted if there exist opinion(s) in stronger layer(s).'''

        # create new stage
        cmds.file(new=True, force=True)
        import mayaUsd_createStageWithNewLayer
        mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        proxyShapePath = proxyShapes[0]
        proxyShapeItem = ufe.Hierarchy.createItem(ufe.PathString.path(proxyShapePath))
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()

        # create 3 sub-layers ( LayerA, LayerB, LayerC ) and set the edit target to LayerB.
        rootLayer = stage.GetRootLayer()
        subLayerC = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="SubLayerC")[0]
        subLayerB = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="SubLayerB")[0]
        subLayerA = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="SubLayerA")[0]

        # check to see the sublayers added
        addedLayers = [subLayerA, subLayerB, subLayerC]
        self.assertEqual(rootLayer.subLayerPaths, addedLayers)

        # set the edit target to LayerB
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerB)

        # create a prim. This creates the primSpec in the SubLayerB.
        proxyShapeContextOps.doOp(['Add New Prim', 'Sphere'])

        spherePath = ufe.PathString.path('|stage1|stageShape1,/Sphere1')
        sphereItem = ufe.Hierarchy.createItem(spherePath)
        spherePrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(spherePath))

        # create a transform3d
        sphereT3d = ufe.Transform3d.transform3d(sphereItem)

        # create a xformable
        sphereXformable = UsdGeom.Xformable(spherePrim)

        # writing to "transform op order" is expected to be allowed.
        self.assertTrue(mayaUsdUfe.isAttributeEditAllowed(sphereXformable.GetXformOpOrderAttr()))

        # do any transform editing.
        sphereT3d = ufe.Transform3d.transform3d(sphereItem)
        sphereT3d.scale(2.5, 2.5, 2.5)
        sphereT3d.translate(4.0, 2.0, 3.0)

        # check the "transform op order" stack.
        self.assertEqual(sphereXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray(('xformOp:translate', 'xformOp:scale')))

        # check if translate attribute is editable
        translateAttr = spherePrim.GetAttribute('xformOp:translate')
        self.assertIsNotNone(translateAttr)

        # authoring new transformation edit is expected to be allowed.
        self.assertTrue(mayaUsdUfe.isAttributeEditAllowed(translateAttr))
        sphereT3d.translate(5.0, 6.0, 7.0)
        self.assertEqual(translateAttr.Get(), Gf.Vec3d(5.0, 6.0, 7.0))

        # set the edit target to a weaker layer (LayerC)
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerC)

        # authoring new transformation edit is not allowed.
        self.assertFalse(mayaUsdUfe.isAttributeEditAllowed(translateAttr))

        # set the edit target to a stronger layer (LayerA)
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerA)

        # authoring new transformation edit is allowed.
        self.assertTrue(mayaUsdUfe.isAttributeEditAllowed(translateAttr))
        sphereT3d.rotate(0.0, 90.0, 0.0)

        # check the "transform op order" stack.
        self.assertEqual(sphereXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray(('xformOp:translate','xformOp:rotateXYZ', 'xformOp:scale')))

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 3, 'testMetadata is only available in UFE v3 or greater.')
    def testMetadata(self):
        '''Test attribute metadata.'''
        cmds.file(new=True, force=True)

        # Create a capsule.
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
        attr = attrs.attribute('xformOpOrder')

        # Test for non-existing metdata.
        self.assertFalse(attr.hasMetadata('NotAMetadata'))

        # Verify that this attribute has documentation metadata.
        self.assertTrue(attr.hasMetadata('documentation'))
        md = attr.getMetadata('documentation')
        self.assertIsNotNone(md)
        self.assertIsNotNone(str(md))

        # Store this original documentation metadata value for testing of the clear.
        # Note: USD doc has some fallback values, so clearing doc might not actually
        #       set it to empty.
        origDocMD = str(md)

        # Change the metadata and make sure it changed
        self.assertTrue(attr.setMetadata('documentation', 'New doc'))
        self.assertTrue(attr.hasMetadata('documentation'))
        md = attr.getMetadata('documentation')
        self.assertEqual('New doc', str(md))

        # Clear the metadata and make sure it is not equal to what we set it above.
        self.assertTrue(attr.clearMetadata('documentation'))
        md = attr.getMetadata('documentation')
        self.assertEqual(str(md), origDocMD)

        def runMetadataUndoRedo(func, newValue, startValue=None):
            '''Helper to run metadata command on input value with undo/redo test.'''

            # Clear the metadata and make sure it is gone.
            if startValue:
                self.assertTrue(attr.setMetadata('documentation', startValue))
                self.assertFalse(attr.getMetadata('documentation').empty())
            else:
                self.assertTrue(attr.clearMetadata('documentation'))
                md = attr.getMetadata('documentation')
                self.assertEqual(str(md), origDocMD)

            # Set metadata using command.
            cmd = attr.setMetadataCmd('documentation', newValue)
            self.assertIsNotNone(cmd)
            cmd.execute()
            md = attr.getMetadata('documentation')
            self.assertEqual(ufe.Value(newValue), md)
            self.assertEqual(newValue, func(md))

            # Test undo/redo.
            cmd.undo()
            md = attr.getMetadata('documentation')
            if startValue:
                self.assertEqual(ufe.Value(startValue), md)
                self.assertEqual(startValue, func(md))
            else:
                self.assertEqual(str(md), origDocMD)
            cmd.redo()
            md = attr.getMetadata('documentation')
            self.assertEqual(ufe.Value(newValue), md)
            self.assertEqual(newValue, func(md))

        # Set the metadata using the command and verify undo/redo.
        # We test all the known ufe.Value types.
        # First with empty metadata and then with a starting metadata.
        runMetadataUndoRedo(bool, True)
        runMetadataUndoRedo(int, 10)
        runMetadataUndoRedo(float, 65.78)
        runMetadataUndoRedo(str, 'New doc from command')

        runMetadataUndoRedo(bool, True, False)
        runMetadataUndoRedo(int, 10, 2)
        runMetadataUndoRedo(float, 65.78, 0.567)
        runMetadataUndoRedo(str, 'New doc from command', 'New doc starting value')

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 3, 'testAttributeLock is only available in UFE v3 or greater.')
    def testAttributeLock(self):
        '''Test attribute lock/unlock.'''
        cmds.file(new=True, force=True)

        # Create a capsule.
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
        attr = attrs.attribute('height')

        getsetAttrPath = self.getMayaAttrStr(attr)

        # Height attribute should not initially be locked.
        self.assertFalse(attr.hasMetadata(ufe.Attribute.kLocked))
        if self.getAttrSupportsUfe:
            self.assertTrue(cmds.getAttr(getsetAttrPath, settable=True))
            self.assertFalse(cmds.getAttr(getsetAttrPath, lock=True))

        #----------------------------------------------------------------------
        # Test locking modifications to the height using Ufe kLocked key.
        #----------------------------------------------------------------------
        self.assertTrue(attr.setMetadata(ufe.Attribute.kLocked, True))
        self.assertTrue(attr.hasMetadata(ufe.Attribute.kLocked))
        md = attr.getMetadata(ufe.Attribute.kLocked)
        self.assertTrue(bool(md))
        if self.getAttrSupportsUfe:
            self.assertFalse(cmds.getAttr(getsetAttrPath, settable=True))
            self.assertTrue(cmds.getAttr(getsetAttrPath, lock=True))

        # Test that we cannot modify the height now.
        currValue = attr.get()
        self.assertRaises(RuntimeError, attr.set, currValue + 1.0)
        self.assertEqual(currValue, attr.get())
        if self.setAttrSupportsUfe:
            self.assertRaises(RuntimeError, cmds.setAttr, getsetAttrPath, currValue+1.0)

        # Test that we cannot set other Metadata now.
        self.assertRaises(RuntimeError, attr.setMetadata, 'documentation', 'New Doc')

        # Test unlocking modifications to height.
        self.assertTrue(attr.setMetadata(ufe.Attribute.kLocked, False))
        md = attr.getMetadata(ufe.Attribute.kLocked)
        self.assertFalse(bool(md))
        if self.getAttrSupportsUfe:
            self.assertTrue(cmds.getAttr(getsetAttrPath, settable=True))
            self.assertFalse(cmds.getAttr(getsetAttrPath, lock=True))

        # Changes to height should now be allowed.
        currValue = attr.get()
        newValue = currValue + 1.0
        attr.set(newValue)
        self.assertEqual(newValue, attr.get())
        if self.setAttrSupportsUfe:
            # Same test with setAttr command.
            currValue = attr.get()
            newValue = currValue + 1.0
            cmds.setAttr(getsetAttrPath, newValue)
            self.assertEqual(newValue, attr.get())

        # Changes to other Metadata should be allowed now.
        self.assertTrue(attr.setMetadata('documentation', 'New Doc'))

        # Remove the locking metadata.
        self.assertTrue(attr.clearMetadata(ufe.Attribute.kLocked))
        self.assertFalse(attr.hasMetadata(ufe.Attribute.kLocked))

        #----------------------------------------------------------------------
        # Test locking modifications to the height using command.
        #----------------------------------------------------------------------
        cmd = attr.setMetadataCmd(ufe.Attribute.kLocked, True)
        self.assertIsNotNone(cmd)
        cmd.execute()
        self.assertTrue(attr.hasMetadata(ufe.Attribute.kLocked))
        md = attr.getMetadata(ufe.Attribute.kLocked)
        self.assertTrue(bool(md))
        if self.getAttrSupportsUfe:
            self.assertFalse(cmds.getAttr(getsetAttrPath, settable=True))
            self.assertTrue(cmds.getAttr(getsetAttrPath, lock=True))

        # Test that we cannot modify the height now.
        currValue = attr.get()
        self.assertRaises(RuntimeError, attr.set, currValue + 1.0)
        self.assertEqual(currValue, attr.get())
        if self.setAttrSupportsUfe:
            self.assertRaises(RuntimeError, cmds.setAttr, getsetAttrPath, currValue+1.0)

        # Test that we cannot set other Metadata now.
        self.assertRaises(RuntimeError, attr.setMetadata, 'documentation', 'New Doc')

        # Test undo/redo.
        cmd.undo()
        self.assertFalse(attr.hasMetadata(ufe.Attribute.kLocked))

        # Changes to height should now be allowed.
        currValue = attr.get()
        newValue = currValue + 1.0
        attr.set(currValue + 1.0)
        self.assertEqual(newValue, attr.get())
        if self.getAttrSupportsUfe:
            self.assertTrue(cmds.getAttr(getsetAttrPath, settable=True))
            self.assertFalse(cmds.getAttr(getsetAttrPath, lock=True))

        # Changes to other Metadata should be allowed now.
        self.assertTrue(attr.setMetadata('documentation', 'New Doc'))

        cmd.redo()
        self.assertTrue(attr.hasMetadata(ufe.Attribute.kLocked))
        md = attr.getMetadata(ufe.Attribute.kLocked)
        self.assertTrue(bool(md))
        if self.getAttrSupportsUfe:
            self.assertFalse(cmds.getAttr(getsetAttrPath, settable=True))
            self.assertTrue(cmds.getAttr(getsetAttrPath, lock=True))

        # Test that we cannot modify the height again.
        currValue = attr.get()
        self.assertRaises(RuntimeError, attr.set, currValue + 1.0)
        self.assertEqual(currValue, attr.get())
        if self.setAttrSupportsUfe:
            self.assertRaises(RuntimeError, cmds.setAttr, getsetAttrPath, currValue+1.0)

        # Test that we cannot set other Metadata again.
        self.assertRaises(RuntimeError, attr.setMetadata, 'documentation', 'New Doc')

    def testMayaGetAttr(self):
        if not self.getAttrSupportsUfe:
            self.skipTest('Maya getAttr command does not support Ufe')

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

        pathStr = '|transform1|proxyShape1,/Room_set/Props/Ball_35'

        # No attribute was specified.
        self.assertRaisesRegex(RuntimeError,
                               'No attribute was specified\.$',
                               cmds.getAttr, pathStr)

        # Cannot evaluate more than one attribute.
        self.assertRaisesRegex(RuntimeError,
                               'Cannot evaluate more than one attribute\.$',
                               cmds.getAttr,
                               pathStr+'.xformOp:translate',
                               pathStr+'.xformOpOrder')

        # Mixing Maya and non-Maya attributes is an error.
        self.assertRaisesRegex(RuntimeError,
                               'Cannot evaluate more than one attribute\.$',
                               cmds.getAttr,
                               'proxyShape1.shareStage',pathStr+'.xformOp:translate')

    def createAndTestAttribute(self, materialItem, shaderDefName, shaderName, origValue, newValue, validation):
        surfDef = ufe.NodeDef.definition(materialItem.runTimeId(), shaderDefName)
        cmd = surfDef.createNodeCmd(materialItem, ufe.PathComponent(shaderName))
        ufeCmd.execute(cmd)
        shaderItem = cmd.insertedChild
        shaderAttrs = ufe.Attributes.attributes(shaderItem)

        self.assertTrue(shaderAttrs.hasAttribute("info:id"))
        self.assertEqual(shaderAttrs.attribute("info:id").get(), shaderDefName)
        self.assertEqual(ufe.PathString.string(shaderItem.path()), "|stage1|stageShape1,/Material1/" + shaderName + "1")
        materialHier = ufe.Hierarchy.hierarchy(materialItem)
        self.assertTrue(materialHier.hasChildren())

        self.assertTrue(shaderAttrs.hasAttribute("inputs:value"))
        shaderAttr = shaderAttrs.attribute("inputs:value")
        validation(self, shaderAttr.get(), origValue)
        shaderAttr.set(newValue)
        validation(self, shaderAttr.get(), newValue)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4015', 'Test only available in UFE preview version 0.4.15 and greater')
    @unittest.skipUnless(Usd.GetVersion() >= (0, 21, 8), 'Requires CanApplySchema from USD')
    def testCreateAttributeTypes(self):
        """Tests all shader attribute types"""
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create a ContextOps interface for the proxy shape.
        proxyPathSegment = mayaUtils.createUfePathSegment(proxyShape)
        proxyShapePath = ufe.Path([proxyPathSegment])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        cmd = contextOps.doOpCmd(['Add New Prim', 'Capsule'])
        ufeCmd.execute(cmd)
        cmd = contextOps.doOpCmd(['Add New Prim', 'Material'])
        ufeCmd.execute(cmd)

        rootHier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertTrue(rootHier.hasChildren())
        self.assertEqual(len(rootHier.children()), 2)

        materialItem = rootHier.children()[-1]
        contextOps = ufe.ContextOps.contextOps(materialItem)

        floatAssert = lambda self, x, y : self.assertAlmostEqual(x, y)
        colorAssert = lambda self, x, y : testUtils.assertVectorAlmostEqual(self, x.color, y.color)
        vectorAssert = lambda self, x, y : testUtils.assertVectorAlmostEqual(self, x.vector, y.vector)
        matrixAssert = lambda self, x, y : testUtils.assertMatrixAlmostEqual(self, x.matrix, y.matrix)
        normalAssert = lambda self, x, y : self.assertEqual(x, y)

        origFloat = 0.0
        newFloat = 0.6
        origColor3 = ufe.Color3f(0.0, 0.0, 0.0)
        newColor3 = ufe.Color3f(0.2, 0.4, 0.6)
        origColor4 = ufe.Color4f(0.0, 0.0, 0.0, 0.0)
        newColor4 = ufe.Color4f(0.2, 0.4, 0.6, 0.8)
        origVector2 = ufe.Vector2f(0.0, 0.0)
        newVector2 = ufe.Vector2f(0.2, 0.4)
        origVector3 = ufe.Vector3f(0.0, 0.0, 0.0)
        newVector3 = ufe.Vector3f(0.2, 0.4, 0.6)
        origVector4 = ufe.Vector4f(0.0, 0.0, 0.0, 0.0)
        newVector4 = ufe.Vector4f(0.2, 0.4, 0.6, 0.8)
        # Default Matrix33 should be identity, but USD does not store a default value for that type.
        # Requires same fix as Boolean in pxr/usd/usdMtlx/parser.cpp
        #   See: https://github.com/PixarAnimationStudios/USD/pull/1789
        origMatrix3 = ufe.Matrix3d([[0, 0, 0], [0, 0, 0], [0, 0, 0]])
        newMatrix3 = ufe.Matrix3d([[2, 4, 6], [7, 5, 3], [1, 2, 3]])
        origMatrix4 = ufe.Matrix4d([[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]])
        newMatrix4 = ufe.Matrix4d([[1, 2, 1, 1], [0, 1, 0, 1], [2, 3, 4, 1], [1, 1, 1, 1]])
        origBoolean = False
        newBoolean = True
        origInteger = 0
        newInteger = 2
        origString = ""
        newString = "test"

        self.createAndTestAttribute(materialItem, "ND_constant_float", "ConstantFloat", origFloat, newFloat, floatAssert)
        self.createAndTestAttribute(materialItem, "ND_constant_color3", "ConstantColor3_", origColor3, newColor3, colorAssert)
        if os.getenv('USD_HAS_COLOR4_SDR_SUPPORT', 'NOT-FOUND') in ('1', "TRUE"):
            self.createAndTestAttribute(materialItem, "ND_constant_color4", "ConstantColor4_", origColor4, newColor4, colorAssert)
        self.createAndTestAttribute(materialItem, "ND_constant_vector2", "ConstantVector2_", origVector2, newVector2, vectorAssert)
        self.createAndTestAttribute(materialItem, "ND_constant_vector3", "ConstantVector3_", origVector3, newVector3, vectorAssert)
        self.createAndTestAttribute(materialItem, "ND_constant_vector4", "ConstantVector4_", origVector4, newVector4, vectorAssert)
        self.createAndTestAttribute(materialItem, "ND_constant_matrix33", "ConstantMatrix3d_", origMatrix3, newMatrix3, matrixAssert)
        self.createAndTestAttribute(materialItem, "ND_constant_matrix44", "ConstantMatrix4d_", origMatrix4, newMatrix4, matrixAssert)
        self.createAndTestAttribute(materialItem, "ND_constant_boolean", "ConstantBoolean", origBoolean, newBoolean, normalAssert)
        self.createAndTestAttribute(materialItem, "ND_constant_integer", "ConstantInteger", origInteger, newInteger, normalAssert)
        self.createAndTestAttribute(materialItem, "ND_constant_string", "ConstantString", origString, newString, normalAssert)
        self.createAndTestAttribute(materialItem, "ND_constant_filename", "ConstantFilename", origString, newString, normalAssert)

    @unittest.skipIf(os.getenv('USD_HAS_MX_METADATA_SUPPORT', 'NOT-FOUND') not in ('1', "TRUE"), 'Test only available if USD can read MaterialX metadata')
    def testMaterialXMetadata(self):
        """Tests all known metadata"""
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create a ContextOps interface for the proxy shape.
        proxyPathSegment = mayaUtils.createUfePathSegment(proxyShape)
        proxyShapePath = ufe.Path([proxyPathSegment])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        cmd = contextOps.doOpCmd(['Add New Prim', 'Material'])
        ufeCmd.execute(cmd)

        rootHier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertTrue(rootHier.hasChildren())
        self.assertEqual(len(rootHier.children()), 1)

        materialItem = rootHier.children()[0]
        
        surfDef = ufe.NodeDef.definition(materialItem.runTimeId(), "ND_standard_surface_surfaceshader")
        self.assertEqual(str(surfDef.getMetadata("uiname")), "standard_surface")
        self.assertEqual(str(surfDef.getMetadata("doc")), "Autodesk standard surface shader")

        cmd = surfDef.createNodeCmd(materialItem, ufe.PathComponent("MySurf"))
        ufeCmd.execute(cmd)
        shaderItem = cmd.insertedChild

        shaderAttrs = ufe.Attributes.attributes(shaderItem)
        expected = (
            ("uimin", "base", "0.0"),
            ("uimax", "base", "1.0"),
            ("uisoftmin", "transmission_extra_roughness", "0.0"),
            ("uisoftmax", "coat_IOR", "3.0"),
            ("uiname", "base_color", "Base Color"),
            ("uifolder", "transmission_color", "Transmission"),
            ("defaultgeomprop", "coat_normal", "Nworld"),
        )

        for metaName, attrName, metaValue in expected:
            attr = shaderAttrs.attribute("inputs:" + attrName)
            self.assertEqual(str(attr.getMetadata(metaName)), metaValue)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4010', 'Test only available in UFE preview version 0.4.10 and greater')
    @unittest.skipUnless(Usd.GetVersion() >= (0, 21, 8), 'Requires CanApplySchema from USD')
    def testCreateUsdPreviewSurfaceAttribute(self):
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene("UsdPreviewSurface", "DisplayColorCube.usda")
        testDagPath, testStage = mayaUtils.createProxyFromFile(testFile)
        mayaPathSegment = mayaUtils.createUfePathSegment(testDagPath)
        usdPathSegment = usdUtils.createUfePathSegment("/DisplayColorCube/Looks/usdPreviewSurface1SG/usdPreviewSurface1")
        shaderPath = ufe.Path([mayaPathSegment, usdPathSegment])
        shaderItem = ufe.Hierarchy.createItem(shaderPath)
        shaderAttrs = ufe.Attributes.attributes(shaderItem)

        self.assertTrue(shaderAttrs.hasAttribute("inputs:roughness"))
        shaderAttr = shaderAttrs.attribute("inputs:roughness")
        self.assertAlmostEqual(shaderAttr.get(), 0.5)
        shaderAttr.set(0.8)
        self.assertAlmostEqual(shaderAttr.get(), 0.8)

    def testNamePrettification(self):
        '''Test the name prettification routine.'''
        self.assertEqual(mayaUsdLib.Util.prettifyName("standard_surface"), "Standard Surface")
        self.assertEqual(mayaUsdLib.Util.prettifyName("standardSurface"), "Standard Surface")
        self.assertEqual(mayaUsdLib.Util.prettifyName("UsdPreviewSurface"), "Usd Preview Surface")
        self.assertEqual(mayaUsdLib.Util.prettifyName("USDPreviewSurface"), "USD Preview Surface")
        self.assertEqual(mayaUsdLib.Util.prettifyName("ior"), "Ior")
        self.assertEqual(mayaUsdLib.Util.prettifyName("IOR"), "IOR")
        self.assertEqual(mayaUsdLib.Util.prettifyName("specular_IOR"), "Specular IOR")
        # This is as expected as we do not insert space on digit<->alpha transitions:
        self.assertEqual(mayaUsdLib.Util.prettifyName("Dx11Shader"), "Dx11Shader")

if __name__ == '__main__':
    unittest.main(verbosity=2)
