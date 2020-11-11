import inspect
import os

import maya.cmds as cmds

import fixturesUtils
import testUtils
from imageUtils import ImageDiffingTestCase

HD_STORM = "HdStormRendererPlugin"
HD_STORM_OVERRIDE = "mtohRenderOverride_" + HD_STORM


class MtohTestCase(ImageDiffingTestCase):
    DEFAULT_CAM_DIST = 24

    _file = None
    _inputDir = None

    @classmethod
    def setUpClass(cls):
        if cls._file is None:
            raise ValueError("For subclasses of MtohTestCase, set "
                             "`_file = __file__` when defining")

        inputPath = fixturesUtils.setUpClass(
            cls._file, suffix=('_' + cls.__name__),
            initializeStandalone=False, loadPlugin=True)
        cmds.loadPlugin('mtoh', quiet=True)

        if cls._inputDir is None:
            inputDirName = os.path.splitext(os.path.basename(cls._file))[0]
            inputDirName = testUtils.stripPrefix(inputDirName, 'test')
            if not inputDirName.endswith('Test'):
                inputDirName += 'Test'
            cls._inputDir = os.path.join(inputPath, inputDirName)

        cls._testDir = os.path.abspath('.')

    def assertSnapshotClose(self, refImage,
                            maxAvgChannelDiff=\
                                ImageDiffingTestCase.AVG_CHANNEL_DIFF):
        if not os.path.isabs(refImage):
            refImage = os.path.join(self._inputDir, refImage)
        super(MtohTestCase, self).assertSnapshotClose(
            refImage, maxAvgChannelDiff=maxAvgChannelDiff)

    def setHdStormRenderer(self):
        self.activeEditor = cmds.playblast(activeEditor=1)
        cmds.modelEditor(
            self.activeEditor, e=1,
            rendererOverrideName=HD_STORM_OVERRIDE)
        cmds.refresh(f=1)
        self.delegateId = cmds.mtoh(renderer=HD_STORM,
                                    sceneDelegateId="HdMayaSceneDelegate")

    def setBasicCam(self, dist=DEFAULT_CAM_DIST):
        cmds.setAttr('persp.rotate', -30, 45, 0, type='float3')
        cmds.setAttr('persp.translate', dist, .75 * dist, dist, type='float3')

    def makeCubeScene(self, camDist=DEFAULT_CAM_DIST):
        cmds.file(f=1, new=1)
        self.cubeTrans = cmds.polyCube()[0]
        self.cubeShape = cmds.listRelatives(self.cubeTrans)[0]
        self.setHdStormRenderer()
        self.cubeRprim = self.rprimPath(self.cubeShape)
        self.assertInIndex(self.cubeRprim)
        self.assertVisible(self.cubeRprim)
        self.setBasicCam(dist=camDist)
        cmds.select(clear=True)

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
        return cmds.mtoh(renderer=HD_STORM, listRenderIndex=True, **kwargs)

    def getVisibleIndex(self, **kwargs):
        kwargs['visibleOnly'] = True
        return self.getIndex(**kwargs)

    def assertVisible(self, rprim):
        self.assertIn(rprim, self.getVisibleIndex())

    def assertInIndex(self, rprim):
        self.assertIn(rprim, self.getIndex())
