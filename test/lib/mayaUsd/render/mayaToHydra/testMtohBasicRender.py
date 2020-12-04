# Test to make sure that our snapshot-comparison tools work

import os.path
import sys
import unittest

import maya.cmds as cmds

import fixturesUtils
import mtohUtils

class TestSnapshot(mtohUtils.MtohTestCase):
    """Tests whether our snapshot rendering works with basic Viewport 2.0"""

    _file = __file__

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
        # 17515 is the exact 8-bit raw-pixel difference between flat_orange.png
        # flat_orange_bad.png, and 163200000 is the maximum diff for an 8-bit,
        # 40x40, 4-channel image.  So this tests that assertSnapshotClose is
        # working
        self.assertSnapshotClose("flat_orange_bad.png", 17515 / 163200000.0)


class TestHdMayaRender(mtohUtils.MtohTestCase):
    _file = __file__

    def test_cube(self):
        self.makeCubeScene(camDist=6)
        self.assertSnapshotClose("cube_unselected.png")
        cmds.select(self.cubeTrans)
        self.assertSnapshotClose("cube_selected.png")


if __name__ == '__main__':
    fixturesUtils.runTests(globals())

