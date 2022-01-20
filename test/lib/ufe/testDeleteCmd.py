#!/usr/bin/env python

#
# Copyright 2021 Autodesk
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
import ufeUtils
import usdUtils

import mayaUsd.ufe

from maya import cmds
from maya import standalone

import ufe

import os
import unittest


def childrenNames(children):
    return [str(child.path().back()) for child in children]

class TestObserver(ufe.Observer):
    def __init__(self):
        super(TestObserver, self).__init__()
        self.deleteNotif = 0
        self.addNotif = 0

    def __call__(self, notification):
        if isinstance(notification, ufe.ObjectDelete):
            self.deleteNotif += 1
        if isinstance(notification, ufe.ObjectAdd):
            self.addNotif += 1

    def nbDeleteNotif(self):
        return self.deleteNotif

    def nbAddNotif(self):
        return self.addNotif

    def reset(self):
        self.addNotif = 0
        self.deleteNotif = 0

class DeleteCmdTestCase(unittest.TestCase):
    '''Verify the Maya delete command, for multiple runtimes.

    UFE Feature : SceneItemOps
    Maya Feature : delete
    Action : Remove object from scene.
    Applied On Selection / Command Arguments:
        - Multiple Selection [Mixed, Non-Maya].  Maya-only selection tested by
          Maya.
    Undo/Redo Test : Yes
    Expect Results To Test :
        - Delete removes object from scene.
    Edge Cases :
        - None.
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
        ''' Called initially to set up the Maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)
        
        cmds.file(new=True, force=True)

    def testDeleteLeafPrims(self):
        '''Test successful delete of leaf prims'''
        
        # open tree.ma scene in testSamples
        mayaUtils.openTreeScene()

        # Create some extra Maya nodes
        cmds.polySphere()

        # clear selection to start off
        cmds.select(clear=True)

        # create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # validate the default edit target to be the Rootlayer.
        mayaPathSegment = mayaUtils.createUfePathSegment('|Tree_usd|Tree_usdShape')
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))
        self.assertTrue(stage)
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())
        
        # delete two USD prims and Maya's shape
        ufeObs.reset()
        cmds.delete('|Tree_usd|Tree_usdShape,/TreeBase/leavesXform/leaves', '|Tree_usd|Tree_usdShape,/TreeBase/trunk', '|pSphere1|pSphereShape1')
        self.assertEqual(ufeObs.nbDeleteNotif() , 3)
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/trunk'))
        
        # validate undo
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif() , 3)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
        
        # validate redo
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif() , 3)
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/trunk'))
        
    def testDeleteHierarchy(self):
        '''Test successful delete of a prim with children'''
        
        # open tree.ma scene in testSamples
        mayaUtils.openTreeScene()

        # create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # validate the default edit target to be the Rootlayer.
        mayaPathSegment = mayaUtils.createUfePathSegment('|Tree_usd|Tree_usdShape')
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))
        self.assertTrue(stage)
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())
        
        # delete two USD prims and Maya's shape
        ufeObs.reset()
        cmds.delete('|Tree_usd|Tree_usdShape,/TreeBase')
        self.assertEqual(ufeObs.nbDeleteNotif() , 1)
        self.assertFalse(stage.GetPrimAtPath('/TreeBase'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/trunk'))
        
        # validate undo
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif() , 1)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
        
        # validate redo
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif() , 1)
        self.assertFalse(stage.GetPrimAtPath('/TreeBase'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/trunk'))
 
    def testDeleteHierarchyMultiSelect(self):
        '''Test successful delete of a multiple prims, some being children of others'''
        
        # open tree.ma scene in testSamples
        mayaUtils.openTreeScene()

        # create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # validate the default edit target to be the Rootlayer.
        mayaPathSegment = mayaUtils.createUfePathSegment('|Tree_usd|Tree_usdShape')
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))
        self.assertTrue(stage)
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())
        
        # delete two USD prims and Maya's shape
        ufeObs.reset()
        cmds.delete('|Tree_usd|Tree_usdShape,/TreeBase/leavesXform/leaves', '|Tree_usd|Tree_usdShape,/TreeBase', '|Tree_usd|Tree_usdShape,/TreeBase/trunk')
        self.assertEqual(ufeObs.nbDeleteNotif() , 2)
        self.assertFalse(stage.GetPrimAtPath('/TreeBase'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/trunk'))
        
        # validate undo
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif() , 2)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
        
        # validate redo
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif() , 2)
        self.assertFalse(stage.GetPrimAtPath('/TreeBase'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertFalse(stage.GetPrimAtPath('/TreeBase/trunk'))
 
    def testDeleteRestrictionDifferentLayer(self):
        '''Test delete restriction - we don't allow removal of a prim when edit target is different than layer introducing the prim'''
        
        # open tree.ma scene in testSamples
        mayaUtils.openTreeScene()

        # create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # validate the default edit target to be the Rootlayer.
        mayaPathSegment = mayaUtils.createUfePathSegment('|Tree_usd|Tree_usdShape')
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))
        self.assertTrue(stage)
        stage.SetEditTarget(stage.GetSessionLayer())
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetSessionLayer())
        
        # delete two USD prims and Maya's shape
        ufeObs.reset()
        cmds.delete('|Tree_usd|Tree_usdShape,/TreeBase/leavesXform/leaves', '|Tree_usd|Tree_usdShape,/TreeBase/trunk')
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
        
        # validate undo
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
        
        # validate redo
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
 
    def testDeleteRestrictionVariantPrim(self):
        '''Test delete restriction - we don't allow removal of a prim defined in a variant'''
        
        # open Variant.ma scene in testSamples
        mayaUtils.openVariantSetScene()
        
        # create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # validate the default edit target to be the Rootlayer.
        mayaPathSegment = mayaUtils.createUfePathSegment('|Variant_usd|Variant_usdShape')
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))
        self.assertTrue(stage)
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())
        
        # delete two USD prims and Maya's shape
        ufeObs.reset()
        cmds.delete('|Variant_usd|Variant_usdShape,/objects/Geom')
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/objects/Geom'))
        
        # validate undo
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/objects/Geom'))
        
        # validate redo
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/objects/Geom'))

    def testDeleteRestrictionReferencedPrim(self):
        '''Test delete restriction - we don't allow removal of a prim defined in a referenced layer'''
        
        # open appleBite.ma scene in testSamples
        mayaUtils.openAppleBiteScene()

        # create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # validate the default edit target to be the Rootlayer.
        mayaPathSegment = mayaUtils.createUfePathSegment('|Asset_flattened_instancing_and_class_removed_usd|Asset_flattened_instancing_and_class_removed_usdShape')
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))
        self.assertTrue(stage)
        self.assertEqual(stage.GetEditTarget().GetLayer(), stage.GetRootLayer())
        
        # delete two USD prims and Maya's shape
        ufeObs.reset()
        cmds.delete('|Asset_flattened_instancing_and_class_removed_usd|Asset_flattened_instancing_and_class_removed_usdShape,/apple/payload/geo')
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/apple/payload/geo'))
        
        # validate undo
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/apple/payload/geo'))
        
        # validate redo
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/apple/payload/geo'))

    def testDeleteRestrictionHierarchyWithChildrenOnDifferentLayer(self):
        '''Test delete restriction - we don't allow removal of a prim with child/children defined on a different layer'''

        # open tree.ma scene in testSamples
        mayaUtils.openTreeScene()

        # create our UFE notification observer
        ufeObs = TestObserver()
        ufe.Scene.addObserver(ufeObs)

        # add child defined on session layer
        mayaPathSegment = mayaUtils.createUfePathSegment('|Tree_usd|Tree_usdShape')
        stage = mayaUsd.ufe.getStage(str(mayaPathSegment))
        self.assertTrue(stage)
        
        stage.SetEditTarget(stage.GetSessionLayer())
        stage.DefinePrim('/TreeBase/newChild', 'Xform')
        stage.SetEditTarget(stage.GetRootLayer())
        
        # delete two USD prims and Maya's shape
        ufeObs.reset()
        cmds.delete('|Tree_usd|Tree_usdShape,/TreeBase')
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/newChild'))
        
        # validate undo
        ufeObs.reset()
        cmds.undo()
        self.assertEqual(ufeObs.nbAddNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/newChild'))
        
        # validate redo
        cmds.redo()
        self.assertEqual(ufeObs.nbDeleteNotif() , 0)
        self.assertTrue(stage.GetPrimAtPath('/TreeBase'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/leavesXform/leaves'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/trunk'))
        self.assertTrue(stage.GetPrimAtPath('/TreeBase/newChild'))

if __name__ == '__main__':
    unittest.main(verbosity=2)
