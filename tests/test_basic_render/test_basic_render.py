# Test to make sure that our snapshot-comparison tools work

import maya.cmds as cmds

import os.path
import unittest

from hdmaya_test_utils import HdMayaTestCase, snapshot

class TestSnapshot(HdMayaTestCase):
    """Tests whether our snapshot rendering works with basic Viewport 2.0"""

    def test_flat_orange(self):
        cmds.file(new=1, f=1)

        activeEditor = cmds.playblast(activeEditor=1)

        # Note that we use the default viewport2 renderer, because we're not testing
        # whether hdmaya works with this test - just whether we can make a snapshot

        cmds.modelEditor(
            activeEditor, e=1,
            rendererName='vp2Renderer')
        cmds.modelEditor(
            activeEditor, e=1,
            rendererOverrideName="")

        cube = cmds.polyCube()[0]
        shader = cmds.shadingNode("surfaceShader", asShader=1)
        cmds.select(cube)
        cmds.hyperShade(assign=shader)

        COLOR = (.75, .5, .25)
        cmds.setAttr('{}.outColor'.format(shader), type='float3', *COLOR)

        cmds.setAttr('persp.rotate', 0, 0, 0, type='float3')
        cmds.setAttr('persp.translate', 0, .25, .7, type='float3')

        self.assertSnapshotEqual("flat_orange.png")
        self.assertRaises(AssertionError,
                          self.assertSnapshotEqual, "flat_orange_bad.png")
        self.assertSnapshotClose("flat_orange_bad.png", 17515 / 163200000.0)


class TestHdMayaRender(HdMayaTestCase):
    def test_cube(self):
        self.makeCubeScene(camDist=6)
        self.assertSnapshotClose("cube_unselected.png")
        # if we select everything with hdmaya enabled, then all playblast
        # renders after render black... even when playblasting normal vp2
        cmds.select(self.cubeTrans)
        self.assertSnapshotClose("cube_selected.png")


if __name__ == "__main__":
    unittest.main(argv=[""])
