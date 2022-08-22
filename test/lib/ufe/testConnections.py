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
        #
        #
        # Then switch to connection code to connect the shader. Since we never created the
        # attributes, we expect the connection code to create them.
        #
        #
        shaderOutput = shaderAttrs.attribute("outputs:out")
        materialOutput = materialAttrs.attribute("outputs:surface")

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

        materialOutput = materialAttrs.attribute("outputs:mtlx:surface")
        connectionHandler.disconnect(shaderOutput, materialOutput)

        connections = connectionHandler.sourceConnections(materialItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        # Not redirected since already on outputs:mtlx:surface:
        connectionHandler.connect(shaderOutput, materialOutput)

        connections = connectionHandler.sourceConnections(materialItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # TODO: Test the undoable versions of these commands. They MUST restore the prims as they
        #       were before connecting, which might require deleting authored attributes.
        #       The undo must also be aware that the connection on the material got redirected.

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

        materialOutput = materialAttrs.attribute("outputs:mtlx:surface")
        connectionHandler.disconnect(shaderOutput, materialOutput)
        materialObserver.assertNotificationCount(self, numAdded = 1, numConnection = 2)

        connections = connectionHandler.sourceConnections(materialItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 0)

        # Not redirected since already on outputs:mtlx:surface:
        connectionHandler.connect(shaderOutput, materialOutput)
        materialObserver.assertNotificationCount(self, numAdded = 1, numConnection = 3)

        connections = connectionHandler.sourceConnections(materialItem)
        self.assertIsNotNone(connections)
        conns = connections.allConnections()
        self.assertEqual(len(conns), 1)

        # TODO: Test the undoable versions of these commands. They MUST restore the prims as they
        #       were before connecting, which might require deleting authored attributes.
        #       The undo must also be aware that the connection on the material got redirected.


if __name__ == '__main__':
    unittest.main(verbosity=2)
