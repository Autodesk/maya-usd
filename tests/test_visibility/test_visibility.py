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
            cmds.mtoh(listRenderIndex="HdStreamRendererPlugin", visibleOnly=1))
        self.assertSnapshotClose("cube_unselected.png")

        cmds.setAttr("{}.visibility".format(self.cubeTrans), False)
        self.assertFalse(cmds.getAttr("{}.visibility".format(self.cubeTrans)))
        cmds.refresh()
        self.assertNotIn(
            self.cubeRprim,
            cmds.mtoh(listRenderIndex="HdStreamRendererPlugin", visibleOnly=1))
        self.assertSnapshotClose("nothing.png")

        cmds.setAttr("{}.visibility".format(self.cubeTrans), True)
        self.assertTrue(cmds.getAttr("{}.visibility".format(self.cubeTrans)))
        cmds.refresh()
        self.assertIn(
            self.cubeRprim,
            cmds.mtoh(listRenderIndex="HdStreamRendererPlugin", visibleOnly=1))
        self.assertSnapshotClose("cube_unselected.png")

    def test_toggleShapeVis(self):
        cmds.setAttr("{}.visibility".format(self.cubeShape), False)
        self.assertFalse(cmds.getAttr("{}.visibility".format(self.cubeShape)))
        cmds.refresh()
        self.assertNotIn(
            self.cubeRprim,
            cmds.mtoh(listRenderIndex="HdStreamRendererPlugin", visibleOnly=1))

        cmds.setAttr("{}.visibility".format(self.cubeShape), True)
        self.assertTrue(cmds.getAttr("{}.visibility".format(self.cubeShape)))
        cmds.refresh()
        self.assertIn(
            self.cubeRprim,
            cmds.mtoh(listRenderIndex="HdStreamRendererPlugin", visibleOnly=1))

    def test_toggleBothVis(self):
        cmds.setAttr("{}.visibility".format(self.cubeTrans), False)
        self.assertFalse(cmds.getAttr("{}.visibility".format(self.cubeTrans)))
        cmds.setAttr("{}.visibility".format(self.cubeShape), False)
        self.assertFalse(cmds.getAttr("{}.visibility".format(self.cubeShape)))
        cmds.refresh()
        self.assertNotIn(
            self.cubeRprim,
            cmds.mtoh(listRenderIndex="HdStreamRendererPlugin", visibleOnly=1))

        cmds.setAttr("{}.visibility".format(self.cubeTrans), True)
        self.assertTrue(cmds.getAttr("{}.visibility".format(self.cubeTrans)))
        cmds.setAttr("{}.visibility".format(self.cubeShape), True)
        self.assertTrue(cmds.getAttr("{}.visibility".format(self.cubeShape)))
        cmds.refresh()
        self.assertIn(
            self.cubeRprim,
            cmds.mtoh(listRenderIndex="HdStreamRendererPlugin", visibleOnly=1))


if __name__ == "__main__":
    unittest.main(argv=[""])

