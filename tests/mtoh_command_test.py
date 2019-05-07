import maya.cmds as cmds
import maya.utils

import time
import unittest


class TestCommand(unittest.TestCase):
    def test_listRenderers(self):
        renderers = cmds.mtoh(listRenderers=1)
        self.assertEqual(renderers, cmds.mtoh(lr=1))
        self.assertIn("HdStreamRendererPlugin", renderers)
        self.assertIn("HdEmbreeRendererPlugin", renderers)

    def test_listActiveRenderers(self):
        activeRenderers = cmds.mtoh(listActiveRenderers=1)
        self.assertEqual(activeRenderers, cmds.mtoh(lar=1))
        self.assertEqual(activeRenderers, [])

        activeEditor = cmds.playblast(ae=1)
        cmds.modelEditor(
            activeEditor, e=1,
            rendererOverrideName="mtohRenderOverride_HdStreamRendererPlugin")
        cmds.refresh(f=1)

        activeRenderers = cmds.mtoh(listActiveRenderers=1)
        self.assertEqual(activeRenderers, cmds.mtoh(lar=1))
        self.assertEqual(activeRenderers, ["HdStreamRendererPlugin"])

        cmds.modelEditor(
            activeEditor, e=1,
            rendererOverrideName="mtohRenderOverride_HdEmbreeRendererPlugin")
        cmds.refresh(f=1)

        activeRenderers = cmds.mtoh(listActiveRenderers=1)
        self.assertEqual(activeRenderers, cmds.mtoh(lar=1))
        self.assertEqual(activeRenderers, ["HdEmbreeRendererPlugin"])

        cmds.modelEditor(activeEditor, rendererOverrideName="", e=1)
        cmds.refresh(f=1)

        activeRenderers = cmds.mtoh(listActiveRenderers=1)
        self.assertEqual(activeRenderers, cmds.mtoh(lar=1))
        self.assertEqual(activeRenderers, [])


if __name__ == "__main__":
    unittest.main(argv=[""])
