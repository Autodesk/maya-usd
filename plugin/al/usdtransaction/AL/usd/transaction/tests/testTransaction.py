from AL.usd import transaction
from pxr import Usd, Sdf, Tf
import sys
import unittest

class TestTransaction(unittest.TestCase):

    def setUp(self):
        self._stage = Usd.Stage.CreateInMemory()
        self._stage.SetEditTarget(self._stage.GetSessionLayer())
        self._openNoticeKey = Tf.Notice.Register(transaction.OpenNotice, self._openNoticeHandler, self._stage)
        self._closeNoticeKey = Tf.Notice.Register(transaction.CloseNotice, self._closeNoticeHandler, self._stage)
        self._opened = 0
        self._closed = 0
        self._changed = []
        self._resynced = []
        # assertCountEqual in python 3 is equivalent to assertItemsEqual
        if sys.version_info[0] >= 3:
            self.assertItemsEqual = self.assertCountEqual

    def tearDown(self):
        transaction.TransactionManager.CloseAll()

    def _openNoticeHandler(self, notice, stage):
        self.assertEqual(stage, self._stage)
        self._opened += 1

    def _closeNoticeHandler(self, notice, stage):
        self.assertEqual(stage, self._stage)
        self._changed = notice.GetChangedInfoOnlyPaths()
        self._resynced = notice.GetResyncedPaths()
        self._closed += 1

    def createPrimWithAttribute(self, path, attrName='prop'):
        prim = self._stage.DefinePrim(path)
        self.assertTrue(prim)
        if attrName:
            attr = prim.CreateAttribute(attrName, Sdf.ValueTypeNames.Int)
            self.assertTrue(attr.Set(1))
    
    def changePrimAttribute(self, path, value, attrName='prop'):
        prim = self._stage.GetPrimAtPath(path)
        self.assertTrue(prim)
        attr = prim.GetAttribute(attrName)
        self.assertTrue(attr.Set(value))

    ## Test that Transaction Open / Close methods work as expected
    def test_Transaction(self):
        t = transaction.Transaction(self._stage, self._stage.GetSessionLayer())
        ## This should fail and no notices should be sent
        self.assertFalse(t.Close())
        self.assertEqual(self._opened, 0)
        self.assertEqual(self._closed, 0)
        ## Open notice should be triggered
        self.assertTrue(t.Open())
        self.assertEqual(self._opened, 1)
        self.assertEqual(self._closed, 0)
        ## Opening same transaction is allowed, but should not trigger notices
        self.assertTrue(t.Open())
        self.assertEqual(self._opened, 1)
        self.assertEqual(self._closed, 0)
        ## Close notices should not be emitted until last close
        self.assertTrue(t.Close())
        self.assertEqual(self._opened, 1)
        self.assertEqual(self._closed, 0)
        ## Close notice should be triggered
        self.assertTrue(t.Close())
        self.assertEqual(self._opened, 1)
        self.assertEqual(self._closed, 1)
        ## This should fail and no notices should be sent
        self.assertFalse(t.Close())
        self.assertEqual(self._opened, 1)
        self.assertEqual(self._closed, 1)

    ## Test that ScopedTransaction for same stage and layer pair work as expected
    def test_SameScopedTransaction(self):
        self.assertEqual(self._opened, 0)
        self.assertEqual(self._closed, 0)
        ## Open notice should be triggered
        with transaction.ScopedTransaction(self._stage, self._stage.GetSessionLayer()):
            self.assertEqual(self._opened, 1)
            self.assertEqual(self._closed, 0)
            ## Opening same transaction is allowed, but should not trigger notices
            with transaction.ScopedTransaction(self._stage, self._stage.GetSessionLayer()):
                self.assertEqual(self._opened, 1)
                self.assertEqual(self._closed, 0)
            ## Close notices should not be emitted until last close
            self.assertEqual(self._opened, 1)
            self.assertEqual(self._closed, 0)
        ## Close notice should be triggered
        self.assertEqual(self._opened, 1)
        self.assertEqual(self._closed, 1)

    ## Test that ScopedTransaction for different stage and layer pair work as expected
    def test_DifferentScopedTransaction(self):
        self.assertEqual(self._opened, 0)
        self.assertEqual(self._closed, 0)
        ## Open notice should be triggered
        with transaction.ScopedTransaction(self._stage, self._stage.GetSessionLayer()):
            self.assertEqual(self._opened, 1)
            self.assertEqual(self._closed, 0)
            ## Open notice should be triggered as it targets different layer
            with transaction.ScopedTransaction(self._stage, self._stage.GetRootLayer()):
                self.assertEqual(self._opened, 2)
                self.assertEqual(self._closed, 0)
            ## Close notice should be triggered
            self.assertEqual(self._opened, 2)
            self.assertEqual(self._closed, 1)
        ## Close notice should be triggered
        self.assertEqual(self._opened, 2)
        self.assertEqual(self._closed, 2)
    
    ## Test that CloseNotice reports changes as expected
    def test_Changes(self):
        self.assertItemsEqual(self._changed, [])
        self.assertItemsEqual(self._resynced, [])

        with transaction.ScopedTransaction(self._stage, self._stage.GetSessionLayer()):
            self.createPrimWithAttribute('/A')
        self.assertItemsEqual(self._changed, [])
        self.assertItemsEqual(self._resynced, [Sdf.Path(x) for x in ['/A']])

        with transaction.ScopedTransaction(self._stage, self._stage.GetSessionLayer()):
            self.changePrimAttribute('/A', 2)
        self.assertItemsEqual(self._changed, [Sdf.Path(x) for x in ['/A.prop']])
        self.assertItemsEqual(self._resynced, [])

        with transaction.ScopedTransaction(self._stage, self._stage.GetSessionLayer()):
            self.changePrimAttribute('/A', 4)
            self.changePrimAttribute('/A', 2) ## effectively no change
        self.assertItemsEqual(self._changed, [])
        self.assertItemsEqual(self._resynced, [])

    ## Test that CloseNotice reports hierarchy changes as expected
    def test_Hierarchy(self):
        self.assertItemsEqual(self._changed, [])
        self.assertItemsEqual(self._resynced, [])

        with transaction.ScopedTransaction(self._stage, self._stage.GetSessionLayer()):
            self.createPrimWithAttribute('/root')
            self.createPrimWithAttribute('/root/A')
            self.createPrimWithAttribute('/root/A/C')
            self.createPrimWithAttribute('/root/A/D')
            self.createPrimWithAttribute('/root/B')
            self.createPrimWithAttribute('/root/B/E')
        self.assertItemsEqual(self._changed, [])
        self.assertItemsEqual(self._resynced, [Sdf.Path(x) for x in ['/root']])

        with transaction.ScopedTransaction(self._stage, self._stage.GetSessionLayer()):
            self.changePrimAttribute('/root', 2)
            self.changePrimAttribute('/root/A/D', 2)
        self.assertItemsEqual(self._changed, [Sdf.Path(x) for x in ['/root.prop', '/root/A/D.prop']])
        self.assertItemsEqual(self._resynced, [])

        with transaction.ScopedTransaction(self._stage, self._stage.GetSessionLayer()):
            self.changePrimAttribute('/root/B', 2)
            self.changePrimAttribute('/root/A/C', 2)
            self.changePrimAttribute('/root/A/C', 1) ## effectively no change
        self.assertItemsEqual(self._changed, [Sdf.Path(x) for x in ['/root/B.prop']])
        self.assertItemsEqual(self._resynced, [])

        with transaction.ScopedTransaction(self._stage, self._stage.GetSessionLayer()):
            self.createPrimWithAttribute('/root/B/F')
        self.assertItemsEqual(self._changed, [])
        self.assertItemsEqual(self._resynced, [Sdf.Path(x) for x in ['/root/B/F']])

    ## Test that CloseNotice reports clearing layers as expected
    def test_Clear(self):
        self.assertItemsEqual(self._changed, [])
        self.assertItemsEqual(self._resynced, [])

        with transaction.ScopedTransaction(self._stage, self._stage.GetSessionLayer()):
            self.createPrimWithAttribute('/root')
            self.createPrimWithAttribute('/root/A')
            self.createPrimWithAttribute('/root/A/B')
        self.assertItemsEqual(self._changed, [])
        self.assertItemsEqual(self._resynced, [Sdf.Path(x) for x in ['/root']])

        with transaction.ScopedTransaction(self._stage, self._stage.GetSessionLayer()):
            self._stage.GetSessionLayer().Clear()
        self.assertItemsEqual(self._changed, [])
        self.assertItemsEqual(self._resynced, [Sdf.Path(x) for x in ['/root']])
    
        with transaction.ScopedTransaction(self._stage, self._stage.GetSessionLayer()):
            self.createPrimWithAttribute('/root')
            self.createPrimWithAttribute('/root/A')
            self.createPrimWithAttribute('/root/A/B')
        self.assertItemsEqual(self._changed, [])
        self.assertItemsEqual(self._resynced, [Sdf.Path(x) for x in ['/root']])

        with transaction.ScopedTransaction(self._stage, self._stage.GetSessionLayer()):
            self._stage.GetSessionLayer().Clear()
            self.createPrimWithAttribute('/root')
            self.createPrimWithAttribute('/root/A')
            self.createPrimWithAttribute('/root/A/B')
            ### effectively no change
        self.assertItemsEqual(self._changed, [])
        self.assertItemsEqual(self._resynced, [])


if __name__ == '__main__':
    unittest.main()
