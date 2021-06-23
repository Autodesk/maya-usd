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
import testUtils
import ufeUtils
import usdUtils

import mayaUsd.ufe

from pxr import UsdGeom, Gf

from maya import cmds
from maya import standalone

import ufe
import os
import unittest

def createStage():
    ''' create a simple stage '''
    cmds.file(new=True, force=True)
    import mayaUsd_createStageWithNewLayer
    proxyShapePathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
    proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
    proxyShapePath = proxyShapes[0]
    proxyShapeItem = ufe.Hierarchy.createItem(ufe.PathString.path(proxyShapePath))
    proxyShapeContextOps = ufe.ContextOps.contextOps(proxyShapeItem)
    stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
    return (stage, proxyShapePathStr, proxyShapeItem, proxyShapeContextOps)

class SphereGenerator():
    ''' simple sphere generator '''
    def __init__(self, num, contextOp, proxyShapePathStr):
        self.gen = self.__generate(num)
        self.num = num
        self.contextOp = contextOp
        self.proxyShapePathStr = proxyShapePathStr

    def createSphere(self):
        return next(self.gen)

    def __addPrimSphere(self, increment):
        self.contextOp.doOp(['Add New Prim', 'Sphere'])
        return ufe.PathString.path('{},/Sphere{}'.format(self.proxyShapePathStr, increment))

    def __generate(self, num):
        increment = 0
        while increment < self.num:
            increment += 1
            yield self.__addPrimSphere(increment)

def createTransform3d(ufeScenePath):
    return ufe.Transform3d.transform3d(ufe.Hierarchy.createItem(ufeScenePath))

