import unittest

import fixturesUtils
import mayaUsd.lib
import mayaUsd.ufe
from maya import cmds
from pxr import Sdf, UsdShade
import ufe

from testComponentCreatorBase import _ComponentCreatorTestBase

class RenameInComponentTestCase(_ComponentCreatorTestBase, unittest.TestCase):
    """
    Test renaming prims in Adsk USD Component stages.

    Component stages have special rename behavior where prim specs are renamed
    in all layers of the prim stack (not just defining layers).
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def setUp(self):
        self._setUpCC()

    def testRenamePrimInComponentStage(self):
        '''Test successful renaming of a prim in a component stage.'''
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
        testPrim = stage.DefinePrim('/root/geo/OldName', 'Xform')
        self.assertTrue(testPrim.IsValid())
        self.assertEqual(testPrim.GetName(), 'OldName')

        # Rename the prim using cmds.rename.
        cmds.rename(proxyShapePath + ',/root/geo/OldName', 'NewName')

        # Verify the prim was renamed.
        self.assertFalse(stage.GetPrimAtPath('/root/geo/OldName').IsValid())
        renamedPrim = stage.GetPrimAtPath('/root/geo/NewName')
        self.assertTrue(renamedPrim.IsValid())
        self.assertEqual(renamedPrim.GetName(), 'NewName')

        # Validate undo.
        cmds.undo()
        self.assertTrue(stage.GetPrimAtPath('/root/geo/OldName').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/NewName').IsValid())

        # Validate redo.
        cmds.redo()
        self.assertFalse(stage.GetPrimAtPath('/root/geo/OldName').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewName').IsValid())

    def testRenamePrimWithChildrenInComponentStage(self):
        '''Test renaming a prim with children in a component stage.'''
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
        parent = stage.DefinePrim('/root/geo/OldParent', 'Xform')
        child1 = stage.DefinePrim('/root/geo/OldParent/Child1', 'Xform')
        child2 = stage.DefinePrim('/root/geo/OldParent/Child2', 'Sphere')
        self.assertTrue(parent.IsValid())
        self.assertTrue(child1.IsValid())
        self.assertTrue(child2.IsValid())

        # Rename the parent prim.
        cmds.rename(proxyShapePath + ',/root/geo/OldParent', 'NewParent')

        # Verify the parent was renamed and children moved with it.
        self.assertFalse(stage.GetPrimAtPath('/root/geo/OldParent').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/OldParent/Child1').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/OldParent/Child2').IsValid())

        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewParent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewParent/Child1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewParent/Child2').IsValid())

        # Validate undo.
        cmds.undo()
        self.assertTrue(stage.GetPrimAtPath('/root/geo/OldParent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/OldParent/Child1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/OldParent/Child2').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/geo/NewParent').IsValid())

        # Validate redo.
        cmds.redo()
        self.assertFalse(stage.GetPrimAtPath('/root/geo/OldParent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewParent').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewParent/Child1').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/root/geo/NewParent/Child2').IsValid())

    def testRenameRestrictionComponentScopeItself(self):
        '''Test rename restriction - cannot rename the component scope prims themselves.'''
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

        # Try to rename the geo scope itself - should fail.
        # Maya wraps the exception, so we check that it raises any exception.
        with self.assertRaises(Exception):
            cmds.rename(proxyShapePath + ',/root/geo', 'renamed_geo')

        # Verify scope still has original name.
        self.assertTrue(stage.GetPrimAtPath('/root/geo').IsValid())
        self.assertFalse(stage.GetPrimAtPath('/root/renamed_geo').IsValid())

    def testRenameUpdatesBindingAndConnectionInComponentStage(self):
        '''Test renaming a material in a component stage updates the material binding and
        the shader connection authored in the component's layers. Composition cannot
        remap these paths, as the component's payload root does not move.'''
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

        # A mesh in the geo scope bound to a material in the mtl scope, whose surface
        # output is connected to a shader.
        material = UsdShade.Material.Define(stage, '/root/mtl/OldMat')
        shader = UsdShade.Shader.Define(stage, '/root/mtl/OldMat/surf')
        shader.CreateIdAttr('UsdPreviewSurface')
        material.CreateSurfaceOutput().ConnectToSource(shader.ConnectableAPI(), 'surface')
        mesh = stage.DefinePrim('/root/geo/Mesh', 'Mesh')
        UsdShade.MaterialBindingAPI.Apply(mesh).Bind(material)

        def authoredPaths(pathEditor):
            return (list(pathEditor.explicitItems) + list(pathEditor.addedItems)
                    + list(pathEditor.prependedItems) + list(pathEditor.appendedItems)
                    + list(pathEditor.deletedItems) + list(pathEditor.orderedItems))

        def verifyMaterial(name, staleName):
            materialPath = Sdf.Path('/root/mtl').AppendChild(name)
            stalePath = Sdf.Path('/root/mtl').AppendChild(staleName)

            meshPrim = stage.GetPrimAtPath('/root/geo/Mesh')
            boundMaterial = UsdShade.MaterialBindingAPI(meshPrim).ComputeBoundMaterial()[0]
            self.assertEqual(boundMaterial.GetPath(), materialPath)
            surfaceOutput = boundMaterial.GetSurfaceOutput()
            source = surfaceOutput.GetConnectedSource()
            self.assertEqual(source[0].GetPath(), materialPath.AppendChild('surf'))

            # The paths are updated where they are authored: no layer of the component
            # holds the stale paths.
            bindingSpecs = meshPrim.GetRelationship('material:binding').GetPropertyStack()
            for spec in bindingSpecs:
                for path in authoredPaths(spec.targetPathList):
                    self.assertFalse(path.HasPrefix(stalePath), str(spec.path))
            outputSpecs = surfaceOutput.GetAttr().GetPropertyStack()
            for spec in outputSpecs:
                for path in authoredPaths(spec.connectionPathList):
                    self.assertFalse(path.HasPrefix(stalePath), str(spec.path))

        verifyMaterial('OldMat', 'NewMat')

        cmds.rename(proxyShapePath + ',/root/mtl/OldMat', 'NewMat')
        self.assertFalse(stage.GetPrimAtPath('/root/mtl/OldMat').IsValid())
        verifyMaterial('NewMat', 'OldMat')

        # Validate undo.
        cmds.undo()
        verifyMaterial('OldMat', 'NewMat')

        # Validate redo.
        cmds.redo()
        verifyMaterial('NewMat', 'OldMat')



if __name__ == '__main__':
    fixturesUtils.runTests(globals())

