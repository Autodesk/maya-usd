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

        cmd = batchOpsHandler.duplicateSelectionCmd(sel, {"inputConnections": True})
        cmd.execute()

        dGeomPrim = usdUtils.getPrimFromSceneItem(cmd.targetItem(geomItem.path()))
        dGeomBindAPI = UsdShade.MaterialBindingAPI(dGeomPrim)
        # Now seeing and using ss3SG1
        self.assertEqual(dGeomBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/mtl/ss3SG1"))

        cmd.undo()
        cmd.redo()

        dGeomPrim = usdUtils.getPrimFromSceneItem(cmd.targetItem(geomItem.path()))
        dGeomBindAPI = UsdShade.MaterialBindingAPI(dGeomPrim)
        # Now seeing and using ss3SG1
        self.assertEqual(dGeomBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/mtl/ss3SG1"))

        cmd.undo()

        # And currently, as per spec, we are preserving external relationships even with
        # inputConnections set to false. In Maya the shading group connections are
        # always copied. Not sure this will work with other relationships. We will
        # assess when we start seeing exclusive relationships.
        sel = ufe.Selection()
        sel.append(geomItem)
        cmd = batchOpsHandler.duplicateSelectionCmd(sel, {"inputConnections": False})
        cmd.execute()

        dGeomPrim = usdUtils.getPrimFromSceneItem(cmd.targetItem(geomItem.path()))
        dGeomBindAPI = UsdShade.MaterialBindingAPI(dGeomPrim)
        # Still seeing and using the original material
        self.assertEqual(dGeomBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/mtl/ss3SG"))

        cmd.undo()
        cmd.redo()

        dGeomPrim = usdUtils.getPrimFromSceneItem(cmd.targetItem(geomItem.path()))
        dGeomBindAPI = UsdShade.MaterialBindingAPI(dGeomPrim)
        # Still seeing and using the original material
        self.assertEqual(dGeomBindAPI.GetDirectBinding().GetMaterialPath(), Sdf.Path("/mtl/ss3SG"))

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
        conn = set(["{}.{} -> {}.{}".format(ufe.PathString.string(i.src.path), i.src.name,
                                            ufe.PathString.string(i.dst.path), i.dst.name) for i in connections.allConnections()])
        wrongButExpected = {
            # Connections are pointing to the original file3 and place2dTexture3 nodes. We want them
            # to link with the new nodes instead
            '|stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/file3.outputs:out -> |stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:inColor',
            '|stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/place2dTexture3.outputs:out -> |stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:uvCoord'
        }
        self.assertEqual(conn, wrongButExpected)

        dF3p.undoableCommand.undo()
        dF3t.undoableCommand.undo()
        dF3.undoableCommand.undo()

        sel = ufe.Selection()
        sel.append(f3Item)
        sel.append(f3tItem)
        sel.append(f3pItem)

        cmd = batchOpsHandler.duplicateSelectionCmd(sel, {"inputConnections": True})
        cmd.execute()

        expectedF3t = {
            # Connections are pointing to the new file4 and place2dTexture4 nodes.
            '|stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/file4.outputs:out -> |stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:inColor',
            '|stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/place2dTexture4.outputs:out -> |stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:uvCoord'
        }

        def checkConnections1(self, cmd, f3Item, f3tItem, f3pItem, expectedF3t):
            connections = connectionHandler.sourceConnections(cmd.targetItem(f3tItem.path()))
            self.assertIsNotNone(connections)
            conn = set(["{}.{} -> {}.{}".format(ufe.PathString.string(i.src.path), i.src.name,
                                                ufe.PathString.string(i.dst.path), i.dst.name) for i in connections.allConnections()])
            self.assertEqual(conn, expectedF3t)
            self.assertEqual(ufe.PathString.string(cmd.targetItem(f3Item.path()).path()),
                             '|stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/file4')
            self.assertEqual(ufe.PathString.string(cmd.targetItem(f3pItem.path()).path()),
                             '|stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/place2dTexture4')

            connections = connectionHandler.sourceConnections(cmd.targetItem(f3pItem.path()))
            self.assertIsNotNone(connections)
            conn = set(["{}.{} -> {}.{}".format(ufe.PathString.string(i.src.path), i.src.name,
                                                ufe.PathString.string(i.dst.path), i.dst.name) for i in connections.allConnections()])
            expectedF3p = {
                # Connection pointing to the original node graph.
                '|stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG.inputs:file3:varname -> |stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/place2dTexture4.inputs:geomprop',
            }
            self.assertEqual(conn, expectedF3p)

        checkConnections1(self, cmd, f3Item, f3tItem, f3pItem, expectedF3t)

        cmd.undo()
        cmd.redo()

        checkConnections1(self, cmd, f3Item, f3tItem, f3pItem, expectedF3t)

        cmd.undo()

        # Last test, but with options that will also drop connections to SceneItems
        # outside of the duplicated clique.
        cmd = batchOpsHandler.duplicateSelectionCmd(sel, {"inputConnections": False})

        cmd.execute()

        def checkConnections2(self, cmd, f3tItem, f3pItem, expectedF3t):
            connections = connectionHandler.sourceConnections(cmd.targetItem(f3tItem.path()))
            self.assertIsNotNone(connections)
            conn = set(["{}.{} -> {}.{}".format(ufe.PathString.string(i.src.path), i.src.name,
                                                ufe.PathString.string(i.dst.path), i.dst.name) for i in connections.allConnections()])
            self.assertEqual(conn, expectedF3t)

            # The connection between the new place2dTexture and the original NodeGraph will be gone.
            connections = connectionHandler.sourceConnections(cmd.targetItem(f3pItem.path()))
            self.assertIsNotNone(connections)
            self.assertEqual(len(connections.allConnections()), 0)

        checkConnections2(self, cmd, f3tItem, f3pItem, expectedF3t)

        cmd.undo()
        cmd.redo()
        
        checkConnections2(self, cmd, f3tItem, f3pItem, expectedF3t)

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

        cmds.undo()
        cmds.redo()

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

        expectedF3t = {
            # Connections are pointing to the new file4 and place2dTexture4 nodes.
            '|stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/file4.outputs:out -> |stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:inColor',
            '|stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/place2dTexture4.outputs:out -> |stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/file3_MayafileTexture1.inputs:uvCoord'
        }

        def checkState1(self, expectedF3t):
            f3tDupItem, f3DupItem, f3pDupItem = tuple(ufe.GlobalSelection.get())
            connections = connectionHandler.sourceConnections(f3tDupItem)
            self.assertIsNotNone(connections)
            conn = set(["{}.{} -> {}.{}".format(ufe.PathString.string(i.src.path), i.src.name,
                                                ufe.PathString.string(i.dst.path), i.dst.name) for i in connections.allConnections()])
            self.assertEqual(conn, expectedF3t)
            self.assertEqual(ufe.PathString.string(f3DupItem.path()),
                             '|stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/file4')
            self.assertEqual(ufe.PathString.string(f3pDupItem.path()),
                             '|stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/place2dTexture4')

            connections = connectionHandler.sourceConnections(f3pDupItem)
            self.assertIsNotNone(connections)
            conn = set(["{}.{} -> {}.{}".format(ufe.PathString.string(i.src.path), i.src.name,
                                                ufe.PathString.string(i.dst.path), i.dst.name) for i in connections.allConnections()])
            expectedF3p = {
                # Connection pointing to the original node graph.
                '|stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG.inputs:file3:varname -> |stage|stageShape,/mtl/ss3SG/MayaNG_ss3SG/place2dTexture4.inputs:geomprop',
            }
            self.assertEqual(conn, expectedF3p)

        checkState1(self, expectedF3t)

        cmds.undo()
        cmds.redo()

        checkState1(self, expectedF3t)

        # Re-test, but using the default duplicate, that should not copy connections
        # to original SceneItems.
        cmds.undo()
        cmds.duplicate()

        def checkState2(self, expectedF3t):
            f3tDupItem, _, f3pDupItem = tuple(ufe.GlobalSelection.get())
            connections = connectionHandler.sourceConnections(f3tDupItem)
            self.assertIsNotNone(connections)
            conn = set(["{}.{} -> {}.{}".format(ufe.PathString.string(i.src.path), i.src.name,
                                                ufe.PathString.string(i.dst.path), i.dst.name) for i in connections.allConnections()])
            self.assertEqual(conn, expectedF3t)

            # The connection between the new place2dTexture and the original NodeGraph will be gone.
            connections = connectionHandler.sourceConnections(f3pDupItem)
            self.assertIsNotNone(connections)
            self.assertEqual(len(connections.allConnections()), 0)

        checkState2(self, expectedF3t)

        cmds.undo()
        cmds.redo()

        checkState2(self, expectedF3t)

    def testMeshWithEncapsulatedMaterial(self):
        """Duplicate is a complex operation. Test an issue that was found out while debugging:
           Connections on shaders deep in the hierarchy were losing their connections."""
        # Load a scene.
        testFile = testUtils.getTestScene('MaterialX', 'BatchOpsTestScene.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        geomItem = ufeUtils.createUfeSceneItem(shapeNode, '/pPlane2')
        self.assertIsNotNone(geomItem)

        batchOpsHandler = ufe.RunTimeMgr.instance().batchOpsHandler(geomItem.runTimeId())
        self.assertIsNotNone(batchOpsHandler)

        sel = ufe.Selection()
        sel.append(geomItem)

        cmd = batchOpsHandler.duplicateSelectionCmd(sel, {"inputConnections": False})
        cmd.execute()

        def checkStatus(self, cmd, feomItem):
            stage = usdUtils.getPrimFromSceneItem(geomItem).GetStage()
            ss2 = UsdShade.Shader(stage.GetPrimAtPath(Sdf.Path("/pPlane7/mtl/ss2SG/ss2")))
            self.assertTrue(ss2.GetPrim().IsValid())
            base_color = ss2.GetInput("base_color")
            self.assertEqual(base_color.GetAttr().GetPath(), Sdf.Path("/pPlane7/mtl/ss2SG/ss2.inputs:base_color"))
            self.assertTrue(base_color.HasConnectedSource())
            connection = base_color.GetConnectedSources()[0][0]
            self.assertEqual(connection.sourceName, "baseColor")
            self.assertEqual(connection.source.GetPath(), Sdf.Path("/pPlane7/mtl/ss2SG/MayaNG_ss2SG"))
            
        checkStatus(self, cmd, geomItem)

        cmd.undo()
        cmd.redo()

        checkStatus(self, cmd, geomItem)

    def testDuplicatedNodeGraph(self):
        """When cleaning the incoming connections of a duplicated NodeGraph, we want to keep the
           unconnected attributes since they are not intrinsic (like for shader nodes)."""
        testFile = testUtils.getTestScene('MaterialX', 'BatchOpsTestScene.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        ngItem = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss4SG/MayaNG_ss4SG')
        self.assertIsNotNone(ngItem)

        batchOpsHandler = ufe.RunTimeMgr.instance().batchOpsHandler(ngItem.runTimeId())
        self.assertIsNotNone(batchOpsHandler)

        sel = ufe.Selection()
        sel.append(ngItem)

        cmd = batchOpsHandler.duplicateSelectionCmd(sel, {"inputConnections": False})
        cmd.execute()

        dNgPrim = usdUtils.getPrimFromSceneItem(cmd.targetItem(ngItem.path()))
        self.assertTrue(dNgPrim.HasProperty("inputs:file4:varname"))
        self.assertTrue(dNgPrim.HasProperty("outputs:baseColor"))

    @unittest.skipUnless(os.getenv('UFE_BATCH_OPS_HAS_DUPLICATE_TO_TARGET', 'FALSE') == 'TRUE', 'Test requires kDstParentPath option.')
    def testDuplicatedToTarget_invalidDstParentPath(self):
        testFile = testUtils.getTestScene('MaterialX', 'BatchOpsTestScene.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        materialItem = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG')
        childItem = ufe.Hierarchy.createItem(materialItem.path() + 'ss3')

        # Get the USD BatchOpsHandler
        kDstParentPath = ufe.BatchOpsHandler.kDstParentPath
        batchOpsHandler = ufe.RunTimeMgr.instance().batchOpsHandler(materialItem.runTimeId())
        self.assertIsNotNone(batchOpsHandler)

        # Calling the method with a non-existent path should return None.
        nonExistentPath = materialItem.path() + "nonExistentItem"
        nonExistentPathString = ufe.PathString.string(nonExistentPath)
        self.assertIsNone(batchOpsHandler.duplicateSelectionCmd_(ufe.Selection(childItem), {kDstParentPath: nonExistentPathString}))

        # Calling the method with an incorrect type should return None.
        self.assertIsNone(batchOpsHandler.duplicateSelectionCmd_(ufe.Selection(childItem), {kDstParentPath: False}))

        # Calling the method with an empty selection should return None.
        validParentPathString = ufe.PathString.string(materialItem.path())
        self.assertIsNone(batchOpsHandler.duplicateSelectionCmd_(ufe.Selection(), {kDstParentPath: validParentPathString}))

    @unittest.skipUnless(os.getenv('UFE_BATCH_OPS_HAS_DUPLICATE_TO_TARGET', 'FALSE') == 'TRUE', 'Test requires kDstParentPath option.')
    def testDuplicateToTarget_validDstParentPath(self):
        """Test duplicating a selection of items from multiple locations to a target location.
           Verify that connections between the items are maintained but external connections are
           removed."""
        testFile = testUtils.getTestScene('MaterialX', 'BatchOpsTestScene.usda')
        shapeNode,shapeStage = mayaUtils.createProxyFromFile(testFile)

        materialItem1 = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss3SG')
        materialItem2 = ufeUtils.createUfeSceneItem(shapeNode, '/mtl/ss4SG')

        # Items to duplicate:
        ss1 = ufe.Hierarchy.createItem(materialItem1.path() + 'ss3')
        compound1 = ufe.Hierarchy.createItem(materialItem1.path() + 'MayaNG_ss3SG')
        ss2 = ufe.Hierarchy.createItem(materialItem2.path() + 'ss4')
        compound2 = ufe.Hierarchy.createItem(materialItem2.path() + 'MayaNG_ss4SG')
        compound2Child1 = ufe.Hierarchy.createItem(compound2.path() + 'place2dTexture4')
        compound2Child2 = ufe.Hierarchy.createItem(compound2.path() + 'file4')
        selection = ufe.Selection([ss1, compound1, ss2, compound2Child1, compound2Child2])

        # Duplicate this selection to `compound2`.
        dstParentPathString = ufe.PathString.string(compound2.path())
        duplicateCmd = ufe.BatchOpsHandler.duplicateSelectionCmd(selection, {ufe.BatchOpsHandler.kDstParentPath: dstParentPathString})
        self.assertIsNotNone(duplicateCmd)
        numChildrenBefore = len(ufe.Hierarchy.hierarchy(compound2).children())
        duplicateCmd.execute()
        numChildrenAfter = len(ufe.Hierarchy.hierarchy(compound2).children())

        # Delete the materials and undo to make all scene items go stale. Undoing and redoing the 
        # duplicate command should still work correctly afterwards.
        deleteMaterial1Cmd = ufe.SceneItemOps.sceneItemOps(materialItem1).deleteItemCmdNoExecute()
        deleteMaterial1Cmd.execute()
        deleteMaterial1Cmd.undo()
        deleteMaterial2Cmd = ufe.SceneItemOps.sceneItemOps(materialItem2).deleteItemCmdNoExecute()
        deleteMaterial2Cmd.execute()
        deleteMaterial2Cmd.undo()

        # Undo should delete the duplicated items and redo should recreate them.
        compound2Hierarchy = ufe.Hierarchy.hierarchy(ufe.Hierarchy.createItem(compound2.path()))
        duplicateCmd.undo()
        self.assertEqual(numChildrenBefore, len(compound2Hierarchy.children()))
        duplicateCmd.redo()
        self.assertEqual(numChildrenAfter, len(compound2Hierarchy.children()))

        # Verify that all duplicated items are children of the specified parent item.
        self.assertEqual(compound2.path(), duplicateCmd.targetItem(ss1.path()).path().pop())
        self.assertEqual(compound2.path(), duplicateCmd.targetItem(compound1.path()).path().pop())
        self.assertEqual(compound2.path(), duplicateCmd.targetItem(ss2.path()).path().pop())
        self.assertEqual(compound2.path(), duplicateCmd.targetItem(compound2Child1.path()).path().pop())
        self.assertEqual(compound2.path(), duplicateCmd.targetItem(compound2Child2.path()).path().pop())

        # Verify that the connections of the duplicated items are correct. Connections between the
        # items should be maintained but external connections should be removed. The following
        # connections are expected between the duplicated items:
        # - compound1 -> ss1
        # - compound1/someChild -> compound1
        # - compound2Child1 -> compound2Child2
        ch = ufe.RunTimeMgr.instance().connectionHandler(materialItem1.runTimeId())

        ss1Connections = ch.sourceConnections(duplicateCmd.targetItem(ss1.path())).allConnections()
        self.assertEqual(len(ss1Connections), 1)
        self.assertEqual(ss1Connections[0].src.path, duplicateCmd.targetItem(compound1.path()).path())

        compound1Connections = ch.sourceConnections(duplicateCmd.targetItem(compound1.path())).allConnections()
        self.assertEqual(len(compound1Connections), 1)
        self.assertEqual(compound1Connections[0].src.path.pop(), duplicateCmd.targetItem(compound1.path()).path())

        ss2Connections = ch.sourceConnections(duplicateCmd.targetItem(ss2.path())).allConnections()
        self.assertEqual(len(ss2Connections), 0)

        child1Connections = ch.sourceConnections(duplicateCmd.targetItem(compound2Child1.path())).allConnections()
        self.assertEqual(len(child1Connections), 0)

        child2Connections = ch.sourceConnections(duplicateCmd.targetItem(compound2Child2.path())).allConnections()
        self.assertEqual(len(child2Connections), 1)
        self.assertEqual(child2Connections[0].src.path, duplicateCmd.targetItem(compound2Child1.path()).path())

if __name__ == '__main__':
    unittest.main(verbosity=2)
