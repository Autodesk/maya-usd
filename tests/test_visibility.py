import maya.cmds as cmds

import unittest


class TestCommand(unittest.TestCase):
    def setUp(self):
        cmds.file(f=1, new=1)
        self.activeEditor = cmds.playblast(activeEditor=1)
        cmds.modelEditor(
            self.activeEditor, e=1,
            rendererOverrideName="mtohRenderOverride_HdStreamRendererPlugin")
        self.cubeTrans = cmds.polyCube()[0]
        self.cubeShape = cmds.listRelatives(self.cubeTrans, fullPath=1)[0]
        cmds.refresh(f=1)
        self.delegateId = cmds.mtoh(sceneDelegateId=(
            "HdStreamRendererPlugin", "HdMayaSceneDelegate"))
        self.cubeRprim = '/'.join(
            [self.delegateId, "rprims",
             self.cubeShape.replace('|', '/').lstrip('/')])
        self.assertIn(
            self.cubeRprim,
            cmds.mtoh(listRenderIndex="HdStreamRendererPlugin"))
        self.assertIn(
            self.cubeRprim,
            cmds.mtoh(listRenderIndex="HdStreamRendererPlugin", visibleOnly=1))
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

