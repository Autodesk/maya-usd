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

import mayaUsd
import fixturesUtils
import mayaUtils
import usdUtils
import testUtils

from maya import cmds
from maya import standalone

import ufe

from pxr import Usd, Sdf, Vt, Gf

import unittest


class ComponentVariantTestCase(unittest.TestCase):
    '''
    Verify switching component variants on a USD scene.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

        # Ensure the idle queue is running so that MGlobal::executeTaskOnIdle
        # callbacks fire when cmds.flushIdleQueue() is called (needed on Linux).
        cmds.flushIdleQueue(resume=True)

        # Ensure the queue is clear before starting the test so that undo stack is predictable,
        # some idle jobs can append commands on the stack.
        cmds.flushIdleQueue()

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()
    
    def setUp(self):
        # Load plugins
        cmds.file(new=True, force=True)
        self.assertTrue(self.pluginsLoaded)

    def verifyChildren(self, item, expectedChildrenPaths):
        '''
        Verify that the list of children UFE paths really are the children
        of the given UFE item.
        '''
        itemHierarchy = ufe.Hierarchy.hierarchy(item)
        itemChildren = itemHierarchy.children()
        self.assertEqual([ufe.PathString.string(child.path()) for child in itemChildren], expectedChildrenPaths)

    def verifyColor(self, stage, attributePath, expectedColor):
        '''
        Verify that the color attribute of the given UFE item matches the expected color.
        '''
        property = stage.GetPropertyAtPath(attributePath)
        self.assertIsNotNone(property)
        self.assertTrue(property)
        colorValue = Gf.Vec3f(expectedColor)
        self.assertEqual(property.Get()[0], colorValue)

    def testSwitchVariantFromComponent(self):
        '''
        Switch variants declared in a component sub-prim.
        '''
        import maya.internal.ufeSupport.ufeCmdWrapper as ufeCmd

        # Open the component stage.
        shotFileName = testUtils.getTestScene("variantInComponent", "ref_variants", "ref_variants.usda")
        proxyShapePathStr, stage = mayaUtils.createProxyFromFile(shotFileName)

        import AdskUsdComponentCreator
        desc = AdskUsdComponentCreator.ComponentDescription.CreateFromStageMetadata(stage)
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['variant_set_1']
        variantDesc = vsDesc.GetVariantDescription('Xform1')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # set edit target on session so that EF works.
        sessionLayer = stage.GetSessionLayer()
        stage.SetEditTarget(sessionLayer)

        # Paths and item that will be used in the test.
        usdVaryingPathStr  = '/root/geo/Xform1'
        usdCubePathStr = '%s/Cube' % usdVaryingPathStr
        usdSpherePathStr = '%s/Sphere' % usdVaryingPathStr
        ufeVaryingPathStr  = '%s,%s' % (proxyShapePathStr, usdVaryingPathStr)
        ufeCubePathStr = '%s,%s' % (proxyShapePathStr, usdCubePathStr)
        ufeSpherePathStr = '%s,%s' % (proxyShapePathStr, usdSpherePathStr)
        colorAttributeName = 'primvars:displayColor'
        
        varyingItem  = ufe.Hierarchy.createItem(ufe.PathString.path(ufeVaryingPathStr))

        # The sphere is the sole child of Xform1.
        self.verifyChildren(varyingItem, [ufeSpherePathStr])
        self.verifyColor(stage, usdVaryingPathStr + '.' + colorAttributeName, (0.8, 0.1, 0.1))

        # Switch variants using a command: the cube is now the sole child of
        # Xform1.
        xformCtxOps = ufe.ContextOps.contextOps(varyingItem)
        cmd = xformCtxOps.doOpCmd(['Variant Sets', 'modelingVariant', 'B'])
        ufeCmd.execute(cmd)

        # Continuous forwarding happens on the next idle.
        cmds.flushIdleQueue()

        self.verifyChildren(varyingItem, [ufeCubePathStr])
        self.verifyColor(stage, usdVaryingPathStr + '.' + colorAttributeName, (0.8, 0.1, 0.1))

        # Undo: sphere is back.
        cmds.undo()

        self.verifyChildren(varyingItem, [ufeSpherePathStr])
        self.verifyColor(stage, usdVaryingPathStr + '.' + colorAttributeName, (0.8, 0.1, 0.1))

        # Redo: cube is back.
        cmds.redo()

        self.verifyChildren(varyingItem, [ufeCubePathStr])
        self.verifyColor(stage, usdVaryingPathStr + '.' + colorAttributeName, (0.8, 0.1, 0.1))

        xformCtxOps = ufe.ContextOps.contextOps(varyingItem)
        cmd = xformCtxOps.doOpCmd(['Variant Sets', 'shadingVariant', 'Green'])
        ufeCmd.execute(cmd)

        # Continuous forwarding happens on the next idle.
        cmds.flushIdleQueue()
        
        # TODO FIXME BUG: currently, the EF does not correctly forward the variant switch
        #                 when it happens on a component sub-prim. Currently, all existing
        #                 variant selections are overwritten by the last one. The correct
        #                 behavior should be that the two variant selections are independent
        #                 and can be switched separately, and that both of them are correctly
        #                 forwarded. Change the test to verify the correct behavior once the
        #                 bug is fixed.
        # self.verifyChildren(varyingItem, [ufeCubePathStr])
        self.verifyChildren(varyingItem, [ufeSpherePathStr])
        self.verifyColor(stage, usdVaryingPathStr + '.' + colorAttributeName, (0.1, 0.8, 0.1))

        # Undo: sphere is back.
        cmds.undo()

        self.verifyChildren(varyingItem, [ufeCubePathStr])
        self.verifyColor(stage, usdVaryingPathStr + '.' + colorAttributeName, (0.8, 0.1, 0.1))

        # Redo: cube is back.
        cmds.redo()

        # TODO FIXME BUG: currently, the EF does not correctly forward the variant switch
        #                 when it happens on a component sub-prim. Currently, all existing
        #                 variant selections are overwritten by the last one. The correct
        #                 behavior should be that the two variant selections are independent
        #                 and can be switched separately, and that both of them are correctly
        #                 forwarded. Change the test to verify the correct behavior once the
        #                 bug is fixed.
        self.verifyChildren(varyingItem, [ufeSpherePathStr])
        self.verifyColor(stage, usdVaryingPathStr + '.' + colorAttributeName, (0.1, 0.8, 0.1))


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
