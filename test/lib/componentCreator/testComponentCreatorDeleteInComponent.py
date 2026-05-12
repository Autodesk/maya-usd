import unittest

import fixturesUtils
import mayaUsd.lib
import mayaUsd.ufe
from maya import cmds
import ufe

from testComponentCreatorBase import _ComponentCreatorTestBase

class TestObserver(ufe.Observer):
    """Observer to track UFE notifications for delete operations."""
    def __init__(self):
        super(TestObserver, self).__init__()
        self.deleteNotif = 0
        self.addNotif = 0

    def __call__(self, notification):
        if isinstance(notification, ufe.ObjectDelete):
            self.deleteNotif += 1
        if isinstance(notification, ufe.ObjectAdd):
            self.addNotif += 1

    def nbDeleteNotif(self):
        return self.deleteNotif

    def nbAddNotif(self):
        return self.addNotif

    def reset(self):
        self.addNotif = 0
        self.deleteNotif = 0


class DeleteInComponentTestCase(_ComponentCreatorTestBase, unittest.TestCase):
    """
    Test deleting prims in Adsk USD Component stages.

    Component stages have special delete behavior where prim specs are removed
    from all layers in the prim stack (including non-local layers).
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def setUp(self):
        self._setUpCC()

    def testDeleteLeafPrimInComponentStage(self):
        '''Test successful delete of a leaf prim in a component stage.'''
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Create a prim in the component's geo scope.
        stage.SetEditTarget(stage.GetEditTarget())
        geoPrim = stage.GetPrimAtPath('/root/geo')
        self.assertTrue(geoPrim.IsValid())

        testPrim = stage.DefinePrim('/root/geo/TestPrim', 'Xform')
        self.assertTrue(testPrim.IsValid())

        # Create UFE notification observer.
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # Delete the prim.
        ufeObs.reset()
        cmds.delete(proxyShapePath + ',/root/geo/TestPrim')
        self.assertEqual(ufeObs.nbDeleteNotif(), 1)
        self.assertFalse(stage.GetPrimAtPath('/root/geo/TestPrim').IsValid())

        # Validate undo.
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif(), 1)
        self.assertTrue(stage.GetPrimAtPath('/root/geo/TestPrim').IsValid())

        # Validate redo.
        ufeObs.reset()
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif(), 1)
        self.assertFalse(stage.GetPrimAtPath('/root/geo/TestPrim').IsValid())

    def testDeleteHierarchyInComponentStage(self):
        '''Test successful delete of a prim hierarchy in a component stage.'''
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Create a prim hierarchy in the component's geo scope.
        parentPrim = stage.DefinePrim('/root/geo/Parent', 'Xform')
        childPrim1 = stage.DefinePrim('/root/geo/Parent/Child1', 'Xform')
        childPrim2 = stage.DefinePrim('/root/geo/Parent/Child2', 'Sphere')
        self.assertTrue(parentPrim.IsValid())
        self.assertTrue(childPrim1.IsValid())
        self.assertTrue(childPrim2.IsValid())

        # Create UFE notification observer.
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # Delete the parent prim (should delete entire hierarchy).
        ufeObs.reset()
        cmds.delete(proxyShapePath + ',/root/geo/Parent')
        self.assertEqual(ufeObs.nbDeleteNotif(), 1)
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent/Child1').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent/Child2').IsValid())

        # Validate undo.
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif(), 1)
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent/Child1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent/Child2').IsValid())

        # Validate redo.
        ufeObs.reset()
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif(), 1)
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent/Child1').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent/Child2').IsValid())

    def testDeleteMultiplePrimsInComponentStage(self):
        '''Test delete of multiple prims, some being children of others, in a component stage.'''
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Create a prim hierarchy.
        parentPrim = stage.DefinePrim('/root/geo/Parent', 'Xform')
        child1 = stage.DefinePrim('/root/geo/Parent/Child1', 'Xform')
        child2 = stage.DefinePrim('/root/geo/Parent/Child2', 'Sphere')
        sibling = stage.DefinePrim('/root/geo/Sibling', 'Cube')
        self.assertTrue(parentPrim.IsValid())
        self.assertTrue(child1.IsValid())
        self.assertTrue(child2.IsValid())
        self.assertTrue(sibling.IsValid())

        # Create UFE notification observer.
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # Delete parent and one of its children (redundant) plus a sibling.
        ufeObs.reset()
        cmds.delete(
            proxyShapePath + ',/root/geo/Parent/Child1',
            proxyShapePath + ',/root/geo/Parent',
            proxyShapePath + ',/root/geo/Sibling'
        )

        # All prims should be deleted.
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent/Child1').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent/Child2').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Sibling').IsValid())

        # Validate undo.
        ufeObs.reset()
        cmds.undo()
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent/Child1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent/Child2').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Sibling').IsValid())

        # Validate redo.
        ufeObs.reset()
        cmds.redo()
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent/Child1').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent/Child2').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Sibling').IsValid())

    def testDeleteRestrictionComponentScopeItself(self):
        '''Test delete restriction - cannot delete the component scope prims themselves.'''
        import AdskUsdComponentCreator

        proxyShapePath, desc = self._createComponent()
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage)

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Verify the geo scope exists.
        geoScope = stage.GetPrimAtPath('/root/geo')
        self.assertTrue(geoScope.IsValid())

        # Try to delete the geo scope itself - should fail.
        # Maya wraps the exception, so we check that it raises any exception.
        with self.assertRaises(Exception):
            cmds.delete(proxyShapePath + ',/root/geo')

        # Verify scope still exists.
        self.assertTrue(stage.GetPrimAtPath('/root/geo').IsValid())



if __name__ == '__main__':
    fixturesUtils.runTests(globals())

