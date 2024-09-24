#!/usr/bin/env python

import os
import unittest

from maya import cmds, standalone
from pxr import Sdf, Usd
import ufe

import mayaUsd.lib
import mayaUsd.ufe
import usdUfe

import fixturesUtils
import mayaUsd_createStageWithNewLayer
import mayaUtils
import testUtils
import usdUtils


#####################################################################
#
# Custom undoable command with metadata editRouting support


class CustomSetPrimMetadataCommand(ufe.UndoableCommand):
    '''
    Custom UFE undoable command to set an USD prim metadata value
    with editRouting support.
    '''

    def __init__(self, sceneItem, name, value):
        super(CustomSetPrimMetadataCommand, self).__init__()
        self.prim = usdUtils.getPrimFromSceneItem(sceneItem)
        self.name = name
        self.value = value

    def execute(self):
        self.undoItem = mayaUsd.lib.UsdUndoableItem()
        with mayaUsd.lib.UsdUndoBlock(self.undoItem):
            # Note: the edit router context must be kept alive in a variable.
            ctx = mayaUsd.ufe.PrimMetadataEditRouterContext(self.prim, self.name)
            self.prim.SetMetadata(self.name, self.value)

    def undo(self):
        self.undoItem.undo()

    def redo(self):
        self.undoItem.redo()


#####################################################################
#
# Maya usd scene used in test


def createSimpleSceneItem():
    '''
    Creates the maya scene used by following tests.
    '''
    psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()

    # Define a single prim.
    stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
    prim = stage.DefinePrim('/Xf', 'Xform')

    # Add a subLayer that will be used as editRouting destination by all tests.
    # Note: We do not use session layer as some tests verify that we route
    #       edits that are authored in session by default.
    layer = Sdf.Layer.CreateAnonymous("metadataLayerTag")
    stage.GetRootLayer().subLayerPaths.append(layer.identifier)

    return ufe.Hierarchy.createItem(ufe.Path([
        ufe.PathString.path(psPathStr).segments[0],
        usdUtils.createUfePathSegment(str(prim.GetPath())),
    ]))


def getMetadataDestinationLayer(prim):
    '''
    Returns the editRouting destination layer used by the following tests.
    '''
    # Note: We do not use session layer here as some tests verify that we route
    #       edits that are authored in session by default.
    for layer in prim.GetStage().GetLayerStack(False):
        if layer.anonymous and layer.identifier.endswith('metadataLayerTag'):
            destLayer = layer
            break
    else:
        raise Exception("Could not find expected anonymous layer in layer stack")
    return destLayer


def varSelectionCmd(sceneItem, variantSetName, variantName):
    '''
    Creates an ufe command to perform the given variant selection.
    '''
    ctxOps = ufe.ContextOps.contextOps(sceneItem)
    return ctxOps.doOpCmd(['Variant Sets', variantSetName, variantName])


def primFromSceneItem(sceneItem):
    return usdUtils.getPrimFromSceneItem(sceneItem)


#####################################################################
#
# Edit routers used in test


def preventCommandRouter(context, routingData):
    '''
    Edit router that prevents an operation from happening.
    '''
    opName = context.get('operation') or 'operation'
    raise Exception('Sorry, %s is not permitted' % opName)


def routePrimMetadataToDestLayer(context, routingData):
    '''
    Edit router for commands, routing primMetadata to the test destination layer.
    '''
    prim = context.get('prim')
    if prim is None:
        print('Prim not in context')
        return

    if context.get('operation') == 'primMetadata':
        routingData['layer'] = getMetadataDestinationLayer(prim).identifier


def routeOnlySessionVariantSelectionToDestLayer(context, rootingData):
    '''
    Edit router for commands, routing to the the test destination layer
    only variant selections on a variantSet named SessionVariant.
    '''
    if context.get('primMetadata') == 'variantSelection':
        if context.get('keyPath') == 'SessionVariant':
            routePrimMetadataToDestLayer(context, rootingData)


#####################################################################
#
# Tests


