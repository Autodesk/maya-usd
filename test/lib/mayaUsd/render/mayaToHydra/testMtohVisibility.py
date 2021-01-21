import maya.cmds as cmds

import fixturesUtils
import mtohUtils

class TestCommand(mtohUtils.MtohTestCase):
    _file = __file__

    def setUp(self):
        self.makeCubeScene(camDist=6)
        self.assertTrue(cmds.getAttr("{}.visibility".format(self.cubeTrans)))
        self.assertTrue(cmds.getAttr("{}.visibility".format(self.cubeShape)))

    def test_toggleTransVis(self):
        # because snapshotting is slow, we only use it in this test - otherwise
        # we assume the results of `listRenderIndex=..., visibileOnly=1` are
        # sufficient

        cubeUnselectedImg = "cube_unselected.png"
        nothingImg = "nothing.png"

        cmds.refresh()
        self.assertIn(
            self.cubeRprim,
            self.getVisibleIndex())
        self.assertSnapshotClose(cubeUnselectedImg)

        cmds.setAttr("{}.visibility".format(self.cubeTrans), False)
        self.assertFalse(cmds.getAttr("{}.visibility".format(self.cubeTrans)))
        cmds.refresh()
        self.assertNotIn(
            self.cubeRprim,
            self.getVisibleIndex())
        self.assertSnapshotClose(nothingImg)

        cmds.setAttr("{}.visibility".format(self.cubeTrans), True)
        self.assertTrue(cmds.getAttr("{}.visibility".format(self.cubeTrans)))
        cmds.refresh()
        self.assertIn(
            self.cubeRprim,
            self.getVisibleIndex())
        self.assertSnapshotClose(cubeUnselectedImg)

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

    def doHierarchicalVisibilityTest(self, makeNodeVis, makeNodeInvis, prep=None):
        lowGroup = cmds.group(self.cubeTrans, name='lowGroup')
        midGroup = cmds.group(lowGroup, name='midGroup')
        highGroup = cmds.group(midGroup, name='highGroup')
        hier = [midGroup, midGroup, lowGroup, self.cubeTrans, self.cubeShape]
        cmds.select(clear=1)
        cmds.refresh()

        self.cubeRprim = self.rprimPath(self.cubeShape)
        visIndex = [self.cubeRprim]
        self.assertEqual(self.getVisibleIndex(), visIndex)

        if prep is not None:
            for obj in hier:
                prep(obj)
            self.assertEqual(self.getVisibleIndex(), visIndex)

        for obj in hier:
            makeNodeInvis(obj)
            cmds.refresh()
            self.assertEqual(self.getVisibleIndex(), [])
            makeNodeVis(obj)
            cmds.refresh()
            self.assertEqual(self.getVisibleIndex(), visIndex)

    def test_hierarchicalVisibility(self):
        def makeNodeVis(obj):
            cmds.setAttr("{}.visibility".format(obj), True)

        def makeNodeInvis(obj):
            cmds.setAttr("{}.visibility".format(obj), False)

        self.doHierarchicalVisibilityTest(makeNodeVis, makeNodeInvis)

    def test_hierarchicalIntermediateObject(self):
        def makeNodeVis(obj):
            cmds.setAttr("{}.intermediateObject".format(obj), False)

        def makeNodeInvis(obj):
            cmds.setAttr("{}.intermediateObject".format(obj), True)

        self.doHierarchicalVisibilityTest(makeNodeVis, makeNodeInvis)

    def test_hierarchicalOverrideEnabled(self):
        def makeNodeVis(obj):
            cmds.setAttr("{}.overrideEnabled".format(obj), False)

        def makeNodeInvis(obj):
            cmds.setAttr("{}.overrideEnabled".format(obj), True)

        def prep(obj):
            # set the overrideVisibility to False - as long as the
            # overrideEnabled is NOT set, the object should still
            # be visible
            cmds.setAttr("{}.overrideVisibility".format(obj), False)
            cmds.setAttr("{}.overrideEnabled".format(obj), False)

        self.doHierarchicalVisibilityTest(makeNodeVis, makeNodeInvis, prep=prep)

    def test_hierarchicalOverrideVisibility(self):
        def makeNodeVis(obj):
            cmds.setAttr("{}.overrideVisibility".format(obj), True)

        def makeNodeInvis(obj):
            cmds.setAttr("{}.overrideVisibility".format(obj), False)

        def prep(obj):
            # set the overrideEnabled to True - as long as the
            # overrideVisibility is True, the object should still
            # be visible
            cmds.setAttr("{}.overrideEnabled".format(obj), True)
            cmds.setAttr("{}.overrideVisibility".format(obj), True)

        self.doHierarchicalVisibilityTest(makeNodeVis, makeNodeInvis, prep=prep)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
