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

from pxr import Gf
from pxr import Sdf
from pxr import Usd
from pxr import Tf
from pxr import Vt

from mayaUsd import lib as mayaUsdLib
from maya import cmds

import unittest

class MayaUsdConverterTestCase(unittest.TestCase):
    """
    Verify mayaUsd converter.
    """

    def createStage(self, layerName):
        """
        Create in-memory stage
        """
        layer = Sdf.Layer.CreateAnonymous(layerName)
        stage = Usd.Stage.Open(layer.identifier)
        self.assertTrue(stage)

        return stage

    def createMPlugAndUsdAttribute(self, sdfValueType, nodeName, stage, primPath):
        """
        Create a plug and a usd attribute of given sdf value type on provided Maya's node and Usd prim path.
        """
        plugAndAttrName = "my"+str(sdfValueType).replace('[]','Array');
        
        cmds.group(name=nodeName, empty=True)
        plug = mayaUsdLib.ReadUtil.FindOrCreateMayaAttr(
            sdfValueType,
            Sdf.VariabilityUniform,
            nodeName,
            plugAndAttrName)
                
        self.assertEqual(cmds.attributeQuery(plugAndAttrName, n=nodeName, nn=True), plugAndAttrName)
        
        p = stage.OverridePrim(primPath)
        self.assertTrue(p)
        
        attr = p.CreateAttribute(plugAndAttrName, sdfValueType)
        
        return plug, attr
    
    def runTypeChecks(self, sdfValueType, value1, value2):
        """
        Test for Sdf.ValueTypeNames.XXX type, veryfing:
        - we can find the converter of given sdf type name
        - we can find the converter with a pair of maya plug and usd attribute
        - we can convert vtvalue --> maya plug (value1)
        - we can convert maya plug --> usd attribute (value1)
        - we can convert maya plug --> vtvalue (value1)
        - we can convert usd attribute --> maya plug (value2)
        - we can convert maya plug --> vtvalue (value2)
        - we can convert usd attribute --> maya plug and set plug value with DGModifier(value1)
        - we can convert vtvalue --> maya plug and set plug value with DGModifier(value2)
        """
        cmds.file(new=True, force=True)
        self.assertNotEqual(mayaUsdLib.Converter.find(sdfValueType, False), None)
        
        stage = self.createStage("layer"+str(sdfValueType).replace('[]','Array'))
        plug, attr = self.createMPlugAndUsdAttribute(sdfValueType, "group1", stage, "/Foo")
        
        args = mayaUsdLib.ConverterArgs()
        converter = mayaUsdLib.Converter.find(plug, attr)
        self.assertNotEqual(converter, None)
        
        # value1
        converter.convertVt(value1,plug,args)
        converter.convert(plug, attr, args)
        result1 = attr.Get()
        self.assertEqual(result1, Vt._test_Ident(value1))
        resultVt1 = converter.convertVt(plug,args)
        self.assertEqual(resultVt1, Vt._test_Ident(value1))
        
        # value2
        attr.Set(value2);
        converter.convert(attr, plug, args)
        resultVt2 = converter.convertVt(plug,args)
        self.assertEqual(resultVt2, Vt._test_Ident(value2))
        
        # value1
        attr.Set(value1);
        converter.test_convertAndSetWithModifier(attr, plug, args)
        resultVt3 = converter.convertVt(plug,args)
        self.assertEqual(resultVt3, Vt._test_Ident(value1))
        
        # value2
        converter.test_convertVtAndSetWithModifier(value2,plug,args)
        resultVt4 = converter.convertVt(plug,args)
        self.assertEqual(resultVt4, Vt._test_Ident(value2))
        
    def runErrorHandlingChecks(self, sdfValueType, value1, errorSdfValueType):
        """
        Test for type mismatch. Use validate method to check supported
        and unsupported pair of src and dst for given converter.
        """
        cmds.file(new=True, force=True)
        
        stage = self.createStage("layer"+str(sdfValueType).replace('[]','Array'))
        plug, attr = self.createMPlugAndUsdAttribute(sdfValueType, "group1", stage, "/Foo")
        errPlug, errAttr = self.createMPlugAndUsdAttribute(errorSdfValueType, "groupErr1", stage, "/FooErr")
        
        args = mayaUsdLib.ConverterArgs()
        converter = mayaUsdLib.Converter.find(plug, attr)
        self.assertNotEqual(converter, None)
        
        converter.convertVt(value1,plug,args)
        converter.convert(plug, attr, args)
        
        resultVt1 = converter.convertVt(plug,args)
        self.assertEqual(resultVt1, Vt._test_Ident(value1))
        
        self.assertTrue(converter.validate(plug,attr))
        self.assertFalse(converter.validate(errPlug,errAttr))
        self.assertFalse(converter.validate(plug,errAttr))
        self.assertFalse(converter.validate(errPlug,attr))
        
        with self.assertRaises(Exception):
            converter.convert(plug, errAttr, args)

    def testBoolConverter(self):
        """
        Test for Sdf.ValueTypeNames.Bool
        """
        #
        value1 = Vt.Bool(True)
        value2 = Vt.Bool(False)
        sdfValueType = Sdf.ValueTypeNames.Bool
        errSdfValueType = Sdf.ValueTypeNames.String
        #
        self.runTypeChecks(sdfValueType,value1,value2)
        self.runErrorHandlingChecks(sdfValueType,value1,errSdfValueType)

    def testFloatConverter(self):
        """
        Test for Sdf.ValueTypeNames.Float
        """
        #
        value1 = Vt.Float(3.14)
        value2 = Vt.Float(9.98)
        sdfValueType = Sdf.ValueTypeNames.Float
        errSdfValueType = Sdf.ValueTypeNames.String
        #
        self.runTypeChecks(sdfValueType,value1,value2)
        self.runErrorHandlingChecks(sdfValueType,value1,errSdfValueType)

    def testDoubleConverter(self):
        """
        Test for Sdf.ValueTypeNames.Double
        """
        #
        value1 = Vt.Double(3.14)
        value2 = Vt.Double(9.98)
        sdfValueType = Sdf.ValueTypeNames.Double
        errSdfValueType = Sdf.ValueTypeNames.String
        #
        self.runTypeChecks(sdfValueType,value1,value2)
        self.runErrorHandlingChecks(sdfValueType,value1,errSdfValueType)

    def testStringConverter(self):
        """
        Test for Sdf.ValueTypeNames.String
        """
        #
        value1 = "kittens"
        value2 = "more kittens"
        sdfValueType = Sdf.ValueTypeNames.String
        errSdfValueType = Sdf.ValueTypeNames.Float3
        #
        self.runTypeChecks(sdfValueType,value1,value2)
        self.runErrorHandlingChecks(sdfValueType,value1,errSdfValueType)

    def testFloat3Converter(self):
        """
        Test for Sdf.ValueTypeNames.Float3
        """
        #
        value1 = Gf.Vec3f(1.0, 2.0, 3.0)
        value2 = Gf.Vec3f(4.0, 5.0, 6.0)
        sdfValueType = Sdf.ValueTypeNames.Float3
        errSdfValueType = Sdf.ValueTypeNames.String
        #
        self.runTypeChecks(sdfValueType,value1,value2)
        self.runErrorHandlingChecks(sdfValueType,value1,errSdfValueType)
        
    def testDouble3Converter(self):
        """
        Test for Sdf.ValueTypeNames.Double3
        """
        #
        value1 = Gf.Vec3d(1.0, 2.0, 3.0)
        value2 = Gf.Vec3d(4.0, 5.0, 6.0)
        sdfValueType = Sdf.ValueTypeNames.Double3
        errSdfValueType = Sdf.ValueTypeNames.String
        #
        self.runTypeChecks(sdfValueType,value1,value2)
        self.runErrorHandlingChecks(sdfValueType,value1,errSdfValueType)

    def testMatrix4dConverter(self):
        """
        Test for Sdf.ValueTypeNames.Matrix4d
        """
        #
        value1 = Gf.Matrix4d(1.0, 0.0, 0.0, 0.0,
                            0.0, 1.0, 0.0, 0.0,
                            0.0, 0.0, 1.0, 0.0,
                            1.0, 2.0, 3.0, 1.0)
        value2 = Gf.Matrix4d(2.0, 0.0, 0.0, 0.0,
                            0.0, 2.0, 0.0, 0.0,
                            0.0, 0.0, 2.0, 0.0,
                            2.0, 3.0, 4.0, 1.0)
        sdfValueType = Sdf.ValueTypeNames.Matrix4d
        errSdfValueType = Sdf.ValueTypeNames.String
        #
        self.runTypeChecks(sdfValueType,value1,value2)
        self.runErrorHandlingChecks(sdfValueType,value1,errSdfValueType)

    def testIntArrayConverter(self):
        """
        Test for Sdf.ValueTypeNames.IntArray
        """
        #
        value1 = Vt.IntArray([1,2,3])
        value2 = Vt.IntArray([4,5])
        sdfValueType = Sdf.ValueTypeNames.IntArray
        errSdfValueType = Sdf.ValueTypeNames.String
        #
        self.runTypeChecks(sdfValueType,value1,value2)
        self.runErrorHandlingChecks(sdfValueType,value1,errSdfValueType)
        
