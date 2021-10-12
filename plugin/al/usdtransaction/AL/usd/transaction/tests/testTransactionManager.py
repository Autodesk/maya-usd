from AL.usd.transaction import TransactionManager
from pxr import Usd, Sdf
import unittest

class TestManger(unittest.TestCase):

    ## Test that TransactionManager reports transactions for multiple stages as expected
    def test_InProgress_Stage(self):
        stageA = Usd.Stage.CreateInMemory()
        stageB = Usd.Stage.CreateInMemory()
        layer = Sdf.Layer.CreateAnonymous()
        
        self.assertFalse(TransactionManager.InProgress(stageA))
        self.assertFalse(TransactionManager.InProgress(stageB))
        
        self.assertTrue(TransactionManager.Open(stageA, layer))
      
        self.assertTrue(TransactionManager.InProgress(stageA))
        self.assertFalse(TransactionManager.InProgress(stageB))
      
        self.assertTrue(TransactionManager.Open(stageB, layer))
      
        self.assertTrue(TransactionManager.InProgress(stageA))
        self.assertTrue(TransactionManager.InProgress(stageB))
      
        self.assertTrue(TransactionManager.Close(stageA, layer))
      
        self.assertFalse(TransactionManager.InProgress(stageA))
        self.assertTrue(TransactionManager.InProgress(stageB))
        
        self.assertTrue(TransactionManager.Close(stageB, layer))
      
        self.assertFalse(TransactionManager.InProgress(stageA))
        self.assertFalse(TransactionManager.InProgress(stageB))

        TransactionManager.CloseAll()

    ## Test that TransactionManager reports transactions for multiple layers as expected
    def test_InProgress_Layer(self):
        stage = Usd.Stage.CreateInMemory()
        layerA = Sdf.Layer.CreateAnonymous()
        layerB = Sdf.Layer.CreateAnonymous()
        
        self.assertFalse(TransactionManager.InProgress(stage))
        self.assertFalse(TransactionManager.InProgress(stage, layerA))
        self.assertFalse(TransactionManager.InProgress(stage, layerB))
        
        self.assertTrue(TransactionManager.Open(stage, layerA))
    
        self.assertTrue(TransactionManager.InProgress(stage))
        self.assertTrue(TransactionManager.InProgress(stage, layerA))
        self.assertFalse(TransactionManager.InProgress(stage, layerB))
    
        self.assertTrue(TransactionManager.Open(stage, layerB))
    
        self.assertTrue(TransactionManager.InProgress(stage))
        self.assertTrue(TransactionManager.InProgress(stage, layerA))
        self.assertTrue(TransactionManager.InProgress(stage, layerB))
    
        self.assertTrue(TransactionManager.Close(stage, layerA))
    
        self.assertTrue(TransactionManager.InProgress(stage))
        self.assertFalse(TransactionManager.InProgress(stage, layerA))
        self.assertTrue(TransactionManager.InProgress(stage, layerB))
    
        self.assertTrue(TransactionManager.Close(stage, layerB))
    
        self.assertFalse(TransactionManager.InProgress(stage))
        self.assertFalse(TransactionManager.InProgress(stage, layerA))
        self.assertFalse(TransactionManager.InProgress(stage, layerB))

        TransactionManager.CloseAll()


if __name__ == '__main__':
    unittest.main()
