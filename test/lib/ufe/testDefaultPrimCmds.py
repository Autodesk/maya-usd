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

import fixturesUtils
import mayaUtils
import testUtils

from pxr import Sdf

from maya import cmds
from maya import standalone

import ufe

import unittest

class DefaultPrimCmdsTestCase(unittest.TestCase):

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
        self.assertTrue(self.pluginsLoaded)

    def testClearDefaultPrim(self):
        '''
        Verify that we can clear the default prim.
        '''
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('defaultPrimInSub', 'root.usda')
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)

        capsulePathStr = '|stage|stageShape,/Capsule1'
        capsulePath = ufe.PathString.path(capsulePathStr)
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)
        self.assertIsNotNone(capsuleItem)
        
        contextOps = ufe.ContextOps.contextOps(capsuleItem)
        contextOps.doOp(['Clear Default Prim'])

        self.assertFalse(stage.GetDefaultPrim())

    def testSetDefaultPrimRestriction(self):
        '''
        Verify that we can set the default prim.
        '''
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('defaultPrimInSub', 'root.usda')
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)

        x1 = stage.DefinePrim('/Xform1', 'Xform')
        self.assertIsNotNone(x1)
        x1PathStr = '|stage|stageShape,/Xform1'
        x1Path = ufe.PathString.path(x1PathStr)
        x1Item = ufe.Hierarchy.createItem(x1Path)
        self.assertIsNotNone(x1Item)

        x1Prim = stage.GetPrimAtPath('/Xform1')
        self.assertIsNotNone(x1Prim)
        self.assertIsTrue(x1Prim)
        
        contextOps = ufe.ContextOps.contextOps(x1Item)
        contextOps.doOp(['Set as Default Prim'])

        self.assertEqual(stage.GetDefaultPrim(), x1Prim)

    def testClearDefaultPrimRestriction(self):
        '''
        Verify that we cannot clear the default prim
        when not targeting the root layer.
        '''
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('defaultPrimInSub', 'root.usda')
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)

        layer = Sdf.Layer.FindRelativeToLayer(stage.GetRootLayer(), stage.GetRootLayer().subLayerPaths[0])
        self.assertIsNotNone(layer)
        stage.SetEditTarget(layer)
        self.assertEqual(stage.GetEditTarget().GetLayer(), layer)

        capsulePathStr = '|stage|stageShape,/Capsule1'
        capsulePath = ufe.PathString.path(capsulePathStr)
        capsuleItem = ufe.Hierarchy.createItem(capsulePath)
        self.assertIsNotNone(capsuleItem)
        
        contextOps = ufe.ContextOps.contextOps(capsuleItem)
        with self.assertRaises(RuntimeError):
            contextOps.doOp(['Clear Default Prim'])

    def testSetDefaultPrimRestriction(self):
        '''
        Verify that we cannot set the default prim
        when not targeting the root layer.
        '''
        cmds.file(new=True, force=True)
        testFile = testUtils.getTestScene('defaultPrimInSub', 'root.usda')
        shapeNode, stage = mayaUtils.createProxyFromFile(testFile)

        layer = Sdf.Layer.FindRelativeToLayer(stage.GetRootLayer(), stage.GetRootLayer().subLayerPaths[0])
        self.assertIsNotNone(layer)
        stage.SetEditTarget(layer)
        self.assertEqual(stage.GetEditTarget().GetLayer(), layer)

        x1 = stage.DefinePrim('/Xform1', 'Xform')
        self.assertIsNotNone(x1)
        x1PathStr = '|stage|stageShape,/Xform1'
        x1Path = ufe.PathString.path(x1PathStr)
        x1Item = ufe.Hierarchy.createItem(x1Path)
        self.assertIsNotNone(x1Item)
        
        contextOps = ufe.ContextOps.contextOps(x1Item)
        with self.assertRaises(RuntimeError):
            contextOps.doOp(['Set as Default Prim'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
