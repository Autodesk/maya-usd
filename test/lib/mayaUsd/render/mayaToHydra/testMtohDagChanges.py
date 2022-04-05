import sys
import unittest

import maya.cmds as cmds
import maya.mel

import fixturesUtils
import mtohUtils

class TestDagChanges(mtohUtils.MtohTestCase):
    _file = __file__

    def setUp(self):
        self.makeCubeScene()

        self.grp1 = cmds.createNode('transform', name='group1')
        self.grp1Rprim = self.rprimPath(self.grp1)

        self.grp2 = cmds.createNode('transform', name='group2')
        self.grp2Rprim = self.rprimPath(self.grp2)

        self.imageVersion = None
        if maya.mel.eval("defaultShaderName") != "standardSurface1":
            self.imageVersion = 'lambertDefaultMaterial'

    def test_reparent_transform(self):
        cmds.parent(self.cubeTrans, self.grp1)
        grp1ShapeRprim = self.rprimPath(self.cubeShape)
        self.assertEqual(
            grp1ShapeRprim,
            self.rprimPath("|{self.grp1}|{self.cubeTrans}|{self.cubeShape}"
                           .format(self=self)))
        cmds.refresh()
        index = self.getIndex()
        self.assertIn(grp1ShapeRprim, index)
        self.assertNotIn(self.cubeRprim, index)

        cmds.parent(self.grp1, self.grp2)
        grp2ShapeRprim = self.rprimPath(self.cubeShape)
        self.assertEqual(
            grp2ShapeRprim,
            self.rprimPath("|{self.grp2}|{self.grp1}|{self.cubeTrans}|{self.cubeShape}"
                           .format(self=self)))
        cmds.refresh()
        index = self.getIndex()
        self.assertIn(grp2ShapeRprim, index)
        self.assertNotIn(grp1ShapeRprim, index)
        self.assertNotIn(self.cubeRprim, index)

        cmds.parent(self.cubeTrans, world=True)
        origShapePrim = self.rprimPath(self.cubeShape)
        self.assertEqual(origShapePrim, self.cubeRprim)
        cmds.refresh()
        index = self.getIndex()
        self.assertIn(self.cubeRprim, index)
        self.assertNotIn(grp2ShapeRprim, index)
        self.assertNotIn(grp1ShapeRprim, index)

    def test_reparent_shape(self):
        cmds.parent(self.cubeShape, self.grp1, shape=1, r=1)
        grp1ShapeRprim = self.rprimPath(self.cubeShape)
        self.assertEqual(
            grp1ShapeRprim,
            self.rprimPath("|{self.grp1}|{self.cubeShape}"
                           .format(self=self)))
        cmds.refresh()
        index = self.getIndex()
        self.assertIn(grp1ShapeRprim, index)
        self.assertNotIn(self.cubeRprim, index)

        cmds.parent(self.cubeShape, self.cubeTrans, shape=1, r=1)
        origShapePrim = self.rprimPath(self.cubeShape)
        self.assertEqual(origShapePrim, self.cubeRprim)
        cmds.refresh()
        index = self.getIndex()
        self.assertIn(self.cubeRprim, index)
        self.assertNotIn(grp1ShapeRprim, index)

    def test_new_shape(self):
        otherCube = cmds.polyCube()[0]
        otherCubeShape = cmds.listRelatives(otherCube, fullPath=1)[0]
        otherRprim = self.rprimPath(otherCubeShape)
        cmds.refresh()
        index = self.getIndex()
        self.assertIn(self.cubeRprim, index)
        self.assertIn(otherRprim, index)

    def test_instances(self):
        undoWasEnabled = cmds.undoInfo(q=1, state=1)

        cmds.undoInfo(state=0)
        try:
            pCube2 = cmds.createNode('transform', name='pCube2')

            cmds.setAttr('{}.tz'.format(self.grp1), 5)
            cmds.setAttr('{}.tx'.format(self.grp2), 8)
            cmds.setAttr('{}.ty'.format(pCube2), 5)

            # No instances to start
            #   (1) |pCube1|pCubeShape1
            self.assertSnapshotClose("instances_1.png", self.imageVersion)

            # Add |group1|pCube1 instance
            #   (1) |pCube1|pCubeShape1
            #   (2) |group1|pCube1|pCubeShape1
            cmds.parent(self.cubeTrans, self.grp1, add=1, r=1)
            cmds.select(clear=1)
            self.assertSnapshotClose("instances_12.png", self.imageVersion)

            # Add |pCube2|pCubeShape1 instance
            #   (1) |pCube1|pCubeShape1
            #   (2) |group1|pCube1|pCubeShape1
            #   (3) |pCube2|pCubeShape1
            cmds.parent(self.cubeShape, pCube2, add=1, r=1, shape=1)
            cmds.select(clear=1)
            self.assertSnapshotClose("instances_123.png", self.imageVersion)

            # Add |group2|group1|pCube1 instance
            #   (1) |pCube1|pCubeShape1
            #   (2) |group1|pCube1|pCubeShape1
            #   (3) |pCube2|pCubeShape1
            #   (4) |group2||group1|pCube1|pCubeShape1
            cmds.parent(self.grp1, self.grp2, add=1, r=1)
            cmds.select(clear=1)
            self.assertSnapshotClose("instances_1234.png", self.imageVersion)

            # Add |group1|pCube2 instance
            #   (1) |pCube1|pCubeShape1
            #   (3) |pCube2|pCubeShape1
            #   (2) |group1|pCube1|pCubeShape1
            #   (4) |group2||group1|pCube1|pCubeShape1
            #   (5) |group1|pCube2|pCubeShape1
            #   (6) |group2||group1|pCube2|pCubeShape1
            cmds.parent(pCube2, self.grp1, add=1, r=1)
            cmds.select(clear=1)
            self.assertSnapshotClose("instances_123456.png", self.imageVersion)

            # Delete group2
            #   [no shapes]
            cmds.undoInfo(state=1)
            cmds.undoInfo(openChunk=1)
            try:
                cmds.delete(self.grp2)
                self.assertNotIn(self.cubeRprim, self.getIndex())
                self.assertSnapshotClose("instances_0.png", self.imageVersion)
            finally:
                cmds.undoInfo(closeChunk=1)

            # Undo group2 deletion
            #   (1) |pCube1|pCubeShape1
            #   (3) |pCube2|pCubeShape1
            #   (2) |group1|pCube1|pCubeShape1
            #   (4) |group2||group1|pCube1|pCubeShape1
            #   (5) |group1|pCube2|pCubeShape1
            #   (6) |group2||group1|pCube2|pCubeShape1
            cmds.undo()
            self.assertSnapshotClose("instances_123456.png", self.imageVersion)

            # Remove |group2|group1 instance
            #   (1) |pCube1|pCubeShape1
            #   (2) |group1|pCube1|pCubeShape1
            #   (3) |pCube2|pCubeShape1
            #   (5) |group1|pCube2|pCubeShape1
            cmds.parent('|{self.grp2}|{self.grp1}'.format(self=self),
                        removeObject=1)
            self.assertSnapshotClose("instances_1235.png", self.imageVersion)

            # Remove pCube2|pCubeShape1 instance
            #   (1) |pCube1|pCubeShape1
            #   (2) |group1|pCube1|pCubeShape1
            cmds.undoInfo(openChunk=1)
            try:
                cmds.parent('|{pCube2}|{self.cubeShape}'.format(self=self,
                                                                    pCube2=pCube2),
                            removeObject=1, shape=1)
                self.assertSnapshotClose("instances_12.png", self.imageVersion)
            finally:
                cmds.undoInfo(closeChunk=1)

            # Undo remove pCube2|pCubeShape1 instance
            #   (1) |pCube1|pCubeShape1
            #   (2) |group1|pCube1|pCubeShape1
            #   (3) |pCube2|pCubeShape1
            #   (5) |group1|pCube2|pCubeShape1
            cmds.undo()
            self.assertSnapshotClose("instances_1235.png", self.imageVersion)

            # Remove pCube1|pCubeShape1 (the "master" instance)
            #   (3) |pCube2|pCubeShape1
            #   (5) |group1|pCube2|pCubeShape1
            cmds.undoInfo(openChunk=1)
            try:
                cmds.parent('|{self.cubeTrans}|{self.cubeShape}'.format(self=self),
                            removeObject=1, shape=1)
                self.assertSnapshotClose("instances_35.png", self.imageVersion)
            finally:
                cmds.undoInfo(closeChunk=1)

            # Undo remove pCube1|pCubeShape1 (the "master" instance)
            #   (1) |pCube1|pCubeShape1
            #   (2) |group1|pCube1|pCubeShape1
            #   (3) |pCube2|pCubeShape1
            #   (5) |group1|pCube2|pCubeShape1
            cmds.undo()

            # the playblast command is entered into the undo queue, so we
            # need to disable without flusing the queue, so we can test redo
            cmds.undoInfo(stateWithoutFlush=0)
            try:
                self.assertSnapshotClose("instances_1235.png", self.imageVersion)
            finally:
                cmds.undoInfo(stateWithoutFlush=1)

            # Redo remove pCube2|pCubeShape1 instance
            #   (3) |pCube2|pCubeShape1
            #   (5) |group1|pCube2|pCubeShape1
            cmds.redo()
            self.assertSnapshotClose("instances_35.png", self.imageVersion)

            # Remove |group1|pCube2 instance
            #   (3) |pCube2|pCubeShape1
            cmds.parent('{self.grp1}|{pCube2}'.format(self=self,
                                                      pCube2=pCube2),
                        removeObject=1)
            self.assertSnapshotClose("instances_3.png", self.imageVersion)
        finally:
            cmds.undoInfo(state=undoWasEnabled)

    def test_move(self):
        self.assertSnapshotClose("instances_1.png", self.imageVersion)
        cmds.setAttr('{}.ty'.format(self.cubeTrans), 5)
        self.assertSnapshotClose("instances_3.png", self.imageVersion)

    def test_instance_move(self):
        # Add |group1|pCube1 instance
        #   (1) |pCube1|pCubeShape1
        #   (2) |group1|pCube1|pCubeShape1
        cmds.parent(self.cubeTrans, self.grp1, add=1, r=1)
        cmds.select(clear=1)

        # because we haven't moved anything, it should initially look like only
        # one cube...
        self.assertSnapshotClose("instances_1.png", self.imageVersion)

        cmds.setAttr('{}.tz'.format(self.grp1), 5)
        # Now that we moved one, it should look like 2 cubes
        self.assertSnapshotClose("instances_12.png", self.imageVersion)


