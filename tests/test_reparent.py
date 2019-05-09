import maya.cmds as cmds

import inspect
import os
import sys
import unittest

THIS_FILE = os.path.normpath(os.path.abspath(inspect.getsourcefile(lambda: None)))
THIS_DIR = os.path.dirname(THIS_FILE)

if THIS_DIR not in sys.path:
    sys.path.append(THIS_DIR)

from hdmaya_test_utils import HdMayaTestCase

class TestReparent(HdMayaTestCase):
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


if __name__ == "__main__":
    unittest.main(argv=[""])

