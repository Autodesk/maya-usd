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
        '''Test that a duplication guard using Ufe API allows relationship fixups.'''

        # Load a scene.
        testFile = testUtils.getTestScene('MaterialX', 'BatchOpsTestScene.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        geomItem = ufeUtils.createUfeSceneItem(shapeNode, '/pPlane1')
        self.assertIsNotNone(geomItem)
        matItem = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG')
        self.assertIsNotNone(matItem)

        batchOpsHandler = ufe.RunTimeMgr.instance().batchOpsHandler(matItem.runTimeId())
        self.assertIsNotNone(batchOpsHandler)

        # Duplicating without a guard means the new plane will not see the new material:
        dGeom = ufe.SceneItemOps.sceneItemOps(geomItem).duplicateItemCmd()
        dMat = ufe.SceneItemOps.sceneItemOps(matItem).duplicateItemCmd()

        dGeomPrim = usdUtils.getPrimFromSceneItem(dGeom.item)
        dGeomBindAPI = UsdShade.MaterialBindingAPI(dGeomPrim)
        # Points to original ss3SG, we would like ss3SG1
        self.assertEqual(dGeomBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/mtl/ss3SG"))

        dMat.undoableCommand.undo()
        dGeom.undoableCommand.undo()

        # Setup a guard, with Maya flag to duplicate connections (and properly manage them)
        batchOpsHandler.beginDuplicationGuard({"inputConnections": True})

        dGeom = ufe.SceneItemOps.sceneItemOps(geomItem).duplicateItemCmd()
        dMat = ufe.SceneItemOps.sceneItemOps(matItem).duplicateItemCmd()

        batchOpsHandler.endDuplicationGuard()

        dGeomPrim = usdUtils.getPrimFromSceneItem(dGeom.item)
        dGeomBindAPI = UsdShade.MaterialBindingAPI(dGeomPrim)
        # Now seeing and using ss3SG1
        self.assertEqual(dGeomBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/mtl/ss3SG1"))

    def testUfeDuplicateConnections(self):
        '''Test that a duplication guard using Ufe API allows connection fixups.'''

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

        # Duplicating without a guard means the duplicated nodes will not see each other:
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

        # Setup a guard, with Maya flag to duplicate connections (and properly manage them)
        batchOpsHandler.beginDuplicationGuard({"inputConnections": True})

        dF3 = ufe.SceneItemOps.sceneItemOps(f3Item).duplicateItemCmd()
        dF3t = ufe.SceneItemOps.sceneItemOps(f3tItem).duplicateItemCmd()
        dF3p = ufe.SceneItemOps.sceneItemOps(f3pItem).duplicateItemCmd()

        endCmd = batchOpsHandler.endDuplicationGuardCmd()
        endCmd.execute()

        connections = connectionHandler.sourceConnections(dF3t.item)
        self.assertIsNotNone(connections)
        conn = set(["{}.{} -> {}.{}".format(i.src.path, i.src.name, i.dst.path, i.dst.name) for i in connections.allConnections()])
        expected = {
            # Connections are pointing to the new file4 and place2dTexture4 nodes.
            '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file4.outputs:out -> |world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:inColor',
            '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/place2dTexture4.outputs:out -> |world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:uvCoord'
        }
        self.assertEqual(conn, expected)
        self.assertEqual(str(dF3.item.path()), '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file4')
        self.assertEqual(str(dF3p.item.path()), '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/place2dTexture4')

        # Last test, but with a guard that will delete connections instead of rearranging them:
        endCmd.undo()
        dF3p.undoableCommand.undo()
        dF3t.undoableCommand.undo()
        dF3.undoableCommand.undo()

        batchOpsHandler.beginDuplicationGuard({"inputConnections": False})

        dF3 = ufe.SceneItemOps.sceneItemOps(f3Item).duplicateItemCmd()
        dF3t = ufe.SceneItemOps.sceneItemOps(f3tItem).duplicateItemCmd()
        dF3p = ufe.SceneItemOps.sceneItemOps(f3pItem).duplicateItemCmd()

        batchOpsHandler.endDuplicationGuard()

        connections = connectionHandler.sourceConnections(dF3t.item)
        self.assertIsNotNone(connections)
        # No connections were duplicated:
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
        expected = {
            # Connections are pointing to the new file4 and place2dTexture4 nodes.
            '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file4.outputs:out -> |world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:inColor',
            '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/place2dTexture4.outputs:out -> |world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:uvCoord'
        }
        self.assertEqual(conn, expected)
        self.assertEqual(str(f3DupItem.path()), '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/file4')
        self.assertEqual(str(f3pDupItem.path()), '|world|stage|stageShape/mtl/ss3SG/MayaNG_ss3SG/place2dTexture4')

        # Re-test, but using the default duplicate, that should not copy connections:
        cmds.undo()
        cmds.duplicate()

        connections = connectionHandler.sourceConnections(ufe.GlobalSelection.get().front())
        self.assertIsNotNone(connections)
        # No connections were duplicated:
        self.assertEqual(len(connections.allConnections()), 0)


if __name__ == '__main__':
    unittest.main(verbosity=2)
