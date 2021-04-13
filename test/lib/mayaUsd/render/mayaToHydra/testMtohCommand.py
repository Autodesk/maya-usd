import sys
import unittest

import maya.cmds as cmds
import maya.mel as mel

import fixturesUtils
import mtohUtils

class TestCommand(unittest.TestCase):
    _file = __file__
    _has_embree = None

    @classmethod
    def setUpClass(cls):
        cmds.loadPlugin('mtoh', quiet=True)

    @classmethod
    def has_embree(cls):
        import pxr.Plug
        if cls._has_embree is None:
            plug_reg = pxr.Plug.Registry()
            cls._has_embree = bool(plug_reg.GetPluginWithName('hdEmbree'))
        return cls._has_embree

    def test_invalidFlag(self):
        self.assertRaises(TypeError, cmds.mtoh, nonExistantFlag=1)

    def test_listRenderers(self):
        renderers = cmds.mtoh(listRenderers=1)
        self.assertEqual(renderers, cmds.mtoh(lr=1))
        self.assertIn(mtohUtils.HD_STORM, renderers)
        if self.has_embree():
            self.assertIn("HdEmbreeRendererPlugin", renderers)

    def test_listActiveRenderers(self):
        activeRenderers = cmds.mtoh(listActiveRenderers=1)
        self.assertEqual(activeRenderers, cmds.mtoh(lar=1))
        self.assertEqual(activeRenderers, [])

        activeEditor = cmds.playblast(ae=1)
        cmds.modelEditor(
            activeEditor, e=1,
            rendererOverrideName=mtohUtils.HD_STORM_OVERRIDE)
        cmds.refresh(f=1)

        activeRenderers = cmds.mtoh(listActiveRenderers=1)
        self.assertEqual(activeRenderers, cmds.mtoh(lar=1))
        self.assertEqual(activeRenderers, [mtohUtils.HD_STORM])

        if self.has_embree():
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
                          "mtoh -getRendererDisplayName")

        displayName = cmds.mtoh(renderer=mtohUtils.HD_STORM,
                                getRendererDisplayName=True)
        self.assertEqual(displayName, cmds.mtoh(r=mtohUtils.HD_STORM, gn=True))
        self.assertEqual(displayName, "GL")

        if self.has_embree():
            displayName = cmds.mtoh(renderer="HdEmbreeRendererPlugin",
                                    getRendererDisplayName=True)
            self.assertEqual(displayName, cmds.mtoh(r="HdEmbreeRendererPlugin",
                                                    gn=True))
            self.assertEqual(displayName, "Embree")

    def test_listDelegates(self):
        delegates = cmds.mtoh(listDelegates=1)
        self.assertEqual(delegates, cmds.mtoh(ld=1))
        self.assertIn("HdMayaSceneDelegate", delegates)

    def test_createRenderGlobals(self):
        for flag in ("createRenderGlobals", "crg"):
            cmds.file(f=1, new=1)
            self.assertFalse(cmds.objExists(
                "defaultRenderGlobals.mtohMotionSampleStart"))
            cmds.mtoh(**{flag: 1})
            self.assertTrue(cmds.objExists(
                "defaultRenderGlobals.mtohMotionSampleStart"))
            self.assertFalse(cmds.getAttr(
                "defaultRenderGlobals.mtohMotionSampleStart"))

    # TODO: test_updateRenderGlobals


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
