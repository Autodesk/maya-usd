import os.path
import sys
import unittest

import maya.cmds as cmds
import maya.mel

import fixturesUtils
import mtohUtils

class TestImageDiffing(mtohUtils.MtohTestCase):
    """Test the image diffing setup to make sure it works and returns the expected results."""

    _file = __file__

    # Image files for comparison
    COLORED_STRIPES_REFERENCE = "colored_stripes.png"
    COLORED_STRIPES_ONE_PIXEL_OFF = "colored_stripes_one_pixel_off.png"
    COLORED_STRIPES_SLIGHT_NOISE = "colored_stripes_slight_noise.png"

    CUBE_SCENE_REFERENCE = "cube_scene.png"
    CUBE_SCENE_ONE_PIXEL_OFF = "cube_scene_one_pixel_off.png"
    CUBE_SCENE_SLIGHT_NOISE = "cube_scene_slight_noise.png"

    # Thresholds used in image diffing to tolerate differences only if they are rare
    RARE_DIFFS_FAIL_THRESHOLD = 0
    RARE_DIFFS_FAIL_PERCENT = 5

    # Thresholds used in image diffing to tolerate differences only if they are small
    SMALL_DIFFS_FAIL_THRESHOLD = 0.1
    SMALL_DIFFS_FAIL_PERCENT = 0

    # Defines the camera distance used when rendering the cube scene
    CUBE_SCENE_CAMERA_DISTANCE = 2

    def setupCubeScene(self):
        self.setHdStormRenderer()
        self.makeCubeScene(self.CUBE_SCENE_CAMERA_DISTANCE)

    def test_assertImagesClose(self):
        # Closeness criteria : tolerate differences only if they are rare
        self.assertImagesClose(self.COLORED_STRIPES_REFERENCE, self.COLORED_STRIPES_ONE_PIXEL_OFF, 
                                self.RARE_DIFFS_FAIL_THRESHOLD, self.RARE_DIFFS_FAIL_PERCENT)
        with self.assertRaises(AssertionError):
            self.assertImagesClose(self.COLORED_STRIPES_REFERENCE, self.COLORED_STRIPES_SLIGHT_NOISE, 
                                   self.RARE_DIFFS_FAIL_THRESHOLD, self.RARE_DIFFS_FAIL_PERCENT)

        # Closeness criteria : tolerate differences only if they are small
        self.assertImagesClose(self.COLORED_STRIPES_REFERENCE, self.COLORED_STRIPES_SLIGHT_NOISE, 
                               self.SMALL_DIFFS_FAIL_THRESHOLD, self.SMALL_DIFFS_FAIL_PERCENT)
        with self.assertRaises(AssertionError):
            self.assertImagesClose(self.COLORED_STRIPES_REFERENCE, self.COLORED_STRIPES_ONE_PIXEL_OFF, 
                                   self.SMALL_DIFFS_FAIL_THRESHOLD, self.SMALL_DIFFS_FAIL_PERCENT)
        
    def test_assertImagesEqual(self):
        self.assertImagesEqual(self.COLORED_STRIPES_REFERENCE, self.COLORED_STRIPES_REFERENCE)
        with self.assertRaises(AssertionError):
            self.assertImagesEqual(self.COLORED_STRIPES_REFERENCE, self.COLORED_STRIPES_ONE_PIXEL_OFF)
        with self.assertRaises(AssertionError):
            self.assertImagesEqual(self.COLORED_STRIPES_REFERENCE, self.COLORED_STRIPES_SLIGHT_NOISE)

    def test_assertSnapshotClose(self):
        self.setupCubeScene()

        # Closeness criteria : tolerate differences only if they are rare
        self.assertSnapshotClose(self.CUBE_SCENE_ONE_PIXEL_OFF, self.RARE_DIFFS_FAIL_THRESHOLD, self.RARE_DIFFS_FAIL_PERCENT)
        with self.assertRaises(AssertionError):
            self.assertSnapshotClose(self.CUBE_SCENE_SLIGHT_NOISE, self.RARE_DIFFS_FAIL_THRESHOLD, self.RARE_DIFFS_FAIL_PERCENT)

        # Closeness criteria : tolerate differences only if they are small
        self.assertSnapshotClose(self.CUBE_SCENE_SLIGHT_NOISE, self.SMALL_DIFFS_FAIL_THRESHOLD, self.SMALL_DIFFS_FAIL_PERCENT)
        with self.assertRaises(AssertionError):
            self.assertSnapshotClose(self.CUBE_SCENE_ONE_PIXEL_OFF, self.SMALL_DIFFS_FAIL_THRESHOLD, self.SMALL_DIFFS_FAIL_PERCENT)

    # We do not test for assertSnapshotEqual, as we don't have a scene that renders identically between renderer architectures.
    # Since we can't guarantee equality of renders in general, the use of this method is discouraged.

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
