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

from pxr import UsdShade, Sdf

import os
import ufe
import unittest

class BatchOpsHandlerTestCase(unittest.TestCase):
    '''Test batch operations.'''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    def setUp(self):
        self.assertTrue(self.pluginsLoaded)

        cmds.file(new=True, force=True)

    def testUfeDuplicateRelationships(self):
        '''Test that a batched duplication using Ufe API allows relationship fixups.'''

        # Load a scene.
        testFile = testUtils.getTestScene('MaterialX', 'BatchOpsTestScene.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        geomItem = ufeUtils.createUfeSceneItem(shapeNode, '/pPlane1')
        self.assertIsNotNone(geomItem)
        matItem = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG')
        self.assertIsNotNone(matItem)

        batchOpsHandler = ufe.RunTimeMgr.instance().batchOpsHandler(matItem.runTimeId())
        self.assertIsNotNone(batchOpsHandler)

        # Duplicating without batching means the new plane will not see the new material:
        dGeom = ufe.SceneItemOps.sceneItemOps(geomItem).duplicateItemCmd()
        dMat = ufe.SceneItemOps.sceneItemOps(matItem).duplicateItemCmd()

        dGeomPrim = usdUtils.getPrimFromSceneItem(dGeom.item)
        dGeomBindAPI = UsdShade.MaterialBindingAPI(dGeomPrim)
        # Points to original ss3SG, we would like ss3SG1
        self.assertEqual(dGeomBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/mtl/ss3SG"))

        dMat.undoableCommand.undo()
        dGeom.undoableCommand.undo()

        sel = ufe.Selection()
        sel.append(geomItem)
        sel.append(matItem)

        cmd = batchOpsHandler.duplicateSelection(sel, {"inputConnections": True})
        cmd.execute()

        dGeomPrim = usdUtils.getPrimFromSceneItem(cmd.targetItem(geomItem.path()))
        dGeomBindAPI = UsdShade.MaterialBindingAPI(dGeomPrim)
        # Now seeing and using ss3SG1
        self.assertEqual(dGeomBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/mtl/ss3SG1"))

    def testUfeDuplicateConnections(self):
        '''Test that a batched duplication using Ufe API allows connection fixups.'''

        # Load a scene.
        testFile = testUtils.getTestScene('MaterialX', 'BatchOpsTestScene.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        f3Item = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/MayaNG_ss3SG/file3')
        self.assertIsNotNone(f3Item)
        f3tItem = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture')
        self.assertIsNotNone(f3tItem)
        f3pItem = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/MayaNG_ss3SG/place2dTexture3')
        self.assertIsNotNone(f3pItem)

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(f3Item.runTimeId())
        self.assertIsNotNone(connectionHandler)
        batchOpsHandler = ufe.RunTimeMgr.instance().batchOpsHandler(f3Item.runTimeId())
        self.assertIsNotNone(batchOpsHandler)

        # Duplicating without batching: the duplicated nodes will not see each other.
        dF3 = ufe.SceneItemOps.sceneItemOps(f3Item).duplicateItemCmd()
        dF3t = ufe.SceneItemOps.sceneItemOps(f3tItem).duplicateItemCmd()
        dF3p = ufe.SceneItemOps.sceneItemOps(f3pItem).duplicateItemCmd()

        connections = connectionHandler.sourceConnections(dF3t.item)
        self.assertIsNotNone(connections)
        conn = set(["{}.{} -> {}.{}".format(i.src.path, i.src.name, i.dst.path, i.dst.name) for i in connections.allConnections()])
        wrongButExpected = {
            # Connections are pointing to the original file3 and place2dTexture3 nodes. We want them
            # to link with the new nodes instead
            '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file3.outputs:out -> |world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:inColor',
            '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/place2dTexture3.outputs:out -> |world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:uvCoord'
        }
        self.assertEqual(conn, wrongButExpected)

        dF3p.undoableCommand.undo()
        dF3t.undoableCommand.undo()
        dF3.undoableCommand.undo()

        sel = ufe.Selection()
        sel.append(f3Item)
        sel.append(f3tItem)
        sel.append(f3pItem)

        cmd = batchOpsHandler.duplicateSelection(sel, {"inputConnections": True})
        cmd.execute()

        connections = connectionHandler.sourceConnections(cmd.targetItem(f3tItem.path()))
        self.assertIsNotNone(connections)
        conn = set(["{}.{} -> {}.{}".format(i.src.path, i.src.name, i.dst.path, i.dst.name) for i in connections.allConnections()])
        expectedF3t = {
            # Connections are pointing to the new file4 and place2dTexture4 nodes.
            '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file4.outputs:out -> |world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:inColor',
            '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/place2dTexture4.outputs:out -> |world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:uvCoord'
        }
        self.assertEqual(conn, expectedF3t)
        self.assertEqual(str(cmd.targetItem(f3Item.path()).path()), '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file4')
        self.assertEqual(str(cmd.targetItem(f3pItem.path()).path()), '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/place2dTexture4')

        connections = connectionHandler.sourceConnections(cmd.targetItem(f3pItem.path()))
        self.assertIsNotNone(connections)
        conn = set(["{}.{} -> {}.{}".format(i.src.path, i.src.name, i.dst.path, i.dst.name) for i in connections.allConnections()])
        expectedF3p = {
            # Connection pointing to the original node graph.
            '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG.inputs:file3:varname -> |world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/place2dTexture4.inputs:geomprop',
        }
        self.assertEqual(conn, expectedF3p)

        # Last test, but with options that will also drop connections to SceneItems
        # outside of the duplicated clique.
        cmd.undo()

        cmd = batchOpsHandler.duplicateSelection(sel, {"inputConnections": False})

        cmd.execute()

        connections = connectionHandler.sourceConnections(cmd.targetItem(f3tItem.path()))
        self.assertIsNotNone(connections)
        conn = set(["{}.{} -> {}.{}".format(i.src.path, i.src.name, i.dst.path, i.dst.name) for i in connections.allConnections()])
        expectedF3t = {
            # Connections are pointing to the new file4 and place2dTexture4 nodes.
            '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file4.outputs:out -> |world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:inColor',
            '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/place2dTexture4.outputs:out -> |world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:uvCoord'
        }
        self.assertEqual(conn, expectedF3t)

        # The connection between the new place2dTexture and the original NodeGraph will be gone.
        connections = connectionHandler.sourceConnections(cmd.targetItem(f3pItem.path()))
        self.assertIsNotNone(connections)
        self.assertEqual(len(connections.allConnections()), 0)
        

    def testUfeDuplicateRelationshipsMaya(self):
        '''Test that Maya uses relationship fixups.'''

        # Load a scene.
        testFile = testUtils.getTestScene('MaterialX', 'BatchOpsTestScene.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        geomItem = ufeUtils.createUfeSceneItem(shapeNode, '/pPlane1')
        self.assertIsNotNone(geomItem)
        matItem = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG')
        self.assertIsNotNone(matItem)

        ufe.GlobalSelection.get().clear()
        ufe.GlobalSelection.get().append(geomItem)
        ufe.GlobalSelection.get().append(matItem)
        cmds.duplicate(inputConnections = True)

        dGeomPrim = usdUtils.getPrimFromSceneItem(ufe.GlobalSelection.get().front())
        dGeomBindAPI = UsdShade.MaterialBindingAPI(dGeomPrim)
        # Now seeing and using ss3SG1
        self.assertEqual(dGeomBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/mtl/ss3SG1"))

    def testUfeDuplicateConnectionsMaya(self):
        '''Test that a duplication using Maya does connection fixups.'''

        # Load a scene.
        testFile = testUtils.getTestScene('MaterialX', 'BatchOpsTestScene.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        f3Item = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/MayaNG_ss3SG/file3')
        self.assertIsNotNone(f3Item)
        f3tItem = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture')
        self.assertIsNotNone(f3tItem)
        f3pItem = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG/MayaNG_ss3SG/place2dTexture3')
        self.assertIsNotNone(f3pItem)

        connectionHandler = ufe.RunTimeMgr.instance().connectionHandler(f3Item.runTimeId())
        self.assertIsNotNone(connectionHandler)

        ufe.GlobalSelection.get().clear()
        ufe.GlobalSelection.get().append(f3tItem)
        ufe.GlobalSelection.get().append(f3Item)
        ufe.GlobalSelection.get().append(f3pItem)
        cmds.duplicate(inputConnections = True)

        f3tDupItem, f3DupItem, f3pDupItem = tuple(ufe.GlobalSelection.get())
        connections = connectionHandler.sourceConnections(f3tDupItem)
        self.assertIsNotNone(connections)
        conn = set(["{}.{} -> {}.{}".format(i.src.path, i.src.name, i.dst.path, i.dst.name) for i in connections.allConnections()])
        expectedF3t = {
            # Connections are pointing to the new file4 and place2dTexture4 nodes.
            '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file4.outputs:out -> |world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:inColor',
            '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/place2dTexture4.outputs:out -> |world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:uvCoord'
        }
        self.assertEqual(conn, expectedF3t)
        self.assertEqual(str(f3DupItem.path()), '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file4')
        self.assertEqual(str(f3pDupItem.path()), '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/place2dTexture4')

        connections = connectionHandler.sourceConnections(f3pDupItem)
        self.assertIsNotNone(connections)
        conn = set(["{}.{} -> {}.{}".format(i.src.path, i.src.name, i.dst.path, i.dst.name) for i in connections.allConnections()])
        expectedF3p = {
            # Connection pointing to the original node graph.
            '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG.inputs:file3:varname -> |world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/place2dTexture4.inputs:geomprop',
        }
        self.assertEqual(conn, expectedF3p)

        # Re-test, but using the default duplicate, that should not copy connections
        # to original SceneItems.
        cmds.undo()
        cmds.duplicate()

        f3tDupItem, f3DupItem, f3pDupItem = tuple(ufe.GlobalSelection.get())
        connections = connectionHandler.sourceConnections(f3tDupItem)
        self.assertIsNotNone(connections)
        conn = set(["{}.{} -> {}.{}".format(i.src.path, i.src.name, i.dst.path, i.dst.name) for i in connections.allConnections()])
        self.assertEqual(conn, expectedF3t)

        # The connection between the new place2dTexture and the original NodeGraph will be gone.
        connections = connectionHandler.sourceConnections(f3pDupItem)
        self.assertIsNotNone(connections)
        self.assertEqual(len(connections.allConnections()), 0)


if __name__ == '__main__':
    unittest.main(verbosity=2)
