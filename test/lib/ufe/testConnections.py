#!/usr/bin/env python

#
# Copyright 2022 Autodesk
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

import mayaUtils
import ufeUtils
import usdUtils
import testUtils

import mayaUsd.lib as mayaUsdLib

from maya import cmds
from pxr import Sdr

import os
import ufe
import unittest

class TestObserver(ufe.Observer):
    def __init__(self):
        super(TestObserver, self).__init__()
        self._addedNotifications = 0
        self._removedNotifications = 0
        self._valueChangedNotifications = 0
        self._connectionChangedNotifications = 0
        self._metadataChangedNotifications = 0
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
        elif isinstance(notification, ufe.AttributeMetadataChanged):
            self._metadataChangedNotifications += 1
        else:
            self._unknownNotifications += 1

    def assertNotificationCount(self, test, **counters):
        test.assertEqual(self._addedNotifications, counters.get("numAdded", 0))
        test.assertEqual(self._removedNotifications, counters.get("numRemoved", 0))
        test.assertEqual(self._valueChangedNotifications, counters.get("numValue", 0))
        test.assertEqual(self._connectionChangedNotifications, counters.get("numConnection", 0))
        test.assertEqual(self._metadataChangedNotifications, counters.get("numMetadata", 0))
        test.assertEqual(self._unknownNotifications, 0)


