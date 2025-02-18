#!/usr/bin/env python

#
# Copyright 2025 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License")
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import fixturesUtils
import mayaUtils
import testUtils
import ufeUtils

from maya import cmds
from maya import standalone

import ufe
from ufe.extensions import lookdevXUfe as lxufe
import mayaUsd.ufe

import os
import unittest

class TestIsComponentConnection:
    def __init__(self, unitTest, srcAttrOrItem, srcComponent, dstAttr, dstComponent):
        if isinstance(srcAttrOrItem, ufe.SceneItem):
            extendedConnectionHandler = lxufe.ExtendedConnectionHandler.get(srcAttrOrItem.runTimeId())
            attrs = ufe.Attributes.attributes(srcAttrOrItem)
            attr = attrs.attribute("outputs:out")
            self._cmd = extendedConnectionHandler.createConnectionCmd(attr, "", attr, "")
        else:
            extendedConnectionHandler = lxufe.ExtendedConnectionHandler.get(srcAttrOrItem.sceneItem().runTimeId())
            self._cmd = extendedConnectionHandler.createConnectionCmd(srcAttrOrItem, srcComponent, dstAttr, dstComponent)
        self._args = (srcAttrOrItem, srcComponent, dstAttr, dstComponent)
        self._unitTest = unitTest

    def testComponentConnected(self, test):
        self._unitTest.assertEqual(test, self._cmd.isComponentConnected(*self._args))

    def testComponentNames(self, attr, attrComponentNames):
        self._unitTest.assertEqual(set(self._cmd.componentNames(attr)), set(attrComponentNames))

def verifyTypeToTypeConnection(unitTest, connectionHandler,
                               srcAttr, srcComponent,
                               dstSceneItem, dstAttr, dstComponent,
                               separatePath, combinePath):
    # Get the separate and combine nodes.
    separate1SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(separatePath))
    unitTest.assertTrue(separate1SceneItem)
    combine1SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(combinePath))
    unitTest.assertTrue(combine1SceneItem)

    # Verify the combine to standard surface connection.
    dstConnections1 = connectionHandler.sourceConnections(dstSceneItem)
    unitTest.assertEqual(len(dstConnections1.allConnections()), 1)
    connection = dstConnections1.allConnections()[0]
    unitTest.assertTrue(connection)
    combine1Attrs = ufe.Attributes.attributes(combine1SceneItem)
    unitTest.assertEqual(connection.src.attribute(), combine1Attrs.attribute("outputs:out"))
    unitTest.assertEqual(connection.dst.attribute(), dstAttr)

    combineInputMap = {
        "r": "inputs:in1",
        "g": "inputs:in2",
        "b": "inputs:in3",
        "a": "inputs:in4",
        "x": "inputs:in1",
        "y": "inputs:in2",
        "z": "inputs:in3",
        "w": "inputs:in4"
    }
    combineInput = combineInputMap.get(dstComponent, None)
    unitTest.assertIsNotNone(combineInput)

    separateOutputMap = {
        "r": "outputs:outr",
        "g": "outputs:outg",
        "b": "outputs:outb",
        "a": "outputs:outa",
        "x": "outputs:outx",
        "y": "outputs:outy",
        "z": "outputs:outz",
        "w": "outputs:outw"
    }
    separateOutput = separateOutputMap.get(srcComponent, None)
    unitTest.assertIsNotNone(separateOutput)

    # Verify the separate to combine connection.
    combine1Connections = connectionHandler.sourceConnections(combine1SceneItem)
    unitTest.assertEqual(len(combine1Connections.allConnections()), 1)
    connection = combine1Connections.allConnections()[0]
    unitTest.assertTrue(connection)
    separate1Attrs = ufe.Attributes.attributes(separate1SceneItem)
    unitTest.assertEqual(connection.src.attribute(), separate1Attrs.attribute(separateOutput))
    unitTest.assertEqual(connection.dst.attribute(), combine1Attrs.attribute(combineInput))

    # Verify the src to separate connection.
    separate1Connections = connectionHandler.sourceConnections(separate1SceneItem)
    unitTest.assertEqual(len(separate1Connections.allConnections()), 1)
    connection = separate1Connections.allConnections()[0]
    unitTest.assertTrue(connection)
    unitTest.assertEqual(connection.src.attribute(), srcAttr)
    unitTest.assertEqual(connection.dst.attribute(), separate1Attrs.attribute("inputs:in"))

