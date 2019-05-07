import maya.cmds as cmds
import maya.mel as mel

import unittest


class TestCommand(unittest.TestCase):
    def test_listRenderers(self):
        renderers = cmds.mtoh(listRenderers=1)
        self.assertEqual(renderers, cmds.mtoh(lr=1))
        self.assertIn("HdStreamRendererPlugin", renderers)
        self.assertIn("HdEmbreeRendererPlugin", renderers)


if __name__ == "__main__":
    unittest.main(argv=[""])
