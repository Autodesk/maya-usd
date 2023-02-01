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
import usdUtils
import testUtils

import mayaUsd.lib as mayaUsdLib

from pxr import UsdGeom

from maya import cmds
from maya import standalone
from maya.internal.ufeSupport import ufeCmdWrapper as ufeCmd

import ufe

import os
import unittest

class TestObserver(ufe.Observer):
    def __init__(self):
        super(TestObserver, self).__init__()
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
        

class AttributesTestCase(unittest.TestCase):
    '''Verify the Attributes UFE interface, for multiple runtimes.
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
        self.assertTrue(self.pluginsLoaded)

        cmds.file(new=True, force=True)

        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()

    def testAttributes(self):
        '''Engine method to run attributes test.'''

        # Get a UFE scene item for one of the balls in the scene.
        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment("|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)

        # Then create the attributes interface for that item.
        ball35Attrs = ufe.Attributes.attributes(ball35Item)
        self.assertIsNotNone(ball35Attrs)

        # Test that we get the same scene item back.
        self.assertEqual(ball35Item, ball35Attrs.sceneItem())

        # Verify that ball35 contains the visibility attribute.
        self.assertTrue(ball35Attrs.hasAttribute(UsdGeom.Tokens.visibility))

        # Verify the attribute type of 'visibility' which we know is a enum token.
        self.assertEqual(ball35Attrs.attributeType(UsdGeom.Tokens.visibility), ufe.Attribute.kEnumString)

        # Get all the attribute names for this item.
        ball35AttrNames = ball35Attrs.attributeNames

        # Visibility should be in this list.
        self.assertIn(UsdGeom.Tokens.visibility, ball35AttrNames)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4024', 'Test for UFE preview version 0.4.24 and later')
    def testAddRemoveAttribute(self):
        '''Test adding and removing custom attributes'''

        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment("|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)

        
        # Then create the attributes interface for that item.
        ball35Attrs = ufe.Attributes.attributes(ball35Item)
        self.assertIsNotNone(ball35Attrs)

        ballObserver = TestObserver()
        ball35Attrs.addObserver(ball35Item, ballObserver)
        ballObserver.assertNotificationCount(self, numAdded = 0, numRemoved = 0)

        cmd = ball35Attrs.addAttributeCmd("MyAttribute", ufe.Attribute.kString)
        self.assertIsNotNone(cmd)

        ufeCmd.execute(cmd)
        ballObserver.assertNotificationCount(self, numAdded = 1, numRemoved = 0)

        self.assertIsNotNone(cmd.attribute)
        self.assertIn("MyAttribute", ball35Attrs.attributeNames)
        attr = ball35Attrs.attribute("MyAttribute")
        self.assertEqual(repr(attr),"ufe.AttributeString(<|transform1|proxyShape1,/Room_set/Props/Ball_35.MyAttribute>)")

        cmds.undo()
        ballObserver.assertNotificationCount(self, numAdded = 1, numRemoved = 1)

        self.assertNotIn("MyAttribute", ball35Attrs.attributeNames)
        with self.assertRaisesRegex(KeyError, "Attribute 'MyAttribute' does not exist") as cm:
            with mayaUsdLib.DiagnosticBatchContext(1000) as bc:
                attr = ball35Attrs.attribute("MyAttribute")

        cmds.redo()
        ballObserver.assertNotificationCount(self, numAdded = 2, numRemoved = 1)

        self.assertIn("MyAttribute", ball35Attrs.attributeNames)
        attr = ball35Attrs.attribute("MyAttribute")

        cmd = ball35Attrs.removeAttributeCmd("MyAttribute")
        self.assertIsNotNone(cmd)

        ufeCmd.execute(cmd)
        ballObserver.assertNotificationCount(self, numAdded = 2, numRemoved = 2)

        self.assertNotIn("MyAttribute", ball35Attrs.attributeNames)
        with self.assertRaisesRegex(KeyError, "Attribute 'MyAttribute' does not exist") as cm:
            with mayaUsdLib.DiagnosticBatchContext(1000) as bc:
                attr = ball35Attrs.attribute("MyAttribute")
        with self.assertRaisesRegex(ValueError, "Requested attribute with empty name") as cm:
            with mayaUsdLib.DiagnosticBatchContext(1000) as bc:
                attr = ball35Attrs.attribute("")

        cmds.undo()
        ballObserver.assertNotificationCount(self, numAdded = 3, numRemoved = 2)

        self.assertIn("MyAttribute", ball35Attrs.attributeNames)
        attr = ball35Attrs.attribute("MyAttribute")

        cmds.redo()
        ballObserver.assertNotificationCount(self, numAdded = 3, numRemoved = 3)

        self.assertNotIn("MyAttribute", ball35Attrs.attributeNames)
        with self.assertRaisesRegex(KeyError, "Attribute 'MyAttribute' does not exist") as cm:
            with mayaUsdLib.DiagnosticBatchContext(1000) as bc:
                attr = ball35Attrs.attribute("MyAttribute")

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4024', 'Test requires remove attribute and its connections feature only available on Ufe 0.4.24 and later')
    def testRemoveCompoundAttribute(self):
        '''Test removing compound attributes'''

        # Load a scene with a compound.
      
        testFile = testUtils.getTestScene("MaterialX", "MayaSurfaces.usda")
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        # Compounds scene item.
        ufeCompound1Item = ufeUtils.createUfeSceneItem(shapeNode,
            "/pCube2/Looks/standardSurface2SG/MayaNG_standardSurface2SG")
        self.assertIsNotNone(ufeCompound1Item)
        ufeCompound2Item = ufeUtils.createUfeSceneItem(shapeNode,
            "/pCube2/Looks/standardSurface2SG/NodeGraph1")
        self.assertIsNotNone(ufeCompound2Item)

        # Then create the attributes interface for that item.
        compound1Attrs = ufe.Attributes.attributes(ufeCompound1Item)
        self.assertIsNotNone(compound1Attrs)
        compound2Attrs = ufe.Attributes.attributes(ufeCompound2Item)
        self.assertIsNotNone(compound2Attrs)

        # 1. Remove an output compound attribute.

        # 1.1 Remove an output compound attribute connected to prim and child prim.
        cmd = compound1Attrs.removeAttributeCmd("outputs:baseColor")
        self.assertIsNotNone(cmd)

        ufeCmd.execute(cmd)
        
        self.assertNotIn("outputs:baseColor", compound1Attrs.attributeNames)

        # Test we removed the connections.
        ufeItemStandardSurface2 = ufeUtils.createUfeSceneItem(shapeNode,
            "/pCube2/Looks/standardSurface2SG/standardSurface2")
        self.assertIsNotNone(ufeItemStandardSurface2)
        ufeItemMayaSwizzle = ufeUtils.createUfeSceneItem(shapeNode,
            "/pCube2/Looks/standardSurface2SG/MayaNG_standardSurface2SG/MayaSwizzle_file2_rgb")
        self.assertIsNotNone(ufeItemMayaSwizzle)

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeItemStandardSurface2.runTimeId())
        self.assertIsNotNone(connectionHandler)

        connections = connectionHandler.sourceConnections(ufeItemStandardSurface2)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        connections = connectionHandler.sourceConnections(ufeItemMayaSwizzle)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # 1.2 Remove an output compound attribute connected to parent prim.
        cmd = compound2Attrs.removeAttributeCmd("outputs:out")
        self.assertIsNotNone(cmd)

        ufeCmd.execute(cmd)
        
        self.assertNotIn("outputs:out", compound2Attrs.attributeNames)

        # Test we removed the connections.
        ufeItemParent = ufeUtils.createUfeSceneItem(shapeNode,
            "/pCube2/Looks/standardSurface2SG")
        self.assertIsNotNone(ufeItemParent)

        connections = connectionHandler.sourceConnections(ufeItemParent)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # 2. Remove an input compound attribute.
        # 2.1 Remove an input compound attribute connected to parent and child prim.
        cmd = compound1Attrs.removeAttributeCmd("inputs:file2:varnameStr")
        self.assertIsNotNone(cmd)

        ufeCmd.execute(cmd)
        
        self.assertNotIn("inputs:file2:varnameStr", compound1Attrs.attributeNames)

        # Test we removed the connection.

        ufeItemTexture = ufeUtils.createUfeSceneItem(shapeNode,
            "/pCube2/Looks/standardSurface2SG/MayaNG_standardSurface2SG/place2dTexture2")
        self.assertIsNotNone(ufeItemTexture)

        connections = connectionHandler.sourceConnections(ufeItemTexture)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        connections = connectionHandler.sourceConnections(ufeItemParent)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4024', 'Test for UFE preview version 0.4.24 and later')
    def testUniqueNameAttribute(self):
        '''Test unique name attribute'''

        ball35Path = ufe.Path([
            mayaUtils.createUfePathSegment("|transform1|proxyShape1"), 
            usdUtils.createUfePathSegment("/Room_set/Props/Ball_35")])
        ball35Item = ufe.Hierarchy.createItem(ball35Path)
        
        # Create an attribute with name 'MyAttribute'
        ball35Attrs = ufe.Attributes.attributes(ball35Item)
        self.assertIsNotNone(ball35Attrs)

        cmd = ball35Attrs.addAttributeCmd("MyAttribute", ufe.Attribute.kString)
        self.assertIsNotNone(cmd)

        ufeCmd.execute(cmd)

        self.assertIsNotNone(cmd.attribute)
        self.assertIn("MyAttribute", ball35Attrs.attributeNames)
        attr = ball35Attrs.attribute("MyAttribute")
        self.assertEqual(repr(attr),"ufe.AttributeString(<|transform1|proxyShape1,/Room_set/Props/Ball_35.MyAttribute>)")

        # Create another attribute with same name 'MyAttribute'
        cmd = ball35Attrs.addAttributeCmd("MyAttribute", ufe.Attribute.kString)
        self.assertIsNotNone(cmd)

        ufeCmd.execute(cmd)

        self.assertIsNotNone(cmd.attribute)

        # Test the new attribute has name 'MyAttribute1'
        self.assertIn("MyAttribute1", ball35Attrs.attributeNames)
        attr = ball35Attrs.attribute("MyAttribute1")
        self.assertEqual(repr(attr),"ufe.AttributeString(<|transform1|proxyShape1,/Room_set/Props/Ball_35.MyAttribute1>)")

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4034', 'Test for UFE preview version 0.4.34 and later')
    def testRenamingAttribute(self):
        '''Test renaming an attribute'''

        # Load a scene.
      
        testFile = testUtils.getTestScene('MaterialX', 'MayaSurfaces.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)
        ufeItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/pCube2/Looks/standardSurface2SG/MayaNG_standardSurface2SG')
        self.assertIsNotNone(ufeItem)

        # Then create the attributes interface for that item.
        standardSurfaceAttrs = ufe.Attributes.attributes(ufeItem)
        self.assertIsNotNone(standardSurfaceAttrs)

        # Rename the attribute.
        cmd = standardSurfaceAttrs.renameAttributeCmd("inputs:file2:varnameStr","inputs:file2:varname")
        self.assertIsNotNone(cmd)

        ufeCmd.execute(cmd)

        self.assertNotIn("inputs:file2:varnameStr", standardSurfaceAttrs.attributeNames)
        self.assertIn("inputs:file2:varname", standardSurfaceAttrs.attributeNames)
        
        # Test undo.
        cmds.undo()
        self.assertIn("inputs:file2:varnameStr", standardSurfaceAttrs.attributeNames)
        self.assertNotIn("inputs:file2:varname", standardSurfaceAttrs.attributeNames)
        
        # Test redo.
        cmds.redo()
        self.assertNotIn("inputs:file2:varnameStr", standardSurfaceAttrs.attributeNames)
        self.assertIn("inputs:file2:varname", standardSurfaceAttrs.attributeNames)
        
        # Test the connections.

        ufeItemTexture = ufeUtils.createUfeSceneItem(shapeNode,
            '/pCube2/Looks/standardSurface2SG/MayaNG_standardSurface2SG/place2dTexture2')
        self.assertIsNotNone(ufeItemTexture)

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeItemTexture.runTimeId())
        self.assertIsNotNone(connectionHandler)
        connections = connectionHandler.sourceConnections(ufeItemTexture)
        self.assertIsNotNone(connectionHandler)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        srcAttr = conns[0].src
        dstAttr = conns[0].dst

        self.assertEqual(ufe.PathString.string(srcAttr.path),
            '|stage|stageShape,/pCube2/Looks/standardSurface2SG/MayaNG_standardSurface2SG')
        self.assertEqual(srcAttr.name, 'inputs:file2:varname')

        self.assertEqual(ufe.PathString.string(dstAttr.path),
            '|stage|stageShape,/pCube2/Looks/standardSurface2SG/MayaNG_standardSurface2SG/place2dTexture2')
        self.assertEqual(dstAttr.name, 'inputs:geomprop')

        # Rename the attribute.
        cmd = standardSurfaceAttrs.renameAttributeCmd("outputs:baseColor","outputs:MyColor")
        self.assertIsNotNone(cmd)

        ufeCmd.execute(cmd)

        self.assertNotIn("outputs:baseColor", standardSurfaceAttrs.attributeNames)
        self.assertIn("outputs:MyColor", standardSurfaceAttrs.attributeNames)

         # Test the connections.

        ufeItemStandardSurface = ufeUtils.createUfeSceneItem(shapeNode,
            '/pCube2/Looks/standardSurface2SG/standardSurface2')
        self.assertIsNotNone(ufeItemStandardSurface)

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeItemStandardSurface.runTimeId())
        self.assertIsNotNone(connectionHandler)
        connections = connectionHandler.sourceConnections(ufeItemStandardSurface)
        self.assertIsNotNone(connectionHandler)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        srcAttr = conns[0].src
        dstAttr = conns[0].dst

        self.assertEqual(ufe.PathString.string(srcAttr.path),
            '|stage|stageShape,/pCube2/Looks/standardSurface2SG/MayaNG_standardSurface2SG')
        self.assertEqual(srcAttr.name, 'outputs:MyColor')

        self.assertEqual(ufe.PathString.string(dstAttr.path),
            '|stage|stageShape,/pCube2/Looks/standardSurface2SG/standardSurface2')
        self.assertEqual(dstAttr.name, 'inputs:base_color')
        
        # Create the SceneItem for a compound with out connection to the parent.
        ufeCompoundItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/pCube2/Looks/standardSurface2SG/NodeGraph1')
        self.assertIsNotNone(ufeCompoundItem)

        # Then create the attributes interface for that item.
        compoundAttrs = ufe.Attributes.attributes(ufeCompoundItem)
        self.assertIsNotNone(compoundAttrs)

        # Rename the attribute.
        cmd = compoundAttrs.renameAttributeCmd("outputs:out", "outputs:out1")
        self.assertIsNotNone(cmd)

        ufeCmd.execute(cmd)

        self.assertNotIn("outputs:out", compoundAttrs.attributeNames)
        self.assertIn("outputs:out1", compoundAttrs.attributeNames)

        # Test the connection.

        ufeParentItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/pCube2/Looks/standardSurface2SG')
        self.assertIsNotNone(ufeParentItem)

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeParentItem.runTimeId())
        self.assertIsNotNone(connectionHandler)
        connections = connectionHandler.sourceConnections(ufeParentItem)
        self.assertIsNotNone(connectionHandler)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 2)

        srcAttr = conns[1].src
        dstAttr = conns[1].dst

        self.assertEqual(ufe.PathString.string(srcAttr.path),
            '|stage|stageShape,/pCube2/Looks/standardSurface2SG/NodeGraph1')
        self.assertEqual(srcAttr.name, 'outputs:out1')

        self.assertEqual(ufe.PathString.string(dstAttr.path),
            '|stage|stageShape,/pCube2/Looks/standardSurface2SG')
        self.assertEqual(dstAttr.name, 'outputs:out')

if __name__ == '__main__':
    unittest.main(verbosity=2)