class PrimMetadataEditRoutingTestCase(unittest.TestCase):
    '''Verify the Maya Edit Router for prim metadata operations.'''

    pluginsLoaded = False
    ufeMetadataSupported = os.getenv('UFE_SCENEITEM_HAS_METADATA', 'FALSE') != 'FALSE'

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

    def tearDown(self):
        # Restore default edit routers.
        mayaUsd.lib.clearAllEditRouters()
        mayaUsd.lib.restoreAllDefaultEditRouters()

    def _verifyEditRoutingForMetadata(
        self,
        prepCmdFn,
        verifyFn,
        routeFn=None,
        useFastRouting=None,
    ):
        '''
        Utility to verify edit routing for an ufe undoable command which is setting
        or clearing an USD prim metadata value.
        It can route using a custom router, or fast routing to the session layer,
        else it will route to the session layer using a router command.
        '''
        # Create a stage with a single prim.
        sceneItem = createSimpleSceneItem()
        prim = primFromSceneItem(sceneItem)
        stage = prim.GetStage()
        dstLayer = getMetadataDestinationLayer(prim)

        # Verify our test case is valid.
        self.assertNotEqual(stage.GetEditTarget().GetLayer(), dstLayer)
        self.assertTrue(dstLayer.empty)

        # Customize destination prim and get the undoable command to verify.
        undoableCmd = prepCmdFn(sceneItem)
        
        # Setup Edit routing.
        if useFastRouting:
            if routeFn is not None:
                raise Exception('routeToStageLayer and routeFn cannot be used together')
            mayaUsd.lib.registerStageLayerEditRouter('primMetadata', stage, dstLayer)
        else:
            mayaUsd.lib.registerEditRouter('primMetadata', routeFn or routePrimMetadataToDestLayer)

        def verify():
            verifyFn(dstLayer.GetPrimAtPath(prim.GetPath()))

        # Verify that the verify func is valid.
        hadPrimSpecBeforeExec = bool(dstLayer.GetPrimAtPath(prim.GetPath()))
        if hadPrimSpecBeforeExec:
            with self.assertRaises(AssertionError):
                verify()

        # Verify that the command is routed to the session layer.
        undoableCmd.execute()
        verify()

        # Verify that undo is performed in session layer.
        undoableCmd.undo()

        hasPrimSpecAfterUndo = bool(dstLayer.GetPrimAtPath(prim.GetPath()))
        if hasPrimSpecAfterUndo != hadPrimSpecBeforeExec:
            with self.assertRaises(AssertionError):
                verify()

        # Verify that redo correctly re-route edits.
        undoableCmd.redo()
        verify()

    def _verifyEditRoutingForSetMetadata(self, cmdFn, verifyFn, **kwargs):
        '''
        Utility to verify edit routing for a command setting a prim metadata value.
        '''

        def verifySpecFn(primSpec):
            # verify that we have a valid spec authored.
            self.assertIsNotNone(primSpec)
            verifyFn(primSpec)

        self._verifyEditRoutingForMetadata(cmdFn, verifySpecFn, **kwargs)

    def _verifyEditRoutingForClearMetadata(self, prepFn, cmdFn, verifyFn, **kwargs):
        '''
        Utility to verify edit routing for a command clearing a prim metadata value.
        '''

        def prepCmdFn(sceneItem):
            # prepare the prim, adding metadata to be cleared.
            prim = primFromSceneItem(sceneItem)
            with Usd.EditContext(prim.GetStage(), getMetadataDestinationLayer(prim)):
                prepFn(prim)
            # return the clear command.
            return cmdFn(sceneItem)

        self._verifyEditRoutingForMetadata(prepCmdFn, verifyFn, **kwargs)

    def testPreventEditRoutingOfMetadataOperations(self):
        '''
        Test that edit routing can prevent commands for the primMetadata operation.
        '''
        # Create a stage with a single prim
        sceneItem = createSimpleSceneItem()
        prim = primFromSceneItem(sceneItem)
        stage = prim.GetStage()
        varSet = prim.GetVariantSets().AddVariantSet('TestVariant')
        refFile = testUtils.getTestScene('twoSpheres', 'sphere.usda')

        stage.SetEditTarget(getMetadataDestinationLayer(prim))

        # Prevent edits.
        mayaUsd.lib.registerEditRouter('primMetadata', preventCommandRouter)

        # Try to affect prim via various commands, all should be prevented.
        for cmd in (
            usdUfe.ToggleActiveCommand(prim),
            usdUfe.ToggleInstanceableCommand(prim),
            usdUfe.SetKindCommand(prim, 'group'),
            usdUfe.AddPayloadCommand(prim, refFile, True),
            usdUfe.AddReferenceCommand(prim, refFile, True),
            varSelectionCmd(sceneItem, varSet.GetName(), 'On'),
        ):
            # Check that the command fails.
            with self.assertRaises(RuntimeError):
                cmd.execute()

        # Check that nothing was written to the layer.
        self.assertTrue(stage.GetEditTarget().GetLayer().empty)

        # Check that no changes were done in any layer.
        self.assertTrue(prim.IsActive())
        self.assertFalse(prim.IsInstanceable())
        self.assertEqual(Usd.ModelAPI(prim).GetKind(), '')
        self.assertFalse(prim.HasAuthoredPayloads())
        self.assertFalse(prim.HasAuthoredReferences())
        self.assertFalse(varSet.HasAuthoredVariantSelection())

    def testEditRouterWithFastRouting(self):
        '''
        Test metadata fast edit routing.
        '''
        self._verifyEditRoutingForMetadata(
            lambda item: usdUfe.ToggleActiveCommand(primFromSceneItem(item)),
            lambda spec: self.assertFalse(spec.active),
            useFastRouting=True,
        )

    def testEditRouterForVariantSelection(self):
        '''
        Test edit router functionality for the variant selection commands.
        '''
        self._verifyEditRoutingForSetMetadata(
            lambda item: varSelectionCmd(item, 'TestVariant', 'On'),
            lambda spec: self.assertEqual(dict(spec.variantSelections), {'TestVariant': 'On'}),
        )

    def testEditRouterForSpecificVariantSelection(self):
        '''
        Test that edit router can decide a route based on the variantSet
        for variant selection editions.
        '''
        # Verify that selection in SessionVariant variantSet is correctly routed
        # when the callback only routes SessionVariant.
        self._verifyEditRoutingForSetMetadata(
            lambda item: varSelectionCmd(item, 'SessionVariant', 'On'),
            lambda spec: self.assertEqual(dict(spec.variantSelections), {'SessionVariant': 'On'}),
            routeFn=routeOnlySessionVariantSelectionToDestLayer,
        )

        # Verify that selection of a another variantSet is NOT routed
        # when the callback only routes SessionVariant.
        self._verifyEditRoutingForMetadata(
            lambda item: varSelectionCmd(item, 'NotSessionVariant', 'On'),
            lambda spec: self.assertIsNone(spec),
            routeFn=routeOnlySessionVariantSelectionToDestLayer,
        )

    def testEditRouterForToggleActive(self):
        '''
        Test edit router functionality for the UsdUfe ToggleActive context op command.
        '''
        self._verifyEditRoutingForSetMetadata(
            lambda item: usdUfe.ToggleActiveCommand(primFromSceneItem(item)),
            lambda spec: self.assertFalse(spec.active),
        )

    def testEditRouterForCustomUndoableCommand(self):
        '''
        Test edit router functionality for a custom Python undoable command.
        '''
        self._verifyEditRoutingForSetMetadata(
            lambda item: CustomSetPrimMetadataCommand(item, 'comment', 'Edited'),
            lambda spec: self.assertEqual(spec.comment, 'Edited'),
        )

    def testEditRouterForToggleInstanceable(self):
        '''
        Test edit router functionality for the UsdUfe ToggleInstanceable command.
        '''
        self._verifyEditRoutingForSetMetadata(
            lambda item: usdUfe.ToggleInstanceableCommand(primFromSceneItem(item)),
            lambda spec: self.assertTrue(spec.instanceable),
        )

    def testEditRouterForSetKind(self):
        '''
        Test edit router functionality for the UsdUfe SetKind command.
        '''
        self._verifyEditRoutingForSetMetadata(
            lambda item: usdUfe.SetKindCommand(primFromSceneItem(item), 'group'),
            lambda spec: self.assertEqual(spec.kind, 'group'),
        )

    def testEditRouterForAddReference(self):
        '''
        Test edit router functionality for the UsdUfe AddReference command.
        '''
        refFile = testUtils.getTestScene('twoSpheres', 'sphere.usda')

        self._verifyEditRoutingForSetMetadata(
            lambda item: usdUfe.AddReferenceCommand(primFromSceneItem(item), refFile, True),
            lambda spec: self.assertTrue(spec.hasReferences),
        )

    def testEditRouterForAddPayload(self):
        '''
        Test edit router functionality for the UsdUfe AddPayload command.
        '''
        refFile = testUtils.getTestScene('twoSpheres', 'sphere.usda')

        self._verifyEditRoutingForSetMetadata(
            lambda item: usdUfe.AddPayloadCommand(primFromSceneItem(item), refFile, True),
            lambda spec: self.assertTrue(spec.hasPayloads),
        )

    def testEditRouterForClearReferences(self):
        '''
        Test edit router functionality for the UsdUfe ClearReferences command.
        '''
        refFile = testUtils.getTestScene('twoSpheres', 'sphere.usda')

        self._verifyEditRoutingForClearMetadata(
            lambda prim: prim.GetReferences().AddReference(Sdf.Reference(refFile)),
            lambda item: usdUfe.ClearReferencesCommand(primFromSceneItem(item)),
            lambda spec: self.assertFalse(spec.hasReferences),
        )

    def testEditRouterForClearPayloads(self):
        '''
        Test edit router functionality for the UsdUfe ClearPayloads command.
        '''
        refFile = testUtils.getTestScene('twoSpheres', 'sphere.usda')

        self._verifyEditRoutingForClearMetadata(
            lambda prim: prim.GetPayloads().AddPayload(Sdf.Payload(refFile)),
            lambda item: usdUfe.ClearPayloadsCommand(primFromSceneItem(item)),
            lambda spec: self.assertFalse(spec.hasPayloads),
        )

    @unittest.skipUnless(ufeMetadataSupported, 'Available only if UFE supports metadata.')
    def testEditRouterForSetSceneItemMetadata(self):
        '''
        Test edit router functionality for setting Ufe sceneItem metadata.
        '''
        self._verifyEditRoutingForSetMetadata(
            lambda item: item.setMetadataCmd('Key', 'Edited'),
            lambda spec: self.assertEqual(spec.customData.get('Key'), 'Edited'),
        )

    @unittest.skipUnless(ufeMetadataSupported, 'Available only if UFE supports metadata.')
    def testEditRouterForSetSceneItemGroupMetadata(self):
        '''
        Test edit router functionality for setting Ufe sceneItem metadata.
        '''
        self._verifyEditRoutingForSetMetadata(
            lambda item: item.setGroupMetadataCmd('Group', 'Key', 'Edited'),
            lambda spec: self.assertEqual(spec.customData.get('Group'), {'Key': 'Edited'}),
        )

    @unittest.skipUnless(ufeMetadataSupported, 'Available only if UFE supports metadata.')
    def testEditRouterForClearSceneItemMetadata(self):
        '''
        Test edit router functionality for clearing Ufe sceneItem metadata.
        '''
        self._verifyEditRoutingForClearMetadata(
            lambda prim: prim.SetCustomDataByKey('Key', 'Edited'),
            lambda item: item.clearMetadataCmd('Key'),
            lambda spec: self.assertNotIn('Key', spec.customData),
        )

    @unittest.skipUnless(ufeMetadataSupported, 'Available only if UFE supports metadata.')
    def testEditRouterForClearSceneItemGroupMetadata(self):
        '''
        Test edit router functionality for clearing Ufe sceneItem metadata.
        '''
        self._verifyEditRoutingForClearMetadata(
            lambda prim: prim.SetCustomDataByKey('Group:Key', 'Edited'),
            lambda item: item.clearGroupMetadataCmd('Group', 'Key'),
            lambda spec: self.assertNotIn('Group', spec.customData),
        )

    @unittest.skipUnless(ufeMetadataSupported, 'Available only if UFE supports metadata.')
    def testEditRouterForSetSceneItemSessionMetadata(self):
        '''
        Test edit router functionality for setting Ufe sceneItem metadata with 'SessionLayer-'
        group prefix. They are authored in session layer by default, verify that we can
        route them to another layer.
        '''
        self._verifyEditRoutingForSetMetadata(
            lambda item: item.setGroupMetadataCmd('SessionLayer-Autodesk', 'Key', 'Edited'),
            lambda spec: self.assertEqual(spec.customData.get('Autodesk'), {'Key': 'Edited'}),
        )

    @unittest.skipUnless(ufeMetadataSupported, 'Available only if UFE supports metadata.')
    def testEditRouterForClearSceneItemSessionMetadata(self):
        '''
        Test edit router functionality for clearing Ufe sceneItem metadata with 'SessionLayer-'
        group prefix. They are authored in session layer by default, verify that we can
        route them to another layer.
        '''
        self._verifyEditRoutingForClearMetadata(
            lambda prim: prim.SetCustomDataByKey('Autodesk:Key', 'Edited'),
            lambda item: item.clearGroupMetadataCmd('SessionLayer-Autodesk', 'Key'),
            lambda spec: self.assertNotIn('Autodesk', spec.customData),
        )

if __name__ == '__main__':
    unittest.main(verbosity=2)
