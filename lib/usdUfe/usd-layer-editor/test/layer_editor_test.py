#
# Copyright 2024 Autodesk
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

import unittest
import sys
import os
import tempfile
import shutil
from stat import S_IREAD

from pxr import Sdf, Usd, UsdUtils, UsdGeom
import ufe
import usdUfe
import UsdLayerEditor

import test_utils

class UsdLayerEditorTest(unittest.TestCase):

    def setUp(self):
        UsdLayerEditorTest._resetScene()
        self.script_folder = os.path.dirname(os.path.realpath(__file__))
        
    def tearDown(self) -> None:
        return None
    
    @staticmethod
    def _createStage():
        raise Exception("_createStage() function not set - must be configured by the DCC")
        
    @staticmethod
    def _resetScene():
        raise Exception("_resetScene() function not set - must be configured by the DCC")        
        
    @staticmethod
    def _undo():
        raise Exception("_undo() function not set - must be configured by the DCC")
        return None
        
    @staticmethod
    def _redo():
        raise Exception("_redo() function not set - must be configured by the DCC")
        return None
        
    @staticmethod
    def _openStageLayerEditor():
        raise Exception("_openStageLayerEditor() function not set - must be configured by the DCC")
        return None

    @staticmethod
    def _executeCmd(cmd):
        # DCC-specific. 3dsmax/pybind11 hosts can delegate to ufe.UndoableCommandMgr.
        # Maya wraps cmd.execute() in an MEL undo chunk because its UFE Python is
        # pybind11 while UsdLayerEditor commands are boost.python-bound.
        raise Exception("_executeCmd() function not set - must be configured by the DCC")
        
    def test_edit_target_cmd(self):
        
        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/root.usda")
        stage.Reload()
        
        mgr = ufe.UndoableCommandMgr.instance()
        
        # Original target is the root anonymous layer.
        stage.SetEditTarget(stage.GetRootLayer())
        rootLayer = stage.GetRootLayer()
        rootLayerId = rootLayer.identifier
        
        currentTargetID = stage.GetEditTarget().GetLayer().identifier
        self.assertEqual(rootLayerId, currentTargetID)

        sublayer = Sdf.Layer.FindOrOpen(self.script_folder + "/data/sublayer.usda")
        
        cmd = UsdLayerEditor.SetEditTargetCommand(stage, sublayer)
        UsdLayerEditorTest._executeCmd(cmd);

        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, sublayer.identifier)
        
        # Can we set the target back to the root?
        cmd2 = UsdLayerEditor.SetEditTargetCommand(stage, rootLayer)
        UsdLayerEditorTest._executeCmd(cmd2);

        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, rootLayer.identifier)
          
        # Undo
        UsdLayerEditorTest._undo()
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, sublayer.identifier)
        
        # Redo
        UsdLayerEditorTest._redo()
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, rootLayer.identifier)
        
        # Test with anonymous sublayer created by AddAnonSubLayerCommand
        addAnonCmd = UsdLayerEditor.AddAnonSubLayerCommand(stage, rootLayer)
        UsdLayerEditorTest._executeCmd(addAnonCmd)
        anonLayerId = addAnonCmd.addedLayer()
        anonLayer = Sdf.Layer.Find(anonLayerId)
        
        # Set edit target to the anonymous layer
        cmd3 = UsdLayerEditor.SetEditTargetCommand(stage, anonLayer)
        UsdLayerEditorTest._executeCmd(cmd3)
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, anonLayerId)
        
        # Verify we can set target back to root from anonymous layer
        cmd4 = UsdLayerEditor.SetEditTargetCommand(stage, rootLayer)
        UsdLayerEditorTest._executeCmd(cmd4)
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, rootLayer.identifier)
        
        # Test undo/redo with anonymous layer
        UsdLayerEditorTest._undo()  # Should go back to anon layer
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, anonLayerId)
        
        UsdLayerEditorTest._redo()  # Should go back to root
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, rootLayer.identifier)
        
    def test_clear_cmd(self):
    
       stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/root.usda")
       rootLayer = stage.GetRootLayer()

       prim_count = sum(1 for _ in stage.Traverse())
       
       self.assertEqual(prim_count, 4)
       self.assertEqual(len(rootLayer.subLayerPaths), 1)
       
       # Clear the layer
       cmd = UsdLayerEditor.ClearLayerCommand(rootLayer)
       mgr = ufe.UndoableCommandMgr.instance()
       UsdLayerEditorTest._executeCmd(cmd)
       
       prim_count = sum(1 for _ in stage.Traverse())
       self.assertEqual(prim_count, 0)
       self.assertEqual(len(rootLayer.subLayerPaths), 0)
       
       # Undo clear
       UsdLayerEditorTest._undo()
       
       prim_count = sum(1 for _ in stage.Traverse())
       self.assertEqual(prim_count, 4)
       self.assertEqual(len(rootLayer.subLayerPaths), 1)
       
       # Redo clear
       UsdLayerEditorTest._redo()
       
       prim_count = sum(1 for _ in stage.Traverse())
       self.assertEqual(len(rootLayer.subLayerPaths), 0)
       self.assertEqual(len(rootLayer.subLayerPaths), 0)
       
       # Test clearing a layer with anonymous sublayers created by AddAnonSubLayerCommand
       UsdLayerEditorTest._undo()  # Go back to state with sublayers
       
       # Add anonymous sublayers using AddAnonSubLayerCommand
       addAnonCmd1 = UsdLayerEditor.AddAnonSubLayerCommand(stage, rootLayer)
       UsdLayerEditorTest._executeCmd(addAnonCmd1)
       anonLayerId1 = addAnonCmd1.addedLayer()
       
       addAnonCmd2 = UsdLayerEditor.AddAnonSubLayerCommand(stage, rootLayer)
       UsdLayerEditorTest._executeCmd(addAnonCmd2)
       anonLayerId2 = addAnonCmd2.addedLayer()
       
       # Should now have 3 sublayers (1 original + 2 anonymous)
       self.assertEqual(len(rootLayer.subLayerPaths), 3)
       self.assertIn(anonLayerId1, rootLayer.subLayerPaths)
       self.assertIn(anonLayerId2, rootLayer.subLayerPaths)
       
       # Clear the layer - should remove all sublayers including anonymous ones
       clearCmd = UsdLayerEditor.ClearLayerCommand(rootLayer)
       UsdLayerEditorTest._executeCmd(clearCmd)
       
       self.assertEqual(len(rootLayer.subLayerPaths), 0)
       
       # Undo clear - should restore all sublayers including anonymous ones
       UsdLayerEditorTest._undo()
       
       self.assertEqual(len(rootLayer.subLayerPaths), 3)
       self.assertIn(anonLayerId1, rootLayer.subLayerPaths)
       self.assertIn(anonLayerId2, rootLayer.subLayerPaths)
              
    def test_mute_cmd(self):
        
        path = self.script_folder + "/data/sublayer.usda"
    
        def testMuteLayerImpl(addLayerFunc):
        
            def checkMuted(layer, stage):
                # Make sure the layer is muted inside the stage.
                self.assertTrue(stage.IsLayerMuted(layer.identifier))
                self.assertTrue(layer.identifier in stage.GetMutedLayers())
                # Make sure the stage does not use the muted layer
                self.assertFalse(layer in stage.GetLayerStack(False))
                self.assertFalse(layer in stage.GetUsedLayers(False))

            def checkUnMuted(layer, stage):
                self.assertFalse(stage.IsLayerMuted(layer.identifier))
                self.assertFalse(layer.identifier in stage.GetMutedLayers())
                self.assertTrue(layer in stage.GetLayerStack(False))
                self.assertTrue(layer in stage.GetUsedLayers(False))

            stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
            
            rootLayer = stage.GetRootLayer()

            layer = addLayerFunc(rootLayer)
            
            checkUnMuted(layer, stage)
            
            # Mute the layer            
            cmd = UsdLayerEditor.MuteLayerCommand(stage, layer, True)
            mgr = ufe.UndoableCommandMgr.instance()
            UsdLayerEditorTest._executeCmd(cmd);
            
            # undo mute
            UsdLayerEditorTest._undo()
            checkUnMuted(layer, stage)

            # redo mute
            UsdLayerEditorTest._redo()
            checkMuted(layer, stage)
        
        # Add an anonymous layer under the "parentLayer" 
        def addAnonymousLayer(parentLayer):
            anon = Sdf.Layer.CreateAnonymous()
            parentLayer.subLayerPaths.append(anon.identifier)
            return anon

        # Add a layer baked file under the "parentLayer"
        # The layer is added by using it's identifier.
        def addFileBakedLayer(parentLayer):
            layer = Sdf.Layer.FindOrOpen(path)
            parentLayer.subLayerPaths.append(layer.identifier)
            return layer

        # Add a layer baked file under the "parentLayer"
        # The layer is added by using it's filesystem path.
        def addFileBakedLayerByPath(parentLayer):
            layer = Sdf.Layer.FindOrOpen(path)
            parentLayer.subLayerPaths.append(path)
            return layer
            
        testMuteLayerImpl(addAnonymousLayer)
        testMuteLayerImpl(addFileBakedLayer)
        testMuteLayerImpl(addFileBakedLayerByPath)
    
    def test_discard_edits_cmd(self):
        
        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        rootLayer = stage.GetRootLayer()
        self.assertEqual(len(rootLayer.subLayerPaths), 0)

        childLayer = Sdf.Layer.CreateAnonymous()
        
        rootLayer.subLayerPaths.append(childLayer.identifier)        
        
        self.assertEqual(len(rootLayer.subLayerPaths), 1)

        cmd = UsdLayerEditor.DiscardEditsCommand(rootLayer)
        mgr = ufe.UndoableCommandMgr.instance()
        UsdLayerEditorTest._executeCmd(cmd);

        # Test everything is gone
        self.assertEqual(len(rootLayer.subLayerPaths), 0)

        # Undo reload
        UsdLayerEditorTest._undo()
        self.assertEqual(len(rootLayer.subLayerPaths), 1)
        
        # Redo reload
        UsdLayerEditorTest._redo()  
        self.assertEqual(len(rootLayer.subLayerPaths), 0)
        
        # Test discarding edits on a layer with anonymous sublayers created by AddAnonSubLayerCommand
        # First, add some anonymous sublayers
        addAnonCmd1 = UsdLayerEditor.AddAnonSubLayerCommand(stage, rootLayer)
        UsdLayerEditorTest._executeCmd(addAnonCmd1)
        anonLayerId1 = addAnonCmd1.addedLayer()
        
        addAnonCmd2 = UsdLayerEditor.AddAnonSubLayerCommand(stage, rootLayer)
        UsdLayerEditorTest._executeCmd(addAnonCmd2)
        anonLayerId2 = addAnonCmd2.addedLayer()
        
        # Verify anonymous layers were added
        self.assertEqual(len(rootLayer.subLayerPaths), 2)
        self.assertIn(anonLayerId1, rootLayer.subLayerPaths)
        self.assertIn(anonLayerId2, rootLayer.subLayerPaths)
        
        # Discard edits - this should remove the anonymous sublayers
        discardCmd = UsdLayerEditor.DiscardEditsCommand(rootLayer)
        UsdLayerEditorTest._executeCmd(discardCmd)
        
        self.assertEqual(len(rootLayer.subLayerPaths), 0)
        
        # Undo discard - should restore the anonymous sublayers
        UsdLayerEditorTest._undo()
        self.assertEqual(len(rootLayer.subLayerPaths), 2)
        self.assertIn(anonLayerId1, rootLayer.subLayerPaths)
        self.assertIn(anonLayerId2, rootLayer.subLayerPaths)
        
        # Redo discard - should remove them again
        UsdLayerEditorTest._redo()
        self.assertEqual(len(rootLayer.subLayerPaths), 0)
        
    def test_lock_layer_cmd(self):
    
        mgr = ufe.UndoableCommandMgr.instance()
        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/root.usda")
        self.assertTrue(stage)
        
        subLayer = Sdf.Layer.FindOrOpen(self.script_folder + "/data/sublayer.usda")
        
        # Make sure we start with an unlocked layer.
        cmd = UsdLayerEditor.LockLayerCommand(stage, subLayer,  UsdLayerEditor.LayerLock_Unlocked)
        UsdLayerEditorTest._executeCmd(cmd);        
        self.assertTrue(subLayer.permissionToEdit)
                        
        # Locking a layer
        cmd = UsdLayerEditor.LockLayerCommand(stage, subLayer,  UsdLayerEditor.LayerLock_Locked)
        UsdLayerEditorTest._executeCmd(cmd);
                
        self.assertFalse(subLayer.permissionToEdit)
        UsdLayerEditorTest._undo()
        self.assertTrue(subLayer.permissionToEdit)
        UsdLayerEditorTest._redo()
        self.assertFalse(subLayer.permissionToEdit)
        
        # Unlocking a layer
        cmd = UsdLayerEditor.LockLayerCommand(stage, subLayer,  UsdLayerEditor.LayerLock_Unlocked)
        UsdLayerEditorTest._executeCmd(cmd);
        
        self.assertTrue(subLayer.permissionToEdit)
        UsdLayerEditorTest._undo()
        self.assertFalse(subLayer.permissionToEdit)
        UsdLayerEditorTest._redo()
        self.assertTrue(subLayer.permissionToEdit)
        
        # System locking a layer
        cmd = UsdLayerEditor.LockLayerCommand(stage, subLayer,  UsdLayerEditor.LayerLock_SystemLocked)
        UsdLayerEditorTest._executeCmd(cmd);
                
        self.assertFalse(subLayer.permissionToEdit)
        self.assertFalse(subLayer.permissionToSave)
        UsdLayerEditorTest._undo()
        self.assertTrue(subLayer.permissionToEdit)
        self.assertTrue(subLayer.permissionToSave)        
        UsdLayerEditorTest._redo()
        self.assertFalse(subLayer.permissionToEdit)
        self.assertFalse(subLayer.permissionToSave)
        
        # Unlock the system lock
        cmd = UsdLayerEditor.LockLayerCommand(stage, subLayer,  UsdLayerEditor.LayerLock_Unlocked)
        UsdLayerEditorTest._executeCmd(cmd);        
        self.assertTrue(subLayer.permissionToEdit)
        self.assertTrue(subLayer.permissionToSave)        
        
        # Test locking anonymous layers created by AddAnonSubLayerCommand
        addAnonCmd = UsdLayerEditor.AddAnonSubLayerCommand(stage, stage.GetRootLayer())
        UsdLayerEditorTest._executeCmd(addAnonCmd)
        anonLayerId = addAnonCmd.addedLayer()
        anonLayer = Sdf.Layer.Find(anonLayerId)
        
        # Verify anonymous layer starts unlocked
        self.assertTrue(anonLayer.permissionToEdit)
        
        # Lock the anonymous layer
        # Note: permissionToSave is related to filepath, and since anonymous layers are not file-backed,
        #       it is always false.
        lockAnonCmd = UsdLayerEditor.LockLayerCommand(stage, anonLayer, UsdLayerEditor.LayerLock_Locked)
        UsdLayerEditorTest._executeCmd(lockAnonCmd)
        
        self.assertFalse(anonLayer.permissionToEdit)
   
        # Undo lock
        UsdLayerEditorTest._undo()
        self.assertTrue(anonLayer.permissionToEdit)
        
        # Redo lock
        UsdLayerEditorTest._redo()
        self.assertFalse(anonLayer.permissionToEdit)

        # System lock the anonymous layer
        sysLockAnonCmd = UsdLayerEditor.LockLayerCommand(stage, anonLayer, UsdLayerEditor.LayerLock_SystemLocked)
        UsdLayerEditorTest._executeCmd(sysLockAnonCmd)
        
        self.assertFalse(anonLayer.permissionToEdit)
        
        # Unlock the anonymous layer
        unlockAnonCmd = UsdLayerEditor.LockLayerCommand(stage, anonLayer, UsdLayerEditor.LayerLock_Unlocked)
        UsdLayerEditorTest._executeCmd(unlockAnonCmd)
        
        self.assertTrue(anonLayer.permissionToEdit)
        
    def test_recursive_lock_single_layer(self):

        mgr = ufe.UndoableCommandMgr.instance()
        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/layerLocking.usda")
        
        topLayer = stage.GetRootLayer();
        subLayer1 = Sdf.Layer.FindRelativeToLayer(topLayer, topLayer.subLayerPaths[0])
        subLayer1_1 = Sdf.Layer.FindRelativeToLayer(subLayer1, subLayer1.subLayerPaths[0])

        # Start all unlocked.
        cmd = UsdLayerEditor.LockLayerCommand(stage, topLayer,  UsdLayerEditor.LayerLock_Unlocked, True)
        UsdLayerEditorTest._executeCmd(cmd);

        self.assertTrue(topLayer.permissionToEdit)
        self.assertTrue(topLayer.permissionToSave)
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        
        # Locking a layer recursively
        cmd = UsdLayerEditor.LockLayerCommand(stage, subLayer1_1,  UsdLayerEditor.LayerLock_Locked, True)
        UsdLayerEditorTest._executeCmd(cmd);
        
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._undo()
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._redo()
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
    
        # Unlocking a layer recursively
        cmd = UsdLayerEditor.LockLayerCommand(stage, subLayer1_1,  UsdLayerEditor.LayerLock_Unlocked, True)
        UsdLayerEditorTest._executeCmd(cmd);
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._undo()
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._redo()
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)

        # System locking a layer recursively
        cmd = UsdLayerEditor.LockLayerCommand(stage, subLayer1_1,  UsdLayerEditor.LayerLock_SystemLocked, True)
        UsdLayerEditorTest._executeCmd(cmd);
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._undo()
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._redo()
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)

        # Unlocking a system-locked layer recursively
        cmd = UsdLayerEditor.LockLayerCommand(stage, subLayer1_1,  UsdLayerEditor.LayerLock_Unlocked, True)
        UsdLayerEditorTest._executeCmd(cmd);
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._undo()
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._redo()
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
       
    def test_recursive_lock_multiLayers(self):
    
        mgr = ufe.UndoableCommandMgr.instance()
        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/layerLocking.usda")

        topLayer = stage.GetRootLayer();
        subLayer1 = Sdf.Layer.FindRelativeToLayer(topLayer, topLayer.subLayerPaths[0])
        subLayer1_1 = Sdf.Layer.FindRelativeToLayer(subLayer1, subLayer1.subLayerPaths[0])

        # Start all unlocked.
        cmd = UsdLayerEditor.LockLayerCommand(stage, topLayer,  UsdLayerEditor.LayerLock_Unlocked, True)
        UsdLayerEditorTest._executeCmd(cmd);
                
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        
        # Locking a layer recursively
        cmd = UsdLayerEditor.LockLayerCommand(stage, subLayer1,  UsdLayerEditor.LayerLock_Locked, True)
        UsdLayerEditorTest._executeCmd(cmd);
        
        self.assertFalse(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._undo()
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._redo()
        self.assertFalse(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
    
        # Unlocking a layer recursively
        cmd = UsdLayerEditor.LockLayerCommand(stage, subLayer1,  UsdLayerEditor.LayerLock_Unlocked, True)
        UsdLayerEditorTest._executeCmd(cmd);
        
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._undo()
        self.assertFalse(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._redo()
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)

        # System locking a layer
        cmd = UsdLayerEditor.LockLayerCommand(stage, subLayer1,  UsdLayerEditor.LayerLock_SystemLocked, True)
        UsdLayerEditorTest._executeCmd(cmd);
        
        self.assertFalse(subLayer1.permissionToEdit)
        self.assertFalse(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._undo()
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._redo()
        self.assertFalse(subLayer1.permissionToEdit)
        self.assertFalse(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)

        # Unlocking a system-locked layer recursively
        #
        # Note: we use the flag to skip system-locked layer to *only* unlock
        #       the layer itself because by design we don't want to recursively
        #       unlock system-locked layers from the UI.
        #
        #       Otherwise, unlocking recursively inthe UI would unlock system
        #       layers, which is not something we want the user to do.
        cmd = UsdLayerEditor.LockLayerCommand(stage, subLayer1,  UsdLayerEditor.LayerLock_Unlocked, True, True)
        UsdLayerEditorTest._executeCmd(cmd);
        
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._undo()
        self.assertFalse(subLayer1.permissionToEdit)
        self.assertFalse(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._redo()
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._undo()

        # Unlocking a system-locked layer recursively
        cmd = UsdLayerEditor.LockLayerCommand(stage, subLayer1,  UsdLayerEditor.LayerLock_Unlocked, True)
        UsdLayerEditorTest._executeCmd(cmd);
        
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._undo()
        self.assertFalse(subLayer1.permissionToEdit)
        self.assertFalse(subLayer1.permissionToSave)
        self.assertFalse(subLayer1_1.permissionToEdit)
        self.assertFalse(subLayer1_1.permissionToSave)
        UsdLayerEditorTest._redo()
        self.assertTrue(subLayer1.permissionToEdit)
        self.assertTrue(subLayer1.permissionToSave)
        self.assertTrue(subLayer1_1.permissionToEdit)
        self.assertTrue(subLayer1_1.permissionToSave)

    def test_remove_sub_path_using_index(self):
    
        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/root_3_layers.usda")
        
        rootLayer = stage.GetRootLayer()
        layer1Id = "sublayer.usda"
        layer2Id = "sublayer_1.usda"
        layer3Id = "sublayer_2.usda"
        
        layer3 = Sdf.Layer.FindRelativeToLayer(rootLayer, rootLayer.subLayerPaths[2])
        childLayer = Sdf.Layer.FindRelativeToLayer(layer3, layer3.subLayerPaths[0])
        layer3ChildId = "child_layer.usda"
        layer3GrandChildId = "grandchild_layer.usda"
        
        mgr = ufe.UndoableCommandMgr.instance()       
        
        # Remove second sublayer
        cmd = UsdLayerEditor.RemoveSubPathCommand(stage, rootLayer, 1)
        UsdLayerEditorTest._executeCmd(cmd)
        self.assertEqual(rootLayer.subLayerPaths, [layer1Id, layer3Id])

        # Remove second sublayer again to leave only one
        cmd = UsdLayerEditor.RemoveSubPathCommand(stage, rootLayer, 1)
        UsdLayerEditorTest._executeCmd(cmd)
        self.assertEqual(rootLayer.subLayerPaths, [layer1Id])

        # Remove second sublayer,  out of bounds -> Exception
        with self.assertRaises(Exception):
            cmd = UsdLayerEditor.RemoveSubPathCommand(stage, rootLayer, 1)
            cmd.execute()
            self.assertEqual(rootLayer.subLayerPaths, [layer1Id])
                
        # undo twice to get back to three layers
        UsdLayerEditorTest._undo()
        UsdLayerEditorTest._undo()
        
        self.assertEqual(rootLayer.subLayerPaths, [layer1Id, layer2Id, layer3Id])
        # redo deletion of second layer
        UsdLayerEditorTest._redo()       
                
        self.assertEqual(rootLayer.subLayerPaths, [layer1Id, layer3Id])
        UsdLayerEditorTest._undo()

        # layer3 has a sub layer which it self also has a sublayer.
        # delete the top layer.  See if it comes back after redo.
        cmd = UsdLayerEditor.RemoveSubPathCommand(stage, rootLayer, 2)
        UsdLayerEditorTest._executeCmd(cmd)
        # check layer3 was deleted
        self.assertEqual(rootLayer.subLayerPaths, [layer1Id, layer2Id])
        # bring it back
        UsdLayerEditorTest._undo()

        layer3 = Sdf.Layer.FindRelativeToLayer(rootLayer, rootLayer.subLayerPaths[2])
        self.assertIsNotNone(layer3)
        
        # check the children were not deleted
        self.assertEqual(layer3.subLayerPaths[0], layer3ChildId)
        childLayer = Sdf.Layer.FindRelativeToLayer(layer3, layer3.subLayerPaths[0])
        self.assertIsNotNone(childLayer)
        self.assertEqual(childLayer.subLayerPaths[0], layer3GrandChildId)
        grandChildLayer = Sdf.Layer.FindRelativeToLayer(childLayer, layer3GrandChildId)
        self.assertIsNotNone(grandChildLayer)
        
        # Test removing anonymous sublayers created by AddAnonSubLayerCommand using index
        # Start fresh with empty stage
        stage2 = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        rootLayer2 = stage2.GetRootLayer()
        
        # Add anonymous sublayers using AddAnonSubLayerCommand
        addAnonCmd1 = UsdLayerEditor.AddAnonSubLayerCommand(stage2, rootLayer2)
        UsdLayerEditorTest._executeCmd(addAnonCmd1)
        anonLayerId1 = addAnonCmd1.addedLayer()
        
        addAnonCmd2 = UsdLayerEditor.AddAnonSubLayerCommand(stage2, rootLayer2)
        UsdLayerEditorTest._executeCmd(addAnonCmd2)
        anonLayerId2 = addAnonCmd2.addedLayer()
        
        addAnonCmd3 = UsdLayerEditor.AddAnonSubLayerCommand(stage2, rootLayer2)
        UsdLayerEditorTest._executeCmd(addAnonCmd3)
        anonLayerId3 = addAnonCmd3.addedLayer()
        
        # Should have 3 anonymous sublayers
        self.assertEqual(len(rootLayer2.subLayerPaths), 3)
        self.assertEqual(rootLayer2.subLayerPaths, [anonLayerId3, anonLayerId2, anonLayerId1])  # Most recent first
        
        # Remove middle anonymous sublayer (index 1)
        removeCmd1 = UsdLayerEditor.RemoveSubPathCommand(stage2, rootLayer2, 1)
        UsdLayerEditorTest._executeCmd(removeCmd1)
        self.assertEqual(len(rootLayer2.subLayerPaths), 2)
        self.assertEqual(rootLayer2.subLayerPaths, [anonLayerId3, anonLayerId1])
        
        # Remove first anonymous sublayer (index 0)
        removeCmd2 = UsdLayerEditor.RemoveSubPathCommand(stage2, rootLayer2, 0)
        UsdLayerEditorTest._executeCmd(removeCmd2)
        self.assertEqual(len(rootLayer2.subLayerPaths), 1)
        self.assertEqual(rootLayer2.subLayerPaths, [anonLayerId1])
        
        # Test undo/redo
        UsdLayerEditorTest._undo()  # Should restore anonLayerId3
        self.assertEqual(len(rootLayer2.subLayerPaths), 2)
        self.assertEqual(rootLayer2.subLayerPaths, [anonLayerId3, anonLayerId1])
        
        UsdLayerEditorTest._undo()  # Should restore anonLayerId2
        self.assertEqual(len(rootLayer2.subLayerPaths), 3)
        self.assertEqual(rootLayer2.subLayerPaths, [anonLayerId3, anonLayerId2, anonLayerId1])

    def test_remove_sub_path_using_path(self):
    
        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/root_3_layers.usda")
        
        rootLayer = stage.GetRootLayer()
        layer1Id = "sublayer.usda"
        layer2Id = "sublayer_1.usda"
        layer3Id = "sublayer_2.usda"
        
        layer3 = Sdf.Layer.FindRelativeToLayer(rootLayer, rootLayer.subLayerPaths[2])
        childLayer = Sdf.Layer.FindRelativeToLayer(layer3, layer3.subLayerPaths[0])
        layer3ChildId = "child_layer.usda"
        layer3GrandChildId = "grandchild_layer.usda"
        
        mgr = ufe.UndoableCommandMgr.instance()       
        
        # Remove second sublayer
        cmd = UsdLayerEditor.RemoveSubPathCommand(stage, rootLayer, layer2Id)
        UsdLayerEditorTest._executeCmd(cmd)
        self.assertEqual(rootLayer.subLayerPaths, [layer1Id, layer3Id])

        # Remove second sublayer again to leave only one
        cmd = UsdLayerEditor.RemoveSubPathCommand(stage, rootLayer, layer3Id)
        UsdLayerEditorTest._executeCmd(cmd)
        self.assertEqual(rootLayer.subLayerPaths, [layer1Id])
                
        # undo twice to get back to three layers
        UsdLayerEditorTest._undo()
        UsdLayerEditorTest._undo()
        
        self.assertEqual(rootLayer.subLayerPaths, [layer1Id, layer2Id, layer3Id])
        # redo deletion of second layer
        UsdLayerEditorTest._redo()       
                
        self.assertEqual(rootLayer.subLayerPaths, [layer1Id, layer3Id])
        UsdLayerEditorTest._undo()

        # layer3 has a sub layer which it self also has a sublayer.
        # delete the top layer.  See if it comes back after redo.
        cmd = UsdLayerEditor.RemoveSubPathCommand(stage, rootLayer, layer3Id)
        UsdLayerEditorTest._executeCmd(cmd)
        # check layer3 was deleted
        self.assertEqual(rootLayer.subLayerPaths, [layer1Id, layer2Id])
        # bring it back
        UsdLayerEditorTest._undo()

        layer3 = Sdf.Layer.FindRelativeToLayer(rootLayer, rootLayer.subLayerPaths[2])
        self.assertIsNotNone(layer3)
        
        # check the children were not deleted
        self.assertEqual(layer3.subLayerPaths[0], layer3ChildId)
        childLayer = Sdf.Layer.FindRelativeToLayer(layer3, layer3.subLayerPaths[0])
        self.assertIsNotNone(childLayer)
        self.assertEqual(childLayer.subLayerPaths[0], layer3GrandChildId)
        grandChildLayer = Sdf.Layer.FindRelativeToLayer(childLayer, layer3GrandChildId)
        self.assertIsNotNone(grandChildLayer)

        # Test removing anonymous sublayers created by AddAnonSubLayerCommand using path
        # Start fresh with empty stage
        stage2 = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        rootLayer2 = stage2.GetRootLayer()
        
        # Add anonymous sublayers using AddAnonSubLayerCommand
        addAnonCmd1 = UsdLayerEditor.AddAnonSubLayerCommand(stage2, rootLayer2)
        UsdLayerEditorTest._executeCmd(addAnonCmd1)
        anonLayerId1 = addAnonCmd1.addedLayer()
        
        addAnonCmd2 = UsdLayerEditor.AddAnonSubLayerCommand(stage2, rootLayer2)
        UsdLayerEditorTest._executeCmd(addAnonCmd2)
        anonLayerId2 = addAnonCmd2.addedLayer()
        
        # Should have 2 anonymous sublayers
        self.assertEqual(len(rootLayer2.subLayerPaths), 2)
        self.assertIn(anonLayerId1, rootLayer2.subLayerPaths)
        self.assertIn(anonLayerId2, rootLayer2.subLayerPaths)
        
        # Remove first anonymous sublayer by path
        removeCmd1 = UsdLayerEditor.RemoveSubPathCommand(stage2, rootLayer2, anonLayerId1)
        UsdLayerEditorTest._executeCmd(removeCmd1)
        self.assertEqual(len(rootLayer2.subLayerPaths), 1)
        self.assertNotIn(anonLayerId1, rootLayer2.subLayerPaths)
        self.assertIn(anonLayerId2, rootLayer2.subLayerPaths)
        
        # Remove second anonymous sublayer by path
        removeCmd2 = UsdLayerEditor.RemoveSubPathCommand(stage2, rootLayer2, anonLayerId2)
        UsdLayerEditorTest._executeCmd(removeCmd2)
        self.assertEqual(len(rootLayer2.subLayerPaths), 0)
        
        # Test undo/redo
        UsdLayerEditorTest._undo()  # Should restore anonLayerId2
        self.assertEqual(len(rootLayer2.subLayerPaths), 1)
        self.assertIn(anonLayerId2, rootLayer2.subLayerPaths)
        
        UsdLayerEditorTest._undo()  # Should restore anonLayerId1
        self.assertEqual(len(rootLayer2.subLayerPaths), 2)
        self.assertIn(anonLayerId1, rootLayer2.subLayerPaths)
        self.assertIn(anonLayerId2, rootLayer2.subLayerPaths)

    def test_insert_sub_path(self):
        
        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        mgr = ufe.UndoableCommandMgr.instance()
        
        rootLayer = stage.GetRootLayer()
        
        # Test insertion of layers at different indices..
        first = "sublayer.usda"
        second = "sublayer_1.usda"
        middle = "sublayer_2.usda"
        end = "child_layer.usda"
        
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, second, 0)
        UsdLayerEditorTest._executeCmd(cmd)
                
        self.assertEqual(rootLayer.subLayerPaths, [second])
        
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, first, 0)
        UsdLayerEditorTest._executeCmd(cmd)
        
        self.assertEqual(rootLayer.subLayerPaths, [first, second])
        
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, middle, 1)
        UsdLayerEditorTest._executeCmd(cmd)
        
        self.assertEqual(rootLayer.subLayerPaths, [first, middle, second])
        
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, end, 3)
        UsdLayerEditorTest._executeCmd(cmd)
        
        self.assertEqual(rootLayer.subLayerPaths, [first, middle, second, end])
        
        # Bad indices -> Exception
        with self.assertRaises(Exception):
            cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, "bogus", -2)
            cmd.execute()
        with self.assertRaises(Exception):
            cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, "bogus", 10)
            cmd.execute() 
        
        # No change expected
        self.assertEqual(rootLayer.subLayerPaths, [first, middle, second, end])    

        # Test duplicate layer exception with original stage
        # Duplicate layer  -> Exception
        with self.assertRaises(Exception):
            cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, first, 0)
            cmd.execute()

        # No change expected
        self.assertEqual(rootLayer.subLayerPaths, [first, middle, second, end])
        
        # Test inserting sublayers with anonymous layers created by AddAnonSubLayerCommand
        # Start with a fresh stage
        stage2 = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        rootLayer2 = stage2.GetRootLayer()
        
        # First add some anonymous layers using AddAnonSubLayerCommand
        addAnonCmd1 = UsdLayerEditor.AddAnonSubLayerCommand(stage2, rootLayer2)
        UsdLayerEditorTest._executeCmd(addAnonCmd1)
        anonLayerId1 = addAnonCmd1.addedLayer()
        
        addAnonCmd2 = UsdLayerEditor.AddAnonSubLayerCommand(stage2, rootLayer2)
        UsdLayerEditorTest._executeCmd(addAnonCmd2)
        anonLayerId2 = addAnonCmd2.addedLayer()
        
        # Should have 2 anonymous layers, most recent first
        self.assertEqual(len(rootLayer2.subLayerPaths), 2)
        self.assertEqual(rootLayer2.subLayerPaths, [anonLayerId2, anonLayerId1])
        
        # Insert a regular layer at the beginning (index 0)
        insertCmd1 = UsdLayerEditor.InsertSubPathCommand(stage2, rootLayer2, first, 0)
        UsdLayerEditorTest._executeCmd(insertCmd1)
        self.assertEqual(rootLayer2.subLayerPaths, [first, anonLayerId2, anonLayerId1])
        
        # Insert a regular layer in the middle (index 2)
        insertCmd2 = UsdLayerEditor.InsertSubPathCommand(stage2, rootLayer2, middle, 2)
        UsdLayerEditorTest._executeCmd(insertCmd2)
        self.assertEqual(rootLayer2.subLayerPaths, [first, anonLayerId2, middle, anonLayerId1])
        
        # Insert a regular layer at the end
        insertCmd3 = UsdLayerEditor.InsertSubPathCommand(stage2, rootLayer2, end, 4)
        UsdLayerEditorTest._executeCmd(insertCmd3)
        self.assertEqual(rootLayer2.subLayerPaths, [first, anonLayerId2, middle, anonLayerId1, end])
        
        # Test undo/redo
        UsdLayerEditorTest._undo()  # Remove end
        self.assertEqual(rootLayer2.subLayerPaths, [first, anonLayerId2, middle, anonLayerId1])
        
        UsdLayerEditorTest._undo()  # Remove middle
        self.assertEqual(rootLayer2.subLayerPaths, [first, anonLayerId2, anonLayerId1])
        
        UsdLayerEditorTest._redo()  # Restore middle
        self.assertEqual(rootLayer2.subLayerPaths, [first, anonLayerId2, middle, anonLayerId1])


    def test_refresh_system_lock(self):
        
        # FileBacked Layer Write Permission
        # 1- Loading the test scene
        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        topLayer = stage.GetRootLayer();
        
        mgr = ufe.UndoableCommandMgr.instance()
        
        # 2- Setting a system lock on a layer loaded from a file
        # System locking a layer
        cmd = UsdLayerEditor.LockLayerCommand(stage, topLayer,  UsdLayerEditor.LayerLock_SystemLocked)
        UsdLayerEditorTest._executeCmd(cmd);
        
        self.assertFalse(topLayer.permissionToEdit)
        self.assertFalse(topLayer.permissionToSave)
        
        # 3- Refreshing the system lock should remove the lock if the file is writable
        cmd = UsdLayerEditor.RefreshSystemLockLayerCommand(stage, topLayer, False)
        UsdLayerEditorTest._executeCmd(cmd);
        
        self.assertTrue(topLayer.permissionToEdit)
        self.assertTrue(topLayer.permissionToSave)

    def test_refresh_system_lock_callback(self):
        # FileBacked Layer Write Permission
        # 1- Loading the test scene
        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/layerLocking.usda")
        topLayer = stage.GetRootLayer();
        subLayer = Sdf.Layer.FindRelativeToLayer(topLayer, topLayer.subLayerPaths[0])
        
        mgr = ufe.UndoableCommandMgr.instance()
        
        # 2- Setting a system lock on a layer loaded from a file and its sub-layer
        cmd = UsdLayerEditor.LockLayerCommand(stage, topLayer,  UsdLayerEditor.LayerLock_SystemLocked)
        UsdLayerEditorTest._executeCmd(cmd);
        
        self.assertFalse(topLayer.permissionToEdit)
        self.assertFalse(topLayer.permissionToSave)
        # 3- Attach callbacks to capture any system-lock changes due to refreshSystemLock
        self.callCount = 0
        def refreshSystemLockCallback(context, callbackData):
            layerIds = callbackData.get('affectedLayerIds')
            # Check that only the top layers is affected
            self.assertTrue(len(layerIds), 1)
            self.assertEqual(layerIds[0], topLayer.identifier)
            # Check that the top layer is unlocked due to the refresh
            self.assertTrue(topLayer.permissionToEdit)
            self.assertTrue(topLayer.permissionToSave)
            # Check that the sublayer is unchanged
            self.assertTrue(subLayer.permissionToEdit)
            self.assertTrue(subLayer.permissionToSave)
            self.callCount = self.callCount + 1
            
        usdUfe.registerUICallback('onRefreshSystemLock', refreshSystemLockCallback)        
        # 4- Refreshing the system lock should remove the lock.
        cmd = UsdLayerEditor.RefreshSystemLockLayerCommand(stage, topLayer, False)
        UsdLayerEditorTest._executeCmd(cmd);
        self.assertEqual(self.callCount, 1)

        # 5- Unregistering the callback and refreshing the system lock should not call
        #    call the callback again.
        #
        # Note: we must relock the layer for the callback to be called, otherwise it does not get
        #       called as the status of the layer would not have changed during the refresh.
        cmd = UsdLayerEditor.LockLayerCommand(stage, topLayer,  UsdLayerEditor.LayerLock_SystemLocked)
        UsdLayerEditorTest._executeCmd(cmd);
        
        usdUfe.unregisterUICallback('onRefreshSystemLock', refreshSystemLockCallback)
        cmd = UsdLayerEditor.RefreshSystemLockLayerCommand(stage, topLayer, False)
        UsdLayerEditorTest._executeCmd(cmd);
        self.assertEqual(self.callCount, 1)

        # 6- Unregistering again should do nothing and not crash.
        usdUfe.unregisterUICallback('onRefreshSystemLock', refreshSystemLockCallback)        

    def _verifyStageAfterRefreshSystemLock(
            self, writableFiles, expectedLayerModifiable, callback=None):

        with test_utils.TemporaryDirectory(prefix='RefreshLock') as testDir:
            # Create a stage with a simple layer stack.
            rootLayerPath = os.path.join(testDir, "root.usda")
            rootLayer = Sdf.Layer.CreateNew(rootLayerPath)

            subLayerPath = os.path.join(testDir, "sub.usda")
            subLayer = Sdf.Layer.CreateNew(subLayerPath)

            rootLayer.subLayerPaths.append(subLayer.identifier)
            rootLayer.Save()

            stage = UsdLayerEditorTest._createStage(rootLayerPath)
            stage.SetEditTarget(stage.GetRootLayer())

            # Apply requested file permissions if needed.
            if not writableFiles:
                for layer in stage.GetLayerStack(False):
                    os.chmod(layer.realPath, S_IREAD)

            mgr = ufe.UndoableCommandMgr.instance()
            
            if callback is not None:
                # Install the given callback.
                usdUfe.registerUICallback('onRefreshSystemLock', callback)

                # Alter a layer lock to ensure the callback is triggered on
                # refreshSystemLock.
                lockStatus = UsdLayerEditor.LayerLock_Unlocked if UsdLayerEditor.isLayerSystemLocked(rootLayer) else UsdLayerEditor.LayerLock_SystemLocked
                cmd = UsdLayerEditor.LockLayerCommand(stage, rootLayer,  lockStatus)
                UsdLayerEditorTest._executeCmd(cmd);
            
            cmd = UsdLayerEditor.RefreshSystemLockLayerCommand(stage, rootLayer, True)
            UsdLayerEditorTest._executeCmd(cmd);

            if callback is not None:
                usdUfe.unregisterUICallback('onRefreshSystemLock', callback)

            # Verify that the expected locks were applied
            # e.g. during the callback.
            self.assertEqual(usdUfe.isAnyLayerModifiable(stage),
                             expectedLayerModifiable)

            # Verify that refreshSystemLock properly handled the editTarget
            # e.g. that it accounts for lock changes during the callback.
            if expectedLayerModifiable:
                # The initial target should be preserved in this case.
                expectedTargetLayer = rootLayer
            else:
                # Edit target should have been forced to session layer.
                expectedTargetLayer = stage.GetSessionLayer()

            self.assertEqual(stage.GetEditTarget().GetLayer(),
                             expectedTargetLayer)

    def test_refresh_system_lock_without_callback(self):
        """
        Test refreshSystemLocks without any callback.
        """
        self._verifyStageAfterRefreshSystemLock(
            writableFiles=False, expectedLayerModifiable=False)

        self._verifyStageAfterRefreshSystemLock(
            writableFiles=True, expectedLayerModifiable=True)

    def test_refresh_system_lock_callback_locking_all(self):
        """
        Test refreshSystemLocks with a callback that force a systemLock on all
        layers even if the usd files are writable.
        """

        def callback(context, callbackData):
            objectPath = context.get('objectPath')
            stage = usdUfe.getStage(objectPath)
            for layer in stage.GetLayerStack(False):
                cmd = UsdLayerEditor.LockLayerCommand(stage, layer,  UsdLayerEditor.LayerLock_Locked)
                cmd.execute()
                            
        self._verifyStageAfterRefreshSystemLock(
            writableFiles=True, expectedLayerModifiable=False,
            callback=callback)

    def test_refresh_system_lock_with_callback_unlocking_all(self):
        """
        Test refreshSystemLocks with a callback that will unlock all
        layers while they were automatically locked according to usd files
        permissions.
        """
        def callback(context, callbackData):
            objectPath = context.get('objectPath')
            stage = usdUfe.getStage(objectPath)
            for layerId in callbackData.get('affectedLayerIds'):
                cmd = UsdLayerEditor.LockLayerCommand(stage, Sdf.Find(layerId), UsdLayerEditor.LayerLock_Unlocked)
                cmd.execute()

        self._verifyStageAfterRefreshSystemLock(
            writableFiles=False, expectedLayerModifiable=True,
            callback=callback)

    def test_refresh_systemlock_with_callback_unlocking_edit_target(self):
        """
        Test refreshSystemLocks with a callback that unlocks the current
        edit target layer while it was automatically locked according
        to usd file permission.
        """
        def callback(context, callbackData):
            objectPath = context.get('objectPath')
            stage = usdUfe.getStage(objectPath)
            cmd = UsdLayerEditor.LockLayerCommand(stage, stage.GetEditTarget().GetLayer(), UsdLayerEditor.LayerLock_Unlocked)
            cmd.execute()

        self._verifyStageAfterRefreshSystemLock(
            writableFiles=False, expectedLayerModifiable=True,
            callback=callback)

    def test_layer_editor_selection(self):
        stage = UsdLayerEditorTest._openStageLayerEditor(self.script_folder + "/data/root_3_layers.usda")
            
        # Make sure there are no selected layers
        selectedLayers = UsdLayerEditor.getSelectedLayers()
        self.assertEqual(len(selectedLayers), 0)

        # There should only be 6 layers total in this stage
        layers = stage.GetLayerStack(False)
        self.assertEqual(len(layers), 6)

        # Set to a layer that exists in the stage layer stack
        newLayerSelection = [layers[0].identifier]

        # Register a callback for later testing
        self.callCount = 0
        def selectionChangedCallback(context, callbackData):
            self.callCount = self.callCount + 1
            # check that the callbackData contains the selected layers
            layers = callbackData.get("layerIds")
            self.assertEqual(newLayerSelection, layers)
            
        usdUfe.registerUICallback('onLayerEditorSelectionChanged', selectionChangedCallback)

        # Select 1 layer that exists
        UsdLayerEditor.setSelectedLayers(newLayerSelection)

        # Unregister the callback
        usdUfe.unregisterUICallback('onLayerEditorSelectionChanged', selectionChangedCallback)

        # Make sure the number of selected layers is now 1
        selectedLayers = UsdLayerEditor.getSelectedLayers()
        self.assertEqual(len(selectedLayers), 1)

        # Check that called was incremented due to the registered callback
        self.assertEqual(self.callCount, 1)

        # New selection with all layers, inclues sublayers and sublayer to sublayer
        newLayerSelection = [layers[0].identifier, layers[1].identifier, layers[2].identifier, layers[3].identifier, layers[4].identifier, layers[5].identifier]

        UsdLayerEditor.setSelectedLayers(newLayerSelection)

        # Make sure the number of selected layers is now 6
        selectedLayers = UsdLayerEditor.getSelectedLayers()
        self.assertEqual(len(selectedLayers), 6)

        # Deselect
        UsdLayerEditor.setSelectedLayers([])
        # Make sure the number of selected layers is now 0
        selectedLayers = UsdLayerEditor.getSelectedLayers()
        self.assertEqual(len(selectedLayers), 0)

        # Test selection with anonymous layers created by AddAnonSubLayerCommand
        # Add some anonymous layers to the stage
        mgr = ufe.UndoableCommandMgr.instance()
        
        addAnonCmd1 = UsdLayerEditor.AddAnonSubLayerCommand(stage, stage.GetRootLayer())
        UsdLayerEditorTest._executeCmd(addAnonCmd1)
        anonLayerId1 = addAnonCmd1.addedLayer()
        
        addAnonCmd2 = UsdLayerEditor.AddAnonSubLayerCommand(stage, stage.GetRootLayer())
        UsdLayerEditorTest._executeCmd(addAnonCmd2)
        anonLayerId2 = addAnonCmd2.addedLayer()
        
        # Verify anonymous layers were added to the stage
        updatedLayers = stage.GetLayerStack(False)
        self.assertIn(Sdf.Layer.Find(anonLayerId1), updatedLayers)
        self.assertIn(Sdf.Layer.Find(anonLayerId2), updatedLayers)
        
        # Test selecting a single anonymous layer
        # KNOWN ISSUE: due to QTimer::singleShot in modelrebuild being called async
        UsdLayerEditor.setSelectedLayers([anonLayerId1])
        selectedLayers = UsdLayerEditor.getSelectedLayers()
        self.assertEqual(len(selectedLayers), 1)
        self.assertEqual(selectedLayers[0], anonLayerId1)
        
        # Test selecting multiple anonymous layers
        UsdLayerEditor.setSelectedLayers([anonLayerId1, anonLayerId2])
        selectedLayers = UsdLayerEditor.getSelectedLayers()
        self.assertEqual(len(selectedLayers), 2)
        self.assertIn(anonLayerId1, selectedLayers)
        self.assertIn(anonLayerId2, selectedLayers)
        
        # Test selecting mix of anonymous and regular layers
        regularLayerId = updatedLayers[0].identifier  # Get a regular layer
        if regularLayerId != anonLayerId1 and regularLayerId != anonLayerId2:
            mixedSelection = [regularLayerId, anonLayerId1]
            UsdLayerEditor.setSelectedLayers(mixedSelection)
            selectedLayers = UsdLayerEditor.getSelectedLayers()
            self.assertEqual(len(selectedLayers), 2)
            self.assertIn(regularLayerId, selectedLayers)
            self.assertIn(anonLayerId1, selectedLayers)
        
        # Clear selection
        UsdLayerEditor.setSelectedLayers([])
        selectedLayers = UsdLayerEditor.getSelectedLayers()
        self.assertEqual(len(selectedLayers), 0)

    def test_add_anon_sublayer_cmd(self):
        """Test basic functionality of AddAnonSubLayerCommand"""
        
        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        rootLayer = stage.GetRootLayer()
        mgr = ufe.UndoableCommandMgr.instance()
        
        # Initially, root layer should have no sublayers
        self.assertEqual(len(rootLayer.subLayerPaths), 0)
        
        # Create and execute AddAnonSubLayerCommand
        cmd = UsdLayerEditor.AddAnonSubLayerCommand(stage, rootLayer)
        UsdLayerEditorTest._executeCmd(cmd)
        
        # Verify that a sublayer was added
        self.assertEqual(len(rootLayer.subLayerPaths), 1)
        
        # Get the added layer identifier and verify it's anonymous
        addedLayerId = cmd.addedLayer()
        self.assertIsNotNone(addedLayerId)
        self.assertNotEqual(addedLayerId, "")
        
        # Verify the sublayer path matches the added layer identifier
        self.assertEqual(rootLayer.subLayerPaths[0], addedLayerId)
        
        # Verify the added layer exists and is anonymous
        addedLayer = Sdf.Layer.Find(addedLayerId)
        self.assertIsNotNone(addedLayer)
        self.assertTrue(addedLayer.anonymous)
        
        # Test undo
        UsdLayerEditorTest._undo()
        self.assertEqual(len(rootLayer.subLayerPaths), 0)
        
        # Test redo
        UsdLayerEditorTest._redo()
        self.assertEqual(len(rootLayer.subLayerPaths), 1)
        self.assertEqual(rootLayer.subLayerPaths[0], addedLayerId)
        
        # Verify the layer still exists after redo
        addedLayer = Sdf.Layer.Find(addedLayerId)
        self.assertIsNotNone(addedLayer)
        self.assertTrue(addedLayer.anonymous)

    def test_add_anon_sublayer_cmd_multiple_layers(self):
        """Test adding multiple anonymous sublayers"""
        
        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        rootLayer = stage.GetRootLayer()
        mgr = ufe.UndoableCommandMgr.instance()
        
        # Add first anonymous layer
        cmd1 = UsdLayerEditor.AddAnonSubLayerCommand(stage, rootLayer)
        UsdLayerEditorTest._executeCmd(cmd1)
        addedLayerId1 = cmd1.addedLayer()
        
        self.assertEqual(len(rootLayer.subLayerPaths), 1)
        self.assertEqual(rootLayer.subLayerPaths[0], addedLayerId1)
        
        # Add second anonymous layer (should be inserted at index 0, becoming the first)
        cmd2 = UsdLayerEditor.AddAnonSubLayerCommand(stage, rootLayer)
        UsdLayerEditorTest._executeCmd(cmd2)
        addedLayerId2 = cmd2.addedLayer()
        
        self.assertEqual(len(rootLayer.subLayerPaths), 2)
        self.assertEqual(rootLayer.subLayerPaths[0], addedLayerId2)  # Most recent at index 0
        self.assertEqual(rootLayer.subLayerPaths[1], addedLayerId1)  # Previous at index 1
        
        # Verify both layers are different and anonymous
        self.assertNotEqual(addedLayerId1, addedLayerId2)
        
        addedLayer1 = Sdf.Layer.Find(addedLayerId1)
        addedLayer2 = Sdf.Layer.Find(addedLayerId2)
        self.assertIsNotNone(addedLayer1)
        self.assertIsNotNone(addedLayer2)
        self.assertTrue(addedLayer1.anonymous)
        self.assertTrue(addedLayer2.anonymous)
        
        # Test undo of second command
        UsdLayerEditorTest._undo()
        self.assertEqual(len(rootLayer.subLayerPaths), 1)
        self.assertEqual(rootLayer.subLayerPaths[0], addedLayerId1)
        
        # Test undo of first command
        UsdLayerEditorTest._undo()
        self.assertEqual(len(rootLayer.subLayerPaths), 0)
        
        # Test redo of first command
        UsdLayerEditorTest._redo()
        self.assertEqual(len(rootLayer.subLayerPaths), 1)
        self.assertEqual(rootLayer.subLayerPaths[0], addedLayerId1)
        
        # Test redo of second command
        UsdLayerEditorTest._redo()
        self.assertEqual(len(rootLayer.subLayerPaths), 2)
        self.assertEqual(rootLayer.subLayerPaths[0], addedLayerId2)
        self.assertEqual(rootLayer.subLayerPaths[1], addedLayerId1)

    def test_add_anon_sublayer_cmd_to_existing_sublayers(self):
        """Test adding anonymous sublayer to a layer that already has sublayers"""
        
        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/root.usda")
        rootLayer = stage.GetRootLayer()
        mgr = ufe.UndoableCommandMgr.instance()
        
        # root.usda should already have one sublayer
        initialSubLayerCount = len(rootLayer.subLayerPaths)
        self.assertEqual(initialSubLayerCount, 1)
        originalSubLayerPath = rootLayer.subLayerPaths[0]
        
        # Add anonymous sublayer
        cmd = UsdLayerEditor.AddAnonSubLayerCommand(stage, rootLayer)
        UsdLayerEditorTest._executeCmd(cmd)
        addedLayerId = cmd.addedLayer()
        
        # Should now have one more sublayer, with the anonymous layer at index 0
        self.assertEqual(len(rootLayer.subLayerPaths), initialSubLayerCount + 1)
        self.assertEqual(rootLayer.subLayerPaths[0], addedLayerId)
        self.assertEqual(rootLayer.subLayerPaths[1], originalSubLayerPath)
        
        # Verify the added layer is anonymous
        addedLayer = Sdf.Layer.Find(addedLayerId)
        self.assertIsNotNone(addedLayer)
        self.assertTrue(addedLayer.anonymous)
        
        # Test undo - should restore original state
        UsdLayerEditorTest._undo()
        self.assertEqual(len(rootLayer.subLayerPaths), initialSubLayerCount)
        self.assertEqual(rootLayer.subLayerPaths[0], originalSubLayerPath)
        
        # Test redo
        UsdLayerEditorTest._redo()
        self.assertEqual(len(rootLayer.subLayerPaths), initialSubLayerCount + 1)
        self.assertEqual(rootLayer.subLayerPaths[0], addedLayerId)
        self.assertEqual(rootLayer.subLayerPaths[1], originalSubLayerPath)

    def _createAnonymousLayer(self, stage, parentLayer, name=""):
        """Helper to create an anonymous sublayer and return its identifier."""
        mgr = ufe.UndoableCommandMgr.instance()
        cmd = UsdLayerEditor.AddAnonSubLayerCommand(stage, parentLayer)
        UsdLayerEditorTest._executeCmd(cmd)
        return cmd.addedLayer()

    def test_stitch_layers(self):
        """Test basic stitch of 3 layers with undo/redo"""

        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        rootLayer = stage.GetRootLayer()
        rootLayerId = rootLayer.identifier
        mgr = ufe.UndoableCommandMgr.instance()

        # Create a hierarchy of layers with content
        # Root
        #   Layer1 (selected, strongest)
        #   Layer2 (selected)
        #   Layer3 (selected, weakest)

        layer1Id = self._createAnonymousLayer(stage, rootLayer)
        layer2Id = self._createAnonymousLayer(stage, rootLayer)
        layer3Id = self._createAnonymousLayer(stage, rootLayer)

        # Clear and reinsert in desired strength order
        rootLayer.subLayerPaths.clear()
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, layer1Id, 0)
        UsdLayerEditorTest._executeCmd(cmd)
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, layer2Id, 1)
        UsdLayerEditorTest._executeCmd(cmd)
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, layer3Id, 2)
        UsdLayerEditorTest._executeCmd(cmd)

        layer1 = Sdf.Layer.Find(layer1Id)
        layer2 = Sdf.Layer.Find(layer2Id)
        layer3 = Sdf.Layer.Find(layer3Id)

        with Sdf.ChangeBlock():
            prim1 = Sdf.CreatePrimInLayer(layer1, '/Sphere')
            prim1.SetInfo('typeName', 'Sphere')
            attr1 = Sdf.AttributeSpec(prim1, 'color', Sdf.ValueTypeNames.String)
            attr1.default = "red"

        with Sdf.ChangeBlock():
            prim2 = Sdf.CreatePrimInLayer(layer2, '/Sphere')
            prim2.SetInfo('typeName', 'Sphere')
            attr2 = Sdf.AttributeSpec(prim2, 'radius', Sdf.ValueTypeNames.Double)
            attr2.default = 2.0

        with Sdf.ChangeBlock():
            prim3 = Sdf.CreatePrimInLayer(layer3, '/Sphere')
            prim3.SetInfo('typeName', 'Sphere')
            attr3 = Sdf.AttributeSpec(prim3, 'visible', Sdf.ValueTypeNames.Bool)
            attr3.default = False

        self.assertEqual(len(rootLayer.subLayerPaths), 3)
        self.assertIn(layer1Id, rootLayer.subLayerPaths)
        self.assertIn(layer2Id, rootLayer.subLayerPaths)
        self.assertIn(layer3Id, rootLayer.subLayerPaths)

        # Stitch - order passed does not matter, strongest receives all other layers.
        cmd = UsdLayerEditor.StitchLayersCommand(stage, [layer2Id, layer1Id, layer3Id])
        UsdLayerEditorTest._executeCmd(cmd)

        self.assertEqual(len(rootLayer.subLayerPaths), 1)
        self.assertEqual(rootLayer.subLayerPaths[0], layer1Id)

        stitchedPrim = layer1.GetPrimAtPath('/Sphere')
        self.assertIsNotNone(stitchedPrim)

        self.assertIsNotNone(stitchedPrim.properties.get('color'))
        self.assertIsNotNone(stitchedPrim.properties.get('radius'))
        self.assertIsNotNone(stitchedPrim.properties.get('visible'))

        self.assertEqual(stitchedPrim.properties.get('color').default, "red")
        self.assertEqual(stitchedPrim.properties.get('radius').default, 2.0)
        self.assertEqual(stitchedPrim.properties.get('visible').default, False)

        UsdLayerEditorTest._undo()

        self.assertEqual(len(rootLayer.subLayerPaths), 3)
        self.assertIn(layer1Id, rootLayer.subLayerPaths)
        self.assertIn(layer2Id, rootLayer.subLayerPaths)
        self.assertIn(layer3Id, rootLayer.subLayerPaths)

        restoredPrim = layer1.GetPrimAtPath('/Sphere')
        self.assertIsNotNone(restoredPrim.properties.get('color'))
        self.assertIsNone(restoredPrim.properties.get('radius'))
        self.assertIsNone(restoredPrim.properties.get('visible'))

        UsdLayerEditorTest._redo()

        self.assertEqual(len(rootLayer.subLayerPaths), 1)
        self.assertEqual(rootLayer.subLayerPaths[0], layer1Id)

        restitchedPrim = layer1.GetPrimAtPath('/Sphere')
        self.assertIsNotNone(restitchedPrim.properties.get('color'))
        self.assertIsNotNone(restitchedPrim.properties.get('radius'))
        self.assertIsNotNone(restitchedPrim.properties.get('visible'))

    def test_stitch_layers_with_edit_target(self):
        """Test stitching layers when edit target is on a layer being stitched"""

        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        rootLayer = stage.GetRootLayer()
        mgr = ufe.UndoableCommandMgr.instance()

        strongLayerId = self._createAnonymousLayer(stage, rootLayer)
        weakLayerId = self._createAnonymousLayer(stage, rootLayer)
        rootLayer.subLayerPaths.clear()
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, strongLayerId, 0)
        UsdLayerEditorTest._executeCmd(cmd)
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, weakLayerId, 1)
        UsdLayerEditorTest._executeCmd(cmd)

        weakLayer = Sdf.Layer.Find(weakLayerId)
        stage.SetEditTarget(weakLayer)
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, weakLayerId)

        cmd = UsdLayerEditor.StitchLayersCommand(stage, [strongLayerId, weakLayerId])
        UsdLayerEditorTest._executeCmd(cmd)

        currentTarget = stage.GetEditTarget().GetLayer().identifier
        self.assertNotEqual(currentTarget, weakLayerId)

        UsdLayerEditorTest._undo()
        self.assertEqual(stage.GetEditTarget().GetLayer().identifier, weakLayerId)

    def test_stitch_layers_partial_selection(self):
        """Test stitching only some layers while leaving others untouched"""

        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        rootLayer = stage.GetRootLayer()
        mgr = ufe.UndoableCommandMgr.instance()

        layer1Id = self._createAnonymousLayer(stage, rootLayer)
        layer2Id = self._createAnonymousLayer(stage, rootLayer)
        layer3Id = self._createAnonymousLayer(stage, rootLayer)
        layer4Id = self._createAnonymousLayer(stage, rootLayer)
        rootLayer.subLayerPaths.clear()
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, layer1Id, 0)
        UsdLayerEditorTest._executeCmd(cmd)
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, layer2Id, 1)
        UsdLayerEditorTest._executeCmd(cmd)
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, layer3Id, 2)
        UsdLayerEditorTest._executeCmd(cmd)
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, layer4Id, 3)
        UsdLayerEditorTest._executeCmd(cmd)

        self.assertEqual(len(rootLayer.subLayerPaths), 4)

        # Stitch only layer1 and layer3 (layer1 is stronger, so it receives layer3's content)
        cmd = UsdLayerEditor.StitchLayersCommand(stage, [layer1Id, layer3Id])
        UsdLayerEditorTest._executeCmd(cmd)

        self.assertEqual(len(rootLayer.subLayerPaths), 3)
        self.assertIn(layer1Id, rootLayer.subLayerPaths)
        self.assertIn(layer2Id, rootLayer.subLayerPaths)
        self.assertNotIn(layer3Id, rootLayer.subLayerPaths)
        self.assertIn(layer4Id, rootLayer.subLayerPaths)

        UsdLayerEditorTest._undo()
        self.assertEqual(len(rootLayer.subLayerPaths), 4)

    def test_stitch_layers_hierarchical(self):
        """Test stitching layers in a hierarchical structure"""
        # Root
        #   ParentLayer
        #       ChildStrong (strongest)
        #       ChildWeak (weakest)

        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        rootLayer = stage.GetRootLayer()
        mgr = ufe.UndoableCommandMgr.instance()

        parentLayerId = self._createAnonymousLayer(stage, rootLayer)
        parentLayer = Sdf.Layer.Find(parentLayerId)

        childStrongId = self._createAnonymousLayer(stage, parentLayer)
        childWeakId = self._createAnonymousLayer(stage, parentLayer)
        parentLayer.subLayerPaths.clear()
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, parentLayer, childStrongId, 0)
        UsdLayerEditorTest._executeCmd(cmd)
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, parentLayer, childWeakId, 1)
        UsdLayerEditorTest._executeCmd(cmd)

        childStrong = Sdf.Layer.Find(childStrongId)
        childWeak = Sdf.Layer.Find(childWeakId)

        with Sdf.ChangeBlock():
            prim1 = Sdf.CreatePrimInLayer(childStrong, '/Cube')
            prim1.SetInfo('typeName', 'Cube')
            attr1 = Sdf.AttributeSpec(prim1, 'size', Sdf.ValueTypeNames.Double)
            attr1.default = 1.0

        with Sdf.ChangeBlock():
            prim2 = Sdf.CreatePrimInLayer(childWeak, '/Cube')
            prim2.SetInfo('typeName', 'Cube')
            attr2 = Sdf.AttributeSpec(prim2, 'color', Sdf.ValueTypeNames.String)
            attr2.default = "blue"

        self.assertEqual(len(parentLayer.subLayerPaths), 2)

        cmd = UsdLayerEditor.StitchLayersCommand(stage, [childStrongId, childWeakId])
        UsdLayerEditorTest._executeCmd(cmd)

        self.assertEqual(len(parentLayer.subLayerPaths), 1)
        self.assertEqual(parentLayer.subLayerPaths[0], childStrongId)

        stitchedPrim = childStrong.GetPrimAtPath('/Cube')
        self.assertIsNotNone(stitchedPrim.properties.get('size'))
        self.assertIsNotNone(stitchedPrim.properties.get('color'))

        UsdLayerEditorTest._undo()
        self.assertEqual(len(parentLayer.subLayerPaths), 2)

    def test_stitch_layers_invalid_input(self):
        """Test error handling with invalid inputs"""

        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        rootLayer = stage.GetRootLayer()
        mgr = ufe.UndoableCommandMgr.instance()

        layerId = self._createAnonymousLayer(stage, rootLayer)

        with self.assertRaises(RuntimeError):
            cmd = UsdLayerEditor.StitchLayersCommand(stage, ["nonExistentLayer.usda", layerId])
            cmd.execute()

    def test_stitch_layers_with_dirty_anonymous_layers(self):
        """Test that dirty anonymous layers are properly held onto during stitch"""

        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        rootLayer = stage.GetRootLayer()
        mgr = ufe.UndoableCommandMgr.instance()

        strongId = self._createAnonymousLayer(stage, rootLayer)
        weakId = self._createAnonymousLayer(stage, rootLayer)

        strong = Sdf.Layer.Find(strongId)
        weak = Sdf.Layer.Find(weakId)

        # Make layers dirty
        with Sdf.ChangeBlock():
            Sdf.CreatePrimInLayer(strong, '/StrongPrim')
            Sdf.CreatePrimInLayer(weak, '/WeakPrim')

        self.assertTrue(strong.dirty)
        self.assertTrue(weak.dirty)

        cmd = UsdLayerEditor.StitchLayersCommand(stage, [strongId, weakId])
        UsdLayerEditorTest._executeCmd(cmd)

        UsdLayerEditorTest._undo()

        self.assertEqual(len(rootLayer.subLayerPaths), 2)

        restoredWeak = Sdf.Layer.Find(weakId)
        self.assertIsNotNone(restoredWeak)
        self.assertIsNotNone(restoredWeak.GetPrimAtPath('/WeakPrim'))

    def test_stitch_layers_with_single_sublayer(self):
        """Test basic sublayer movement when stitching parent with its child layer"""
        # Before:
        # Root
        #   Parent1 (selected, strongest)
        #        Layer1
        #        Layer2 (selected, weakest)
        #           Sub1
        #
        # After:
        # Root
        #   Parent1 (has merged/stitched layer changes)
        #        Layer1
        #        Sub1

        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        rootLayer = stage.GetRootLayer()
        mgr = ufe.UndoableCommandMgr.instance()

        parent1Id = self._createAnonymousLayer(stage, rootLayer)
        parent1Layer = Sdf.Layer.Find(parent1Id)

        layer1Id = self._createAnonymousLayer(stage, rootLayer)
        parent1Layer.subLayerPaths.append(layer1Id)
        layer1Layer = Sdf.Layer.Find(layer1Id)

        with Sdf.ChangeBlock():
            prim1 = Sdf.CreatePrimInLayer(layer1Layer, '/Layer1Prim')
            prim1.SetInfo('typeName', 'Sphere')

        layer2Id = self._createAnonymousLayer(stage, rootLayer)
        parent1Layer.subLayerPaths.append(layer2Id)
        layer2Layer = Sdf.Layer.Find(layer2Id)

        with Sdf.ChangeBlock():
            prim2 = Sdf.CreatePrimInLayer(layer2Layer, '/Layer2Prim')
            prim2.SetInfo('typeName', 'Cube')

        sub1Id = self._createAnonymousLayer(stage, rootLayer)
        layer2Layer.subLayerPaths.append(sub1Id)
        sub1Layer = Sdf.Layer.Find(sub1Id)

        with Sdf.ChangeBlock():
            prim3 = Sdf.CreatePrimInLayer(sub1Layer, '/Sub1Prim')
            prim3.SetInfo('typeName', 'Cone')

        rootLayer.subLayerPaths.clear()
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, parent1Id, 0)
        UsdLayerEditorTest._executeCmd(cmd)

        self.assertEqual(len(rootLayer.subLayerPaths), 1)
        self.assertEqual(rootLayer.subLayerPaths[0], parent1Id)
        self.assertEqual(len(parent1Layer.subLayerPaths), 2)
        self.assertEqual(len(layer2Layer.subLayerPaths), 1)

        cmd = UsdLayerEditor.StitchLayersCommand(stage, [parent1Id, layer2Id])
        UsdLayerEditorTest._executeCmd(cmd)

        self.assertEqual(len(rootLayer.subLayerPaths), 1)
        self.assertEqual(rootLayer.subLayerPaths[0], parent1Id)

        self.assertEqual(len(parent1Layer.subLayerPaths), 2,
                        "Parent1 should have 2 sublayers after stitch")

        firstSublayer = Sdf.Layer.FindRelativeToLayer(parent1Layer, parent1Layer.subLayerPaths[0])
        self.assertIsNotNone(firstSublayer)
        self.assertEqual(firstSublayer.identifier, layer1Id,
                        "Layer1 should remain at position 0")

        secondSublayer = Sdf.Layer.FindRelativeToLayer(parent1Layer, parent1Layer.subLayerPaths[1])
        self.assertIsNotNone(secondSublayer)
        self.assertEqual(secondSublayer.identifier, sub1Id,
                        "Sub1 should be at position 1 (moved from Layer2)")

        self.assertIsNotNone(parent1Layer.GetPrimAtPath('/Layer2Prim'),
                            "Parent1 should have merged content from Layer2")

        self.assertIsNotNone(layer1Layer.GetPrimAtPath('/Layer1Prim'))
        self.assertIsNotNone(sub1Layer.GetPrimAtPath('/Sub1Prim'))

        UsdLayerEditorTest._undo()

        self.assertEqual(len(parent1Layer.subLayerPaths), 2)

        restored1 = Sdf.Layer.FindRelativeToLayer(parent1Layer, parent1Layer.subLayerPaths[0])
        self.assertEqual(restored1.identifier, layer1Id)

        restored2 = Sdf.Layer.FindRelativeToLayer(parent1Layer, parent1Layer.subLayerPaths[1])
        self.assertEqual(restored2.identifier, layer2Id)

        restoredLayer2 = Sdf.Layer.Find(layer2Id)
        self.assertEqual(len(restoredLayer2.subLayerPaths), 1)
        restoredSub1 = Sdf.Layer.FindRelativeToLayer(restoredLayer2, restoredLayer2.subLayerPaths[0])
        self.assertEqual(restoredSub1.identifier, sub1Id)

        self.assertIsNone(parent1Layer.GetPrimAtPath('/Layer2Prim'),
                         "Layer2's content should not be in Parent1 after undo")

        UsdLayerEditorTest._redo()

        self.assertEqual(len(parent1Layer.subLayerPaths), 2)
        self.assertEqual(Sdf.Layer.FindRelativeToLayer(parent1Layer, parent1Layer.subLayerPaths[0]).identifier, layer1Id)
        self.assertEqual(Sdf.Layer.FindRelativeToLayer(parent1Layer, parent1Layer.subLayerPaths[1]).identifier, sub1Id)

    def test_batch_stitch(self):
        """Test batch stitching (3+ layers) across parent hierarchies with sublayer movement"""
        # Before:
        # Root
        #   Parent1
        #        Layer1
        #           Sub1 (selected, strongest)
        #   Parent2
        #       Layer2 (selected)
        #           Sub2
        #           Sub3 (selected, weakest)
        #               SubSub3
        #
        # After:
        # Root
        #   Parent1
        #        Layer1
        #           Sub1 (has merged/stitched changes from Layer2 and Sub3)
        #               Sub2 (moved from Layer2)
        #               SubSub3 (moved from Sub3)

        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        rootLayer = stage.GetRootLayer()
        mgr = ufe.UndoableCommandMgr.instance()

        parent1Id = self._createAnonymousLayer(stage, rootLayer)
        layer1Id = self._createAnonymousLayer(stage, rootLayer)
        sub1Id = self._createAnonymousLayer(stage, rootLayer)

        parent1Layer = Sdf.Layer.Find(parent1Id)
        layer1Layer = Sdf.Layer.Find(layer1Id)
        sub1Layer = Sdf.Layer.Find(sub1Id)

        parent1Layer.subLayerPaths.append(layer1Id)
        layer1Layer.subLayerPaths.append(sub1Id)

        with Sdf.ChangeBlock():
            prim1 = Sdf.CreatePrimInLayer(sub1Layer, '/Sub1Prim')
            prim1.SetInfo('typeName', 'Sphere')
            attr1 = Sdf.AttributeSpec(prim1, 'color', Sdf.ValueTypeNames.String)
            attr1.default = "red"

        parent2Id = self._createAnonymousLayer(stage, rootLayer)
        layer2Id = self._createAnonymousLayer(stage, rootLayer)
        sub2Id = self._createAnonymousLayer(stage, rootLayer)
        sub3Id = self._createAnonymousLayer(stage, rootLayer)
        subSub3Id = self._createAnonymousLayer(stage, rootLayer)

        parent2Layer = Sdf.Layer.Find(parent2Id)
        layer2Layer = Sdf.Layer.Find(layer2Id)
        sub2Layer = Sdf.Layer.Find(sub2Id)
        sub3Layer = Sdf.Layer.Find(sub3Id)
        subSub3Layer = Sdf.Layer.Find(subSub3Id)

        parent2Layer.subLayerPaths.append(layer2Id)
        layer2Layer.subLayerPaths.append(sub2Id)
        layer2Layer.subLayerPaths.append(sub3Id)
        sub3Layer.subLayerPaths.append(subSub3Id)

        with Sdf.ChangeBlock():
            prim2 = Sdf.CreatePrimInLayer(layer2Layer, '/Layer2Prim')
            prim2.SetInfo('typeName', 'Cube')
            attr2 = Sdf.AttributeSpec(prim2, 'size', Sdf.ValueTypeNames.Double)
            attr2.default = 2.0

        with Sdf.ChangeBlock():
            prim3 = Sdf.CreatePrimInLayer(sub2Layer, '/Sub2Prim')
            prim3.SetInfo('typeName', 'Cylinder')

        with Sdf.ChangeBlock():
            prim4 = Sdf.CreatePrimInLayer(sub3Layer, '/Sub3Prim')
            prim4.SetInfo('typeName', 'Cone')

        with Sdf.ChangeBlock():
            prim5 = Sdf.CreatePrimInLayer(subSub3Layer, '/SubSub3Prim')
            prim5.SetInfo('typeName', 'Torus')

        rootLayer.subLayerPaths.clear()
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, parent1Id, 0)
        UsdLayerEditorTest._executeCmd(cmd)
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, parent2Id, 1)
        UsdLayerEditorTest._executeCmd(cmd)

        self.assertEqual(len(rootLayer.subLayerPaths), 2)
        self.assertEqual(len(parent1Layer.subLayerPaths), 1)
        self.assertEqual(len(layer1Layer.subLayerPaths), 1)
        self.assertEqual(len(parent2Layer.subLayerPaths), 1)
        self.assertEqual(len(layer2Layer.subLayerPaths), 2)
        self.assertEqual(len(sub3Layer.subLayerPaths), 1)

        cmd = UsdLayerEditor.StitchLayersCommand(stage, [sub1Id, layer2Id, sub3Id])
        UsdLayerEditorTest._executeCmd(cmd)

        self.assertEqual(len(parent2Layer.subLayerPaths), 0,
                        "Layer2 should be removed from Parent2")

        self.assertEqual(len(sub1Layer.subLayerPaths), 2,
                        "Sub1 should have 2 sublayers (Sub2 and SubSub3)")

        firstSub = Sdf.Layer.FindRelativeToLayer(sub1Layer, sub1Layer.subLayerPaths[0])
        self.assertIsNotNone(firstSub)
        self.assertEqual(firstSub.identifier, sub2Id,
                        "Sub2 should be at position 0")

        secondSub = Sdf.Layer.FindRelativeToLayer(sub1Layer, sub1Layer.subLayerPaths[1])
        self.assertIsNotNone(secondSub)
        self.assertEqual(secondSub.identifier, subSub3Id,
                        "SubSub3 should be at position 1")

        self.assertIsNotNone(sub1Layer.GetPrimAtPath('/Sub1Prim'),
                            "Sub1 should have original content")
        self.assertIsNotNone(sub1Layer.GetPrimAtPath('/Layer2Prim'),
                            "Sub1 should have merged content from Layer2")
        self.assertIsNotNone(sub1Layer.GetPrimAtPath('/Sub3Prim'),
                            "Sub1 should have merged content from Sub3")

        self.assertIsNotNone(sub2Layer.GetPrimAtPath('/Sub2Prim'))
        self.assertIsNotNone(subSub3Layer.GetPrimAtPath('/SubSub3Prim'))

        UsdLayerEditorTest._undo()

        self.assertEqual(len(parent2Layer.subLayerPaths), 1,
                        "After undo, Parent2 should have Layer2 again")
        restoredLayer2Path = parent2Layer.subLayerPaths[0]
        restoredLayer2 = Sdf.Layer.FindRelativeToLayer(parent2Layer, restoredLayer2Path)
        self.assertIsNotNone(restoredLayer2)
        self.assertEqual(restoredLayer2.identifier, layer2Id)

        self.assertEqual(len(restoredLayer2.subLayerPaths), 2,
                        "After undo, Layer2 should have 2 sublayers")

        restoredSub2 = Sdf.Layer.FindRelativeToLayer(restoredLayer2, restoredLayer2.subLayerPaths[0])
        self.assertIsNotNone(restoredSub2)
        self.assertEqual(restoredSub2.identifier, sub2Id)

        restoredSub3 = Sdf.Layer.FindRelativeToLayer(restoredLayer2, restoredLayer2.subLayerPaths[1])
        self.assertIsNotNone(restoredSub3)
        self.assertEqual(restoredSub3.identifier, sub3Id)

        restoredSub3Layer = Sdf.Layer.Find(sub3Id)
        self.assertEqual(len(restoredSub3Layer.subLayerPaths), 1)
        restoredSubSub3 = Sdf.Layer.FindRelativeToLayer(restoredSub3Layer, restoredSub3Layer.subLayerPaths[0])
        self.assertIsNotNone(restoredSubSub3)
        self.assertEqual(restoredSubSub3.identifier, subSub3Id)

        restoredSub1 = Sdf.Layer.Find(sub1Id)
        self.assertEqual(len(restoredSub1.subLayerPaths), 0,
                        "After undo, Sub1 should have no sublayers")

        self.assertIsNotNone(restoredSub1.GetPrimAtPath('/Sub1Prim'))
        self.assertIsNone(restoredSub1.GetPrimAtPath('/Layer2Prim'),
                         "Layer2's content should not be in Sub1 after undo")
        self.assertIsNone(restoredSub1.GetPrimAtPath('/Sub3Prim'),
                         "Sub3's content should not be in Sub1 after undo")

        UsdLayerEditorTest._redo()

        self.assertEqual(len(parent2Layer.subLayerPaths), 0)
        self.assertEqual(len(sub1Layer.subLayerPaths), 2)
        self.assertIsNotNone(sub1Layer.GetPrimAtPath('/Layer2Prim'))
        self.assertIsNotNone(sub1Layer.GetPrimAtPath('/Sub3Prim'))

    def test_shared_sublayers(self):
        """Test stitch with shared sublayers - ensures no duplicate"""
        # Before:
        # Root
        #   Layer1 (selected, strongest)
        #       SharedSublayer
        #   Layer2 (selected, weakest)
        #       SharedSublayer
        #
        # After:
        # Root
        #   Layer1 (has merged/stitched changes)
        #       SharedSublayer

        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        rootLayer = stage.GetRootLayer()
        mgr = ufe.UndoableCommandMgr.instance()

        sharedSubId = self._createAnonymousLayer(stage, rootLayer)
        sharedSubLayer = Sdf.Layer.Find(sharedSubId)

        with Sdf.ChangeBlock():
            prim = Sdf.CreatePrimInLayer(sharedSubLayer, '/SharedPrim')
            prim.SetInfo('typeName', 'Sphere')
            attr = Sdf.AttributeSpec(prim, 'radius', Sdf.ValueTypeNames.Double)
            attr.default = 5.0

        layer1Id = self._createAnonymousLayer(stage, rootLayer)
        layer1Layer = Sdf.Layer.Find(layer1Id)
        layer1Layer.subLayerPaths.append(sharedSubId)

        with Sdf.ChangeBlock():
            prim1 = Sdf.CreatePrimInLayer(layer1Layer, '/Layer1Prim')
            prim1.SetInfo('typeName', 'Cube')
            attr1 = Sdf.AttributeSpec(prim1, 'color', Sdf.ValueTypeNames.String)
            attr1.default = "blue"

        layer2Id = self._createAnonymousLayer(stage, rootLayer)
        layer2Layer = Sdf.Layer.Find(layer2Id)
        layer2Layer.subLayerPaths.append(sharedSubId)

        with Sdf.ChangeBlock():
            prim2 = Sdf.CreatePrimInLayer(layer2Layer, '/Layer2Prim')
            prim2.SetInfo('typeName', 'Cone')
            attr2 = Sdf.AttributeSpec(prim2, 'size', Sdf.ValueTypeNames.Double)
            attr2.default = 3.0

        rootLayer.subLayerPaths.clear()
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, layer1Id, 0)
        UsdLayerEditorTest._executeCmd(cmd)
        cmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, layer2Id, 1)
        UsdLayerEditorTest._executeCmd(cmd)

        self.assertEqual(len(layer1Layer.subLayerPaths), 1)
        self.assertEqual(len(layer2Layer.subLayerPaths), 1)

        layer1Sub = Sdf.Layer.FindRelativeToLayer(layer1Layer, layer1Layer.subLayerPaths[0])
        layer2Sub = Sdf.Layer.FindRelativeToLayer(layer2Layer, layer2Layer.subLayerPaths[0])
        self.assertEqual(layer1Sub.identifier, sharedSubId)
        self.assertEqual(layer2Sub.identifier, sharedSubId)

        cmd = UsdLayerEditor.StitchLayersCommand(stage, [layer1Id, layer2Id])
        UsdLayerEditorTest._executeCmd(cmd)

        self.assertEqual(len(layer1Layer.subLayerPaths), 1,
                        "SharedSublayer should appear only once in Layer1 (no duplicate)")

        remainingSub = Sdf.Layer.FindRelativeToLayer(layer1Layer, layer1Layer.subLayerPaths[0])
        self.assertIsNotNone(remainingSub)
        self.assertEqual(remainingSub.identifier, sharedSubId,
                        "The single sublayer should be SharedSublayer")

        subLayerIds = []
        for path in layer1Layer.subLayerPaths:
            layer = Sdf.Layer.FindRelativeToLayer(layer1Layer, path)
            if layer:
                subLayerIds.append(layer.identifier)

        self.assertEqual(len(subLayerIds), len(set(subLayerIds)),
                        "No duplicate sublayer identifiers should exist")

        sharedPrim = sharedSubLayer.GetPrimAtPath('/SharedPrim')
        self.assertIsNotNone(sharedPrim)
        self.assertEqual(sharedPrim.properties.get('radius').default, 5.0)

        self.assertIsNotNone(layer1Layer.GetPrimAtPath('/Layer1Prim'))
        self.assertIsNotNone(layer1Layer.GetPrimAtPath('/Layer2Prim'),
                            "Layer1 should have merged content from Layer2")

        self.assertEqual(len(rootLayer.subLayerPaths), 1)
        self.assertEqual(rootLayer.subLayerPaths[0], layer1Id)

        UsdLayerEditorTest._undo()

        self.assertEqual(len(rootLayer.subLayerPaths), 2)
        self.assertIn(layer1Id, rootLayer.subLayerPaths)
        self.assertIn(layer2Id, rootLayer.subLayerPaths)

        restoredLayer1 = Sdf.Layer.Find(layer1Id)
        self.assertEqual(len(restoredLayer1.subLayerPaths), 1)
        restoredLayer1Sub = Sdf.Layer.FindRelativeToLayer(restoredLayer1, restoredLayer1.subLayerPaths[0])
        self.assertIsNotNone(restoredLayer1Sub)
        self.assertEqual(restoredLayer1Sub.identifier, sharedSubId,
                        "Layer1 should have SharedSublayer after undo")

        restoredLayer2 = Sdf.Layer.Find(layer2Id)
        self.assertEqual(len(restoredLayer2.subLayerPaths), 1)
        restoredLayer2Sub = Sdf.Layer.FindRelativeToLayer(restoredLayer2, restoredLayer2.subLayerPaths[0])
        self.assertIsNotNone(restoredLayer2Sub)
        self.assertEqual(restoredLayer2Sub.identifier, sharedSubId,
                        "Layer2 should have SharedSublayer restored after undo")

        self.assertIsNone(restoredLayer1.GetPrimAtPath('/Layer2Prim'),
                         "Layer2's content should not be in Layer1 after undo")

        UsdLayerEditorTest._redo()

        self.assertEqual(len(rootLayer.subLayerPaths), 1)
        self.assertEqual(len(layer1Layer.subLayerPaths), 1)
        self.assertIsNotNone(layer1Layer.GetPrimAtPath('/Layer2Prim'))


    def testMergeWithSublayers(self):
        """ test FlattenLayerCommand to merge a layer with its sublayers """
        stage = UsdLayerEditorTest._createStage(self.script_folder + "/data/empty.usda")
        rootLayer = stage.GetRootLayer()
        mgr = ufe.UndoableCommandMgr.instance()

        # Create two separate layer hierarchies under root
        # Root
        #   Layer1
        #     Layer1Sub
        #   Layer2
        #     Layer2Sub
        addLayer1Cmd = UsdLayerEditor.AddAnonSubLayerCommand(stage, rootLayer)
        UsdLayerEditorTest._executeCmd(addLayer1Cmd)
        layer1Id = addLayer1Cmd.addedLayer()

        addLayer2Cmd = UsdLayerEditor.AddAnonSubLayerCommand(stage, rootLayer)
        UsdLayerEditorTest._executeCmd(addLayer2Cmd)
        layer2Id = addLayer2Cmd.addedLayer()

        layer1 = Sdf.Layer.Find(layer1Id)
        layer2 = Sdf.Layer.Find(layer2Id)

        addLayer1SubCmd = UsdLayerEditor.AddAnonSubLayerCommand(stage, layer1)
        UsdLayerEditorTest._executeCmd(addLayer1SubCmd)
        layer1SubId = addLayer1SubCmd.addedLayer()

        addLayer2SubCmd = UsdLayerEditor.AddAnonSubLayerCommand(stage, layer2)
        UsdLayerEditorTest._executeCmd(addLayer2SubCmd)
        layer2SubId = addLayer2SubCmd.addedLayer()

        layer1Sub = Sdf.Layer.Find(layer1SubId)
        layer2Sub = Sdf.Layer.Find(layer2SubId)

        # Ball1 defined in Layer1Sub, overridden in Layer1
        ball1Spec = Sdf.PrimSpec(layer1Sub, "Ball1", Sdf.SpecifierDef, "Sphere")
        ball1OverSpec = Sdf.PrimSpec(layer1, "Ball1", Sdf.SpecifierOver)
        ball1Attr = Sdf.AttributeSpec(ball1OverSpec, "radius", Sdf.ValueTypeNames.Double)
        ball1Attr.default = 5.0

        # Ball2 defined in Layer2Sub, overridden in Layer2
        ball2Spec = Sdf.PrimSpec(layer2Sub, "Ball2", Sdf.SpecifierDef, "Sphere")
        ball2OverSpec = Sdf.PrimSpec(layer2, "Ball2", Sdf.SpecifierOver)
        ball2Attr = Sdf.AttributeSpec(ball2OverSpec, "radius", Sdf.ValueTypeNames.Double)
        ball2Attr.default = 10.0

        self.assertEqual(len(layer1.subLayerPaths), 1)
        self.assertEqual(len(layer2.subLayerPaths), 1)
        self.assertEqual(len(rootLayer.subLayerPaths), 2)

        flattenLayer1Cmd = UsdLayerEditor.FlattenLayerCommand(layer1)
        UsdLayerEditorTest._executeCmd(flattenLayer1Cmd)
        self.assertEqual(len(layer1.subLayerPaths), 0)
        self.assertIsNotNone(layer1.GetPrimAtPath("/Ball1"))

        flattenLayer2Cmd = UsdLayerEditor.FlattenLayerCommand(layer2)
        UsdLayerEditorTest._executeCmd(flattenLayer2Cmd)
        self.assertEqual(len(layer2.subLayerPaths), 0)
        self.assertIsNotNone(layer2.GetPrimAtPath("/Ball2"))

        self.assertEqual(len(rootLayer.subLayerPaths), 2)
        flattenRootCmd = UsdLayerEditor.FlattenLayerCommand(rootLayer)
        UsdLayerEditorTest._executeCmd(flattenRootCmd)
        self.assertEqual(len(rootLayer.subLayerPaths), 0)

        self.assertIsNotNone(rootLayer.GetPrimAtPath("/Ball1"))
        self.assertIsNotNone(rootLayer.GetPrimAtPath("/Ball2"))

        UsdLayerEditorTest._undo()
        self.assertEqual(len(rootLayer.subLayerPaths), 2)

        UsdLayerEditorTest._undo()
        self.assertEqual(len(layer2.subLayerPaths), 1)

        UsdLayerEditorTest._undo()
        self.assertEqual(len(layer1.subLayerPaths), 1)

        UsdLayerEditorTest._redo()
        self.assertEqual(len(layer1.subLayerPaths), 0)

        UsdLayerEditorTest._redo()
        self.assertEqual(len(layer2.subLayerPaths), 0)

        UsdLayerEditorTest._redo()
        self.assertEqual(len(rootLayer.subLayerPaths), 0)


    def testMergeWithSublayersWithDirtySavedLayers(self):
        """
        Test that merge with sublayers and undoing preserves:
        1. Anonymous layers and their content
        2. Clean saved layers and their content (not dirty after undo)
        3. Dirty saved layers with their in-memory changes (dirty after undo)
        """
        testDir = tempfile.mkdtemp(prefix='FlattenDirty')
        try:
            rootLayerPath = os.path.join(testDir, "rootLayer.usda")
            rootLayer = Sdf.Layer.CreateNew(rootLayerPath)
            rootLayer.Save()

            stage = UsdLayerEditorTest._createStage(rootLayerPath)
            rootLayer = stage.GetRootLayer()
            mgr = ufe.UndoableCommandMgr.instance()

            addAnonCmd = UsdLayerEditor.AddAnonSubLayerCommand(stage, rootLayer)
            UsdLayerEditorTest._executeCmd(addAnonCmd)
            anonLayerId = addAnonCmd.addedLayer()
            anonLayer = Sdf.Layer.Find(anonLayerId)

            cleanLayerPath = os.path.join(testDir, "cleanLayer.usda")
            cleanLayer = Sdf.Layer.CreateNew(cleanLayerPath)
            cleanLayer.Save()
            insertCleanCmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, cleanLayerPath, 0)
            UsdLayerEditorTest._executeCmd(insertCleanCmd)
            cleanLayer = Sdf.Layer.FindOrOpen(cleanLayerPath)

            dirtyLayerPath = os.path.join(testDir, "dirtyLayer.usda")
            dirtyLayer = Sdf.Layer.CreateNew(dirtyLayerPath)
            dirtyLayer.Save()
            insertDirtyCmd = UsdLayerEditor.InsertSubPathCommand(stage, rootLayer, dirtyLayerPath, 0)
            UsdLayerEditorTest._executeCmd(insertDirtyCmd)
            dirtyLayer = Sdf.Layer.FindOrOpen(dirtyLayerPath)

            ball1Spec = Sdf.PrimSpec(anonLayer, "Ball1", Sdf.SpecifierDef, "Sphere")
            ball1Attr = Sdf.AttributeSpec(ball1Spec, "radius", Sdf.ValueTypeNames.Double)
            ball1Attr.default = 5.0

            ball2Spec = Sdf.PrimSpec(cleanLayer, "Ball2", Sdf.SpecifierDef, "Sphere")
            ball2Attr = Sdf.AttributeSpec(ball2Spec, "radius", Sdf.ValueTypeNames.Double)
            ball2Attr.default = 10.0
            cleanLayer.Save()

            ball3Spec = Sdf.PrimSpec(dirtyLayer, "Ball3", Sdf.SpecifierDef, "Sphere")
            ball3Attr = Sdf.AttributeSpec(ball3Spec, "radius", Sdf.ValueTypeNames.Double)
            ball3Attr.default = 15.0
            dirtyLayer.Save()

            # In-memory change to make the layer dirty.
            ball3Attr.default = 20.0

            self.assertEqual(len(rootLayer.subLayerPaths), 3)
            self.assertFalse(cleanLayer.dirty)
            self.assertTrue(dirtyLayer.dirty)

            # Save root layer so it's clean before flatten.
            rootLayer.Save()
            self.assertFalse(rootLayer.dirty, "Root layer should be clean after save")

            # Verify the in-memory value for dirty layer is 20, not the disk value of 15.
            self.assertEqual(dirtyLayer.GetPrimAtPath("/Ball3").attributes["radius"].default, 20.0)

            flattenCmd = UsdLayerEditor.FlattenLayerCommand(rootLayer)
            UsdLayerEditorTest._executeCmd(flattenCmd)

            self.assertEqual(len(rootLayer.subLayerPaths), 0)
            self.assertIsNotNone(rootLayer.GetPrimAtPath("/Ball1"))
            self.assertIsNotNone(rootLayer.GetPrimAtPath("/Ball2"))
            self.assertIsNotNone(rootLayer.GetPrimAtPath("/Ball3"))
            # Ball3 should have the in-memory value 20.0 not the disk value 15.0.
            self.assertEqual(rootLayer.GetPrimAtPath("/Ball3").attributes["radius"].default, 20.0)

            UsdLayerEditorTest._undo()

            self.assertEqual(len(rootLayer.subLayerPaths), 3)

            anonLayer = Sdf.Layer.Find(anonLayerId)
            cleanLayer = Sdf.Layer.FindOrOpen(cleanLayerPath)
            dirtyLayer = Sdf.Layer.FindOrOpen(dirtyLayerPath)

            self.assertIsNotNone(anonLayer)
            self.assertIsNotNone(cleanLayer)
            self.assertIsNotNone(dirtyLayer)

            self.assertIsNotNone(anonLayer.GetPrimAtPath("/Ball1"))
            self.assertEqual(anonLayer.GetPrimAtPath("/Ball1").attributes["radius"].default, 5.0)
            self.assertIsNotNone(cleanLayer.GetPrimAtPath("/Ball2"))
            self.assertEqual(cleanLayer.GetPrimAtPath("/Ball2").attributes["radius"].default, 10.0)
            self.assertIsNotNone(dirtyLayer.GetPrimAtPath("/Ball3"))

            # Dirty layer should have the in-memory value 20.0, not disk value.
            self.assertEqual(dirtyLayer.GetPrimAtPath("/Ball3").attributes["radius"].default, 20.0,
                           "Dirty layer should restore with in-memory changes, not clean disk version")

            self.assertFalse(cleanLayer.dirty, "Clean layer should remain clean after undo")
            self.assertTrue(dirtyLayer.dirty, "Dirty layer should remain dirty after undo")

            # Verify disk still has the old value 15.0.
            dirtyLayer.Reload()
            self.assertEqual(dirtyLayer.GetPrimAtPath("/Ball3").attributes["radius"].default, 15.0,
                           "Disk should still have the original saved value")
        finally:
            shutil.rmtree(testDir, ignore_errors=True)

    def testFlattenLayerUndoRestoresDeeplyNestedDirtyLayer(self):
        """
        Test that FlattenLayer undo restores in-memory (dirty) changes to a saved sublayer
        that is 2 or more levels deep from the flattened root layer.

        All three layers are saved to disk. Layer hierarchy:
            layer1 (file-backed)
                layer2 (file-backed, clean)
                    layer3* (file-backed, dirty unsaved in-memory changes)

        After flatten:
            layer1  (contains all changes, including layer3's in-memory value 20.0)

        After undo:
            layer1
                layer2
                    layer3*  (dirty, in-memory value 20.0 restored, not disk value 15.0)
        """
        testDir = tempfile.mkdtemp(prefix='FlattenDirtyDeep')
        try:
            layer1Path = os.path.join(testDir, "layer1.usda")
            layer2Path = os.path.join(testDir, "layer2.usda")
            layer3Path = os.path.join(testDir, "layer3.usda")

            layer3 = Sdf.Layer.CreateNew(layer3Path)
            ballSpec = Sdf.PrimSpec(layer3, "Ball", Sdf.SpecifierDef, "Sphere")
            ballAttr = Sdf.AttributeSpec(ballSpec, "radius", Sdf.ValueTypeNames.Double)
            ballAttr.default = 15.0
            layer3.Save()
            del ballAttr, ballSpec

            layer2 = Sdf.Layer.CreateNew(layer2Path)
            layer2.Save()

            layer1 = Sdf.Layer.CreateNew(layer1Path)
            layer1.Save()

            stage = UsdLayerEditorTest._createStage(layer1Path)
            layer1 = stage.GetRootLayer()
            mgr = ufe.UndoableCommandMgr.instance()

            insertLayer2Cmd = UsdLayerEditor.InsertSubPathCommand(stage, layer1, layer2Path, 0)
            UsdLayerEditorTest._executeCmd(insertLayer2Cmd)
            layer2 = Sdf.Layer.FindOrOpen(layer2Path)
            self.assertIsNotNone(layer2)

            insertLayer3Cmd = UsdLayerEditor.InsertSubPathCommand(stage, layer2, layer3Path, 0)
            UsdLayerEditorTest._executeCmd(insertLayer3Cmd)
            layer3 = Sdf.Layer.FindOrOpen(layer3Path)
            self.assertIsNotNone(layer3)

            layer1.Save()
            layer2.Save()
            self.assertFalse(layer1.dirty, "layer1 should be clean after save")
            self.assertFalse(layer2.dirty, "layer2 should be clean after save")
            self.assertFalse(layer3.dirty, "layer3 should be clean before in-memory change")

            layer3.GetPrimAtPath("/Ball").attributes["radius"].default = 20.0
            self.assertTrue(layer3.dirty, "layer3 should be dirty after in-memory change")
            self.assertEqual(
                layer3.GetPrimAtPath("/Ball").attributes["radius"].default, 20.0,
                "In-memory value should be 20.0 before flatten")

            self.assertEqual(len(layer1.subLayerPaths), 1)
            self.assertEqual(len(layer2.subLayerPaths), 1)

            # Release references so holdOntoSubLayers is the only mechanism keeping them alive.
            # Without the holdOnPathIfDirty fix (moving holdOntoSubLayers outside the dirty check),
            # layer3 would not be held and this test would fail.
            layer2 = None
            layer3 = None

            flattenCmd = UsdLayerEditor.FlattenLayerCommand(layer1)
            UsdLayerEditorTest._executeCmd(flattenCmd)

            self.assertEqual(len(layer1.subLayerPaths), 0)
            self.assertIsNotNone(layer1.GetPrimAtPath("/Ball"),
                                 "layer1 should contain /Ball after flatten")
            self.assertEqual(
                layer1.GetPrimAtPath("/Ball").attributes["radius"].default, 20.0,
                "Flattened layer1 should contain the in-memory value 20.0")

            UsdLayerEditorTest._undo()

            self.assertEqual(len(layer1.subLayerPaths), 1,
                             "layer1 should have one sublayer after undo")

            layer2 = Sdf.Layer.FindOrOpen(layer2Path)
            layer3 = Sdf.Layer.FindOrOpen(layer3Path)

            self.assertIsNotNone(layer2, "layer2 should exist after undo")
            self.assertIsNotNone(layer3, "layer3 should exist after undo")

            self.assertEqual(len(layer2.subLayerPaths), 1,
                             "layer2 should have one sublayer after undo")

            self.assertEqual(
                layer3.GetPrimAtPath("/Ball").attributes["radius"].default, 20.0,
                "Undo must restore in-memory value 20.0, not disk value 15.0")

            self.assertTrue(layer3.dirty, "layer3 should still be dirty after undo")

            layer3.Reload()
            self.assertEqual(
                layer3.GetPrimAtPath("/Ball").attributes["radius"].default, 15.0,
                "Disk should still have the original saved value 15.0")
        finally:
            shutil.rmtree(testDir, ignore_errors=True)


def run_tests():
    return unittest.TextTestRunner(stream=sys.stdout, verbosity=2).run(unittest.TestLoader().loadTestsFromTestCase(UsdLayerEditorTest))


if __name__ == '__main__':
    run_tests()
