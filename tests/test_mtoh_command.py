import maya.cmds as cmds
import maya.mel as mel

import time
import unittest


class TestCommand(unittest.TestCase):
    def test_invalidFlag(self):
        self.assertRaises(TypeError, cmds.mtoh, nonExistantFlag=1)

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

    def test_getRendererDisplayName(self):
        # needs at least one arg
        self.assertRaises(RuntimeError, mel.eval,
                          "moth -getRendererDisplayName")

        displayName = cmds.mtoh(
            getRendererDisplayName="HdStreamRendererPlugin")
        self.assertEqual(displayName, cmds.mtoh(gn="HdStreamRendererPlugin"))
        self.assertEqual(displayName, "GL")

        displayName = cmds.mtoh(
            getRendererDisplayName="HdEmbreeRendererPlugin")
        self.assertEqual(displayName, cmds.mtoh(gn="HdEmbreeRendererPlugin"))
        self.assertEqual(displayName, "Embree")

    def test_listDelegates(self):
        delegates = cmds.mtoh(listDelegates=1)
        self.assertEqual(delegates, cmds.mtoh(ld=1))
        self.assertIn("HdMayaSceneDelegate", delegates)

    def test_createRenderGlobals(self):
        for flag in ("createRenderGlobals", "crg"):
            cmds.file(f=1, new=1)
            self.assertFalse(cmds.objExists(
                "defaultRenderGlobals.mtohEnableMotionSamples"))
            cmds.mtoh(**{flag: 1})
            self.assertTrue(cmds.objExists(
                "defaultRenderGlobals.mtohEnableMotionSamples"))
            self.assertFalse(cmds.getAttr(
                "defaultRenderGlobals.mtohEnableMotionSamples"))

    # TODO: test_updateRenderGlobals


if __name__ == "__main__":
    unittest.main(argv=[""])
