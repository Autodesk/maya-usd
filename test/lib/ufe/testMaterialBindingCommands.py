#!/usr/bin/env python

#
# Copyright 2026 Autodesk
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
from maya import cmds
from maya import standalone
import mayaUsd.ufe
import mayaUtils
import usdUfe
import ufe
import unittest
import usdUtils

from pxr import Usd, UsdShade

#####################################################################
#
# Tests

class MaterialBindingCommandsTestCase(unittest.TestCase):
    '''
    Test the material binding commands exposed to Python:
        - BindMaterialCommand
        - UnbindMaterialCommand
        - SetMaterialBindingStrengthCommand
        - CreateCollectionMaterialBindingCommand
        - BindCollectionMaterialCommand
        - UnbindCollectionMaterialCommand
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        self.assertTrue(self.pluginsLoaded)

        cmds.file(new=True, force=True)
        import mayaUsd_createStageWithNewLayer

        # Create the following hierarchy:
        #
        # proxy shape
        #  |_ A
        #  |_ B
        #  |_ mtl
        #      |_ Material1
        #      |_ Material2
        #
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        self.stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        self.stage.DefinePrim('/A', 'Xform')
        self.stage.DefinePrim('/B', 'Xform')
        UsdShade.Material.Define(self.stage, '/mtl/Material1')
        UsdShade.Material.Define(self.stage, '/mtl/Material2')

        psPath = ufe.PathString.path(psPathStr)
        psPathSegment = psPath.segments[0]
        aPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/A')])
        bPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/B')])
        matPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/mtl/Material1')])

        self.aPathStr = ufe.PathString.string(aPath)
        self.bPathStr = ufe.PathString.string(bPath)
        self.matPathStr = ufe.PathString.string(matPath)

        self.aPrim = mayaUsd.ufe.ufePathToPrim(self.aPathStr)
        self.bPrim = mayaUsd.ufe.ufePathToPrim(self.bPathStr)

        self.mat1PathStr = '/mtl/Material1'
        self.mat2PathStr = '/mtl/Material2'

        self.allPurpose = UsdShade.Tokens.allPurpose
        self.preview = UsdShade.Tokens.preview
        self.full = UsdShade.Tokens.full

        self.collectionName = 'myColl'
        Usd.CollectionAPI.Apply(self.aPrim, self.collectionName)

        cmds.select(clear=True)

    def _directBinding(self, prim, purpose=''):
        purpose = purpose if purpose else self.allPurpose
        return UsdShade.MaterialBindingAPI(prim).GetDirectBinding(purpose)

    def verifyBinding(self, prim, purpose, expectedMaterialPathStr):
        purpose = purpose if purpose else self.allPurpose
        binding = self._directBinding(prim, purpose)
        self.assertEqual(binding.GetMaterialPath().pathString, expectedMaterialPathStr)

    def _collectionBindingRel(self, prim, bindingName, purpose=''):
        purpose = purpose if purpose else self.allPurpose
        return UsdShade.MaterialBindingAPI(prim).GetCollectionBindingRel(bindingName, purpose)

    def _collectionBindingMaterialPath(self, prim, bindingName, purpose=''):
        rel = self._collectionBindingRel(prim, bindingName, purpose)
        if not rel:
            return ''
        collectionPath = Usd.CollectionAPI.GetNamedCollectionPath(prim, bindingName)
        for target in rel.GetTargets():
            if target != collectionPath:
                return target.pathString
        return ''

    def verifyCollectionBinding(self, prim, bindingName, purpose, expectedMaterialPathStr):
        self.assertEqual(
            self._collectionBindingMaterialPath(prim, bindingName, purpose),
            expectedMaterialPathStr)

    #####################################################################
    # BindMaterialCommand

    def testBindMaterialCommandAllPurpose(self):
        '''
        Bind a material to a prim using the default (all purpose) binding.
        '''
        self.verifyBinding(self.aPrim, self.allPurpose, '')

        cmd = usdUfe.BindMaterialCommand(self.aPathStr, self.mat1PathStr, '')
        cmd.execute()
        self.verifyBinding(self.aPrim, self.allPurpose, self.mat1PathStr)

        cmd.undo()
        self.verifyBinding(self.aPrim, self.allPurpose, '')

        cmd.redo()
        self.verifyBinding(self.aPrim, self.allPurpose, self.mat1PathStr)

    def testBindMaterialCommandWithPurpose(self):
        '''
        Bind a material to a prim under a specific purpose only.
        '''
        cmd = usdUfe.BindMaterialCommand(self.aPathStr, self.mat1PathStr, self.preview)
        cmd.execute()

        self.verifyBinding(self.aPrim, self.allPurpose, '')
        self.verifyBinding(self.aPrim, self.preview, self.mat1PathStr)
        self.verifyBinding(self.aPrim, self.full, '')

        cmd.undo()
        self.verifyBinding(self.aPrim, self.preview, '')

        cmd.redo()
        self.verifyBinding(self.aPrim, self.preview, self.mat1PathStr)

    def testBindMaterialCommandRebind(self):
        '''
        Binding a second material under a purpose that is already bound should
        retarget the existing binding relationship instead of creating a new one.
        '''
        cmd1 = usdUfe.BindMaterialCommand(self.aPathStr, self.mat1PathStr, '')
        cmd1.execute()
        self.verifyBinding(self.aPrim, self.allPurpose, self.mat1PathStr)

        cmd2 = usdUfe.BindMaterialCommand(self.aPathStr, self.mat2PathStr, '')
        cmd2.execute()
        self.verifyBinding(self.aPrim, self.allPurpose, self.mat2PathStr)

        cmd2.undo()
        self.verifyBinding(self.aPrim, self.allPurpose, self.mat1PathStr)

        cmd2.redo()
        self.verifyBinding(self.aPrim, self.allPurpose, self.mat2PathStr)

    def testBindMaterialCommandInvalidPrim(self):
        '''
        Constructing a BindMaterialCommand with an invalid prim path should raise.
        '''
        badPathStr = self.aPathStr + '_NoSuchPrim'
        self.assertRaises(
            RuntimeError, usdUfe.BindMaterialCommand, badPathStr, self.mat1PathStr, '')

    def testBindMaterialCommandInvalidMaterial(self):
        '''
        Constructing a BindMaterialCommand with an invalid material path should raise.
        '''
        self.assertRaises(
            RuntimeError, usdUfe.BindMaterialCommand, self.aPathStr, '/mtl/NoSuchMaterial', '')

        # A path that exists but is not a material is also invalid.
        self.assertRaises(
            RuntimeError, usdUfe.BindMaterialCommand, self.aPathStr, '/B', '')

    def testBindMaterialCommandIncompatiblePrim(self):
        '''
        Binding a material onto a material prim itself is not allowed.
        '''
        self.assertRaises(
            RuntimeError,
            usdUfe.BindMaterialCommand, self.matPathStr, self.mat2PathStr, '')

    def testBindMaterialCommandRestriction(self):
        '''
        Binding a material should fail when there is already a binding
        opinion authored in a stronger layer (the session layer) than the
        current edit target (the root layer).
        '''
        self.stage.SetEditTarget(self.stage.GetSessionLayer())
        sessionBindCmd = usdUfe.BindMaterialCommand(self.aPathStr, self.mat1PathStr, '')
        sessionBindCmd.execute()
        self.verifyBinding(self.aPrim, self.allPurpose, self.mat1PathStr)

        self.stage.SetEditTarget(self.stage.GetRootLayer())
        cmd = usdUfe.BindMaterialCommand(self.aPathStr, self.mat2PathStr, '')
        with self.assertRaises(RuntimeError):
            cmd.execute()

    #####################################################################
    # UnbindMaterialCommand

    def testUnbindMaterialCommandOnePurpose(self):
        '''
        Unbind a material bound under a single purpose.
        '''
        bindCmd = usdUfe.BindMaterialCommand(self.aPathStr, self.mat1PathStr, '')
        bindCmd.execute()
        self.verifyBinding(self.aPrim, self.allPurpose, self.mat1PathStr)

        unbindCmd = usdUfe.UnbindMaterialCommand(self.aPathStr, '')
        unbindCmd.execute()
        self.verifyBinding(self.aPrim, self.allPurpose, '')

        unbindCmd.undo()
        self.verifyBinding(self.aPrim, self.allPurpose, self.mat1PathStr)

        unbindCmd.redo()
        self.verifyBinding(self.aPrim, self.allPurpose, '')

    def testUnbindMaterialCommandUnassignAll(self):
        '''
        Unbind all direct material bindings across all purposes at once.
        '''
        bindAllCmd = usdUfe.BindMaterialCommand(self.aPathStr, self.mat1PathStr, '')
        bindAllCmd.execute()
        bindPreviewCmd = usdUfe.BindMaterialCommand(self.aPathStr, self.mat2PathStr, self.preview)
        bindPreviewCmd.execute()

        self.verifyBinding(self.aPrim, self.allPurpose, self.mat1PathStr)
        self.verifyBinding(self.aPrim, self.preview, self.mat2PathStr)

        unbindCmd = usdUfe.UnbindMaterialCommand(self.aPathStr, True)
        unbindCmd.execute()
        self.verifyBinding(self.aPrim, self.allPurpose, '')
        self.verifyBinding(self.aPrim, self.preview, '')

        unbindCmd.undo()
        self.verifyBinding(self.aPrim, self.allPurpose, self.mat1PathStr)
        self.verifyBinding(self.aPrim, self.preview, self.mat2PathStr)

        unbindCmd.redo()
        self.verifyBinding(self.aPrim, self.allPurpose, '')
        self.verifyBinding(self.aPrim, self.preview, '')

    def testUnbindMaterialCommandInvalidPrim(self):
        '''
        Constructing an UnbindMaterialCommand with an invalid prim path should raise.
        '''
        badPathStr = self.aPathStr + '_NoSuchPrim'
        self.assertRaises(RuntimeError, usdUfe.UnbindMaterialCommand, badPathStr, '')
        self.assertRaises(RuntimeError, usdUfe.UnbindMaterialCommand, badPathStr, True)

    def testUnbindMaterialCommandRestriction(self):
        '''
        Unbinding a material should fail when there is already a binding
        opinion authored in a stronger layer (the session layer) than the
        current edit target (the root layer).
        '''
        self.stage.SetEditTarget(self.stage.GetSessionLayer())
        sessionBindCmd = usdUfe.BindMaterialCommand(self.aPathStr, self.mat1PathStr, '')
        sessionBindCmd.execute()
        self.verifyBinding(self.aPrim, self.allPurpose, self.mat1PathStr)

        self.stage.SetEditTarget(self.stage.GetRootLayer())
        cmd = usdUfe.UnbindMaterialCommand(self.aPathStr, '')
        with self.assertRaises(RuntimeError):
            cmd.execute()

    #####################################################################
    # SetMaterialBindingStrengthCommand

    def testSetMaterialBindingStrengthOnePurpose(self):
        '''
        Change the binding strength of a single purpose's direct binding.
        '''
        bindCmd = usdUfe.BindMaterialCommand(self.aPathStr, self.mat1PathStr, '')
        bindCmd.execute()

        directRel = self._directBinding(self.aPrim, self.allPurpose).GetBindingRel()
        originalStrength = UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(directRel)

        strengthCmd = usdUfe.SetMaterialBindingStrengthCommand(
            self.aPathStr, UsdShade.Tokens.strongerThanDescendants, '')
        strengthCmd.execute()
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(directRel),
            UsdShade.Tokens.strongerThanDescendants)

        strengthCmd.undo()
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(directRel),
            originalStrength)

        strengthCmd.redo()
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(directRel),
            UsdShade.Tokens.strongerThanDescendants)

    def testSetMaterialBindingStrengthAllPurposes(self):
        '''
        Change the binding strength of all bound purposes at once.
        '''
        bindAllCmd = usdUfe.BindMaterialCommand(self.aPathStr, self.mat1PathStr, '')
        bindAllCmd.execute()
        bindPreviewCmd = usdUfe.BindMaterialCommand(self.aPathStr, self.mat2PathStr, self.preview)
        bindPreviewCmd.execute()

        allPurposeRel = self._directBinding(self.aPrim, self.allPurpose).GetBindingRel()
        previewRel = self._directBinding(self.aPrim, self.preview).GetBindingRel()
        originalAllPurposeStrength = UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(allPurposeRel)
        originalPreviewStrength = UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(previewRel)

        strengthCmd = usdUfe.SetMaterialBindingStrengthCommand(
            self.aPathStr, UsdShade.Tokens.strongerThanDescendants, True)
        strengthCmd.execute()
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(allPurposeRel),
            UsdShade.Tokens.strongerThanDescendants)
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(previewRel),
            UsdShade.Tokens.strongerThanDescendants)

        strengthCmd.undo()
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(allPurposeRel),
            originalAllPurposeStrength)
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(previewRel),
            originalPreviewStrength)

        strengthCmd.redo()
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(allPurposeRel),
            UsdShade.Tokens.strongerThanDescendants)
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(previewRel),
            UsdShade.Tokens.strongerThanDescendants)

    def testSetMaterialBindingStrengthNoOpWhenUnbound(self):
        '''
        Setting the binding strength on a prim with no material binding for the
        given purpose should be a no-op and must not raise.
        '''
        self.assertEqual(
            self._directBinding(self.bPrim, self.allPurpose).GetMaterialPath().pathString,
            '')

        strengthCmd = usdUfe.SetMaterialBindingStrengthCommand(
            self.bPathStr, UsdShade.Tokens.strongerThanDescendants, '')
        strengthCmd.execute()

        self.assertEqual(
            self._directBinding(self.bPrim, self.allPurpose).GetMaterialPath().pathString,
            '')

    def testSetMaterialBindingStrengthInvalidPrim(self):
        '''
        Constructing a SetMaterialBindingStrengthCommand with an invalid prim path
        should raise.
        '''
        badPathStr = self.aPathStr + '_NoSuchPrim'
        self.assertRaises(
            RuntimeError,
            usdUfe.SetMaterialBindingStrengthCommand,
            badPathStr, UsdShade.Tokens.strongerThanDescendants, '')
        self.assertRaises(
            RuntimeError,
            usdUfe.SetMaterialBindingStrengthCommand,
            badPathStr, UsdShade.Tokens.strongerThanDescendants, True)

    def testSetMaterialBindingStrengthRestriction(self):
        '''
        Setting the binding strength should fail when there is already a
        strength opinion authored in a stronger layer (the session layer)
        than the current edit target (the root layer).
        '''
        self.stage.SetEditTarget(self.stage.GetSessionLayer())
        sessionBindCmd = usdUfe.BindMaterialCommand(self.aPathStr, self.mat1PathStr, '')
        sessionBindCmd.execute()

        directRel = self._directBinding(self.aPrim, self.allPurpose).GetBindingRel()
        UsdShade.MaterialBindingAPI.SetMaterialBindingStrength(
            directRel, UsdShade.Tokens.strongerThanDescendants)

        self.stage.SetEditTarget(self.stage.GetRootLayer())
        cmd = usdUfe.SetMaterialBindingStrengthCommand(
            self.aPathStr, UsdShade.Tokens.strongerThanDescendants, '')
        with self.assertRaises(RuntimeError):
            cmd.execute()

    #####################################################################
    # CreateCollectionMaterialBindingCommand

    def testCreateCollectionMaterialBindingCommand(self):
        '''
        Creating a collection material binding authors an (initially unbound)
        allPurpose collection binding relationship targeting the collection.
        '''
        self.assertFalse(self._collectionBindingRel(self.aPrim, self.collectionName))

        cmd = usdUfe.CreateCollectionMaterialBindingCommand(self.aPathStr, self.collectionName)
        cmd.execute()
        self.assertTrue(self._collectionBindingRel(self.aPrim, self.collectionName))
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.allPurpose, '')

        cmd.undo()
        self.assertFalse(self._collectionBindingRel(self.aPrim, self.collectionName))

        cmd.redo()
        self.assertTrue(self._collectionBindingRel(self.aPrim, self.collectionName))

    def testCreateCollectionMaterialBindingCommandInvalidCollection(self):
        '''
        Constructing the command with a name that is not an existing collection
        on the prim should raise.
        '''
        self.assertRaises(
            RuntimeError,
            usdUfe.CreateCollectionMaterialBindingCommand, self.aPathStr, 'noSuchCollection')

    def testCreateCollectionMaterialBindingCommandInvalidPrim(self):
        '''
        Constructing the command with an invalid prim path should raise.
        '''
        badPathStr = self.aPathStr + '_NoSuchPrim'
        self.assertRaises(
            RuntimeError,
            usdUfe.CreateCollectionMaterialBindingCommand, badPathStr, self.collectionName)

    #####################################################################
    # BindCollectionMaterialCommand

    def testBindCollectionMaterialCommandAllPurpose(self):
        '''
        Bind a material to a named collection using the default (all purpose) binding.
        '''
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.allPurpose, '')

        cmd = usdUfe.BindCollectionMaterialCommand(
            self.aPathStr, self.mat1PathStr, self.collectionName, '')
        cmd.execute()
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.allPurpose, self.mat1PathStr)

        cmd.undo()
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.allPurpose, '')

        cmd.redo()
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.allPurpose, self.mat1PathStr)

    def testBindCollectionMaterialCommandWithPurpose(self):
        '''
        Bind a material to a named collection under a specific purpose only.
        '''
        cmd = usdUfe.BindCollectionMaterialCommand(
            self.aPathStr, self.mat1PathStr, self.collectionName, self.preview)
        cmd.execute()

        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.allPurpose, '')
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.preview, self.mat1PathStr)
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.full, '')

        cmd.undo()
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.preview, '')

        cmd.redo()
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.preview, self.mat1PathStr)

    def testBindCollectionMaterialCommandRebind(self):
        '''
        Binding a second material under a purpose that is already bound should
        retarget the existing binding relationship instead of creating a new one.
        '''
        cmd1 = usdUfe.BindCollectionMaterialCommand(
            self.aPathStr, self.mat1PathStr, self.collectionName, '')
        cmd1.execute()
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.allPurpose, self.mat1PathStr)

        cmd2 = usdUfe.BindCollectionMaterialCommand(
            self.aPathStr, self.mat2PathStr, self.collectionName, '')
        cmd2.execute()
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.allPurpose, self.mat2PathStr)

        cmd2.undo()
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.allPurpose, self.mat1PathStr)

        cmd2.redo()
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.allPurpose, self.mat2PathStr)

    def testBindCollectionMaterialCommandInvalidPrim(self):
        '''
        Constructing a BindCollectionMaterialCommand with an invalid prim path should raise.
        '''
        badPathStr = self.aPathStr + '_NoSuchPrim'
        self.assertRaises(
            RuntimeError,
            usdUfe.BindCollectionMaterialCommand,
            badPathStr, self.mat1PathStr, self.collectionName, '')

    def testBindCollectionMaterialCommandInvalidMaterial(self):
        '''
        Constructing a BindCollectionMaterialCommand with an invalid material path should raise.
        '''
        self.assertRaises(
            RuntimeError,
            usdUfe.BindCollectionMaterialCommand,
            self.aPathStr, '/mtl/NoSuchMaterial', self.collectionName, '')

    def testBindCollectionMaterialCommandInvalidCollection(self):
        '''
        Constructing a BindCollectionMaterialCommand with a name that is not an
        existing collection on the prim should raise.
        '''
        self.assertRaises(
            RuntimeError,
            usdUfe.BindCollectionMaterialCommand,
            self.aPathStr, self.mat1PathStr, 'noSuchCollection', '')

    #####################################################################
    # UnbindCollectionMaterialCommand

    def testUnbindCollectionMaterialCommandOnePurpose(self):
        '''
        Unbind a collection material binding under a single purpose.
        '''
        bindCmd = usdUfe.BindCollectionMaterialCommand(
            self.aPathStr, self.mat1PathStr, self.collectionName, '')
        bindCmd.execute()
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.allPurpose, self.mat1PathStr)

        unbindCmd = usdUfe.UnbindCollectionMaterialCommand(self.aPathStr, self.collectionName, '')
        unbindCmd.execute()
        self.assertFalse(self._collectionBindingRel(self.aPrim, self.collectionName, self.allPurpose))

        unbindCmd.undo()
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.allPurpose, self.mat1PathStr)

        unbindCmd.redo()
        self.assertFalse(self._collectionBindingRel(self.aPrim, self.collectionName, self.allPurpose))

    def testUnbindCollectionMaterialCommandUnassignAll(self):
        '''
        Unbind all purposes of a collection material binding at once.
        '''
        bindAllCmd = usdUfe.BindCollectionMaterialCommand(
            self.aPathStr, self.mat1PathStr, self.collectionName, '')
        bindAllCmd.execute()
        bindPreviewCmd = usdUfe.BindCollectionMaterialCommand(
            self.aPathStr, self.mat2PathStr, self.collectionName, self.preview)
        bindPreviewCmd.execute()

        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.allPurpose, self.mat1PathStr)
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.preview, self.mat2PathStr)

        unbindCmd = usdUfe.UnbindCollectionMaterialCommand(self.aPathStr, self.collectionName, True)
        unbindCmd.execute()
        self.assertFalse(self._collectionBindingRel(self.aPrim, self.collectionName, self.allPurpose))
        self.assertFalse(self._collectionBindingRel(self.aPrim, self.collectionName, self.preview))

        unbindCmd.undo()
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.allPurpose, self.mat1PathStr)
        self.verifyCollectionBinding(self.aPrim, self.collectionName, self.preview, self.mat2PathStr)

        unbindCmd.redo()
        self.assertFalse(self._collectionBindingRel(self.aPrim, self.collectionName, self.allPurpose))
        self.assertFalse(self._collectionBindingRel(self.aPrim, self.collectionName, self.preview))

    def testUnbindCollectionMaterialCommandInvalidPrim(self):
        '''
        Constructing an UnbindCollectionMaterialCommand with an invalid prim path should raise.
        '''
        badPathStr = self.aPathStr + '_NoSuchPrim'
        self.assertRaises(
            RuntimeError,
            usdUfe.UnbindCollectionMaterialCommand, badPathStr, self.collectionName, '')
        self.assertRaises(
            RuntimeError,
            usdUfe.UnbindCollectionMaterialCommand, badPathStr, self.collectionName, True)

    #####################################################################
    # SetMaterialBindingStrengthCommand (collection binding)

    def testSetMaterialBindingStrengthCollectionOnePurpose(self):
        '''
        Change the binding strength of a single purpose's collection binding.
        '''
        bindCmd = usdUfe.BindCollectionMaterialCommand(
            self.aPathStr, self.mat1PathStr, self.collectionName, '')
        bindCmd.execute()

        bindingRel = self._collectionBindingRel(self.aPrim, self.collectionName, self.allPurpose)
        originalStrength = UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(bindingRel)

        strengthCmd = usdUfe.SetMaterialBindingStrengthCommand(
            self.aPathStr, UsdShade.Tokens.strongerThanDescendants, '', self.collectionName)
        strengthCmd.execute()
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(bindingRel),
            UsdShade.Tokens.strongerThanDescendants)

        strengthCmd.undo()
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(bindingRel),
            originalStrength)

        strengthCmd.redo()
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(bindingRel),
            UsdShade.Tokens.strongerThanDescendants)

    def testSetMaterialBindingStrengthCollectionAllPurposes(self):
        '''
        Change the binding strength of all bound purposes of a collection
        binding at once.
        '''
        bindAllCmd = usdUfe.BindCollectionMaterialCommand(
            self.aPathStr, self.mat1PathStr, self.collectionName, '')
        bindAllCmd.execute()
        bindPreviewCmd = usdUfe.BindCollectionMaterialCommand(
            self.aPathStr, self.mat2PathStr, self.collectionName, self.preview)
        bindPreviewCmd.execute()

        allPurposeRel = self._collectionBindingRel(self.aPrim, self.collectionName, self.allPurpose)
        previewRel = self._collectionBindingRel(self.aPrim, self.collectionName, self.preview)

        strengthCmd = usdUfe.SetMaterialBindingStrengthCommand(
            self.aPathStr, UsdShade.Tokens.strongerThanDescendants, True, self.collectionName)
        strengthCmd.execute()
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(allPurposeRel),
            UsdShade.Tokens.strongerThanDescendants)
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(previewRel),
            UsdShade.Tokens.strongerThanDescendants)

        strengthCmd.undo()

        strengthCmd.redo()
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(allPurposeRel),
            UsdShade.Tokens.strongerThanDescendants)
        self.assertEqual(
            UsdShade.MaterialBindingAPI.GetMaterialBindingStrength(previewRel),
            UsdShade.Tokens.strongerThanDescendants)


if __name__ == '__main__':
    unittest.main(verbosity=2)