def verifyDisconnectComponentConnections(unitTest,
                                        connectionHandler,
                                        addColor3SceneItem, 
                                        constantFloatAttrs, 
                                        addColor3Attrs, 
                                        numDisconnected):
    separate1SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(
        "|stage|stageShape,/mtl/standard_surface1/separate1"))
    if numDisconnected <= 1:
        unitTest.assertTrue(separate1SceneItem)
    else:
        unitTest.assertFalse(separate1SceneItem)

    combine1SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(
        "|stage|stageShape,/mtl/standard_surface1/combine1"))
    if numDisconnected <= 2:
        unitTest.assertTrue(combine1SceneItem)
    else:
        unitTest.assertFalse(combine1SceneItem)

    separate1Attrs = ufe.Attributes.attributes(separate1SceneItem)
    combine1Attrs = ufe.Attributes.attributes(combine1SceneItem)
    if combine1SceneItem:
        combine1Connections = connectionHandler.sourceConnections(combine1SceneItem).allConnections()
        unitTest.assertEqual(len(combine1Connections), 3 - numDisconnected)

        if numDisconnected <= 2:
            unitTest.assertEqual(combine1Connections[0].src.attribute(), constantFloatAttrs.attribute("outputs:out"))
            unitTest.assertEqual(combine1Connections[0].dst.attribute(), combine1Attrs.attribute("inputs:in1"))
        elif numDisconnected <= 1:
            unitTest.assertEqual(combine1Connections[1].src.attribute(), separate1Attrs.attribute("outputs:outg"))
            unitTest.assertEqual(combine1Connections[1].dst.attribute(), combine1Attrs.attribute("inputs:in2"))
        else:
            unitTest.assertEqual(combine1Connections[2].src.attribute(), separate1Attrs.attribute("outputs:outr"))
            unitTest.assertEqual(combine1Connections[2].dst.attribute(), combine1Attrs.attribute("inputs:in3"))

    addColor3Connections = connectionHandler.sourceConnections(addColor3SceneItem).allConnections()
    if numDisconnected == 3:
        unitTest.assertEqual(len(addColor3Connections), 0)
    else:
        unitTest.assertEqual(len(addColor3Connections), 1)
        unitTest.assertEqual(addColor3Connections[0].src.attribute(), combine1Attrs.attribute("outputs:out"))
        unitTest.assertEqual(addColor3Connections[0].dst.attribute(), addColor3Attrs.attribute("inputs:in1"))

