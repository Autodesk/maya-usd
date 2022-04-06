import os
import unittest

import maya.cmds as cmds
import maya.mel

import testUtils
import mayaUtils

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

"""If the current Maya version supports setting the default light intensity,
    then restore it to 1 so snapshots look equal across versions."""
if maya.mel.eval("optionVar -exists defaultLightIntensity"):
    maya.mel.eval("optionVar -fv defaultLightIntensity 1")

def snapshot(outputPath, width=400, height=None, hud=False, grid=False, camera=None):
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
    oldColorTransform = cmds.colorManagementPrefs(q=1, outputTarget="playblast",
                                                  outputTransformName=1)

    # Some environments use legacy synColor transforms with 2022 and above.
    # Find whether the color config should be Raw or Raw legacy
    # However depending on the MAYA_COLOR_MANAGEMENT_SYNCOLOR env var or the loaded
    # configs, this may be under a different names. So procedurally find it.
    colorTransforms = cmds.colorManagementPrefs(q=1, outputTransformNames=True)
    if "Raw" in colorTransforms:
        newColorTransform = "Raw"
    elif "Raw (legacy)" in colorTransforms:
        newColorTransform = "Raw (legacy)"
    else:
        # RAW should be reliably raw-like in most configs, so find the first ending in RAW
        newColorTransform = [c for c in colorTransforms if c.startswith("Raw ")]
        if newColorTransform:
            newColorTransform = newColorTransform[0]
        else:
            raise RuntimeError("Could not find Raw color space in available color transforms")

    # Some environments have locked color policies that prevent changing color policies
    # so we must disable and restore this accordingly.
    lockedColorTransforms = os.environ.get("MAYA_COLOR_MANAGEMENT_POLICY_LOCK") == '1'
    if lockedColorTransforms:
        os.environ['MAYA_COLOR_MANAGEMENT_POLICY_LOCK'] = '0'


    # Find the current model panel for playblasting
    # to make sure the desired camera is set, if any
    panel = mayaUtils.activeModelPanel()
    oldCamera = cmds.modelPanel(panel, q=True, cam=True)
    if camera:
        cmds.modelEditor(panel, edit=True, camera=camera)


    # do these in a separate try/finally from color management, because
    # color management seems a bit more finicky
    cmds.setAttr("defaultRenderGlobals.imageFormat", formatNum)
    cmds.headsUpDisplay(layoutVisibility=hud)
    cmds.grid(toggle=grid)
    try:
        cmds.colorManagementPrefs(e=1, outputTarget="playblast",
                                  outputTransformName=newColorTransform)
        try:
            cmds.playblast(cf=outputPath, viewer=False, format="image",
                           frame=cmds.currentTime(q=1), offScreen=1,
                           widthHeight=(width, height), percent=100)
        finally:
            cmds.colorManagementPrefs(e=1, outputTarget="playblast",
                                      outputTransformName=oldColorTransform)
    finally:
        cmds.setAttr("defaultRenderGlobals.imageFormat", oldFormat)
        cmds.headsUpDisplay(layoutVisibility=oldHud)
        cmds.grid(toggle=oldGrid)
        if lockedColorTransforms:
            os.environ['MAYA_COLOR_MANAGEMENT_POLICY_LOCK'] = '1'

        if camera:
            cmds.lookThru(panel, oldCamera)


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

    def loadImage(imagePath):
        img = PySide2.QtGui.QImage()
        if not img.load(imagePath):
            raise ValueError("unable to load image: {}".format(imagePath))
        return img

    img1 = loadImage(imagePath1)
    img2 = loadImage(imagePath2)

    if img1.size() != img2.size():
        def sizeStr(img):
            return '{}x{}'.format(img.size().width(), img.size().height())

        raise ValueError("cannot compare images of different size:\n"
                         "  ({}) {}\n"
                         "  ({}) {}".format(sizeStr(img1), imagePath1,
                                            sizeStr(img2), imagePath2))
    if img1.format() != img2.format():
        def formatStr(img):
            return testUtils.stripPrefix(img.format().name, 'Format_')

        raise ValueError("cannot compare images of different format:\n"
                         "  ({}) {}\n"
                         "  ({}) {}".format(formatStr(img1), imagePath1,
                                               formatStr(img2), imagePath2))
    if img1.hasAlphaChannel() != img2.hasAlphaChannel():
        raise ValueError("cannot compare images with alpha to one without:\n"
                         "  (alpha: {}) {}\n"
                         "  (alpha: {}) {}".format(img1.hasAlphaChannel(),
                                                   imagePath1,
                                                   img2.hasAlphaChannel(),
                                                   imagePath2))

    height = img1.height()
    width = img1.width()
    nChannels = 4 if img1.hasAlphaChannel() else 3
    nPixels = height * width

    # QImage doesn't deal with floating point, so we use 8-bit int
    # math (each channel maxes at 255), then divide at the end
    currDiff = 0
    for y in range(height):
        for x in range(width):
            p1 = img1.pixel(x, y)
            p2 = img2.pixel(x, y)
            if p1 == p2:
                continue
            for c in range(nChannels):
                currDiff += abs((p1 & 255) - (p2 & 255))
                p1 >>= 8
                p2 >>= 8
    return currDiff / float(nPixels * nChannels * 255)

class ImageDiffingTestCase(unittest.TestCase):
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
        snapDir = os.path.join(os.path.abspath('.'), self._testMethodName)
        if not os.path.isdir(snapDir):
            os.makedirs(snapDir)
        snapImage = os.path.join(snapDir, os.path.basename(refImage))
        snapshot(snapImage)
        return self.assertImagesClose(refImage, snapImage,
                                      maxAvgChannelDiff=maxAvgChannelDiff)

    def assertSnapshotEqual(self, refImage):
        return self.assertSnapshotClose(refImage, maxAvgChannelDiff=0)