import maya.cmds as cmds

import math
import os.path
import unittest


HD_STREAM = "HdStreamRendererPlugin"
HD_STREAM_OVERRIDE = "mtohRenderOverride_HdStreamRendererPlugin"


KNOWN_FORMATS = {
    'gif': 0,
    'tif': 3,
    'tiff': 3,
    'sgi': 5,
    'iff': 7,
    'jpg': 8,
    'jpeg': 8,
    'tga': 19,
    'bmp': 20,
    'png': 32,
}


def snapshot(outputPath, width=400, height=None, hud=False, grid=False):
    if height is None:
        height = width

    outputExt = os.path.splitext(outputPath)[1].lower().lstrip('.')

    formatNum = KNOWN_FORMATS.get(outputExt)
    if formatNum is None:
        raise ValueError("input image had unrecognized extension: {}"
                         .format(outputExt))

    # if given relative path, make it relative to current dir (the test
    # temp base), rather than the workspace dir
    outputPath = os.path.abspath(outputPath)

    # save the old output image format
    oldFormat = cmds.getAttr("defaultRenderGlobals.imageFormat")
    # save the hud setting
    oldHud = cmds.headsUpDisplay(q=1, layoutVisibility=1)
    # save the grid setting
    oldGrid = cmds.grid(q=1, toggle=1)
    # save the old view transform
    oldViewTransform = cmds.colorManagementPrefs(q=1, viewTransformName=1)

    # do these in a separate try/finally from color management, because
    # color management seems a bit more finicky
    cmds.setAttr("defaultRenderGlobals.imageFormat", 32)
    cmds.headsUpDisplay(layoutVisibility=hud)
    cmds.grid(toggle=grid)
    try:
        # Doing this would be more direct:
        #   cmds.colorManagementPrefs(e=1, outputTarget="playblast",
        #                             outputTransformName="Raw")
        # ... but it causes maya-2018.3 to crash. It seems to be fixed in 2019.0.
        cmds.colorManagementPrefs(e=1, viewTransformName="Raw")
        try:
            cmds.playblast(cf=outputPath, viewer=False, format="image",
                           frame=cmds.currentTime(q=1), offScreen=1,
                           widthHeight=(width, height), percent=100)
        finally:
            cmds.colorManagementPrefs(e=1, viewTransformName=oldViewTransform)
    finally:
        cmds.setAttr("defaultRenderGlobals.imageFormat", oldFormat)
        cmds.headsUpDisplay(layoutVisibility=oldHud)
        cmds.grid(toggle=oldGrid)


def imageDiff(imagePath1, imagePath2):
    '''Returns the average per-pixel, per-channel difference between two images.

    The maximum return is 1.0, and the minimum 0.0
    '''
    # TODO: write a c++ tool to do this faster...

    # Considered having a maxDiff, and aborting early if it was exceeded...
    # but the "normal" case is that whole image is compared (since it's the
    # same), so we'd only get speedups when things are going wrong - and it's
    # useful to have a utility to return the total diff

    # We use QImage because we know we should have access to
    # it in maya...
    import PySide2.QtGui

    img1 = PySide2.QtGui.QImage()
    img1.load(imagePath1)

    img2 = PySide2.QtGui.QImage()
    img2.load(imagePath2)

    if img1.size() != img2.size():
        raise ValueError("cannot compare images of different size")
    if img1.format() != img2.format():
        raise ValueError("cannot compare images of different format")
    if img1.hasAlphaChannel() != img2.hasAlphaChannel():
        raise ValueError("cannot compare images with alpha to one without ")

    height = img1.height()
    width = img1.width()
    nChannels = 4 if img1.hasAlphaChannel() else 3
    nPixels = height * width

    # QImage doesn't deal with floating point, so we use 8-bit int
    # math (each channel maxes at 255), then divide at the end
    currDiff = 0
    for y in xrange(height):
        for x in xrange(width):
            p1 = img1.pixel(x, y)
            p2 = img2.pixel(x, y)
            if p1 == p2:
                continue
            for c in xrange(nChannels):
                currDiff += abs((p1 & 255) - (p2 & 255))
                p1 >>= 8
                p2 >>= 8
    print "{}: {} - {}".format(currDiff, imagePath1, imagePath2)
    return currDiff / float(nPixels * nChannels * 255)


class HdMayaTestCase(unittest.TestCase):
    DEFAULT_CAM_DIST = 24

    def setHdStreamRenderer(self):
        self.activeEditor = cmds.playblast(activeEditor=1)
        cmds.modelEditor(
            self.activeEditor, e=1,
            rendererOverrideName=HD_STREAM_OVERRIDE)
        cmds.refresh(f=1)
        self.delegateId = cmds.mtoh(sceneDelegateId=(
            HD_STREAM, "HdMayaSceneDelegate"))

    def setBasicCam(self, dist=DEFAULT_CAM_DIST):
        cmds.setAttr('persp.rotate', -30, 45, 0, type='float3')
        cmds.setAttr('persp.translate', dist, .75 * dist, dist, type='float3')

    def makeCubeScene(self, camDist=DEFAULT_CAM_DIST):
        cmds.file(f=1, new=1)
        self.cubeTrans = cmds.polyCube()[0]
        self.cubeShape = cmds.listRelatives(self.cubeTrans)[0]
        self.setHdStreamRenderer()
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
        return cmds.mtoh(listRenderIndex=HD_STREAM, **kwargs)

    def assertVisible(self, rprim):
        self.assertIn(rprim, self.getIndex(visibleOnly=1))

    def assertInIndex(self, rprim):
        self.assertIn(rprim, self.getIndex(visibleOnly=1))

    # 1e-04 equates to at most 16 pixels in a 400x400 image
    # are "completely wrong" - ie, pure black/white, when they
    # should be pure white/black
    AVG_CHANNEL_DIFF = 1e-04

    def assertImagesClose(self, imagePath1, imagePath2,
                          maxAvgChannelDiff=AVG_CHANNEL_DIFF):
        if maxAvgChannelDiff < 0:
            raise ValueError("maxAvgChannelDiff cannot be negative")

        diff = imageDiff(imagePath1, imagePath2)
        if diff > maxAvgChannelDiff:
            absPath1 = os.path.abspath(imagePath1)
            absPath2 = os.path.abspath(imagePath2)
            msg = "Images differed by {} (max allowed: {}):\n" \
                  "  {}\n" \
                  "  {}".format(diff, maxAvgChannelDiff, absPath1, absPath2)
            self.fail(msg)
        # if we succeed, return the avg diff
        return diff

    def assertSnapshotClose(self, refImage, maxAvgChannelDiff=AVG_CHANNEL_DIFF):
        if not os.path.isabs(refImage):
            test_dir = os.path.dirname(os.environ['HDMAYA_TEST_SCRIPT'])
            refImage = os.path.join(test_dir, refImage)

        snapImage = os.path.basename(refImage)
        snapshot(snapImage)
        return self.assertImagesClose(refImage, snapImage,
                                      maxAvgChannelDiff=maxAvgChannelDiff)

    def assertSnapshotEqual(self, refImage):
        return self.assertSnapshotClose(refImage, maxAvgChannelDiff=0)
