#!/usr/bin/env python 

import fixturesUtils
from maya import cmds
from maya import standalone
import mayaUsd.ufe
import mayaUtils
import ufeUtils
import ufe
import unittest
import usdUtils
from pxr import UsdGeom

#####################################################################
#
# Helper to compare strings.

def filterUsdStr(usdSceneStr):
    '''Remove empty lines and lines starting with pound character.'''
    nonBlankLines = filter(None, [l.rstrip() for l in usdSceneStr.splitlines()])
    finalLines = [l for l in nonBlankLines if not l.startswith('#')]
    return '\n'.join(finalLines)


#####################################################################
#
# Edit routers used in test

def routeCmdToSessionLayer(context, routingData):
    '''
    Edit router for commands, routing to the session layer.
    '''
    prim = context.get('prim')
    if prim is None:
        print('Prim not in context')
        return
    
    routingData['layer'] = prim.GetStage().GetSessionLayer().identifier
    
def routeCmdToRootLayer(context, routingData):
    '''
    Edit router for commands, routing to the root layer.
    '''
    prim = context.get('prim')
    if prim is None:
        print('Prim not in context')
        return
    
    routingData['layer'] = prim.GetStage().GetRootLayer().identifier
    
def routeVisibilityAttribute(context, routingData):
    '''
    Edit router for attributes, routing to the session layer.
    '''
    prim = context.get('prim')
    if prim is None:
        print('Prim not in context')
        return
    
    attrName = context.get('attribute')
    if attrName != UsdGeom.Tokens.visibility:
        return
    
    routingData['layer'] = prim.GetStage().GetSessionLayer().identifier
    
def preventCommandRouter(context, routingData):
    '''
    Edit router that prevents an operation from happening.
    '''
    opName = context.get('operation') or 'operation'
    raise Exception('Sorry, %s is not permitted' % opName)

class CustomCompositeCmd(ufe.CompositeUndoableCommand):
    '''
    Custom composite command that route using the 'custom' operation.
    '''

    # The name of the composite operation. Used for edit routing.
    # Registering an edit router for this operation will affect this
    # composite command and its sub-commands.
    customOpName = 'custom'

    def __init__(self, prim, sceneItem):
        super().__init__()
        self._prim = prim
        # Note: the edit router must be kept alive in a variable to affect
        #       subsequent code, in particular the creation of sub-commands.
        ctx = mayaUsd.lib.OperationEditRouterContext(self.customOpName, self._prim)
        o3d = ufe.Object3d.object3d(sceneItem)
        self.append(o3d.setVisibleCmd(False))

    def execute(self):
        # Note: the edit router must be kept alive in a variable to affect
        #       the call to the base class implementation where sub-commands
        #       are executed.
        ctx = mayaUsd.lib.OperationEditRouterContext(self.customOpName, self._prim)
        super().execute()

    def undo(self):
        # Note: the edit router must be kept alive in a variable to affect
        #       the call to the base class implementation where sub-commands
        #       are undone.
        ctx = mayaUsd.lib.OperationEditRouterContext(self.customOpName, self._prim)
        super().undo()

    def redo(self):
        # Note: the edit router must be kept alive in a variable to affect
        #       the call to the base class implementation where sub-commands
        #       are redone.
        ctx = mayaUsd.lib.OperationEditRouterContext('custom', self._prim)
        super().redo()


#####################################################################
#
# Tests

