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
import testUtils

from maya import cmds

import ufe
import unittest


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

if __name__ == '__main__':
    unittest.main(verbosity=2)