class UngroupCmdTestCase(unittest.TestCase):

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
        # load plugins
        self.assertTrue(self.pluginsLoaded)

        # clear selection to start off
        cmds.select(clear=True)

        # global selection
        self.globalSn = ufe.GlobalSelection.get()
        self.globalSn.clear()

        # create a stage
        (self.stage, self.proxyShapePathStr, self.proxyShapeItem, self.contextOp) = createStage();

    def testUngroupUndoRedo(self):
        '''Verify multiple undo/redo.'''

        # sphere generator
        sphereGen = SphereGenerator(2, self.contextOp, self.proxyShapePathStr)

        # create 2 spheres
        sphere1Path = sphereGen.createSphere()
        sphere2Path = sphereGen.createSphere()

        # create a group
        cmds.group(ufe.PathString.string(sphere1Path), 
                   ufe.PathString.string(sphere2Path))

        # verify selected item is "group1"
        self.assertEqual(len(self.globalSn), 1)
        groupItem = self.globalSn.front()
        self.assertEqual(groupItem.nodeName(), "group1")

        # verify that groupItem has 2 children
        groupHierarchy = ufe.Hierarchy.hierarchy(groupItem)
        self.assertEqual(len(groupHierarchy.children()), 2)

        # remove group1 from the hierarchy. What should remain
        # is /Sphere1, /Sphere2.
        cmds.ungroup("{},/group1".format(self.proxyShapePathStr), absolute=True)

        # TODO: after ungroup all children must be selected per Maya native behavior.
        # HS, June 21, 2021 this is not yet implemented
        # self.assertEqual(len(self.globalSn), 2)

        # undo
        cmds.undo();

        # verify the child is group1
        self.assertEqual(self.stage.GetPseudoRoot().GetChildren()[0].GetName(), "group1")

        # verify that pseudoroot has 1 child (group1)
        self.assertEqual(len(self.stage.GetPseudoRoot().GetChildren()), 1)

        # verify group1 is selected
        self.assertEqual(len(self.globalSn), 1)
        self.assertEqual(self.globalSn.front().nodeName(), "group1")

        # redo
        cmds.redo()

        # verify that pseudoroot has 2 children (Sphere1, Sphere2)
        self.assertEqual(len(self.stage.GetPseudoRoot().GetChildren()), 2)

        # undo again
        cmds.undo()

        # redo again
        cmds.redo()

        # verify that pseudoroot has 2 children (Sphere1, Sphere2)
        self.assertEqual(len(self.stage.GetPseudoRoot().GetChildren()), 2)

    def testUngroupMultipleGroupItems(self):
        '''Verify ungrouping of multiple group nodes.'''

        sphereGen = SphereGenerator(2, self.contextOp, self.proxyShapePathStr)

        sphere1Path = sphereGen.createSphere()
        sphere2Path = sphereGen.createSphere()

        # create group1
        cmds.group(ufe.PathString.string(sphere1Path))

        # create group2
        cmds.group(ufe.PathString.string(sphere2Path))

        self.assertEqual([x for x in self.stage.Traverse()],
            [self.stage.GetPrimAtPath("/group1"),
            self.stage.GetPrimAtPath("/group1/Sphere1"), 
            self.stage.GetPrimAtPath("/group2"),
            self.stage.GetPrimAtPath("/group2/Sphere2")])

        # ungroup
        cmds.ungroup("{},/group1".format(self.proxyShapePathStr), 
                     "{},/group2".format(self.proxyShapePathStr))

        self.assertEqual([x for x in self.stage.Traverse()],
            [self.stage.GetPrimAtPath("/Sphere1"), 
            self.stage.GetPrimAtPath("/Sphere2")])

    def testUngroupAbsolute(self):
        '''Verify -absolute flag.'''

        # create a sphere generator
        sphereGen = SphereGenerator(2, self.contextOp, self.proxyShapePathStr)

        # create a sphere and move it on x axis by +2
        sphere1Path = sphereGen.createSphere()
        sphere1T3d = createTransform3d(sphere1Path)
        sphere1T3d.translate(2.0, 0.0, 0.0)

        # create another sphere and move it on x axis by -2
        sphere2Path = sphereGen.createSphere()
        sphere2T3d = createTransform3d(sphere2Path)
        sphere2T3d.translate(-2.0, 0.0, 0.0)

        # create a group
        cmds.group(ufe.PathString.string(sphere1Path), 
                   ufe.PathString.string(sphere2Path))

        # move the group 
        cmds.move(7.0, 8.0, 12.0, r=True)

        # verify selected item is "group1"
        self.assertEqual(len(self.globalSn), 1)
        groupItem = self.globalSn.front()
        self.assertEqual(groupItem.nodeName(), "group1")

        # remove group1 from the hierarchy. What should remain
        # is /Sphere1, /Sphere2.
        cmds.ungroup("{},/group1".format(self.proxyShapePathStr))

        # the objects don't move since the `absolute` flag is implied by default.
        spherePrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(sphere1Path))
        translateAttr = spherePrim.GetAttribute('xformOp:translate')
        self.assertEqual(translateAttr.Get(), Gf.Vec3d(9.0, 8.0, 12.0))

        spherePrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(sphere2Path))
        translateAttr = spherePrim.GetAttribute('xformOp:translate')
        self.assertEqual(translateAttr.Get(), Gf.Vec3d(5.0, 8.0, 12.0))

    def testUngroupRelative(self):
        '''Verify -relative flag.'''
        # create a sphere generator
        sphereGen = SphereGenerator(2, self.contextOp, self.proxyShapePathStr)

        # create a sphere and move it on x axis by +2
        sphere1Path = sphereGen.createSphere()
        sphere1T3d = createTransform3d(sphere1Path)
        sphere1T3d.translate(2.0, 0.0, 0.0)

        # create another sphere and move it on x axis by -2
        sphere2Path = sphereGen.createSphere()
        sphere2T3d = createTransform3d(sphere2Path)
        sphere2T3d.translate(-2.0, 0.0, 0.0)

        # create a group
        cmds.group(ufe.PathString.string(sphere1Path), 
                   ufe.PathString.string(sphere2Path))

        # move the group 
        cmds.move(20.0, 8.0, 12.0, r=True)

        # verify selected item is "group1"
        self.assertEqual(len(self.globalSn), 1)
        groupItem = self.globalSn.front()
        self.assertEqual(groupItem.nodeName(), "group1")

        # remove group1 from the hierarchy. What should remain
        # is /Sphere1, /Sphere2.
        cmds.ungroup("{},/group1".format(self.proxyShapePathStr), relative=True)

        # the objects move to their relative positions
        spherePrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(sphere1Path))
        translateAttr = spherePrim.GetAttribute('xformOp:translate')
        self.assertEqual(translateAttr.Get(), Gf.Vec3d(2.0, 0.0, 0.0))

        spherePrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(sphere2Path))
        translateAttr = spherePrim.GetAttribute('xformOp:translate')
        self.assertEqual(translateAttr.Get(), Gf.Vec3d(-2.0, 0.0, 0.0))

    def testUngroupWorld(self):
        '''Verify -world flag.'''
        
        # create a sphere generator
        sphereGen = SphereGenerator(4, self.contextOp, self.proxyShapePathStr)

        # create 4 spheres
        pathList = list()
        for _ in range(4):
            pathList.append(sphereGen.createSphere())

        # group /Sphere2, /Sphere3, /Sphere4
        cmds.group(ufe.PathString.string(pathList[1]),
                   ufe.PathString.string(pathList[2]),
                   ufe.PathString.string(pathList[3]))

        # verify the paths after grouping
        self.assertEqual([x for x in self.stage.Traverse()], 
            [self.stage.GetPrimAtPath("/Sphere1"),
            self.stage.GetPrimAtPath("/group1"),
            self.stage.GetPrimAtPath("/group1/Sphere2"),
            self.stage.GetPrimAtPath("/group1/Sphere3"),
            self.stage.GetPrimAtPath("/group1/Sphere4")])

        # group /group1/Sphere2, /group1/Sphere3, /group1/Sphere4
        cmds.group("{},/group1/Sphere2".format(self.proxyShapePathStr),
                   "{},/group1/Sphere3".format(self.proxyShapePathStr),
                   "{},/group1/Sphere4".format(self.proxyShapePathStr))

        # verify the paths after second grouping
        self.assertEqual([x for x in self.stage.Traverse()],
            [self.stage.GetPrimAtPath("/Sphere1"),
            self.stage.GetPrimAtPath("/group1"),
            self.stage.GetPrimAtPath("/group1/group1"),
            self.stage.GetPrimAtPath("/group1/group1/Sphere2"),
            self.stage.GetPrimAtPath("/group1/group1/Sphere3"),
            self.stage.GetPrimAtPath("/group1/group1/Sphere4")])

        # remove /group1/group1 from the hierarchy with the -world flag
        cmds.ungroup("{},/group1/group1".format(self.proxyShapePathStr), world=True)

        # verify the paths after ungroup
        self.assertEqual([x for x in self.stage.Traverse()],
            [self.stage.GetPrimAtPath("/Sphere1"),
            self.stage.GetPrimAtPath("/group1"),
            self.stage.GetPrimAtPath("/Sphere2"),
            self.stage.GetPrimAtPath("/Sphere3"),
            self.stage.GetPrimAtPath("/Sphere4")])

    @unittest.skip("parent flag is not supported yet")
    def testUngroupParent(self):
        '''Verify -parent flag.'''
        pass

    def testUngroupProxyShape(self):
        '''Verify ungrouping of the proxyShape results in error.'''

        # create a sphere generator
        sphereGen = SphereGenerator(5, self.contextOp, self.proxyShapePathStr)

        # create 5 spheres 
        for _ in range(5):
            sphereGen.createSphere()

        # ungrouping the proxy shape with existing USD children should fail
        with self.assertRaises(RuntimeError):
            cmds.ungroup(ufe.PathString.string(self.proxyShapeItem.path()))

    def testUngroupLeaf(self):
        '''Verify ungrouping of a leaf node.'''
        # create a sphere generator
        sphereGen = SphereGenerator(1, self.contextOp, self.proxyShapePathStr)

        # create a sphere
        sphere1Path = sphereGen.createSphere()

        # expect the exception happens
        with self.assertRaises(RuntimeError):
            # ungroup
            cmds.ungroup(ufe.PathString.string(sphere1Path))

    def testUngroupAfterUndoRedo(self):
        ''' '''
        # create a sphere generator
        sphereGen = SphereGenerator(2, self.contextOp, self.proxyShapePathStr)

        # create 2 spheres
        sphere1Path = sphereGen.createSphere()
        sphere2Path = sphereGen.createSphere()

        # create a group
        cmds.group(ufe.PathString.string(sphere1Path), 
                   ufe.PathString.string(sphere2Path))

        # verify selected item is "group1"
        self.assertEqual(len(self.globalSn), 1)
        groupItem = self.globalSn.front()
        self.assertEqual(groupItem.nodeName(), "group1")

        # remove group1 from the hierarchy. What should remain
        # is /Sphere1, /Sphere2.
        cmds.ungroup("{},/group1".format(self.proxyShapePathStr))

        # undo again
        cmds.undo()

        # redo again
        cmds.redo()

        # verify that group1 is in global selection list
        self.assertEqual(len(self.globalSn), 1)
        groupItem = self.globalSn.front()
        self.assertEqual(groupItem.nodeName(), "group1")

        # Hmmm, looks like we have a bug here. HS, June 21, 2021
        # verify that group hierarchy has 2 children
        # groupHierarchy = ufe.Hierarchy.hierarchy(groupItem)
        # self.assertEqual(len(groupHierarchy.children()), 2)

        # remove group1 from the hierarchy. What should remain
        # is /Sphere1, /Sphere2.
        # cmds.ungroup("|stage1|stageShape1,/group1")

if __name__ == '__main__':
    unittest.main(verbosity=2)
