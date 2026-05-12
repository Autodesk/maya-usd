import unittest

import fixturesUtils
import mayaUsd.lib
import mayaUsd.ufe
from maya import cmds
import ufe

from testComponentCreatorBase import _ComponentCreatorTestBase

class ReparentInComponentTestCase(_ComponentCreatorTestBase, unittest.TestCase):
    """
    Test reparenting (insert child) prims in Adsk USD Component stages.

    Component stages have special reparent behavior where prim specs are moved
    from all layers in the prim stack.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def setUp(self):
        self._setUpCC()

    def testReparentPrimInComponentStage(self):
        '''Test successful reparenting of a prim in a component stage.'''
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

        # Create prims in the component's geo scope.
        child = stage.DefinePrim('/root/geo/Child', 'Xform')
        newParent = stage.DefinePrim('/root/geo/NewParent', 'Xform')
        self.assertTrue(child.IsValid())
        self.assertTrue(newParent.IsValid())

        # Reparent child under newParent using cmds.parent.
        cmds.parent(proxyShapePath + ',/root/geo/Child', proxyShapePath + ',/root/geo/NewParent')

        # Verify the child was reparented.
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Child').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewParent/Child').IsValid())

        # Validate undo.
        cmds.undo()
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Child').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/NewParent/Child').IsValid())

        # Validate redo.
        cmds.redo()
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Child').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewParent/Child').IsValid())

    def testReparentPrimWithChildrenInComponentStage(self):
        '''Test reparenting a prim hierarchy in a component stage.'''
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
        parent = stage.DefinePrim('/root/geo/Parent', 'Xform')
        child1 = stage.DefinePrim('/root/geo/Parent/Child1', 'Xform')
        child2 = stage.DefinePrim('/root/geo/Parent/Child2', 'Sphere')
        newGrandparent = stage.DefinePrim('/root/geo/NewGrandparent', 'Xform')
        self.assertTrue(parent.IsValid())
        self.assertTrue(child1.IsValid())
        self.assertTrue(child2.IsValid())
        self.assertTrue(newGrandparent.IsValid())

        # Reparent the entire hierarchy.
        cmds.parent(
            proxyShapePath + ',/root/geo/Parent',
            proxyShapePath + ',/root/geo/NewGrandparent'
        )

        # Verify the hierarchy was reparented.
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewGrandparent/Parent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewGrandparent/Parent/Child1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewGrandparent/Parent/Child2').IsValid())

        # Validate undo.
        cmds.undo()
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent/Child1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Parent/Child2').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/NewGrandparent/Parent').IsValid())

        # Validate redo.
        cmds.redo()
        self.assertFalse(stage.GetPrimAtPath('/root/geo/Parent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewGrandparent/Parent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewGrandparent/Parent/Child1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewGrandparent/Parent/Child2').IsValid())

    def testReparentRestrictionToOutsideComponentScope(self):
        '''Test reparent restriction - cannot reparent prims to outside component scopes.'''
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

        # Create a prim inside the geo scope and a parent outside.
        child = stage.DefinePrim('/root/geo/Child', 'Xform')
        outsideParent = stage.DefinePrim('/root/OutsideScope', 'Xform')
        self.assertTrue(child.IsValid())
        self.assertTrue(outsideParent.IsValid())

        # Reparenting to a destination outside the component scopes must be rejected.
        with self.assertRaises(Exception):
            cmds.parent(
                proxyShapePath + ',/root/geo/Child',
                proxyShapePath + ',/root/OutsideScope')

        # Verify the prim was not moved.
        self.assertTrue(stage.GetPrimAtPath('/root/geo/Child').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/OutsideScope/Child').IsValid())

    def testReparentRestrictionComponentScopeItself(self):
        '''Test reparent restriction - cannot reparent the component scope prims themselves.'''
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

        # Create a new parent.
        newParent = stage.DefinePrim('/root/NewParent', 'Xform')
        self.assertTrue(newParent.IsValid())

        # Try to reparent the geo scope itself - should fail.
        # Maya wraps the exception, so we check that it raises any exception.
        with self.assertRaises(Exception):
            cmds.parent(proxyShapePath + ',/root/geo', proxyShapePath + ',/root/NewParent')

        # Verify scope still at original location.
        self.assertTrue(stage.GetPrimAtPath('/root/geo').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/NewParent/geo').IsValid())



if __name__ == '__main__':
    fixturesUtils.runTests(globals())

