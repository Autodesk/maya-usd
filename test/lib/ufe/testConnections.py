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

from maya import cmds

import ufe
import unittest


class AttributeTestCase(unittest.TestCase):
    '''Verify the Attribute UFE interface, for multiple runtimes.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    def setUp(self):
        self.assertTrue(self.pluginsLoaded)

        cmds.file(new=True, force=True)

    def testConnections(self):
        '''Test list/create/delete connections.'''

        testFile = testUtils.getTestScene('UsdPreviewSurface', 'DisplayColorCube.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)
        ufeItem = ufeUtils.createUfeSceneItem(shapeNode,
            '/DisplayColorCube/Looks/usdPreviewSurface1SG/usdPreviewSurface1')
        self.assertIsNotNone(ufeItem)

        # Find all the existing connections.

        connectionsHandler = ufe.RunTimeMgr.instance().connectionsHandler(ufeItem.runTimeId())
        self.assertIsNotNone(connectionsHandler)
        connections = connectionsHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionsHandler)
        conns = connections.sourceConnections()
        self.assertEqual(len(conns), 1)

        srcAttr = conns[0].src
        dstAttr = conns[0].dst

        self.assertEqual(str(srcAttr.path), 
            '|world|stage|stageShape/DisplayColorCube/Looks/usdPreviewSurface1SG/ColorPrimvar')
        self.assertEqual(srcAttr.name, 'result')

        self.assertEqual(str(dstAttr.path), 
            '|world|stage|stageShape/DisplayColorCube/Looks/usdPreviewSurface1SG/usdPreviewSurface1')
        self.assertEqual(dstAttr.name, 'diffuseColor')

        # Delete the connection.

        connectionsHandler.disconnect(srcAttr, dstAttr)

        connections = connectionsHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionsHandler)
        conns = connections.sourceConnections()
        self.assertEqual(len(conns), 0)

        # Create a connection.

        connectionsHandler.connect(srcAttr, dstAttr)

        connections = connectionsHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionsHandler)
        conns = connections.sourceConnections()
        self.assertEqual(len(conns), 1)

        # Delete the connection with the command.

        src = srcAttr.attribute()
        dst = dstAttr.attribute()

        cmd = ufe.DisconnectCommand(src, dst)
        cmd.execute()

        connections = connectionsHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionsHandler)
        conns = connections.sourceConnections()
        self.assertEqual(len(conns), 0)

        cmd.undo()

        connections = connectionsHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionsHandler)
        conns = connections.sourceConnections()
        self.assertEqual(len(conns), 1)

        cmd.redo()

        connections = connectionsHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionsHandler)
        conns = connections.sourceConnections()
        self.assertEqual(len(conns), 0)

        # Create a connection with the command.

        cmd = ufe.ConnectCommand(src, dst)
        cmd.execute()

        connections = connectionsHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionsHandler)
        conns = connections.sourceConnections()
        self.assertEqual(len(conns), 1)

        cmd.undo()

        connections = connectionsHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionsHandler)
        conns = connections.sourceConnections()
        self.assertEqual(len(conns), 0)

        cmd.redo()

        connections = connectionsHandler.sourceConnections(ufeItem)
        self.assertIsNotNone(connectionsHandler)
        conns = connections.sourceConnections()
        self.assertEqual(len(conns), 1)

if __name__ == '__main__':
    unittest.main(verbosity=2)