class ConnectionTestCase(unittest.TestCase):
    '''Test(s) validating the connections (i.e. list, create and delete).'''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    def setUp(self):
        self.assertTrue(self.pluginsLoaded)

        cmds.file(new=True, force=True)

    def testConnection(self):
        '''Test a connection.'''

        # Load a scene.

        testFile = testUtils.getTestScene('UsdPreviewSurface', 'DisplayColorCube.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)
        ufeItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/DisplayColorCube/Looks/usdPreviewSurface1SG/usdPreviewSurface1')
        self.assertIsNotNone(ufeItem)

        # Find all the existing connections.

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeItem.runTimeId())
        self.assertIsNotNone(connectionHandler)
        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionHandler)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # Test a connection.

        srcAttr = conns[0].src
        dstAttr = conns[0].dst

        conn = ufe.Connection(srcAttr, dstAttr)

        self.assertEqual(conn.src.path, srcAttr.path)
        self.assertEqual(conn.dst.path, dstAttr.path)

    def testConnections_1(self):
        '''Test a list of connections using 'UsdTransform2dTest.usda'.'''

        # Load a scene.

        testFile = testUtils.getTestScene('UsdPreviewSurface', 'UsdTransform2dTest.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        # Step 1 - Find all the existing connections for the Shader 'UsdTransform2d'

        ufeItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/pPlane1/Looks/usdPreviewSurface1SG/file1/UsdTransform2d')
        self.assertIsNotNone(ufeItem)

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeItem.runTimeId())
        self.assertIsNotNone(connectionHandler)
        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionHandler)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # Test the connection.

        srcAttr = conns[0].src
        dstAttr = conns[0].dst

        self.assertEqual(ufe.PathString.string(srcAttr.path),
            '|stage|stageShape,/pPlane1/Looks/usdPreviewSurface1SG/file1/TexCoordReader')
        self.assertEqual(srcAttr.name, 'outputs:result')

        self.assertEqual(ufe.PathString.string(dstAttr.path),
            '|stage|stageShape,/pPlane1/Looks/usdPreviewSurface1SG/file1/UsdTransform2d')
        self.assertEqual(dstAttr.name, 'inputs:in')

        # Test the list of connections.

        self.assertTrue(connections.hasConnection(dstAttr.attribute(), ufe.Connections.ATTR_IS_DESTINATION))
        self.assertFalse(connections.hasConnection(srcAttr.attribute(), ufe.Connections.ATTR_IS_DESTINATION))

        conns = connections.connections(dstAttr.attribute(), ufe.Connections.ATTR_IS_DESTINATION)
        self.assertEqual(len(conns), 1)

        conns = connections.connections(srcAttr.attribute(), ufe.Connections.ATTR_IS_DESTINATION)
        self.assertEqual(len(conns), 0)

        # Step 2 - Find all the existing connections for for the Shader 'TexCoordReader'

        ufeItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/pPlane1/Looks/usdPreviewSurface1SG/file1/TexCoordReader')
        self.assertIsNotNone(ufeItem)

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeItem.runTimeId())
        self.assertIsNotNone(connectionHandler)
        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionHandler)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # Test the connection.

        srcAttr = conns[0].src
        dstAttr = conns[0].dst

        self.assertEqual(ufe.PathString.string(srcAttr.path),
            '|stage|stageShape,/pPlane1/Looks/usdPreviewSurface1SG')
        self.assertEqual(srcAttr.name, 'inputs:file1:varname')

        self.assertEqual(ufe.PathString.string(dstAttr.path),
            '|stage|stageShape,/pPlane1/Looks/usdPreviewSurface1SG/file1/TexCoordReader')
        self.assertEqual(dstAttr.name, 'inputs:varname')

        # Step 3 - Find all the existing connections for for the Shader 'file1'

        ufeItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/pPlane1/Looks/usdPreviewSurface1SG/file1')
        self.assertIsNotNone(ufeItem)

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeItem.runTimeId())
        self.assertIsNotNone(connectionHandler)
        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionHandler)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # Test the connection.

        srcAttr = conns[0].src
        dstAttr = conns[0].dst

        self.assertEqual(ufe.PathString.string(srcAttr.path),
            '|stage|stageShape,/pPlane1/Looks/usdPreviewSurface1SG/file1/UsdTransform2d')
        self.assertEqual(srcAttr.name, 'outputs:result')

        self.assertEqual(ufe.PathString.string(dstAttr.path),
            '|stage|stageShape,/pPlane1/Looks/usdPreviewSurface1SG/file1')
        self.assertEqual(dstAttr.name, 'inputs:st')

        # Step 4 - Find all the existing connections for for the Shader 'usdPreviewSurface1'

        ufeItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/pPlane1/Looks/usdPreviewSurface1SG/usdPreviewSurface1')
        self.assertIsNotNone(ufeItem)

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeItem.runTimeId())
        self.assertIsNotNone(connectionHandler)
        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionHandler)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # Test the connection.

        srcAttr = conns[0].src
        dstAttr = conns[0].dst

        self.assertEqual(ufe.PathString.string(srcAttr.path),
            '|stage|stageShape,/pPlane1/Looks/usdPreviewSurface1SG/file1')
        self.assertEqual(srcAttr.name, 'outputs:rgb')

        self.assertEqual(ufe.PathString.string(dstAttr.path),
            '|stage|stageShape,/pPlane1/Looks/usdPreviewSurface1SG/usdPreviewSurface1')
        self.assertEqual(dstAttr.name, 'inputs:diffuseColor')

        # Step 5 - Find all the existing connections for for the Material 'usdPreviewSurface1SG'

        ufeItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/pPlane1/Looks/usdPreviewSurface1SG')
        self.assertIsNotNone(ufeItem)

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeItem.runTimeId())
        self.assertIsNotNone(connectionHandler)
        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionHandler)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # Test the connection.

        srcAttr = conns[0].src
        dstAttr = conns[0].dst

        self.assertEqual(ufe.PathString.string(srcAttr.path),
            '|stage|stageShape,/pPlane1/Looks/usdPreviewSurface1SG/usdPreviewSurface1')
        self.assertEqual(srcAttr.name, 'outputs:surface')

        self.assertEqual(ufe.PathString.string(dstAttr.path),
            '|stage|stageShape,/pPlane1/Looks/usdPreviewSurface1SG')
        self.assertEqual(dstAttr.name, 'outputs:surface')

    def testConnections_2(self):
        '''Test a list of connections using 'DisplayColorCube.usda'.'''

        # Load a scene.

        testFile = testUtils.getTestScene('UsdPreviewSurface', 'DisplayColorCube.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        # Step 1 - Find all the existing connections for the Shader 'usdPreviewSurface1'

        ufeItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/DisplayColorCube/Looks/usdPreviewSurface1SG/usdPreviewSurface1')
        self.assertIsNotNone(ufeItem)

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeItem.runTimeId())
        self.assertIsNotNone(connectionHandler)
        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionHandler)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # Test a connection.

        srcAttr = conns[0].src
        dstAttr = conns[0].dst

        self.assertEqual(ufe.PathString.string(srcAttr.path),
            '|stage|stageShape,/DisplayColorCube/Looks/usdPreviewSurface1SG/ColorPrimvar')
        self.assertEqual(srcAttr.name, 'outputs:result')

        self.assertEqual(ufe.PathString.string(dstAttr.path),
            '|stage|stageShape,/DisplayColorCube/Looks/usdPreviewSurface1SG/usdPreviewSurface1')
        self.assertEqual(dstAttr.name, 'inputs:diffuseColor')

        # Step 2 - Find all the existing connections for the Shader 'Primvar'

        ufeItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/DisplayColorCube/Looks/usdPreviewSurface1SG/Primvar')
        self.assertIsNotNone(ufeItem)

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeItem.runTimeId())
        self.assertIsNotNone(connectionHandler)
        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionHandler)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # Test the connection.

        srcAttr = conns[0].src
        dstAttr = conns[0].dst

        self.assertEqual(ufe.PathString.string(srcAttr.path),
            '|stage|stageShape,/DisplayColorCube/Looks/usdPreviewSurface1SG')
        self.assertEqual(srcAttr.name, 'inputs:coords')

        self.assertEqual(ufe.PathString.string(dstAttr.path),
            '|stage|stageShape,/DisplayColorCube/Looks/usdPreviewSurface1SG/Primvar')
        self.assertEqual(dstAttr.name, 'inputs:varname')

        # Step 3 - Find all the existing connections for 'ColorPrimvar'

        ufeItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/DisplayColorCube/Looks/usdPreviewSurface1SG/ColorPrimvar')
        self.assertIsNotNone(ufeItem)

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeItem.runTimeId())
        self.assertIsNotNone(connectionHandler)
        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionHandler)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        # Step 4 - Find all the existing connections for the Material 'usdPreviewSurface1SG'

        ufeItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/DisplayColorCube/Looks/usdPreviewSurface1SG')
        self.assertIsNotNone(ufeItem)

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeItem.runTimeId())
        self.assertIsNotNone(connectionHandler)
        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionHandler)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # Test the connection.

        srcAttr = conns[0].src
        dstAttr = conns[0].dst

        self.assertEqual(ufe.PathString.string(srcAttr.path),
            '|stage|stageShape,/DisplayColorCube/Looks/usdPreviewSurface1SG/usdPreviewSurface1')
        self.assertEqual(srcAttr.name, 'outputs:surface')

        self.assertEqual(ufe.PathString.string(dstAttr.path),
            '|stage|stageShape,/DisplayColorCube/Looks/usdPreviewSurface1SG')
        self.assertEqual(dstAttr.name, 'outputs:surface')

    @unittest.skipIf(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test became invalid in UFE v4 or greater')
    def testConnectionsHandler(self):
        '''Test create & delete a connection.'''

        # Load a scene.

        testFile = testUtils.getTestScene('UsdPreviewSurface', 'DisplayColorCube.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)
        ufeItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/DisplayColorCube/Looks/usdPreviewSurface1SG/usdPreviewSurface1')
        self.assertIsNotNone(ufeItem)

        # Find all the existing connections.

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeItem.runTimeId())
        self.assertIsNotNone(connectionHandler)
        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # Test the connection.

        srcAttr = conns[0].src
        dstAttr = conns[0].dst

        self.assertEqual(ufe.PathString.string(srcAttr.path),
            '|stage|stageShape,/DisplayColorCube/Looks/usdPreviewSurface1SG/ColorPrimvar')
        self.assertEqual(srcAttr.name, 'outputs:result')

        self.assertEqual(ufe.PathString.string(dstAttr.path),
            '|stage|stageShape,/DisplayColorCube/Looks/usdPreviewSurface1SG/usdPreviewSurface1')
        self.assertEqual(dstAttr.name, 'inputs:diffuseColor')

        # Delete a connection using the ConnectionsHandler.

        connectionHandler.disconnect(srcAttr, dstAttr)

        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        # Create a connection using the ConnectionsHandler.

        connectionHandler.connect(srcAttr, dstAttr)

        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # Delete the connection with the command.

        src = srcAttr.attribute()
        dst = dstAttr.attribute()

        cmd = ufe.DisconnectCommand(src, dst)
        cmd.execute()

        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        cmd.undo()

        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        cmd.redo()

        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        # Create a connection with the command.

        cmd = ufe.ConnectCommand(src, dst)
        cmd.execute()

        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        cmd.undo()

        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        cmd.redo()

        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test only available in UFE v4 or greater')
    def testConnectionsHandlerWithCnxCommands(self):
        '''Test create & delete a connection.'''

        # Load a scene.

        testFile = testUtils.getTestScene('UsdPreviewSurface', 'DisplayColorCube.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)
        ufeItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/DisplayColorCube/Looks/usdPreviewSurface1SG/usdPreviewSurface1')
        self.assertIsNotNone(ufeItem)

        # Find all the existing connections.

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeItem.runTimeId())
        self.assertIsNotNone(connectionHandler)
        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # Test the connection.

        srcAttr = conns[0].src
        dstAttr = conns[0].dst

        self.assertEqual(ufe.PathString.string(srcAttr.path),
            '|stage|stageShape,/DisplayColorCube/Looks/usdPreviewSurface1SG/ColorPrimvar')
        self.assertEqual(srcAttr.name, 'outputs:result')

        self.assertEqual(ufe.PathString.string(dstAttr.path),
            '|stage|stageShape,/DisplayColorCube/Looks/usdPreviewSurface1SG/usdPreviewSurface1')
        self.assertEqual(dstAttr.name, 'inputs:diffuseColor')

        # Delete a connection using the ConnectionsHandler.

        connectionHandler.disconnect(srcAttr, dstAttr)

        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        # Create a connection using the ConnectionsHandler.

        connectionHandler.connect(srcAttr, dstAttr)

        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # Delete the connection with the new API command.

        src = srcAttr.attribute()
        dst = dstAttr.attribute()

        cmd = connectionHandler.deleteConnectionCmd(src, dst)
        cmd.execute()

        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        cmd.undo()

        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        cmd.redo()

        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        # Create a connection with the new API command.

        cmd = connectionHandler.createConnectionCmd(src, dst)
        cmd.execute()

        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        cmd.undo()

        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        cmd.redo()

        connections = connectionHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

    def testCreateStandardSurface(self):
        '''Test create a working standard surface shader.'''
        #
        #
        # We start with standard code from testContextOps:
        #
        # Not testing undo/redo at this point in time.
        #
        #
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create a ContextOps interface for the proxy shape.
        proxyPathSegment = mayaUtils.createUfePathSegment(proxyShape)
        proxyShapePath = ufe.Path([proxyPathSegment])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        cmd = contextOps.doOp(['Add New Prim', 'Capsule'])
        cmd = contextOps.doOp(['Add New Prim', 'Material'])

        rootHier = ufe.Hierarchy.hierarchy(proxyShapeItem)

        materialItem = rootHier.children()[-1]
        materialAttrs = ufe.Attributes.attributes(materialItem)
        contextOps = ufe.ContextOps.contextOps(materialItem)

        cmd = contextOps.doOp(['Add New Prim', 'Shader'])

        materialHier = ufe.Hierarchy.hierarchy(materialItem)
        materialPrim = usdUtils.getPrimFromSceneItem(materialItem)


        shaderItem = materialHier.children()[0]
        shaderAttrs = ufe.Attributes.attributes(shaderItem)
        shaderPrim = usdUtils.getPrimFromSceneItem(shaderItem)

        shaderAttr = shaderAttrs.attribute("info:id")
        shaderAttr.set("ND_standard_surface_surfaceshader")

        # The native type of the output has changed in recent versions of USD, so we need to
        # check with Sdr to see what native type we are going to get.
        ssNodeDef = Sdr.Registry().GetShaderNodeByIdentifier("ND_standard_surface_surfaceshader")
        ssOutput = ssNodeDef.GetShaderOutput("out")
        ssOutputType = ssOutput.GetType()

        #
        #
        # Then switch to connection code to connect the shader. Since we never created the
        # attributes, we expect the connection code to create them.
        #
        #
        shaderOutput = shaderAttrs.attribute("outputs:out")
        materialOutput = materialAttrs.attribute("outputs:surface")

        self.assertEqual(shaderOutput.type, "Generic")
        self.assertEqual(shaderOutput.nativeType(), ssOutputType)
        self.assertEqual(materialOutput.type, "Generic")
        self.assertEqual(materialOutput.nativeType(), "TfToken")

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(materialItem.runTimeId())

        # The attributes are not created yet:
        self.assertEqual(len(materialPrim.GetAuthoredProperties()), 0)
        self.assertEqual(len(shaderPrim.GetAuthoredProperties()), 1)
        self.assertEqual("info:id", shaderPrim.GetAuthoredProperties()[0].GetName())

        connectionHandler.connect(shaderOutput, materialOutput)

        connections = connectionHandler.sourceConnections(materialItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # The attributes directly used by the connection got created:
        self.assertEqual(len(shaderPrim.GetAuthoredProperties()), 2)
        self.assertIn("info:id", [i.GetName() for i in shaderPrim.GetAuthoredProperties()])
        self.assertIn("outputs:out", [i.GetName() for i in shaderPrim.GetAuthoredProperties()])
        # The connection of a MaterialX shader to the material got redirected to the proper render context.
        self.assertEqual(len(materialPrim.GetAuthoredProperties()), 1)
        self.assertEqual(materialPrim.GetAuthoredProperties()[0].GetName(), "outputs:mtlx:surface")

        materialXOutput = materialAttrs.attribute("outputs:mtlx:surface")
        self.assertEqual(materialXOutput.type, "Generic")
        self.assertEqual(materialXOutput.nativeType(), "TfToken")

        connectionHandler.disconnect(shaderOutput, materialXOutput)

        # Cleanup on disconnection should remove the MaterialX surface output.
        with self.assertRaisesRegex(KeyError, "Attribute 'outputs:mtlx:surface' does not exist") as cm:
            with mayaUsdLib.DiagnosticBatchContext(1000) as bc:
                materialAttrs.attribute("outputs:mtlx:surface")

        connections = connectionHandler.sourceConnections(materialItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        # Still need original outputs:surface since the MaterialX port was cleaned-up:
        connectionHandler.connect(shaderOutput, materialOutput)

        connections = connectionHandler.sourceConnections(materialItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # Shader output is still generic, even in the presence of a USD attribute. We need to
        # refetch shaderAttrs to get a fresh attribute cache.
        shaderAttrs = ufe.Attributes.attributes(shaderItem)
        shaderOutput = shaderAttrs.attribute("outputs:out")
        self.assertEqual(shaderOutput.type, "Generic")
        self.assertEqual(shaderOutput.nativeType(), ssOutputType)

        # TODO: Test the undoable versions of these commands. They MUST restore the prims as they
        #       were before connecting, which might require deleting authored attributes.
        #       The undo must also be aware that the connection on the material got redirected.

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test only available in UFE v4 or greater')
    def testCreateStandardSurfaceWithCnxCommandUndo(self):
        '''Test create a working standard surface shader.'''
        #
        #
        # We start with standard code from testContextOps:
        #
        # Not testing undo/redo at this point in time.
        #
        #
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create a ContextOps interface for the proxy shape.
        proxyPathSegment = mayaUtils.createUfePathSegment(proxyShape)
        proxyShapePath = ufe.Path([proxyPathSegment])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        cmd = contextOps.doOp(['Add New Prim', 'Capsule'])
        cmd = contextOps.doOp(['Add New Prim', 'Material'])

        rootHier = ufe.Hierarchy.hierarchy(proxyShapeItem)

        materialItem = rootHier.children()[-1]
        materialAttrs = ufe.Attributes.attributes(materialItem)
        contextOps = ufe.ContextOps.contextOps(materialItem)

        cmd = contextOps.doOp(['Add New Prim', 'Shader'])

        materialHier = ufe.Hierarchy.hierarchy(materialItem)
        materialPrim = usdUtils.getPrimFromSceneItem(materialItem)


        shaderItem = materialHier.children()[0]
        shaderAttrs = ufe.Attributes.attributes(shaderItem)
        shaderPrim = usdUtils.getPrimFromSceneItem(shaderItem)

        shaderAttr = shaderAttrs.attribute("info:id")
        shaderAttr.set("ND_standard_surface_surfaceshader")

        # The native type of the output has changed in recent versions of USD, so we need to
        # check with Sdr to see what native type we are going to get.
        ssNodeDef = Sdr.Registry().GetShaderNodeByIdentifier("ND_standard_surface_surfaceshader")
        ssOutput = ssNodeDef.GetShaderOutput("out")
        ssOutputType = ssOutput.GetType()

        #
        #
        # Then switch to connection code to connect the shader. Since we never created the
        # attributes, we expect the connection code to create them.
        #
        #
        shaderOutput = shaderAttrs.attribute("outputs:out")
        materialOutput = materialAttrs.attribute("outputs:surface")

        self.assertEqual(shaderOutput.type, "Generic")
        self.assertEqual(shaderOutput.nativeType(), ssOutputType)
        self.assertEqual(materialOutput.type, "Generic")
        self.assertEqual(materialOutput.nativeType(), "TfToken")

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(materialItem.runTimeId())

        # The attributes are not created yet:
        self.assertEqual(len(materialPrim.GetAuthoredProperties()), 0)
        self.assertEqual(len(shaderPrim.GetAuthoredProperties()), 1)
        self.assertEqual("info:id", shaderPrim.GetAuthoredProperties()[0].GetName())

        connectionCommand = connectionHandler.createConnectionCmd(shaderOutput, materialOutput)
        connectionCommand.execute()

        def assertNewConnectionMatchesCommand(self, materialItem):
            connections = connectionHandler.sourceConnections(materialItem)
            self.assertIsNotNone(connections)
            conns = connections.allConnections()
            self.assertEqual(len(conns), 1)
            # Check the command result:
            self.assertEqual(conns[0].src.path, connectionCommand.connection.src.path)
            self.assertEqual(conns[0].src.name, connectionCommand.connection.src.name)
            self.assertEqual(conns[0].dst.path, connectionCommand.connection.dst.path)
            self.assertEqual(conns[0].dst.name, connectionCommand.connection.dst.name)
            self.assertEqual(connectionCommand.connection.dst.name, "outputs:mtlx:surface")
        assertNewConnectionMatchesCommand(self, materialItem)

        connectionCommand.undo()

        def assertNoConnection(self, materialItem):
            connections = connectionHandler.sourceConnections(materialItem)
            self.assertIsNotNone(connections)
            conns = connections.allConnections()
            self.assertEqual(len(conns), 0)
        assertNoConnection(self, materialItem)

        connectionCommand.redo()

        assertNewConnectionMatchesCommand(self, materialItem)

        disconnectionCommand = connectionHandler.deleteConnectionCmd(connectionCommand.connection.src.attribute(), 
                                                                     connectionCommand.connection.dst.attribute())
        disconnectionCommand.execute()

        assertNoConnection(self, materialItem)

        disconnectionCommand.undo()

        def assertMtlxSurfaceConnection(self, materialItem):
            connections = connectionHandler.sourceConnections(materialItem)
            self.assertIsNotNone(connections)
            conns = connections.allConnections()
            self.assertEqual(len(conns), 1)
            self.assertEqual(conns[0].dst.name, "outputs:mtlx:surface")
        assertMtlxSurfaceConnection(self, materialItem)

        connectionCommand.undo()

        assertNoConnection(self, materialItem)

        connectionCommand.redo()

        assertMtlxSurfaceConnection(self, materialItem)

        disconnectionCommand.redo()

        assertNoConnection(self, materialItem)


    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test only available in UFE v4 or greater')
    def testCreateStandardSurfaceWithAddAttribute(self):
        '''Test create a working standard surface shader.'''
        #
        #
        # We start with standard code from testContextOps:
        #
        # Not testing undo/redo at this point in time.
        #
        #
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create a ContextOps interface for the proxy shape.
        proxyPathSegment = mayaUtils.createUfePathSegment(proxyShape)
        proxyShapePath = ufe.Path([proxyPathSegment])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        cmd = contextOps.doOp(['Add New Prim', 'Capsule'])
        cmd = contextOps.doOp(['Add New Prim', 'Material'])

        rootHier = ufe.Hierarchy.hierarchy(proxyShapeItem)

        materialItem = rootHier.children()[-1]
        materialAttrs = ufe.Attributes.attributes(materialItem)
        contextOps = ufe.ContextOps.contextOps(materialItem)
        materialObserver = TestObserver()
        materialAttrs.addObserver(materialItem, materialObserver)
        materialObserver.assertNotificationCount(self)

        cmd = contextOps.doOp(['Add New Prim', 'Shader'])

        materialHier = ufe.Hierarchy.hierarchy(materialItem)
        materialPrim = usdUtils.getPrimFromSceneItem(materialItem)


        shaderItem = materialHier.children()[0]
        shaderAttrs = ufe.Attributes.attributes(shaderItem)
        shaderPrim = usdUtils.getPrimFromSceneItem(shaderItem)

        shaderAttr = shaderAttrs.attribute("info:id")
        shaderAttr.set("ND_standard_surface_surfaceshader")
        #
        #
        # Then switch to connection code to connect the shader. Since we never created the
        # attributes, we expect the connection code to create them.
        #
        #
        shaderOutput = shaderAttrs.attribute("outputs:out")

        #
        # Big difference here: using the addAttribute to add the surface output for MaterialX:
        #
        materialOutput = materialAttrs.addAttribute("outputs:mtlx:surface", ufe.Attribute.kString)
        materialObserver.assertNotificationCount(self, numAdded = 1)

        self.assertEqual(len(materialPrim.GetAuthoredProperties()), 1)
        self.assertEqual(materialPrim.GetAuthoredProperties()[0].GetName(), "outputs:mtlx:surface")
        self.assertFalse(materialPrim.GetAuthoredProperties()[0].IsCustom())

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(materialItem.runTimeId())

        # The attributes are not created yet:
        self.assertEqual(len(shaderPrim.GetAuthoredProperties()), 1)
        self.assertEqual("info:id", shaderPrim.GetAuthoredProperties()[0].GetName())

        connectionHandler.connect(shaderOutput, materialOutput)
        materialObserver.assertNotificationCount(self, numAdded = 1, numConnection = 1)

        connections = connectionHandler.sourceConnections(materialItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # The attributes directly used by the connection got created:
        self.assertEqual(len(shaderPrim.GetAuthoredProperties()), 2)
        self.assertIn("info:id", [i.GetName() for i in shaderPrim.GetAuthoredProperties()])
        self.assertIn("outputs:out", [i.GetName() for i in shaderPrim.GetAuthoredProperties()])
        # The connection of a MaterialX shader to the material got redirected to the proper render context.
        self.assertEqual(len(materialPrim.GetAuthoredProperties()), 1)
        self.assertEqual(materialPrim.GetAuthoredProperties()[0].GetName(), "outputs:mtlx:surface")
        self.assertFalse(materialPrim.GetAuthoredProperties()[0].IsCustom())

        materialXOutput = materialAttrs.attribute("outputs:mtlx:surface")
        connectionHandler.disconnect(shaderOutput, materialXOutput)
        # We get one connection changed from the disconnection, a second one for when we cleanup the
        # connection array, then finally a removed on the MaterialX surface output.
        self.assertEqual(len(materialPrim.GetAuthoredProperties()), 0)
        materialObserver.assertNotificationCount(self, numAdded = 1, numConnection = 3, numRemoved = 1)

        connections = connectionHandler.sourceConnections(materialItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        # Since it got removed on disconnection, we need to add it back before reconnecting (or use
        # the universal output to get redirected)
        materialOutput = materialAttrs.addAttribute("outputs:mtlx:surface", ufe.Attribute.kString)
        connectionHandler.connect(shaderOutput, materialOutput)
        materialObserver.assertNotificationCount(self, numAdded = 2, numConnection = 4, numRemoved = 1)

        connections = connectionHandler.sourceConnections(materialItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # TODO: Test the undoable versions of these commands. They MUST restore the prims as they
        #       were before connecting, which might require deleting authored attributes.
        #       The undo must also be aware that the connection on the material got redirected.

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test only available in UFE v4 or greater')
    def testCreateNodeGraphAttributes(self):
        '''Test create attributes on compound boundaries.'''
        cmds.file(new=True, force=True)

        # Create a proxy shape with empty stage to start with.
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

        # Create a ContextOps interface for the proxy shape.
        proxyPathSegment = mayaUtils.createUfePathSegment(proxyShape)
        proxyShapePath = ufe.Path([proxyPathSegment])
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        contextOps = ufe.ContextOps.contextOps(proxyShapeItem)

        cmd = contextOps.doOp(['Add New Prim', 'Material'])
        cmd = contextOps.doOp(['Add New Prim', 'NodeGraph'])

        rootHier = ufe.Hierarchy.hierarchy(proxyShapeItem)

        for testItem in rootHier.children():
            testAttrs = ufe.Attributes.attributes(testItem)
            testObserver = TestObserver()
            testAttrs.addObserver(testItem, testObserver)
            testObserver.assertNotificationCount(self)

            testAttrs.addAttribute("inputs:foo", ufe.Attribute.kFloat)
            testObserver.assertNotificationCount(self, numAdded = 1)
            testAttrs.addAttribute("outputs:bar", ufe.Attribute.kFloat4)
            testObserver.assertNotificationCount(self, numAdded = 2)
            
            # Testing custom NodeGraph data types
            testAttrs.addAttribute("inputs:edf", "EDF")
            # The custom type is saved as metadata, which emits one value and one metadata changes
            testObserver.assertNotificationCount(self, numAdded = 3, numValue=1, numMetadata=1)
            customAttr = testAttrs.attribute("inputs:edf")
            self.assertEqual(customAttr.type, "Generic")
            # Make sure the custom shader type was remembered
            self.assertEqual(customAttr.nativeType(), "EDF")

            # Same thing, on the output side
            testAttrs.addAttribute("outputs:srf", "surfaceshader")
            testObserver.assertNotificationCount(self, numAdded = 4, numValue=2, numMetadata=2)
            customAttr = testAttrs.attribute("outputs:srf")
            self.assertEqual(customAttr.type, "Generic")
            self.assertEqual(customAttr.nativeType(), "surfaceshader")

            testAttrs.removeAttribute("inputs:foo")
            testObserver.assertNotificationCount(self, numAdded = 4, numRemoved = 1, numValue=2, numMetadata=2)
            testAttrs.removeAttribute("outputs:bar")
            testObserver.assertNotificationCount(self, numAdded = 4, numRemoved = 2, numValue=2, numMetadata=2)
            testAttrs.removeAttribute("inputs:edf")
            testObserver.assertNotificationCount(self, numAdded = 4, numRemoved = 3, numValue=2, numMetadata=2)
            testAttrs.removeAttribute("outputs:srf")
            testObserver.assertNotificationCount(self, numAdded = 4, numRemoved = 4, numValue=2, numMetadata=2)


    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test only available in UFE v4 or greater')
    def testCompoundDisplacementPassthrough(self):
        # Creating the connection in this test scene was causing an infinite loop that should now be fixed.
        testFile = testUtils.getTestScene('MaterialX', 'compound_displacement_passthrough.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)
        mayaPathSegment = mayaUtils.createUfePathSegment(shapeNode)

        materialPathSegment = usdUtils.createUfePathSegment("/Material1")
        materialPath = ufe.Path([mayaPathSegment, materialPathSegment])
        materialSceneItem = ufe.Hierarchy.createItem(materialPath)
        materialAttrs = ufe.Attributes.attributes(materialSceneItem)
        materialDisplacementAttr = materialAttrs.attribute("outputs:displacement")

        compoundPathSegment = usdUtils.createUfePathSegment("/Material1/compound")
        compoundPath = ufe.Path([mayaPathSegment, compoundPathSegment])
        compoundSceneItem = ufe.Hierarchy.createItem(compoundPath)
        compoundAttrs = ufe.Attributes.attributes(compoundSceneItem)
        compoundDisplacementAttr = compoundAttrs.attribute("outputs:displacement")

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(materialSceneItem.runTimeId())
        cmd = connectionHandler.createConnectionCmd(compoundDisplacementAttr, materialDisplacementAttr)
        cmd.execute()

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Test only available in UFE v4 or greater since it uses the UsdUndoDeleteConnectionCommand')
    def testRemovePropertiesWithConnections(self):
        '''Test delete connections and properties.'''

        # Load a scene.

        testFile = testUtils.getTestScene('properties', 'properties.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        ufeParentItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/mtl/UsdPreviewSurface1')
        ufePreviewItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/mtl/UsdPreviewSurface1/UsdPreviewSurface1')
        ufeSurfaceItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/mtl/UsdPreviewSurface1/surface1')
        ufeCompoundItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/mtl/UsdPreviewSurface1/compound')
        ufeChildCompoundItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/mtl/UsdPreviewSurface1/compound/UsdPreviewSurface2')

        self.assertIsNotNone(ufeParentItem)
        self.assertIsNotNone(ufePreviewItem)
        self.assertIsNotNone(ufeSurfaceItem)
        self.assertIsNotNone(ufeCompoundItem)
        self.assertIsNotNone(ufeChildCompoundItem)

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(ufeParentItem.runTimeId())
        self.assertIsNotNone(connectionHandler)

        # Get the prims.
        parentPrim = usdUtils.getPrimFromSceneItem(ufeParentItem)
        previewPrim = usdUtils.getPrimFromSceneItem(ufePreviewItem)
        surfacePrim = usdUtils.getPrimFromSceneItem(ufeSurfaceItem)
        compoundPrim = usdUtils.getPrimFromSceneItem(ufeCompoundItem)
        childCompoundPrim = usdUtils.getPrimFromSceneItem(ufeChildCompoundItem)

        # Get the attributes.
        parentAttrs = ufe.Attributes.attributes(ufeParentItem)
        previewAttrs = ufe.Attributes.attributes(ufePreviewItem)
        surfaceAttrs = ufe.Attributes.attributes(ufeSurfaceItem)
        compoundAttrs = ufe.Attributes.attributes(ufeCompoundItem)
        childCompoundAttrs = ufe.Attributes.attributes(ufeChildCompoundItem)

        # 1. Delete node (not compound) connections.

        previewClearcoat = previewAttrs.attribute('inputs:clearcoat')
        previewSurface = previewAttrs.attribute('outputs:surface')
        parentClearcoat = parentAttrs.attribute('inputs:clearcoat')
        parentSurface = parentAttrs.attribute('outputs:surface')
        surfaceBsdf = surfaceAttrs.attribute('inputs:bsdf')

        self.assertIsNotNone(previewClearcoat)
        self.assertIsNotNone(previewSurface)
        self.assertIsNotNone(parentClearcoat)
        self.assertIsNotNone(parentSurface)
        self.assertIsNotNone(surfaceBsdf)

        # 1.1 Delete the connection between two nodes (not compounds).
        cmd = connectionHandler.deleteConnectionCmd(previewSurface, surfaceBsdf)
        cmd.execute()

        # Property kept since there is a connection to the parent.
        self.assertTrue(previewPrim.HasProperty('outputs:surface'))
        # The property has been deleted since there are no more connections.
        self.assertFalse(surfacePrim.HasProperty('outputs:surface'))

        # 1.2 Delete the connection between the node and its parent (outputs).
        cmd = connectionHandler.deleteConnectionCmd(previewSurface, parentSurface)
        cmd.execute()

        # The property has been deleted since there are no more connections.
        self.assertFalse(previewPrim.HasProperty('outputs:surface'))
        # Property kept since it is on the boundary.
        self.assertTrue(parentPrim.HasProperty('outputs:surface'))

        # 1.3 Delete the connection between the node and its parent (inputs).
        cmd = connectionHandler.deleteConnectionCmd(parentClearcoat, previewClearcoat)
        cmd.execute()

        # The property has been deleted since there are no more connections.
        self.assertFalse(previewPrim.HasProperty('inputs:clearcoat'))
        # Property kept since it is on the boundary.
        self.assertTrue(parentPrim.HasProperty('inputs:clearcoat'))

        # 2. Delete compound connections.

        compoundDisplacement = compoundAttrs.attribute('outputs:displacement')
        compoundPort = compoundAttrs.attribute('outputs:port')
        parentDisplacement = parentAttrs.attribute('outputs:displacement')
        parentPort = parentAttrs.attribute('outputs:port')

        self.assertIsNotNone(compoundDisplacement)
        self.assertIsNotNone(compoundPort)
        self.assertIsNotNone(parentDisplacement)
        self.assertIsNotNone(parentPort)

        # 2.1 Delete compound connections to the parent.
        cmd = connectionHandler.deleteConnectionCmd(compoundPort, parentPort)
        cmd.execute()

        # Properties kept since they are on the boundary.
        self.assertTrue(compoundPrim.HasProperty('outputs:port'))
        self.assertTrue(parentPrim.HasProperty('outputs:port'))

        cmd = connectionHandler.deleteConnectionCmd(compoundDisplacement, parentDisplacement)
        cmd.execute()

        # Properties kept since they are on the boundary.
        self.assertTrue(compoundPrim.HasProperty('outputs:displacement'))
        self.assertTrue(parentPrim.HasProperty('outputs:displacement'))

        # 2.2 Delete compound connections from the parent.
        compoundClearcoatRoughness = compoundAttrs.attribute('inputs:clearcoatRoughness')
        compoundPort = compoundAttrs.attribute('inputs:port')
        parentClearcoatRoughness = parentAttrs.attribute('inputs:clearcoatRoughness')
        parentPort = parentAttrs.attribute('inputs:port')

        self.assertIsNotNone(compoundClearcoatRoughness)
        self.assertIsNotNone(compoundPort)
        self.assertIsNotNone(parentClearcoatRoughness)
        self.assertIsNotNone(parentPort)

        cmd = connectionHandler.deleteConnectionCmd(parentPort, compoundPort)
        cmd.execute()

        # Properties kept since they are on the boundary.
        self.assertTrue(compoundPrim.HasProperty('inputs:port'))
        self.assertTrue(parentPrim.HasProperty('inputs:port'))

        cmd = connectionHandler.deleteConnectionCmd(parentClearcoatRoughness, compoundClearcoatRoughness)
        cmd.execute()

        # Properties kept since they are on the boundary.
        self.assertTrue(compoundPrim.HasProperty('inputs:clearcoatRoughness'))
        self.assertTrue(parentPrim.HasProperty('inputs:clearcoatRoughness'))

        # 3. Delete connections inside the compound.

        childDisplacement = childCompoundAttrs.attribute('outputs:displacement')
        childClearcoatRoughness = childCompoundAttrs.attribute('inputs:clearcoatRoughness')
        childClearcoat = childCompoundAttrs.attribute('inputs:clearcoat')
        compoundClearcoat = compoundAttrs.attribute('inputs:clearcoat')
       
        self.assertIsNotNone(childDisplacement)
        self.assertIsNotNone(childClearcoatRoughness)
        self.assertIsNotNone(childClearcoat)
        self.assertIsNotNone(compoundClearcoat)

        # 3.1 Delete child compound connections to the parent.
        cmd = connectionHandler.deleteConnectionCmd(childDisplacement, compoundDisplacement)
        cmd.execute()

        # Property kept since it is on the boundary.
        self.assertTrue(compoundPrim.HasProperty('outputs:displacement'))
        # The property has been deleted since there are no more connections.
        self.assertFalse(childCompoundPrim.HasProperty('outputs:displacement'))

        # 3.2 Delete child compound connections from the parent.
        cmd = connectionHandler.deleteConnectionCmd(compoundClearcoatRoughness, childClearcoatRoughness)
        cmd.execute()

        # Property kept since it is on the boundary.
        self.assertTrue(compoundPrim.HasProperty('inputs:clearcoatRoughness'))
        # The property has been deleted since there are no more connections.
        self.assertFalse(childCompoundPrim.HasProperty('inputs:clearcoatRoughness'))

        cmd = connectionHandler.deleteConnectionCmd(compoundClearcoat, childClearcoat)
        cmd.execute()

        # Property kept since it is on the boundary.
        self.assertTrue(compoundPrim.HasProperty('inputs:clearcoat'))
        # The property has been deleted since there are no more connections.
        self.assertFalse(childCompoundPrim.HasProperty('inputs:clearcoat'))

if __name__ == '__main__':
    unittest.main(verbosity=2)
