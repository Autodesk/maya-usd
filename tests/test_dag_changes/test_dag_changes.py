import maya.cmds as cmds

import unittest

from hdmaya_test_utils import HdMayaTestCase

class TestDagChanges(HdMayaTestCase):
    def setUp(self):
        self.makeCubeScene()

        self.grp1 = cmds.createNode('transform', name='group1')
        self.grp1Rprim = self.rprimPath(self.grp1)

        self.grp2 = cmds.createNode('transform', name='group2')
        self.grp2Rprim = self.rprimPath(self.grp2)

    def test_reparent_transform(self):
        cmds.parent(self.cubeTrans, self.grp1)
        grp1ShapeRprim = self.rprimPath(self.cubeShapeName)
        self.assertEqual(
            grp1ShapeRprim,
            self.rprimPath("|{self.grp1}|{self.cubeTrans}|{self.cubeShapeName}"
                           .format(self=self)))
        cmds.refresh()
        index = self.getIndex()
        self.assertIn(grp1ShapeRprim, index)
        self.assertNotIn(self.cubeRprim, index)

        cmds.parent(self.grp1, self.grp2)
        grp2ShapeRprim = self.rprimPath(self.cubeShapeName)
        self.assertEqual(
            grp2ShapeRprim,
            self.rprimPath("|{self.grp2}|{self.grp1}|{self.cubeTrans}|{self.cubeShapeName}"
                           .format(self=self)))
        cmds.refresh()
        index = self.getIndex()
        self.assertIn(grp2ShapeRprim, index)
        self.assertNotIn(grp1ShapeRprim, index)
        self.assertNotIn(self.cubeRprim, index)

        cmds.parent(self.cubeTrans, world=True)
        origShapePrim = self.rprimPath(self.cubeShapeName)
        self.assertEqual(origShapePrim, self.cubeRprim)
        cmds.refresh()
        index = self.getIndex()
        self.assertIn(self.cubeRprim, index)
        self.assertNotIn(grp2ShapeRprim, index)
        self.assertNotIn(grp1ShapeRprim, index)

    def test_reparent_shape(self):
        cmds.parent(self.cubeShapeName, self.grp1, shape=1, r=1)
        grp1ShapeRprim = self.rprimPath(self.cubeShapeName)
        self.assertEqual(
            grp1ShapeRprim,
            self.rprimPath("|{self.grp1}|{self.cubeShapeName}"
                           .format(self=self)))
        cmds.refresh()
        index = self.getIndex()
        self.assertIn(grp1ShapeRprim, index)
        self.assertNotIn(self.cubeRprim, index)

        cmds.parent(self.cubeShapeName, self.cubeTrans, shape=1, r=1)
        origShapePrim = self.rprimPath(self.cubeShapeName)
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
            cmds.setAttr('persp.rotate', -30, 45, 0, type='float3')
            cmds.setAttr('persp.translate', 24, 18, 24, type='float3')

            cmds.setAttr('{}.tz'.format(self.grp1), 5)
            cmds.setAttr('{}.tx'.format(self.grp2), 8)
            cmds.setAttr('{}.ty'.format(pCube2), 5)

            # No instances to start
            #   (1) |pCube1|pCubeShape1
            self.assertSnapshotClose("instances_1.png")

            # Add |group1|pCube1 instance
            #   (1) |pCube1|pCubeShape1
            #   (2) |group1|pCube1|pCubeShape1
            cmds.parent(self.cubeTrans, self.grp1, add=1, r=1)
            cmds.select(clear=1)
            self.assertSnapshotClose("instances_12.png")

            # Add |pCube2|pCubeShape1 instance
            #   (1) |pCube1|pCubeShape1
            #   (2) |group1|pCube1|pCubeShape1
            #   (3) |pCube2|pCubeShape1
            cmds.parent(self.cubeShape, pCube2, add=1, r=1, shape=1)
            cmds.select(clear=1)
            self.assertSnapshotClose("instances_123.png")

            # Add |group2|group1|pCube1 instance
            #   (1) |pCube1|pCubeShape1
            #   (2) |group1|pCube1|pCubeShape1
            #   (3) |pCube2|pCubeShape1
            #   (4) |group2||group1|pCube1|pCubeShape1
            cmds.parent(self.grp1, self.grp2, add=1, r=1)
            cmds.select(clear=1)
            self.assertSnapshotClose("instances_1234.png")

            # Add |group1|pCube2 instance
            #   (1) |pCube1|pCubeShape1
            #   (3) |pCube2|pCubeShape1
            #   (2) |group1|pCube1|pCubeShape1
            #   (4) |group2||group1|pCube1|pCubeShape1
            #   (5) |group1|pCube2|pCubeShape1
            #   (6) |group2||group1|pCube2|pCubeShape1
            cmds.parent(pCube2, self.grp1, add=1, r=1)
            cmds.select(clear=1)
            self.assertSnapshotClose("instances_123456.png")

            cmds.undoInfo(state=1)
            cmds.undoInfo(openChunk=1)
            try:
                # Delete group2
                cmds.delete(self.grp2)
                self.assertNotIn(self.cubeRprim, self.getIndex())
                self.assertSnapshotClose("instances_0.png")
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
            self.assertSnapshotClose("instances_123456.png")

            # Remove |group2|group1 instance
            #   (1) |pCube1|pCubeShape1
            #   (2) |group1|pCube1|pCubeShape1
            #   (3) |pCube2|pCubeShape1
            #   (5) |group1|pCube2|pCubeShape1
            cmds.parent('|{self.grp2}|{self.grp1}'.format(self=self),
                        removeObject=1)
            self.assertSnapshotClose("instances_1235.png")

            cmds.undoInfo(openChunk=1)
            try:
                # Remove pCube2|pCubeShape1 instance
                #   (1) |pCube1|pCubeShape1
                #   (2) |group1|pCube1|pCubeShape1
                cmds.parent('|{pCube2}|{self.cubeShapeName}'.format(self=self,
                                                                    pCube2=pCube2),
                            removeObject=1, shape=1)
                self.assertSnapshotClose("instances_12.png")
            finally:
                cmds.undoInfo(closeChunk=1)

            # Undo remove pCube2|pCubeShape1 instance
            #   (1) |pCube1|pCubeShape1
            #   (2) |group1|pCube1|pCubeShape1
            #   (3) |pCube2|pCubeShape1
            #   (5) |group1|pCube2|pCubeShape1
            cmds.undo()

            # the playblast command is entered into the undo queue, so we
            # need to disable without flusing the queue, so we can test redo
            cmds.undoInfo(stateWithoutFlush=0)
            try:
                self.assertSnapshotClose("instances_1235.png")
            finally:
                cmds.undoInfo(stateWithoutFlush=1)

            # Redo remove pCube2|pCubeShape1 instance
            #   (1) |pCube1|pCubeShape1
            #   (2) |group1|pCube1|pCubeShape1
            cmds.redo()
            self.assertSnapshotClose("instances_12.png")

            # Remove |group1|pCube1 instance
            #   (1) |pCube1|pCubeShape1
            cmds.parent('{self.grp1}|{self.cubeTrans}'.format(self=self),
                        removeObject=1)
            self.assertSnapshotClose("instances_1.png")
        finally:
            cmds.undoInfo(state=undoWasEnabled)


if __name__ == "__main__":
    unittest.main(argv=[""])