class EditRoutingTestCase(unittest.TestCase):
    '''Verify the Maya Edit Router for visibility.'''
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
        #
 
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/A', 'Xform')
        stage.DefinePrim('/B', 'Xform')
 
        psPath = ufe.PathString.path(psPathStr)
        psPathSegment = psPath.segments[0]
        aPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/A')])
        bPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/B')])
        self.a = ufe.Hierarchy.createItem(aPath)
        self.b = ufe.Hierarchy.createItem(bPath)
 
        cmds.select(clear=True)

    def tearDown(self):
        # Restore default edit routers.
        mayaUsd.lib.restoreAllDefaultEditRouters()

    def _verifyEditRouterForCmd(self, operationName, cmdFunc, verifyFunc):
        '''
        Test edit router functionality for the given operation name, using the given command and verifier.
        The B xform will be in the global selection and is assumed to be affected by the command.
        '''

        # Get the session layer
        prim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A")
        sessionLayer = prim.GetStage().GetSessionLayer()
 
        # Check that the session layer is empty
        self.assertTrue(sessionLayer.empty)
 
        # Send visibility edits to the session layer.
        mayaUsd.lib.registerEditRouter(operationName, routeCmdToSessionLayer)
 
        # Select /B
        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(self.b)
 
        # Affect B via the command
        cmdFunc()

        # Check that something was written to the session layer
        self.assertIsNotNone(sessionLayer)

        # Verify the command was routed.
        verifyFunc(sessionLayer)

    def testEditRouterForVisibilityCmd(self):
        '''
        Test edit router functionality for the set-visibility command.
        '''

        def setVisibility():
            cmds.hide()

        def verifyVisibility(sessionLayer):
            self.assertIsNotNone(sessionLayer.GetPrimAtPath('/B'))

            # Check that any visibility changes were written to the session layer
            self.assertIsNotNone(sessionLayer.GetAttributeAtPath('/B.visibility').default)
 
            # Check that correct visibility changes were written to the session layer
            self.assertEqual(filterUsdStr(sessionLayer.ExportToString()),
                            'over "B"\n{\n    token visibility = "invisible"\n}')
            
        self._verifyEditRouterForCmd('visibility', setVisibility, verifyVisibility)

    def testEditRouterForDuplicateCmd(self):
        '''
        Test edit router functionality for the duplicate command.
        '''

        def duplicate():
            cmds.duplicate()

        def verifyDuplicate(sessionLayer):
            # Check that the duplicated prim was created in the session layer
            self.assertIsNotNone(sessionLayer.GetPrimAtPath('/B1'))
            self.assertTrue(sessionLayer.GetPrimAtPath('/B1'))
 
            # Check that correct duplicated prim was written to the session layer
            self.assertEqual(filterUsdStr(sessionLayer.ExportToString()),
                            'def Xform "B1"\n{\n}')
            
        self._verifyEditRouterForCmd('duplicate', duplicate, verifyDuplicate)

    def testEditRouterForParentCmd(self):
        '''
        Test edit router functionality for the parent command.
        '''

        def group():
            cmds.group()

        def verifyGroup(sessionLayer):
            # Check that correct grouped prim was written to the session layer
            self.assertEqual(filterUsdStr(sessionLayer.ExportToString()),
                            'over "group1"\n{\n    def Xform "B"\n    {\n    }\n}')

            # Check that the grouped prim was created in the session layer
            self.assertIsNotNone(sessionLayer.GetPrimAtPath('/group1'))
            self.assertTrue(sessionLayer.GetPrimAtPath('/group1'))
            self.assertIsNotNone(sessionLayer.GetPrimAtPath('/group1/B'))
            self.assertTrue(sessionLayer.GetPrimAtPath('/group1/B'))
             
        self._verifyEditRouterForCmd('parent', group, verifyGroup)

    def testEditRouterForSetVisibility(self):
        '''
        Test edit router for the visibility attribute triggered
        via UFE Object3d's setVisibility function.
        '''

        # Get the session layer
        prim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A")
        sessionLayer = prim.GetStage().GetSessionLayer()
 
        # Check that the session layer is empty
        self.assertTrue(sessionLayer.empty)
 
        # Send visibility edits to the session layer.
        mayaUsd.lib.registerEditRouter('attribute', routeVisibilityAttribute)
 
        # Hide B via the UFE Object3d setVisbility function
        object3d = ufe.Object3d.object3d(self.b)
        object3d.setVisibility(False)
 
        # Check that something was written to the session layer
        self.assertIsNotNone(sessionLayer)
        self.assertFalse(sessionLayer.empty)
        self.assertIsNotNone(sessionLayer.GetPrimAtPath('/B'))
 
        # Check that any visibility changes were written to the session layer
        self.assertIsNotNone(sessionLayer.GetAttributeAtPath('/B.visibility').default)
 
        # Check that correct visibility changes were written to the session layer
        self.assertEqual(filterUsdStr(sessionLayer.ExportToString()),
                         'over "B"\n{\n    token visibility = "invisible"\n}')

    def testEditRouterForAttributeVisibility(self):
        '''
        Test edit router for the visibility attribute triggered
         via UFE attribute's set function.
        '''

        # Get the session layer
        prim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A")
        sessionLayer = prim.GetStage().GetSessionLayer()
 
        # Check that the session layer is empty
        self.assertTrue(sessionLayer.empty)
 
        # Send visibility edits to the session layer.
        mayaUsd.lib.registerEditRouter('attribute', routeVisibilityAttribute)
 
        # Hide B via the UFE attribute set function.
        attrs = ufe.Attributes.attributes(self.b)
        visibilityAttr = attrs.attribute(UsdGeom.Tokens.visibility)
        visibilityAttr.set(UsdGeom.Tokens.invisible)
 
        # Check that something was written to the session layer
        self.assertIsNotNone(sessionLayer)
        self.assertFalse(sessionLayer.empty)
        self.assertIsNotNone(sessionLayer.GetPrimAtPath('/B'))
 
        # Check that any visibility changes were written to the session layer
        self.assertIsNotNone(sessionLayer.GetAttributeAtPath('/B.visibility').default)
 
        # Check that correct visibility changes were written to the session layer
        self.assertEqual(filterUsdStr(sessionLayer.ExportToString()),
                         'over "B"\n{\n    token visibility = "invisible"\n}')
        
        # Check we are still allowed to set the attribute without
        # explicitly changing the edit target.
        try:
            self.assertIsNotNone(visibilityAttr.setCmd(UsdGeom.Tokens.invisible))
        except Exception:
            self.assertFalse(True, "Should have been able to create a command")

    def _verifyEditRouterPreventingCmd(self, operationName, cmdFunc, verifyFunc):
        '''
        Test that an edit router can prevent a command for the given operation name,
        using the given command and verifier.
        The B xform will be in the global selection and is assumed to be affected by the command.
        '''
        # Get the session layer
        prim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A")
        stage = prim.GetStage()
        sessionLayer = stage.GetSessionLayer()
 
        # Check that the session layer is empty
        self.assertTrue(sessionLayer.empty)
 
        # Prevent edits.
        mayaUsd.lib.registerEditRouter(operationName, preventCommandRouter)
 
        # Select /B
        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(self.b)
 
        # try to affect B via the command, should be prevented
        with self.assertRaises(RuntimeError):
            cmdFunc()
 
        # Check that nothing was written to the session layer
        self.assertIsNotNone(sessionLayer)
        self.assertIsNone(sessionLayer.GetPrimAtPath('/B'))
 
        # Check that no changes were done in any layer
        verifyFunc(stage)

    def testPreventVisibilityCmd(self):
        '''
        Test edit router preventing a the visibility command by raising an exception.
        '''

        def hide():
            cmds.hide()

        def verifyNoHide(stage):
            # Check that the visibility attribute was not created.
            self.assertFalse(stage.GetAttributeAtPath('/B.visibility').HasAuthoredValue())

        self._verifyEditRouterPreventingCmd('visibility', hide, verifyNoHide)
 
    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2025, 'Requires Maya fixes for duplicate command error messages only available in Maya 2025 or greater.')
    def testPreventDuplicateCmd(self):
        '''
        Test edit router preventing the duplicate command by raising an exception.
        '''

        def duplicate():
            cmds.duplicate()

        def verifyNoDuplicate(stage):
            # Check that the duplicated prim was not created
            self.assertFalse(stage.GetPrimAtPath('/B1'))
 
        self._verifyEditRouterPreventingCmd('duplicate', duplicate, verifyNoDuplicate)
 
    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2025, 'Requires fixes in Maya only from 2025 onward.')
    def testPreventParentingCmd(self):
        '''
        Test edit router preventing the parent operation by raising an exception.
        '''

        def group():
            cmds.group()

        def verifyNoGroup(stage):
            # Check that the grouped prim was not created.
            self.assertFalse(stage.GetPrimAtPath('/group1'))
 
        self._verifyEditRouterPreventingCmd('parent', group, verifyNoGroup)
 
    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'UFE composite commands only available in Python from UFE v4.')
    def testRoutingCompositeCmd(self):
        '''
        Test that an edit router for a composite command prevent routing of sub-commands.
        The B xform will be in the global selection and is assumed to be affected by the command.
        '''
        # Get the session layer
        prim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A")
        stage = prim.GetStage()
        sessionLayer = stage.GetSessionLayer()
 
        # Check that the session layer is empty
        self.assertTrue(sessionLayer.empty)

        # Check to root layer only contains bare A and B xforms.
        rootLayer = stage.GetRootLayer()
        self.assertEqual(filterUsdStr(rootLayer.ExportToString()),
                         'def Xform "A"\n{\n}\ndef Xform "B"\n{\n}')
 
        # Route the visibility command to the session layer.
        # Route the custom composite command to the root layer.
        # The composite command routing shoud win.
        mayaUsd.lib.registerEditRouter('visibility', routeCmdToSessionLayer)
        mayaUsd.lib.registerEditRouter('custom', routeCmdToRootLayer)
 
        # Select /B
        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(self.b)
 
        # try to affect B via the command, should be prevented
        compCmd = CustomCompositeCmd(prim, self.b)
        compCmd.execute()
 
        # Check that nothing was written to the session layer
        self.assertIsNotNone(sessionLayer)
        self.assertEqual(filterUsdStr(sessionLayer.ExportToString()),
                         '')
        self.assertTrue(sessionLayer.empty)
 
        # Check that visibility was written to the root layer
        rootLayer = stage.GetRootLayer()
        self.assertEqual(filterUsdStr(rootLayer.ExportToString()),
                         'def Xform "A"\n{\n}\ndef Xform "B"\n{\n    token visibility = "invisible"\n}')

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'UFE composite commands only available in Python from UFE v4.')
    def testNotRoutingCompositeCmd(self):
        '''
        Test that a composite command with not edit router registered works as expected.
        '''
        # Get the session layer
        prim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A")
        stage = prim.GetStage()
        sessionLayer = stage.GetSessionLayer()
 
        # Check that the session layer is empty
        self.assertTrue(sessionLayer.empty)

        # Check to root layer only contains bare A and B xforms.
        rootLayer = stage.GetRootLayer()
        self.assertEqual(filterUsdStr(rootLayer.ExportToString()),
                         'def Xform "A"\n{\n}\ndef Xform "B"\n{\n}')
 
        # Select /B
        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(self.b)
 
        # try to affect B via the command, should be prevented
        compCmd = CustomCompositeCmd(prim, self.b)
        compCmd.execute()
 
        # Check that nothing was written to the session layer
        self.assertIsNotNone(sessionLayer)
        self.assertEqual(filterUsdStr(sessionLayer.ExportToString()),
                         '')
        self.assertTrue(sessionLayer.empty)
 
        # Check that visibility was written to the root layer
        rootLayer = stage.GetRootLayer()
        self.assertEqual(filterUsdStr(rootLayer.ExportToString()),
                         'def Xform "A"\n{\n}\ndef Xform "B"\n{\n    token visibility = "invisible"\n}')


if __name__ == '__main__':
    unittest.main(verbosity=2)