def typeToTypeConnectionTest(unitTest, srcNodeName, srcComponent, dstNodeName, dstComponent, 
                             outputName = "outputs:out", 
                             inputName = "inputs:in1",
                             mtlPath = "|stage|stageShape,/mtl/standard_surface1/"):
    srcSceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(mtlPath + srcNodeName))
    unitTest.assertTrue(srcSceneItem)

    dstSceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(mtlPath + dstNodeName))
    unitTest.assertTrue(dstSceneItem)

    extendedConnectionHandler = lxufe.ExtendedConnectionHandler.get(srcSceneItem.runTimeId())
    unitTest.assertTrue(extendedConnectionHandler)

    # Verify that there are no connections at the onset.
    runTimeMgr = ufe.RunTimeMgr.instance()
    connectionHandler = runTimeMgr.connectionHandler(dstSceneItem.runTimeId())
    initialDstConnections = connectionHandler.sourceConnections(dstSceneItem)
    unitTest.assertEqual(len(initialDstConnections.allConnections()), 0)

    # Test that the component connection doesn't exist
    srcAttrs = ufe.Attributes.attributes(srcSceneItem)
    unitTest.assertTrue(srcAttrs)
    dstAttrs = ufe.Attributes.attributes(dstSceneItem)
    unitTest.assertTrue(dstAttrs)
    srcAttr = srcAttrs.attribute(outputName)
    unitTest.assertTrue(srcAttr)
    dstAttr = dstAttrs.attribute(inputName)
    unitTest.assertTrue(dstAttr)
    testIsComponentConnection = TestIsComponentConnection(unitTest, srcAttr, srcComponent, dstAttr, dstComponent)
    testIsComponentConnection.testComponentConnected(False)

    # Test creating component connection between src channel of srcNode and dst channel of dstNode.

    # Create the component connection.
    cmd = extendedConnectionHandler.createConnectionCmd(srcAttr, srcComponent, dstAttr, dstComponent)
    cmd.execute()
    extendedConnection = cmd.extendedConnection()
    unitTest.assertTrue(extendedConnection)
    unitTest.assertEqual(extendedConnection.src.component, srcComponent)
    unitTest.assertEqual(extendedConnection.src.name, srcAttr.name)
    unitTest.assertEqual(extendedConnection.src.attribute(), srcAttr)
    unitTest.assertEqual(extendedConnection.dst.component, dstComponent)
    unitTest.assertEqual(extendedConnection.dst.name, dstAttr.name)
    unitTest.assertEqual(extendedConnection.dst.attribute(), dstAttr)

    separatePath = mtlPath + "separate1"
    combinePath = mtlPath + "combine1"
    verifyTypeToTypeConnection(unitTest, connectionHandler, srcAttr, srcComponent, dstSceneItem, dstAttr, dstComponent,
                               separatePath, combinePath)
    testIsComponentConnection.testComponentConnected(True)

    # Test undo
    cmd.undo()
    unitTest.assertEqual(len(connectionHandler.sourceConnections(dstSceneItem).allConnections()), 0)
    testIsComponentConnection.testComponentConnected(False)

    # Test redo
    cmd.redo()
    verifyTypeToTypeConnection(unitTest, connectionHandler, srcAttr, srcComponent, dstSceneItem, dstAttr, dstComponent,
                               separatePath, combinePath)
    testIsComponentConnection.testComponentConnected(True)

    # Undo again so that nothing is left (makes it so we can use separate1 and combine1 again).
    cmd.undo()
    unitTest.assertEqual(len(connectionHandler.sourceConnections(dstSceneItem).allConnections()), 0)
    testIsComponentConnection.testComponentConnected(False)

#line 927
def valueAttrComponentNamesCheck(unitTest, constantSceneItem, componentNames):
    unitTest.assertTrue(constantSceneItem)
    constantSceneItemAttrs = ufe.Attributes.attributes(constantSceneItem)
    constantValueAttr = constantSceneItemAttrs.attribute("inputs:value")
    unitTest.assertTrue(constantValueAttr)
    testIsComponentConnection = TestIsComponentConnection(unitTest, constantSceneItem, "", None, "")
    testIsComponentConnection.testComponentNames(constantValueAttr, componentNames)

