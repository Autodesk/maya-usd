#!/usr/bin/env mayapy
#
# Copyright 2016 Pixar
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

from maya import cmds
from maya import standalone

import fixturesUtils

import os
import unittest

import usdUtils, mayaUtils, ufeUtils

import ufe
import mayaUsd.ufe

class testProxyShapeBase(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        cls.mayaSceneFilePath = os.path.join(inputPath, 'ProxyShapeBaseTest',
            'ProxyShapeBase.ma')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testBoundingBox(self):
        cmds.file(self.mayaSceneFilePath, open=True, force=True)

        # Verify that the proxy shape read something from the USD file.
        bboxSize = cmds.getAttr('Cube_usd.boundingBoxSize')[0]
        self.assertEqual(bboxSize, (1.0, 1.0, 1.0))

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testDuplicateProxyStageAnonymous only available in UFE v2 or greater.')
    def testDuplicateProxyStageAnonymous(self):
        '''
        Verify stage with new anonymous layer is duplicated properly.
        '''
        cmds.file(new=True, force=True)

        # create a proxy shape and add a Capsule prim
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path(proxyShape)
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)
        proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
        proxyShapeContextOps.doOp(['Add New Prim', 'Capsule'])

        # proxy shape is expected to have one child
        proxyShapeHier = ufe.Hierarchy.hierarchy(proxyShapeItem)
        self.assertEqual(1, len(proxyShapeHier.children()))

        # child name is expected to be Capsule1
        self.assertEqual(str(proxyShapeHier.children()[0].nodeName()), "Capsule1")

        # validate session name and anonymous tag name 
        stage = mayaUsd.ufe.getStage(str(proxyShapePath))
        self.assertEqual(stage.GetLayerStack()[0], stage.GetSessionLayer())
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())
        self.assertEqual(True, "-session" in stage.GetSessionLayer().identifier)
        self.assertEqual(True, "anonymousLayer1" in stage.GetRootLayer().identifier)

        # add proxyShapeItem to selection list
        ufe.GlobalSelection.get().append(proxyShapeItem)

        # duplicate the proxyShape. 
        cmds.duplicate()

        # get the next item in UFE selection list.
        snIter = iter(ufe.GlobalSelection.get())
        duplStageItem = next(snIter)

        # duplicated stage name is expected to be incremented correctly
        self.assertEqual(str(duplStageItem.nodeName()), "stage2")

        # duplicated proxyShape name is expected to be incremented correctly
        duplProxyShapeHier = ufe.Hierarchy.hierarchy(duplStageItem)
        duplProxyShapeItem = duplProxyShapeHier.children()[0]
        self.assertEqual(str(duplProxyShapeItem.nodeName()), "stageShape2")

        # duplicated ProxyShapeItem should have exactly one child
        self.assertEqual(1, len(ufe.Hierarchy.hierarchy(duplProxyShapeItem).children()))

        # child name is expected to be Capsule1
        childName = str(ufe.Hierarchy.hierarchy(duplProxyShapeItem).children()[0].nodeName())
        self.assertEqual(childName, "Capsule1")

        # validate session name and anonymous tag name 
        duplStage = mayaUsd.ufe.getStage(str(ufe.PathString.path('|stage2|stageShape2')))
        self.assertEqual(duplStage.GetLayerStack()[0], duplStage.GetSessionLayer())
        self.assertEqual(duplStage.GetEditTarget().GetLayer(), duplStage.GetRootLayer())
        self.assertEqual(True, "-session" in duplStage.GetSessionLayer().identifier)
        self.assertEqual(True, "anonymousLayer1" in duplStage.GetRootLayer().identifier)

        # confirm that edits are not shared (a.k.a divorced). 
        cmds.delete('|stage2|stageShape2,/Capsule1')
        self.assertEqual(0, len(ufe.Hierarchy.hierarchy(duplProxyShapeItem).children()))
        self.assertEqual(1, len(ufe.Hierarchy.hierarchy(proxyShapeItem).children()))

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testDuplicateProxyStageFileBacked only available in UFE v2 or greater.')
    def testDuplicateProxyStageFileBacked(self):
        '''
        Verify stage from file is duplicated properly.
        '''
        # open tree.ma scene
        mayaUtils.openTreeScene()

        # clear selection to start off
        cmds.select(clear=True)

        # select a USD object.
        mayaPathSegment = mayaUtils.createUfePathSegment('|Tree_usd|Tree_usdShape')
        usdPathSegment = usdUtils.createUfePathSegment('/TreeBase')
        treebasePath = ufe.Path([mayaPathSegment])
        treebaseItem = ufe.Hierarchy.createItem(treebasePath)

        # TreeBase has two children
        self.assertEqual(1, len(ufe.Hierarchy.hierarchy(treebaseItem).children()))

        # get the USD stage
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))

        # by default edit target is set to the Rootlayer.
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())

        # add treebaseItem to selection list
        ufe.GlobalSelection.get().append(treebaseItem)

        # duplicate 
        cmds.duplicate()

        # get the next item in UFE selection list.
        snIter = iter(ufe.GlobalSelection.get())
        duplStageItem = next(snIter)

        # duplicated stage name is expected to be incremented correctly
        self.assertEqual(str(duplStageItem.nodeName()), "Tree_usd1")

        # duplicated proxyShape name is expected to be incremented correctly
        duplProxyShapeHier = ufe.Hierarchy.hierarchy(duplStageItem)
        duplProxyShapeItem = duplProxyShapeHier.children()[0]
        self.assertEqual(str(duplProxyShapeItem.nodeName()), "Tree_usd1Shape")

        # duplicated ProxyShapeItem should have exactly one child
        self.assertEqual(1, len(ufe.Hierarchy.hierarchy(duplProxyShapeItem).children()))

        # child name is expected to be Capsule1
        childName = str(ufe.Hierarchy.hierarchy(duplProxyShapeItem).children()[0].nodeName())
        self.assertEqual(childName, "TreeBase")

        # validate session name and anonymous tag name 
        duplStage = mayaUsd.ufe.getStage(str(ufe.PathString.path('|Tree_usd1|Tree_usd1Shape')))
        self.assertEqual(duplStage.GetLayerStack()[0], duplStage.GetSessionLayer())
        self.assertEqual(duplStage.GetEditTarget().GetLayer(), duplStage.GetRootLayer())
        self.assertEqual(True, "-session" in duplStage.GetSessionLayer().identifier)
        self.assertEqual(True, "tree.usd" in duplStage.GetRootLayer().identifier)

        # delete "/TreeBase"
        cmds.delete('|Tree_usd1|Tree_usd1Shape,/TreeBase')

        # confirm that edits are shared. Both source and duplicated proxyShapes have no children now.
        self.assertEqual(0, len(ufe.Hierarchy.hierarchy(duplProxyShapeItem).children()))
        self.assertEqual(0, len(ufe.Hierarchy.hierarchy(treebaseItem).children()))

if __name__ == '__main__':
    unittest.main(verbosity=2)
