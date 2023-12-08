#!/usr/bin/env python

#
# Copyright 2023 Autodesk
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

from pxr import Usd, Sdf

import unittest


class VariantTestCase(unittest.TestCase):
    '''
    Verify variants on a USD scene.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()
    
    @classmethod
    def tearDownClass(cls):
        cmds.file(new=True, force=True)

        standalone.uninitialize()

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

    def testSwitchVariantMultiLayers(self):
        '''
        Switch variants in a layer above a layer that already has a variant selection.
        '''
        import mayaUsd_createStageWithNewLayer
        import maya.internal.ufeSupport.ufeCmdWrapper as ufeCmd

        # Create a scene with three layers.
        proxyShapePathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(proxyShapePathStr).GetStage()
        rootLayer = stage.GetRootLayer()
        subLayers = usdUtils.addNewLayersToLayer(rootLayer, 3)

        # Create two variants in the deepest layer.
        with Usd.EditContext(stage, subLayers[-1]):
            xformPrim = stage.DefinePrim('/Xform1', 'Xform')
            variantSet = xformPrim.GetVariantSets().AddVariantSet('modelingVariant')
            variantSet.AddVariant('cube')
            variantSet.AddVariant('sphere')
            variantSet.SetVariantSelection('cube')
            with variantSet.GetVariantEditContext():
                stage.DefinePrim('/Xform1/Cube', 'Cube')
            variantSet.SetVariantSelection('sphere')
            with variantSet.GetVariantEditContext():
                stage.DefinePrim('/Xform1/Sphere', 'Sphere')

        # Paths and item that will be used in the test.
        xformPathStr  = '%s,/Xform1' % proxyShapePathStr
        cubePathStr = '%s/Cube' % xformPathStr
        spherePathStr = '%s/Sphere' % xformPathStr
        
        xformItem  = ufe.Hierarchy.createItem(ufe.PathString.path(xformPathStr))

        # The sphere is the sole child of Xform1.
        self.verifyChildren(xformItem, [spherePathStr])

        # Switch variants using a command: the cube is now the sole child of
        # Xform1.
        xformCtxOps = ufe.ContextOps.contextOps(xformItem)
        cmd = xformCtxOps.doOpCmd(['Variant Sets', 'modelingVariant', 'cube'])
        ufeCmd.execute(cmd)

        self.verifyChildren(xformItem, [cubePathStr])

        # Undo: sphere is back.
        cmds.undo()

        self.verifyChildren(xformItem, [spherePathStr])

        # Redo: cube is back.
        cmds.redo()

        self.verifyChildren(xformItem, [cubePathStr])

    def testSwitchVariantFromReference(self):
        '''
        Switch variants declared in a reference.
        Make sure the edit restrictions work correctly when the target is a sub-layer
        and the top opinion is in the reference and the layers are using relative paths.
        '''
        import mayaUsd_createStageWithNewLayer
        import maya.internal.ufeSupport.ufeCmdWrapper as ufeCmd

        # Create a scene with three layers and variants in a reference.
        shotFileName = testUtils.getTestScene("variantInRef", "shot.usda")
        proxyShapePathStr, stage = mayaUtils.createProxyFromFile(shotFileName)

        # Paths and item that will be used in the test.
        varyingPathStr  = '%s,/assets/varying' % proxyShapePathStr
        cubePathStr = '%s/CubePrim' % varyingPathStr
        spherePathStr = '%s/SpherePrim' % varyingPathStr
        
        rootItem  = ufe.Hierarchy.createItem(ufe.PathString.path(varyingPathStr))

        # The sphere is the sole child of Xform1.
        self.verifyChildren(rootItem, [spherePathStr])

        # Switch variants using a command: the cube is now the sole child of
        # Xform1.
        rootLayer = stage.GetRootLayer()
        subIdentifier = rootLayer.subLayerPaths[0]
        subLayer = Sdf.Layer.FindRelativeToLayer(rootLayer, subIdentifier)
        with Usd.EditContext(stage, subLayer):
            xformCtxOps = ufe.ContextOps.contextOps(rootItem)
            cmd = xformCtxOps.doOpCmd(['Variant Sets', 'geo', 'cube'])
            ufeCmd.execute(cmd)

        self.verifyChildren(rootItem, [cubePathStr])

        # Undo: sphere is back.
        cmds.undo()

        self.verifyChildren(rootItem, [spherePathStr])

        # Redo: cube is back.
        cmds.redo()

        self.verifyChildren(rootItem, [cubePathStr])


if __name__ == '__main__':
    unittest.main(verbosity=2)
