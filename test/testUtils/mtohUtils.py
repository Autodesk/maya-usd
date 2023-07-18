import inspect
import os
import unittest

import maya.cmds as cmds
import maya.mel

import fixturesUtils
import testUtils
from imageUtils import ImageDiffingTestCase

import sys

HD_STORM = "HdStormRendererPlugin"
HD_STORM_OVERRIDE = "mayaHydraRenderOverride_" + HD_STORM

def checkForMayaUsdPlugin():
    try:
        cmds.loadPlugin('mayaUsdPlugin')
    except:
        return False
    
    return True

class MayaHydraBaseTestCase(unittest.TestCase):
    '''Base class for mayaHydra unit tests without image comparison.'''

    _file = None

    DEFAULT_CAM_DIST = 24

    @classmethod
    def setUpClass(cls):
        if cls._file is None:
            raise ValueError("Subclasses of MayaHydraBaseTestCase must "
                             "define `_file = __file__`")
        fixturesUtils.readOnlySetUpClass(cls._file, 'mayaHydra', 
                                         initializeStandalone=False)

    def setHdStormRenderer(self):
        self.activeEditor = cmds.playblast(activeEditor=1)
        cmds.modelEditor(
            self.activeEditor, e=1,
            rendererOverrideName=HD_STORM_OVERRIDE)
        cmds.refresh(f=1)
        self.delegateId = cmds.mayaHydra(renderer=HD_STORM,
                                    sceneDelegateId="MayaHydraSceneDelegate")

    def setBasicCam(self, dist=DEFAULT_CAM_DIST):
        cmds.setAttr('persp.rotate', -30, 45, 0, type='float3')
        cmds.setAttr('persp.translate', dist, .75 * dist, dist, type='float3')

    def makeCubeScene(self, camDist=DEFAULT_CAM_DIST):
        cmds.file(f=1, new=1)
        self.cubeTrans = cmds.polyCube()[0]
        self.cubeShape = cmds.listRelatives(self.cubeTrans)[0]
        self.setHdStormRenderer()
        self.assertNodeNameInIndex(self.cubeShape)
        # The single Maya cube shape maps to two rprims, the first once of
        # which is the shape's StandardShadedItem.  The list is ordered, as the
        # Hydra call made is HdRenderIndex::GetRprimIds(), which sorts
        # according to std::less<SdfPath>, which will produce
        # lexicographically-ordered paths.
        self.cubeRprim = self.getIndex()[0]
        cmds.select(clear=1)
        cmds.refresh()
        self.assertVisible(self.cubeRprim)
        self.setBasicCam(dist=camDist)
        cmds.select(clear=True)

        # The color and specular roughness of the default standard surface changed, set
        # them back to the old default value so the tests keep on working correctly.
        if maya.mel.eval("defaultShaderName") == "standardSurface1":
            color = (0.8, 0.8, 0.8)
            cmds.setAttr("standardSurface1.baseColor", type='float3', *color)
            cmds.setAttr("standardSurface1.specularRoughness", 0.4)

    def getIndex(self, **kwargs):
        return cmds.mayaHydra(renderer=HD_STORM, listRenderIndex=True, **kwargs)

    def getVisibleIndex(self, **kwargs):
        kwargs['visibleOnly'] = True
        return self.getIndex(**kwargs)

    def assertVisible(self, rprim):
        self.assertIn(rprim, self.getVisibleIndex())

    def assertInIndex(self, rprim):
        self.assertIn(rprim, self.getIndex())

    def assertNodeNameInIndex(self, nodeName):
        for rprim in self.getIndex():
            if nodeName in rprim:
                return True
        return False

    def trace(self, msg):
        sys.__stdout__.write(msg)
        sys.__stdout__.flush()

    def traceIndex(self, msg):
        self.trace(msg.format(str(self.getIndex())))

class MtohTestCase(MayaHydraBaseTestCase, ImageDiffingTestCase):
    '''Base class for mayaHydra unit tests with image comparison.'''

    _inputDir = None

    @classmethod
    def setUpClass(cls):
        if cls._file is None:
            raise ValueError("Subclasses of MtohTestCase, must define "
                             "`_file = __file__`")

        inputPath = fixturesUtils.setUpClass(
            cls._file, 'mayaHydra', initializeStandalone=False, 
            suffix=('_' + cls.__name__))

        if cls._inputDir is None:
            inputDirName = os.path.splitext(os.path.basename(cls._file))[0]
            inputDirName = testUtils.stripPrefix(inputDirName, 'test')
            if not inputDirName.endswith('Test'):
                inputDirName += 'Test'
            cls._inputDir = os.path.join(inputPath, inputDirName)

        cls._testDir = os.path.abspath('.')

    def resolveRefImage(self, refImage, imageVersion):
        if not os.path.isabs(refImage):
            if imageVersion:
                refImage = os.path.join(self._inputDir, imageVersion, refImage)
            else:
                refImage = os.path.join(self._inputDir, refImage)
        return refImage

    def assertSnapshotClose(self, refImage, fail, failpercent, imageVersion=None, hardfail=None, 
                warn=None, warnpercent=None, hardwarn=None, perceptual=False):
        refImage = self.resolveRefImage(refImage, imageVersion)
        super(MtohTestCase, self).assertSnapshotClose(refImage, fail, failpercent, hardfail,
                            warn, warnpercent, hardwarn, perceptual)

    def assertSnapshotEqual(self, refImage, imageVersion=None):
        refImage = self.resolveRefImage(refImage, imageVersion)
        super(MtohTestCase, self).assertSnapshotEqual(refImage)
