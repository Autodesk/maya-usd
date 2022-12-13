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

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') >= '4043', 'Test became invalid in UFE preview version 0.4.43 and greater')
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

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4043', 'Test only available in UFE preview version 0.4.43 and greater')
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

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4043', 'Test only available in UFE preview version 0.4.43 and greater')
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


    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4024', 'Test only available in UFE preview version 0.4.24 and greater')
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

    @unittest.skipIf(os.getenv('UFE_PREVIEW_VERSION_NUM', '0000') < '4024', 'Test only available in UFE preview version 0.4.24 and greater')
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
            testAttrs.removeAttribute("inputs:foo")
            testObserver.assertNotificationCount(self, numAdded = 2, numRemoved = 1)
            testAttrs.removeAttribute("outputs:bar")
            testObserver.assertNotificationCount(self, numAdded = 2, numRemoved = 2)

if __name__ == '__main__':
    unittest.main(verbosity=2)