class TestUndo(mtohUtils.MtohTestCase):
    _file = __file__

    def test_node_creation_undo(self):
        undoWasEnabled = cmds.undoInfo(q=1, state=1)

        self.imageVersion = None
        if maya.mel.eval("defaultShaderName") != "standardSurface1":
            self.imageVersion = 'lambertDefaultMaterial'

        cmds.undoInfo(state=0)
        try:
            cmds.file(new=1, f=1)
            self.setBasicCam()

            self.setHdStormRenderer()

            cmds.undoInfo(state=1)
            cmds.undoInfo(openChunk=1)
            try:
                cubeTrans = cmds.polyCube()
                cubeShape = cmds.listRelatives(cubeTrans)[0]
                cubeRprim = self.rprimPath(cubeShape)
                cmds.select(clear=1)
                cmds.refresh()
                self.assertEqual([cubeRprim], self.getIndex())
                self.assertSnapshotClose("instances_1.png", self.imageVersion)
            finally:
                cmds.undoInfo(closeChunk=1)

            cmds.undo()

            # the playblast command is entered into the undo queue, so we
            # need to disable without flusing the queue, so we can test redo
            cmds.undoInfo(stateWithoutFlush=0)
            try:
                cmds.refresh()
                self.assertEqual([], self.getIndex())
                self.assertSnapshotClose("instances_0.png", self.imageVersion)
            finally:
                cmds.undoInfo(stateWithoutFlush=1)

            cmds.redo()

            cmds.undoInfo(stateWithoutFlush=0)
            try:
                cmds.refresh()
                self.assertEqual([cubeRprim], self.getIndex())
                self.assertSnapshotClose("instances_1.png", self.imageVersion)
            finally:
                cmds.undoInfo(stateWithoutFlush=1)

        finally:
            cmds.undoInfo(state=undoWasEnabled)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
