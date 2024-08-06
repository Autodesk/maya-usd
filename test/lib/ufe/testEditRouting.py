#!/usr/bin/env python 

import fixturesUtils
from maya import cmds
from maya import standalone
import mayaUsd.ufe
import mayaUtils
import ufeUtils
import ufe
import os
import unittest
import usdUtils
from pxr import UsdGeom
from pxr import Gf
from usdUtils import filterUsdStr
import mayaUsd_createStageWithNewLayer


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
    EPSILON = 1e-3

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
        mayaUsd.lib.restoreAllDefaultEditRouters()

    def _prepareSimpleScene(self):
         # Create the following hierarchy:
        #
        # proxy shape
        #  |_ A
        #  |_ B
        #
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        self.stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        self.stage.DefinePrim('/A', 'Xform')
        self.stage.DefinePrim('/B', 'Xform')
 
        psPath = ufe.PathString.path(psPathStr)
        psPathSegment = psPath.segments[0]
        aPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/A')])
        bPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/B')])
        self.a = ufe.Hierarchy.createItem(aPath)
        self.b = ufe.Hierarchy.createItem(bPath)

        self.prim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A")
        self.sessionLayer = self.stage.GetSessionLayer()
 
        cmds.select(clear=True)

        # Select /B
        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(self.b)

    def _verifyEditRouterForCmd(self, operationName, cmdFunc, verifyFunc, useFastRouting=False):
        '''
        Test edit router functionality for the given operation name, using the given command and verifier.
        The B xform will be in the global selection and is assumed to be affected by the command.
        '''
        # Check that the session layer is empty
        self.assertTrue(self.sessionLayer.empty)
 
        # Send visibility edits to the session layer.
        if useFastRouting:
            mayaUsd.lib.registerStageLayerEditRouter(operationName, self.stage, self.sessionLayer)
        else:
            mayaUsd.lib.registerEditRouter(operationName, routeCmdToSessionLayer)
 
        # Affect B via the command
        cmdFunc()

        # Check that something was written to the session layer
        self.assertIsNotNone(self.sessionLayer)

        # Verify the command was routed.
        verifyFunc(self.sessionLayer)

    def _testEditRouterForVisibilityCmd(self, useFastRouting):
        '''
        Test edit router functionality for the set-visibility command.
        '''
        self._prepareSimpleScene()

        def setVisibility():
            cmds.hide()

        def verifyVisibility(sessionLayer):
            self.assertIsNotNone(sessionLayer.GetPrimAtPath('/B'))

            # Check that any visibility changes were written to the session layer
            self.assertIsNotNone(sessionLayer.GetAttributeAtPath('/B.visibility').default)
 
            # Check that correct visibility changes were written to the session layer
            self.assertEqual(filterUsdStr(sessionLayer.ExportToString()),
                            filterUsdStr('over "B"\n{\n    token visibility = "invisible"\n}'))
            
        self._verifyEditRouterForCmd('visibility', setVisibility, verifyVisibility, useFastRouting)

    def testEditRouterForVisibilityCmd(self):
        self._testEditRouterForVisibilityCmd(False)

    def testEditRouterForVisibilityCmdWithFastRouting(self):
        self._testEditRouterForVisibilityCmd(True)

    def _testEditRouterForDuplicateCmd(self, useFastRouting):
        '''
        Test edit router functionality for the duplicate command.
        '''
        self._prepareSimpleScene()

        def duplicate():
            cmds.duplicate()

        def verifyDuplicate(sessionLayer):
            # Check that the duplicated prim was created in the session layer
            self.assertIsNotNone(sessionLayer.GetPrimAtPath('/B1'))
            self.assertTrue(sessionLayer.GetPrimAtPath('/B1'))
 
            # Check that correct duplicated prim was written to the session layer
            self.assertEqual(filterUsdStr(sessionLayer.ExportToString()),
                            filterUsdStr('def Xform "B1"\n{\n}'))
            
        self._verifyEditRouterForCmd('duplicate', duplicate, verifyDuplicate, useFastRouting)

    def testEditRouterForDuplicateCmd(self):
        self._testEditRouterForDuplicateCmd(False)

    def testEditRouterForDuplicateCmdWithFastRouting(self):
        self._testEditRouterForDuplicateCmd(True)

    def testEditRouterForParentCmd(self):
        '''
        Test edit router functionality for the parent command.
        '''
        self._prepareSimpleScene()

        def group():
            cmds.group()

        def verifyGroup(sessionLayer):
            # Check that correct grouped prim was written to the session layer
            self.assertEqual(filterUsdStr(sessionLayer.ExportToString()),
                            filterUsdStr('over "group1"\n{\n    def Xform "B"\n    {\n    }\n}'))

            # Check that the grouped prim was created in the session layer
            self.assertIsNotNone(sessionLayer.GetPrimAtPath('/group1'))
            self.assertTrue(sessionLayer.GetPrimAtPath('/group1'))
            self.assertIsNotNone(sessionLayer.GetPrimAtPath('/group1/B'))
            self.assertTrue(sessionLayer.GetPrimAtPath('/group1/B'))
             
        self._verifyEditRouterForCmd('parent', group, verifyGroup)

    def _verifyTransform(self, sessionLayer, attributeName):
        self.assertIsNotNone(sessionLayer.GetPrimAtPath('/B'))

        # Check that any visibility changes were written to the session layer
        self.assertIsNotNone(sessionLayer.GetAttributeAtPath('/B.' + attributeName))

    def _testEditRouterForCommonAPITranslateCmd(self, useFastRouting):
        '''
        Test edit router functionality for the common API translate commands.
        '''
        self._prepareSimpleScene()

        def set():
            cmds.move(0, 1, 2)

        def verify(sessionLayer):
            self._verifyTransform(sessionLayer, 'xformOp:translate')
            
        self._verifyEditRouterForCmd('transform', set, verify, useFastRouting)

    def testEditRouterForCommonAPITranslateCmd(self):
        self._testEditRouterForCommonAPITranslateCmd(False)

    def testEditRouterForCommonAPITranslateCmdWithFastRouting(self):
        self._testEditRouterForCommonAPITranslateCmd(True)

    def testEditRouterForCommonAPIRotateCmd(self):
        '''
        Test edit router functionality for the common API rotate commands.
        '''
        self._prepareSimpleScene()

        def set():
            cmds.rotate(30, 10, 0)

        def verify(sessionLayer):
            self._verifyTransform(sessionLayer, 'xformOp:rotateXYZ')
            
        self._verifyEditRouterForCmd('transform', set, verify)

    def testEditRouterForCommonAPIScaleCmd(self):
        '''
        Test edit router functionality for the common API scale commands.
        '''
        self._prepareSimpleScene()

        def set():
            cmds.scale(2, 2, 2)

        def verify(sessionLayer):
            self._verifyTransform(sessionLayer, 'xformOp:scale')
            
        self._verifyEditRouterForCmd('transform', set, verify)

    def _preparePointInstanceScene(self):
        '''
        Prepare a stage with a point instance and an instance already selected.
        Return the point instancer and the instance index.
        '''
        mayaUtils.openPointInstancesGrid14Scene()

        # Create a UFE path to a PointInstancer prim with an instanceIndex on
        # the end. This path uniquely identifies a specific point instance.
        instanceIndex = 7

        ufePath = ufe.Path([
            mayaUtils.createUfePathSegment('|UsdProxy|UsdProxyShape'),
            usdUtils.createUfePathSegment(
                '/PointInstancerGrid/PointInstancer/%d' % instanceIndex)])
        ufeItem = ufe.Hierarchy.createItem(ufePath)

        # Select the point instance scene item.
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.append(ufeItem)

        # Get the PointInstancer prim for validating the values in USD.
        ufePathString = ufe.PathString.string(ufePath)
        self.prim = mayaUsd.ufe.ufePathToPrim(ufePathString)
        self.stage = self.prim.GetStage()
        self.sessionLayer = self.stage.GetSessionLayer()
        pointInstancer = UsdGeom.PointInstancer(self.prim)
        self.assertTrue(pointInstancer)

        return pointInstancer, instanceIndex
    
    def testManipulatePointInstancePosition(self):
        '''
        Test that we can route point instance transation.
        '''
        pointInstancer, instanceIndex = self._preparePointInstanceScene()

        def preValidate():
            # The PointInstancer should have 14 authored positions initially.
            positionsAttr = pointInstancer.GetPositionsAttr()
            positions = positionsAttr.Get()
            self.assertEqual(len(positions), 14)

            # Validate the initial position before manipulating
            position = positions[instanceIndex]
            self.assertTrue(
                Gf.IsClose(position, Gf.Vec3f(-4.5, 1.5, 0.0), self.EPSILON))

        def set():
            # Perfom a translate manipulation via the move command.
            cmds.move(1.0, 2.0, 3.0, objectSpace=True, relative=True)

        def verify(sessionLayer):
            # Re-fetch the USD positions and check for the update.
            positionsAttr = pointInstancer.GetPositionsAttr()
            position = positionsAttr.Get()[instanceIndex]
            self.assertTrue(
                Gf.IsClose(position, Gf.Vec3f(-3.5, 3.5, 3.0), self.EPSILON))
            
            instancerPath = pointInstancer.GetPath()
            self.assertIsNotNone(sessionLayer.GetPrimAtPath(instancerPath))
            self.assertIsNotNone(sessionLayer.GetAttributeAtPath(str(instancerPath) + '.positions'))
            
        preValidate()
        self._verifyEditRouterForCmd('transform', set, verify)
        
    def testManipulatePointInstanceOrientation(self):
        '''
        Test that we can route point instance rotation.
        '''
        pointInstancer, instanceIndex = self._preparePointInstanceScene()

        def preValidate():
            # The PointInstancer should not have any authored orientations
            # initially.
            orientationsAttr = pointInstancer.GetOrientationsAttr()
            self.assertFalse(orientationsAttr.HasAuthoredValue())

        def set():
            # Perfom an orientation manipulation via the rotate command.
            cmds.rotate(15.0, 30.0, 45.0, objectSpace=True, relative=True)

        def verify(sessionLayer):
            # The initial rotate should have filled out the orientations attribute.
            orientationsAttr = pointInstancer.GetOrientationsAttr()
            orientations = orientationsAttr.Get()
            self.assertEqual(len(orientations), 14)

            # Validate the rotated item.
            orientation = orientations[instanceIndex]
            self.assertTrue(
                Gf.IsClose(orientation.real, 0.897461, self.EPSILON))
            self.assertTrue(
                Gf.IsClose(orientation.imaginary, Gf.Vec3h(0.01828, 0.2854, 0.335205), self.EPSILON))

            instancerPath = pointInstancer.GetPath()
            self.assertIsNotNone(sessionLayer.GetPrimAtPath(instancerPath))
            self.assertIsNotNone(sessionLayer.GetAttributeAtPath(str(instancerPath) + '.orientations'))

        preValidate()
        self._verifyEditRouterForCmd('transform', set, verify)

    def testManipulatePointInstanceScale(self):
        '''
        Test that we can route point instance transation.
        '''
        pointInstancer, instanceIndex = self._preparePointInstanceScene()

        def preValidate():
            # The PointInstancer should not have any authored scales initially.
            scalesAttr = pointInstancer.GetScalesAttr()
            self.assertFalse(scalesAttr.HasAuthoredValue())

        def set():
            # Perfom a scale manipulation via the scale command.
            cmds.scale(1.0, 2.0, 3.0, objectSpace=True, relative=True)

        def verify(sessionLayer):
            # The initial scale should have filled out the scales attribute.
            scalesAttr = pointInstancer.GetScalesAttr()
            scales = scalesAttr.Get()
            self.assertEqual(len(scales), 14)

            # Validate the scaled item.
            scale = scales[instanceIndex]
            self.assertTrue(
                Gf.IsClose(scale, Gf.Vec3f(1.0, 2.0, 3.0), self.EPSILON))

            # The non-scaled items should all have identity scales.
            for i in [idx for idx in range(14) if idx != instanceIndex]:
                scale = scales[i]
                self.assertTrue(
                    Gf.IsClose(scale, Gf.Vec3f(1.0, 1.0, 1.0), self.EPSILON))

            instancerPath = pointInstancer.GetPath()
            self.assertIsNotNone(sessionLayer.GetPrimAtPath(instancerPath))
            self.assertIsNotNone(sessionLayer.GetAttributeAtPath(str(instancerPath) + '.scales'))


        preValidate()
        self._verifyEditRouterForCmd('transform', set, verify)

    def testEditRouterForSetVisibility(self):
        '''
        Test edit router for the visibility attribute triggered
        via UFE Object3d's setVisibility function.
        '''
        self._prepareSimpleScene()

        # Send visibility edits to the session layer.
        mayaUsd.lib.registerEditRouter('attribute', routeVisibilityAttribute)
 
        # Hide B via the UFE Object3d setVisbility function
        object3d = ufe.Object3d.object3d(self.b)
        object3d.setVisibility(False)
 
        # Check that something was written to the session layer
        self.assertIsNotNone(self.sessionLayer)
        self.assertFalse(self.sessionLayer.empty)
        self.assertIsNotNone(self.sessionLayer.GetPrimAtPath('/B'))
 
        # Check that any visibility changes were written to the session layer
        self.assertIsNotNone(self.sessionLayer.GetAttributeAtPath('/B.visibility').default)
 
        # Check that correct visibility changes were written to the session layer
        self.assertEqual(filterUsdStr(self.sessionLayer.ExportToString()),
                         filterUsdStr('over "B"\n{\n    token visibility = "invisible"\n}'))

    def testEditRouterForAttributeVisibility(self):
        '''
        Test edit router for the visibility attribute triggered
         via UFE attribute's set function.
        '''
        self._prepareSimpleScene()

        # Send visibility edits to the session layer.
        mayaUsd.lib.registerEditRouter('attribute', routeVisibilityAttribute)
 
        # Hide B via the UFE attribute set function.
        attrs = ufe.Attributes.attributes(self.b)
        visibilityAttr = attrs.attribute(UsdGeom.Tokens.visibility)
        visibilityAttr.set(UsdGeom.Tokens.invisible)
 
        # Check that something was written to the session layer
        self.assertIsNotNone(self.sessionLayer)
        self.assertFalse(self.sessionLayer.empty)
        self.assertIsNotNone(self.sessionLayer.GetPrimAtPath('/B'))
 
        # Check that any visibility changes were written to the session layer
        self.assertIsNotNone(self.sessionLayer.GetAttributeAtPath('/B.visibility').default)
 
        # Check that correct visibility changes were written to the session layer
        self.assertEqual(filterUsdStr(self.sessionLayer.ExportToString()),
                         filterUsdStr('over "B"\n{\n    token visibility = "invisible"\n}'))
        
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
        self._prepareSimpleScene()
 
        # Prevent edits.
        mayaUsd.lib.registerEditRouter(operationName, preventCommandRouter)
 
        # try to affect B via the command, should be prevented
        with self.assertRaises(RuntimeError):
            cmdFunc()
 
        # Check that nothing was written to the session layer
        self.assertIsNotNone(self.sessionLayer)
        self.assertIsNone(self.sessionLayer.GetPrimAtPath('/B'))
 
        # Check that no changes were done in any layer
        verifyFunc(self.stage)

    def testPreventVisibilityCmd(self):
        '''
        Test edit router preventing a the visibility command by raising an exception.
        '''
        self._prepareSimpleScene()

        def hide():
            cmds.hide()

        def verifyNoHide(stage):
            # Check that the visibility attribute was not created.
            self.assertFalse(stage.GetAttributeAtPath('/B.visibility').HasAuthoredValue())

        self._verifyEditRouterPreventingCmd('visibility', hide, verifyNoHide)
 
    @unittest.skipUnless(mayaUtils.mayaMajorVersion() > 2024, 'Requires Maya fixes for duplicate command error messages only available in versions of Maya greater than 2024.')
    def testPreventDuplicateCmd(self):
        '''
        Test edit router preventing the duplicate command by raising an exception.
        '''
        self._prepareSimpleScene()

        def duplicate():
            cmds.duplicate()

        def verifyNoDuplicate(stage):
            # Check that the duplicated prim was not created
            self.assertFalse(stage.GetPrimAtPath('/B1'))
 
        self._verifyEditRouterPreventingCmd('duplicate', duplicate, verifyNoDuplicate)
 
    @unittest.skipUnless(mayaUtils.mayaMajorVersion() > 2024, 'Requires fixes in versions of Maya greater than 2024.')
    def testPreventParentingCmd(self):
        '''
        Test edit router preventing the parent operation by raising an exception.
        '''
        self._prepareSimpleScene()

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
        self._prepareSimpleScene()

        # Check to root layer only contains bare A and B xforms.
        rootLayer = self.stage.GetRootLayer()
        self.assertEqual(filterUsdStr(rootLayer.ExportToString()),
                         filterUsdStr('def Xform "A"\n{\n}\ndef Xform "B"\n{\n}'))
 
        # Route the visibility command to the session layer.
        # Route the custom composite command to the root layer.
        # The composite command routing shoud win.
        mayaUsd.lib.registerEditRouter('visibility', routeCmdToSessionLayer)
        mayaUsd.lib.registerEditRouter('custom', routeCmdToRootLayer)
 
        # try to affect B via the command, should be prevented
        compCmd = CustomCompositeCmd(self.prim, self.b)
        compCmd.execute()
 
        # Check that nothing was written to the session layer
        self.assertIsNotNone(self.sessionLayer)
        self.assertEqual(filterUsdStr(self.sessionLayer.ExportToString()),
                         '')
        self.assertTrue(self.sessionLayer.empty)
 
        # Check that visibility was written to the root layer
        rootLayer = self.stage.GetRootLayer()
        self.assertEqual(filterUsdStr(rootLayer.ExportToString()),
                         filterUsdStr('def Xform "A"\n{\n}\ndef Xform "B"\n{\n    token visibility = "invisible"\n}'))

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'UFE composite commands only available in Python from UFE v4.')
    def testNotRoutingCompositeCmd(self):
        '''
        Test that a composite command with not edit router registered works as expected.
        '''
        self._prepareSimpleScene()

        # Check to root layer only contains bare A and B xforms.
        rootLayer = self.stage.GetRootLayer()
        self.assertEqual(filterUsdStr(rootLayer.ExportToString()),
                         filterUsdStr('def Xform "A"\n{\n}\ndef Xform "B"\n{\n}'))
 
        # try to affect B via the command, should be prevented
        compCmd = CustomCompositeCmd(self.prim, self.b)
        compCmd.execute()
 
        # Check that nothing was written to the session layer
        self.assertIsNotNone(self.sessionLayer)
        self.assertEqual(filterUsdStr(self.sessionLayer.ExportToString()),
                         '')
        self.assertTrue(self.sessionLayer.empty)
 
        # Check that visibility was written to the root layer
        rootLayer = self.stage.GetRootLayer()
        self.assertEqual(filterUsdStr(rootLayer.ExportToString()),
                         filterUsdStr('def Xform "A"\n{\n}\ndef Xform "B"\n{\n    token visibility = "invisible"\n}'))

    @unittest.skipIf(os.getenv('UFE_HAS_CODE_WRAPPER', '0') < '1', 'Test requires code wrapper handler only available in UFE 0.5.5 and later')
    def testRoutingCompositeGroupCmd(self):
        '''
        Test that an edit router for the group composite command routes all sub-commands.
        '''
        self._prepareSimpleScene()

        # Check to root layer only contains bare A and B xforms.
        rootLayer = self.stage.GetRootLayer()
        self.assertEqual(filterUsdStr(rootLayer.ExportToString()),
                         filterUsdStr('def Xform "A"\n{\n}\ndef Xform "B"\n{\n}'))
 
        # Route the group command to the session layer.
        mayaUsd.lib.registerEditRouter('group', routeCmdToSessionLayer)
 
        # Group
        cmds.group()
 
        # Check that everything was written to the session layer
        self.assertIsNotNone(self.sessionLayer)
        expectedContents = '''
            def Xform "group1" (
                kind = "group"
            )
            {
                float3 xformOp:translate:rotatePivot = (0, 0, 0)
                float3 xformOp:translate:rotatePivotTranslate = (0, 0, 0)
                float3 xformOp:translate:scalePivot = (0, 0, 0)
                float3 xformOp:translate:scalePivotTranslate = (0, 0, 0)
                uniform token[] xformOpOrder = ["xformOp:translate:rotatePivotTranslate", "xformOp:translate:rotatePivot", "!invert!xformOp:translate:rotatePivot", "xformOp:translate:scalePivotTranslate", "xformOp:translate:scalePivot", "!invert!xformOp:translate:scalePivot"]
                def Xform "B"
                {
                    float3 xformOp:rotateXYZ = (0, -0, 0)
                    float3 xformOp:scale = (1, 1, 1)
                    double3 xformOp:translate = (0, 0, 0)
                    uniform token[] xformOpOrder = ["xformOp:translate", "xformOp:rotateXYZ", "xformOp:scale"]
                }
            }
            '''
        self.maxDiff = None
        self.assertEqual(filterUsdStr(self.sessionLayer.ExportToString()), filterUsdStr(expectedContents))
 

if __name__ == '__main__':
    unittest.main(verbosity=2)
