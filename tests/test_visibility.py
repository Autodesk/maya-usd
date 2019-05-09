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

class TestCommand(HdMayaTestCase):
    def setUp(self):
        self.makeCubeScene()
        self.assertTrue(cmds.getAttr("{}.visibility".format(self.cubeTrans)))
        self.assertTrue(cmds.getAttr("{}.visibility".format(self.cubeShape)))

    def test_toggleTransVis(self):
        cmds.setAttr("{}.visibility".format(self.cubeTrans), False)
        self.assertFalse(cmds.getAttr("{}.visibility".format(self.cubeTrans)))
        cmds.refresh()
        self.assertNotIn(
            self.cubeRprim,
            cmds.mtoh(listRenderIndex="HdStreamRendererPlugin", visibleOnly=1))

        cmds.setAttr("{}.visibility".format(self.cubeTrans), True)
        self.assertTrue(cmds.getAttr("{}.visibility".format(self.cubeTrans)))
        cmds.refresh()
        self.assertIn(
            self.cubeRprim,
            cmds.mtoh(listRenderIndex="HdStreamRendererPlugin", visibleOnly=1))

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

