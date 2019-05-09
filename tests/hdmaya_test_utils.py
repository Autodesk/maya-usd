import maya.cmds as cmds

import unittest

class HdMayaTestCase(unittest.TestCase):
    def makeCubeScene(self):
        cmds.file(f=1, new=1)
        self.activeEditor = cmds.playblast(activeEditor=1)
        cmds.modelEditor(
            self.activeEditor, e=1,
            rendererOverrideName="mtohRenderOverride_HdStreamRendererPlugin")
        self.cubeTrans = cmds.polyCube()[0]
        self.cubeShape = cmds.listRelatives(self.cubeTrans, fullPath=1)[0]
        self.cubeShapeName = self.cubeShape.split('|')[-1]
        cmds.refresh(f=1)
        self.delegateId = cmds.mtoh(sceneDelegateId=(
            "HdStreamRendererPlugin", "HdMayaSceneDelegate"))
        self.cubeRprim = self.rprimPath(self.cubeShape)
        self.assertInIndex(self.cubeRprim)
        self.assertVisible(self.cubeRprim)

    def rprimPath(self, fullPath):
        if not fullPath.startswith('|'):
            nodes = cmds.ls(fullPath, long=1)
            if len(nodes) != 1:
                raise RuntimeError("given fullPath {!r} mapped to {} nodes"
                                   .format(fullPath, len(nodes)))
            fullPath = nodes[0]
        return '/'.join([self.delegateId, "rprims",
                         fullPath.lstrip('|').replace('|', '/')])

    def getIndex(self, **kwargs):
        return cmds.mtoh(listRenderIndex="HdStreamRendererPlugin", **kwargs)

    def assertVisible(self, rprim):
        self.assertIn(rprim, self.getIndex(visibleOnly=1))

    def assertInIndex(self, rprim):
        self.assertIn(rprim, self.getIndex(visibleOnly=1))