#line 1348
class ComponentConnectionsTestCase(unittest.TestCase):
    '''Verify the USD implementation of component connections.
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

    def testTypeToTypeComponentConnection(self):
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('lookdevXUsd', 'test_component_connections.usda')
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)
        typeToTypeConnectionTest(self, "constant_color3", "r", "add_color3", "g")
        typeToTypeConnectionTest(self, "constant_color4", "a", "add_color4", "b")
        typeToTypeConnectionTest(self, "constant_color4", "b", "add_color4", "a")
        typeToTypeConnectionTest(self, "constant_vector2", "x", "add_vector2", "y")
        typeToTypeConnectionTest(self, "constant_vector3", "z", "add_vector3", "z")
        typeToTypeConnectionTest(self, "constant_vector4", "w", "add_vector4", "x")
        typeToTypeConnectionTest(self, "constant_vector4", "x", "add_vector4", "w")
        typeToTypeConnectionTest(self, "constant_vector4", "y", "add_vector4", "y")

    def testMtlxFileComponentConnections(self):
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('lookdevXUsd', 'test_component_connections3.usda')
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)
        typeToTypeConnectionTest(self, "image1", "r", "fileTexture1", "b", "outputs:out", "inputs:inColor")
        typeToTypeConnectionTest(self, "image1", "b", "fileTexture1", "r", "outputs:out", "inputs:inColor")
        typeToTypeConnectionTest(self, "fileTexture1", "r", "standard_surface1", "b", "outputs:outColor", "inputs:base_color")
        typeToTypeConnectionTest(self, "fileTexture1", "b", "standard_surface1", "r", "outputs:outColor", "inputs:base_color")

    def testFloatToColor3ComponentConnection(self):
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('lookdevXUsd', 'test_component_connections.usda')
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)
        stdSurface1SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path("|stage|stageShape,/mtl/standard_surface1/standard_surface1"))
        self.assertTrue(stdSurface1SceneItem)

        constantFloatSceneItem = ufe.Hierarchy.createItem(ufe.PathString.path("|stage|stageShape,/mtl/standard_surface1/constant_float"))
        self.assertTrue(constantFloatSceneItem)

        extendedConnectionHandler = lxufe.ExtendedConnectionHandler.get(stdSurface1SceneItem.runTimeId())

        # Verify that there are no connections at the onset.
        runTimeMgr = ufe.RunTimeMgr.instance()
        connectionHandler = runTimeMgr.connectionHandler(stdSurface1SceneItem.runTimeId())
        initialStdSurface1Connections = connectionHandler.sourceConnections(stdSurface1SceneItem)
        self.assertEqual(len(initialStdSurface1Connections.allConnections()), 0)

        # Test that the component connection doesn't exist
        constantFloatAttrs = ufe.Attributes.attributes(constantFloatSceneItem)
        stdSurface1Attrs = ufe.Attributes.attributes(stdSurface1SceneItem)
        constantFloatAttr = constantFloatAttrs.attribute("outputs:out")
        stdSurface1Attr = stdSurface1Attrs.attribute("inputs:base_color")
        testIsComponentConnection = TestIsComponentConnection(self, constantFloatAttr, "", stdSurface1Attr, "b")
        testIsComponentConnection.testComponentConnected(False)

        # Test creating component connection between constant_float and b channel of standard_surface1.

        # Create the component connection.
        cmd = extendedConnectionHandler.createConnectionCmd(constantFloatAttr, "", stdSurface1Attr, "b")
        cmd.execute()

        # Get the combine node.
        combine1SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path("|stage|stageShape,/mtl/standard_surface1/combine1"))
        self.assertTrue(combine1SceneItem)

        # Verify the combine to standard surface connection.
        stdSurface1Connections1 = connectionHandler.sourceConnections(stdSurface1SceneItem)
        self.assertEqual(len(stdSurface1Connections1.allConnections()), 1)
        connection = stdSurface1Connections1.allConnections()[0]
        self.assertTrue(connection)
        combine1Attrs = ufe.Attributes.attributes(combine1SceneItem)
        self.assertEqual(connection.src.attribute(), combine1Attrs.attribute("outputs:out"))
        self.assertEqual(connection.dst.attribute(), stdSurface1Attr)

        # Verify the constant_float to combine connection.
        combine1Connections = connectionHandler.sourceConnections(combine1SceneItem)
        self.assertEqual(len(combine1Connections.allConnections()), 1)
        connection = combine1Connections.allConnections()[0]
        self.assertTrue(connection)
        self.assertEqual(connection.src.attribute(), constantFloatAttr)
        self.assertEqual(connection.dst.attribute(), combine1Attrs.attribute("inputs:in3"))

        # Test that the component connection exists
        testIsComponentConnection.testComponentConnected(True)

    def testColor3ToFloatComponentConnection(self):
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('lookdevXUsd', 'test_component_connections.usda')
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)

        addSceneItem = ufe.Hierarchy.createItem(ufe.PathString.path("|stage|stageShape,/mtl/standard_surface1/add_float"))
        self.assertTrue(addSceneItem)

        constantColor3SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path("|stage|stageShape,/mtl/standard_surface1/constant_color3"))
        self.assertTrue(constantColor3SceneItem)

        extendedConnectionHandler = lxufe.ExtendedConnectionHandler.get(constantColor3SceneItem.runTimeId())

        # Verify that there are no connections at the onset.
        runTimeMgr = ufe.RunTimeMgr.instance()
        connectionHandler = runTimeMgr.connectionHandler(addSceneItem.runTimeId())
        initialAddConnections = connectionHandler.sourceConnections(addSceneItem)
        self.assertEqual(len(initialAddConnections.allConnections()), 0)

        # Test that the component connection doesn't exist
        addAttrs = ufe.Attributes.attributes(addSceneItem)
        constantColor3Attrs = ufe.Attributes.attributes(constantColor3SceneItem)
        addAttr = addAttrs.attribute("inputs:in1")
        constantColor3Attr = constantColor3Attrs.attribute("outputs:out")
        testIsComponentConnection = TestIsComponentConnection(self, constantColor3Attr, "r", addAttr, "")
        testIsComponentConnection.testComponentConnected(False)

        # Test creating component connection between constant_color3 r channel and add_float.
        # Create the component connection.
        cmd = extendedConnectionHandler.createConnectionCmd(constantColor3Attr, "r", addAttr, "")
        cmd.execute()

        # Get the separate node.
        separate1SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path("|stage|stageShape,/mtl/standard_surface1/separate1"))
        self.assertTrue(separate1SceneItem)

        # Verify the add_float to separate connection.
        addConnections1 = connectionHandler.sourceConnections(addSceneItem)
        self.assertEqual(len(addConnections1.allConnections()), 1)
        connection = addConnections1.allConnections()[0]
        self.assertTrue(connection)
        separate1Attrs = ufe.Attributes.attributes(separate1SceneItem)
        self.assertEqual(connection.src.attribute(), separate1Attrs.attribute("outputs:outr"))
        self.assertEqual(connection.dst.attribute(), addAttr)

        # Verify the constant_color3 to separate connection.
        separate1Connections = connectionHandler.sourceConnections(separate1SceneItem)
        self.assertEqual(len(separate1Connections.allConnections()), 1)
        connection = separate1Connections.allConnections()[0]
        self.assertTrue(connection)
        self.assertEqual(connection.src.attribute(), constantColor3Attr)
        self.assertEqual(connection.dst.attribute(), separate1Attrs.attribute("inputs:in"))

        # Test that the component connection exists
        testIsComponentConnection.testComponentConnected(True)

    def testColor3ToColor3ReuseSeparateAndCombineComponentConnection(self):
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('lookdevXUsd', 'test_component_connections.usda')
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)

        addColor3SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path("|stage|stageShape,/mtl/standard_surface1/add_color3"))
        self.assertTrue(addColor3SceneItem)

        constantColor3SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path("|stage|stageShape,/mtl/standard_surface1/constant_color3"))
        self.assertTrue(constantColor3SceneItem)

        extendedConnectionHandler = lxufe.ExtendedConnectionHandler.get(constantColor3SceneItem.runTimeId())

        # Verify that there are no connections at the onset.
        runTimeMgr = ufe.RunTimeMgr.instance()
        connectionHandler = runTimeMgr.connectionHandler(addColor3SceneItem.runTimeId())
        initialAddConnections = connectionHandler.sourceConnections(addColor3SceneItem)
        self.assertEqual(len(initialAddConnections.allConnections()), 0)

        # Test creating component connection between constant_color3 r channel and add_color3 b channel.
        # Create the component connection.
        addColor3Attrs = ufe.Attributes.attributes(addColor3SceneItem)
        constantColor3Attrs = ufe.Attributes.attributes(constantColor3SceneItem)
        cmd = extendedConnectionHandler.createConnectionCmd(constantColor3Attrs.attribute("outputs:out"), "r",
                                                            addColor3Attrs.attribute("inputs:in1"), "b")
        cmd.execute()

        # Get the separate and combine nodes.
        separate1SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path("|stage|stageShape,/mtl/standard_surface1/separate1"))
        self.assertTrue(separate1SceneItem)
        combine1SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path("|stage|stageShape,/mtl/standard_surface1/combine1"))
        self.assertTrue(combine1SceneItem)

        # Verify the combine to add_color3 connection.
        dstConnections1 = connectionHandler.sourceConnections(addColor3SceneItem)
        self.assertEqual(len(dstConnections1.allConnections()), 1)
        connection = dstConnections1.allConnections()[0]
        self.assertTrue(connection)
        combine1Attrs = ufe.Attributes.attributes(combine1SceneItem)
        self.assertEqual(connection.src.attribute(), combine1Attrs.attribute("outputs:out"))
        self.assertEqual(connection.dst.attribute(), addColor3Attrs.attribute("inputs:in1"))

        # Verify the separate to combine connection.
        combine1Connections = connectionHandler.sourceConnections(combine1SceneItem)
        self.assertEqual(len(combine1Connections.allConnections()), 1)
        connection = combine1Connections.allConnections()[0]
        self.assertTrue(connection)
        separate1Attrs = ufe.Attributes.attributes(separate1SceneItem)
        self.assertEqual(connection.src.attribute(), separate1Attrs.attribute("outputs:outr"))
        self.assertEqual(connection.dst.attribute(), combine1Attrs.attribute("inputs:in3"))

        # Verify the src to separate connection.
        separate1Connections = connectionHandler.sourceConnections(separate1SceneItem)
        self.assertEqual(len(separate1Connections.allConnections()), 1)
        connection = separate1Connections.allConnections()[0]
        self.assertTrue(connection)
        self.assertEqual(connection.src.attribute(), constantColor3Attrs.attribute("outputs:out"))
        self.assertEqual(connection.dst.attribute(), separate1Attrs.attribute("inputs:in"))

        # Create a second component connection between the constant_color3 g channel and add_color3 r channel.
        # This connection should reuse the created separate and combine nodes.
        cmd2 = extendedConnectionHandler.createConnectionCmd(constantColor3Attrs.attribute("outputs:out"), "g",
                                                             addColor3Attrs.attribute("inputs:in1"), "r")
        cmd2.execute()

        # Verify the new separate to combine connection.
        combine2Connections = connectionHandler.sourceConnections(combine1SceneItem)
        self.assertEqual(len(combine2Connections.allConnections()), 2)
        connection = combine2Connections.allConnections()[0]
        self.assertTrue(connection)
        separate1Attrs2 = ufe.Attributes.attributes(separate1SceneItem)
        combine1Attrs2 = ufe.Attributes.attributes(combine1SceneItem)
        self.assertEqual(connection.src.attribute(), separate1Attrs2.attribute("outputs:outg"))
        self.assertEqual(connection.dst.attribute(), combine1Attrs2.attribute("inputs:in1"))

    def testColor3ToColor3ReuseSeparateAndCombineComponentConnection(self):
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('lookdevXUsd', 'test_component_connections.usda')
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)

        addColor3SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(
            "|stage|stageShape,/mtl/standard_surface1/add_color3"))
        self.assertTrue(addColor3SceneItem)

        constantFloatSceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(
            "|stage|stageShape,/mtl/standard_surface1/constant_float"))
        self.assertTrue(constantFloatSceneItem)

        constantColor3SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(
            "|stage|stageShape,/mtl/standard_surface1/constant_color3"))
        self.assertTrue(constantColor3SceneItem)

        extendedConnectionHandler = lxufe.ExtendedConnectionHandler.get(constantColor3SceneItem.runTimeId())

        # Create 3 component connections
        addColor3Attrs = ufe.Attributes.attributes(addColor3SceneItem)
        constantFloatAttrs = ufe.Attributes.attributes(constantFloatSceneItem)
        constantColor3Attrs = ufe.Attributes.attributes(constantColor3SceneItem)
        cmd = extendedConnectionHandler.createConnectionCmd(constantColor3Attrs.attribute("outputs:out"), "r",
                                                            addColor3Attrs.attribute("inputs:in1"), "b")
        cmd.execute()
        cmd2 = extendedConnectionHandler.createConnectionCmd(constantColor3Attrs.attribute("outputs:out"), "g",
                                                             addColor3Attrs.attribute("inputs:in1"), "g")
        cmd2.execute()
        cmd3 = extendedConnectionHandler.createConnectionCmd(constantFloatAttrs.attribute("outputs:out"), "",
                                                             addColor3Attrs.attribute("inputs:in1"), "r")
        cmd3.execute()

        # Verify that the separate / combine scene items exist.
        # Verify that there are two connections between the separate and combine scene items
        # and one connection between the combine and constant_float scene items.
        # Verify that there is a connection to the combine node from the add_color3 scene item.
        runTimeMgr = ufe.RunTimeMgr.instance()
        connectionHandler = runTimeMgr.connectionHandler(constantColor3SceneItem.runTimeId())
        verifyDisconnectComponentConnections(self, connectionHandler, addColor3SceneItem, constantFloatAttrs, addColor3Attrs, 0)

        # Delete the constant_color3 r to add_color3 b connection.
        # Verify that the separate / combine scene items still exist.
        # Verify that there is one connection between the separate and combine scene items
        # and one connection between the combine and constant_float scene items.
        # Verify that there is a connection to the combine node from the add_color3 scene item.
        cmd4 = extendedConnectionHandler.deleteConnectionCmd(constantColor3Attrs.attribute("outputs:out"), "r",
                                                                        addColor3Attrs.attribute("inputs:in1"), "b")
        cmd4.execute()
        verifyDisconnectComponentConnections(self, connectionHandler, addColor3SceneItem, constantFloatAttrs, addColor3Attrs, 1)

        # Delete the constant_color3 g to add_color3 g connection.
        # Verify that the separate node no longer exists and the combine node does still exist.
        # Verify that there is a connection between the combine and constant_float scene items.
        # Verify that there is a connection to the combine node from the add_color3 scene item.
        cmd5 = extendedConnectionHandler.deleteConnectionCmd(constantColor3Attrs.attribute("outputs:out"), "g",
                                                                        addColor3Attrs.attribute("inputs:in1"), "g")
        cmd5.execute()
        verifyDisconnectComponentConnections(self, connectionHandler, addColor3SceneItem, constantFloatAttrs, addColor3Attrs, 2)

        # Delete the constant_float to add_color3 r connection.
        # Verify that the combine node no longer exists.
        # Verify that there is no connection from the add_color3 scene item.
        cmd6 = extendedConnectionHandler.deleteConnectionCmd(constantFloatAttrs.attribute("outputs:out"), "",
                                                                        addColor3Attrs.attribute("inputs:in1"), "r")
        cmd6.execute()
        verifyDisconnectComponentConnections(self, connectionHandler, addColor3SceneItem, constantFloatAttrs, addColor3Attrs, 3)

        # Test undo/redo.
        cmd6.undo()
        verifyDisconnectComponentConnections(self, connectionHandler, addColor3SceneItem, constantFloatAttrs, addColor3Attrs, 2)
        cmd5.undo()
        verifyDisconnectComponentConnections(self, connectionHandler, addColor3SceneItem, constantFloatAttrs, addColor3Attrs, 1)
        cmd4.undo()
        verifyDisconnectComponentConnections(self, connectionHandler, addColor3SceneItem, constantFloatAttrs, addColor3Attrs, 0)
        cmd4.redo()
        verifyDisconnectComponentConnections(self, connectionHandler, addColor3SceneItem, constantFloatAttrs, addColor3Attrs, 1)
        cmd5.redo()
        verifyDisconnectComponentConnections(self, connectionHandler, addColor3SceneItem, constantFloatAttrs, addColor3Attrs, 2)
        cmd6.redo()
        verifyDisconnectComponentConnections(self, connectionHandler, addColor3SceneItem, constantFloatAttrs, addColor3Attrs, 3)

    def testComponentNames(self):
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('lookdevXUsd', 'test_component_connections.usda')
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)
        mtlPath = "|stage|stageShape,/mtl/standard_surface1/"

        constantColor3SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(mtlPath + "constant_color3"))
        valueAttrComponentNamesCheck(self, constantColor3SceneItem, ("r", "g", "b"))

        constantColor4SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(mtlPath + "constant_color4"))
        valueAttrComponentNamesCheck(self, constantColor4SceneItem, ("r", "g", "b", "a"))

        constantVec2SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(mtlPath + "constant_vector2"))
        valueAttrComponentNamesCheck(self, constantVec2SceneItem, ("x", "y"))

        constantVec3SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(mtlPath + "constant_vector3"))
        valueAttrComponentNamesCheck(self, constantVec3SceneItem, ("x", "y", "z"))

        constantVec4SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(mtlPath + "constant_vector4"))
        valueAttrComponentNamesCheck(self, constantVec4SceneItem, ("x", "y", "z", "w"))

        constantStringSceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(mtlPath + "constant_string"))
        valueAttrComponentNamesCheck(self, constantStringSceneItem, tuple())

    def testInvalidComponentConnections(self):
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('lookdevXUsd', 'test_component_connections.usda')
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)
        mtlPath = "|stage|stageShape,/mtl/standard_surface1/"

        constantColor3SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(
            "|stage|stageShape,/mtl/standard_surface1/constant_color3"))
        self.assertTrue(constantColor3SceneItem)
        constantStringSceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(
            "|stage|stageShape,/mtl/standard_surface1/constant_string"))
        self.assertTrue(constantStringSceneItem)
        addColor3SceneItem = ufe.Hierarchy.createItem(ufe.PathString.path(
            "|stage|stageShape,/mtl/standard_surface1/add_color3"))
        self.assertTrue(addColor3SceneItem)
        constantColor3Attrs = ufe.Attributes.attributes(constantColor3SceneItem)
        self.assertTrue(constantColor3Attrs)
        constantStringAttrs = ufe.Attributes.attributes(constantStringSceneItem)
        self.assertTrue(constantStringAttrs)
        addColor3Attrs = ufe.Attributes.attributes(addColor3SceneItem)
        self.assertTrue(addColor3Attrs)
        constantColor3Output = constantColor3Attrs.attribute("outputs:out")
        self.assertTrue(constantColor3Output)
        constantStringOutput = constantStringAttrs.attribute("outputs:out")
        self.assertTrue(constantStringOutput)
        addColor3Input = addColor3Attrs.attribute("inputs:in1")
        self.assertTrue(addColor3Input)

        extendedConnectionHandler = lxufe.ExtendedConnectionHandler.get(constantColor3SceneItem.runTimeId())

        with self.assertRaises(RuntimeError):
            extendedConnectionHandler.createConnectionCmd(constantColor3Output, "q", addColor3Input, "")
        with self.assertRaises(RuntimeError):
            extendedConnectionHandler.createConnectionCmd(constantColor3Output, "r", addColor3Input, "m")
        with self.assertRaises(RuntimeError):
            extendedConnectionHandler.createConnectionCmd(constantStringOutput, "s", addColor3Input, "")


if __name__ == '__main__':
    unittest.main(verbosity=2)
