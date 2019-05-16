import maya.cmds as cmds

import unittest

from hdmaya_test_utils import HdMayaTestCase

class TestCommand(HdMayaTestCase):
    def setUp(self):
        self.makeCubeScene(camDist=6)
        self.assertTrue(cmds.getAttr("{}.visibility".format(self.cubeTrans)))
        self.assertTrue(cmds.getAttr("{}.visibility".format(self.cubeShape)))

    def test_toggleTransVis(self):
        # because snapshotting is slow, we only use it in this test - otherwise
        # we assume the results of `listRenderIndex=..., visibileOnly=1` are
        # sufficient

        cmds.refresh()
        self.assertIn(
            self.cubeRprim,
            self.getVisibleIndex())
        self.assertSnapshotClose("cube_unselected.png")

        cmds.setAttr("{}.visibility".format(self.cubeTrans), False)
        self.assertFalse(cmds.getAttr("{}.visibility".format(self.cubeTrans)))
        cmds.refresh()
        self.assertNotIn(
            self.cubeRprim,
            self.getVisibleIndex())
        self.assertSnapshotClose("nothing.png")

        cmds.setAttr("{}.visibility".format(self.cubeTrans), True)
        self.assertTrue(cmds.getAttr("{}.visibility".format(self.cubeTrans)))
        cmds.refresh()
        self.assertIn(
            self.cubeRprim,
            self.getVisibleIndex())
        self.assertSnapshotClose("cube_unselected.png")

    def test_toggleShapeVis(self):
        cmds.setAttr("{}.visibility".format(self.cubeShape), False)
        self.assertFalse(cmds.getAttr("{}.visibility".format(self.cubeShape)))
        cmds.refresh()
        self.assertNotIn(
            self.cubeRprim,
            self.getVisibleIndex())

        cmds.setAttr("{}.visibility".format(self.cubeShape), True)
        self.assertTrue(cmds.getAttr("{}.visibility".format(self.cubeShape)))
        cmds.refresh()
        self.assertIn(
            self.cubeRprim,
            self.getVisibleIndex())

    def test_toggleBothVis(self):
        cmds.setAttr("{}.visibility".format(self.cubeTrans), False)
        self.assertFalse(cmds.getAttr("{}.visibility".format(self.cubeTrans)))
        cmds.setAttr("{}.visibility".format(self.cubeShape), False)
        self.assertFalse(cmds.getAttr("{}.visibility".format(self.cubeShape)))
        cmds.refresh()
        self.assertNotIn(
            self.cubeRprim,
            self.getVisibleIndex())

        cmds.setAttr("{}.visibility".format(self.cubeTrans), True)
        self.assertTrue(cmds.getAttr("{}.visibility".format(self.cubeTrans)))
        cmds.setAttr("{}.visibility".format(self.cubeShape), True)
        self.assertTrue(cmds.getAttr("{}.visibility".format(self.cubeShape)))
        cmds.refresh()
        self.assertIn(
            self.cubeRprim,
            self.getVisibleIndex())

    def doGroupVisibilityTest(self, makeNodeVis, makeNodeInvis):
        lowGroup = cmds.group(self.cubeTrans, name='lowGroup')
        midGroup = cmds.group(lowGroup, name='midGroup')
        highGroup = cmds.group(midGroup, name='highGroup')
        hier = [midGroup, midGroup, lowGroup, self.cubeTrans, self.cubeShape]
        cmds.refresh()

        self.cubeRprim = self.rprimPath(self.cubeShape)
        visIndex = [self.cubeRprim]
        self.assertEqual(self.getVisibleIndex(), visIndex)

        for obj in hier:
            makeNodeInvis(obj)
            cmds.refresh()
            self.assertEqual(self.getVisibleIndex(), [])
            makeNodeVis(obj)
            cmds.refresh()
            self.assertEqual(self.getVisibleIndex(), visIndex)

    def test_groupVisibility(self):
        def makeNodeVis(obj):
            cmds.setAttr("{}.visibility".format(obj), True)

        def makeNodeInvis(obj):
            cmds.setAttr("{}.visibility".format(obj), False)

        self.doGroupVisibilityTest(makeNodeVis, makeNodeInvis)


if __name__ == "__main__":
    unittest.main(argv=[""])

