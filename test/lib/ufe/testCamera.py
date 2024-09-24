#!/usr/bin/env python

#
# Copyright 2021 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import fixturesUtils
import mayaUtils
import testUtils
import ufeUtils
import usdUtils

from pxr import Gf
from pxr import UsdGeom

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as om

import os
import ufe

from functools import partial
import unittest


def matrix4dTranslation(matrix4d):
    translation = matrix4d.matrix[-1]
    return translation[:-1]

def transform3dTranslation(transform3d):
    return matrix4dTranslation(transform3d.inclusiveMatrix())

def addVec(mayaVec, usdVec):
    return mayaVec + om.MVector(*usdVec)

class TestObserver(ufe.Observer):
    def __init__(self):
        super(TestObserver, self).__init__()
        self.changed = 0

    def __call__(self, notification):
        if isinstance(notification, ufe.CameraChanged):
            self.changed += 1

    def notifications(self):
        return self.changed

class CameraTestCase(unittest.TestCase):
    '''Verify the Camera UFE translate interface, for multiple runtimes.
    
    UFE Feature : Camera
    Maya Feature : camera
    Action : set & read camera parameters
    Applied On Selection : No
    Undo/Redo Test : No
    Expect Results To Test :
        - Setting a value through Ufe correctly updates USD
        - Reading a value through Ufe gets the correct value
    Edge Cases :
        - None.

    '''

    pluginsLoaded = False
    
    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        ''' Called initially to set up the maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

        # value from UsdGeomLinearUnits
        self.inchesToCm = 2.54
        self.mmToCm = 0.1

        # load the file and get ready to test!
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")
        testFile = testUtils.getTestScene("camera", 'TranslateRotate_vs_xform.usda')
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()

        # create some strings
        self.camera2PathString = '|stage|stageShape,/cameras/cam2/camera2'

    def _TestCameraAttributes(self, ufeCamera, usdCamera):
        # Trust that the USD API works correctly, validate that UFE gives us
        # the same answers
        self._TestHorizontalAperture(ufeCamera, usdCamera)
        self._TestVerticalAperture(ufeCamera, usdCamera)
        self._TestHorizontalApertureOffset(ufeCamera, usdCamera)
        self._TestVerticalApertureOffset(ufeCamera, usdCamera)
        self._TestFStop(ufeCamera, usdCamera)
        self._TestFocalLength(ufeCamera, usdCamera)
        self._TestFocusDistance(ufeCamera, usdCamera)
        self._TestClippingRange(ufeCamera, usdCamera)
        self._TestProjection(ufeCamera, usdCamera)
        if(os.getenv('UFE_CAMERA_HAS_RENDERABLE', 'NOT-FOUND') in ('1', 'TRUE')):
            self._TestRenderable(ufeCamera, usdCamera)

    def _TestClippingRange(self, ufeCamera, usdCamera):
        clippingAttr = usdCamera.GetAttribute('clippingRange')
        clippingRange = clippingAttr.Get()

        # read the nearClipPlane and check the value is what we expect.
        self.assertAlmostEqual(ufeCamera.nearClipPlane(), clippingRange[0])
        self.assertAlmostEqual(ufeCamera.farClipPlane(), clippingRange[1])

        # set the clipping planes using UFE
        ufeCamera.nearClipPlane(10)
        ufeCamera.farClipPlane(20)
        clippingRange = clippingAttr.Get()
        self.assertAlmostEqual(10, clippingRange[0])
        self.assertAlmostEqual(20, clippingRange[1])

        # set the clipping planes using USD
        clippingAttr.Set(Gf.Vec2f(1, 50))

        # read the nearClipPlane and check the value is what we just set.
        self.assertAlmostEqual(ufeCamera.nearClipPlane(), 1)
        self.assertAlmostEqual(ufeCamera.farClipPlane(), 50)

    def _TestHorizontalAperture(self, ufeCamera, usdCamera):
        usdAttr = usdCamera.GetAttribute('horizontalAperture')
        horizontalAperture = usdAttr.Get()

        # the USD schema specifics that horizontal aperture is authored in mm
        # the ufeCamera gives us inches
        self.assertAlmostEqual(self.mmToCm * horizontalAperture, self.inchesToCm * ufeCamera.horizontalAperture())

        # set the horizontal aperture using UFE
        ufeCamera.horizontalAperture(0.9)
        self.assertAlmostEqual(0.9, ufeCamera.horizontalAperture())

        # set the horizontal aperture using USD
        usdAttr.Set(0.5)
        self.assertAlmostEqual(self.mmToCm * 0.5, self.inchesToCm * ufeCamera.horizontalAperture())
    
    def _TestVerticalAperture(self, ufeCamera, usdCamera):
        usdAttr = usdCamera.GetAttribute('verticalAperture')
        verticalAperture = usdAttr.Get()

        self.assertAlmostEqual(self.mmToCm * verticalAperture, self.inchesToCm * ufeCamera.verticalAperture())

        # set the vertical aperture using UFE
        ufeCamera.verticalAperture(0.3)
        self.assertAlmostEqual(0.3, ufeCamera.verticalAperture())

        # set the vertical aperture using UFE
        usdAttr.Set(0.5)
        self.assertAlmostEqual(self.mmToCm * 0.5, self.inchesToCm * ufeCamera.verticalAperture())

    def _TestHorizontalApertureOffset(self, ufeCamera, usdCamera):
        usdAttr = usdCamera.GetAttribute('horizontalApertureOffset')
        horizontalApertureOffset = usdAttr.Get()

        self.assertAlmostEqual(self.mmToCm * horizontalApertureOffset, self.inchesToCm * ufeCamera.horizontalApertureOffset())

        # set the horizontal aperture offset using UFE
        ufeCamera.horizontalApertureOffset(0.2)
        self.assertAlmostEqual(0.2, ufeCamera.horizontalApertureOffset())

        # set the horizontal aperture offset using USD
        usdAttr.Set(0.5)
        self.assertAlmostEqual(self.mmToCm * 0.5, self.inchesToCm * ufeCamera.horizontalApertureOffset())
    
    def _TestVerticalApertureOffset(self, ufeCamera, usdCamera):
        usdAttr = usdCamera.GetAttribute('verticalApertureOffset')
        verticalApertureOffset = usdAttr.Get()

        self.assertAlmostEqual(self.mmToCm * verticalApertureOffset, self.inchesToCm * ufeCamera.verticalApertureOffset())

        # set the vertical aperture offset using UFE
        ufeCamera.verticalApertureOffset(0.1)
        self.assertAlmostEqual(0.1, ufeCamera.verticalApertureOffset())

        # set the vertical aperture offset using USD
        usdAttr.Set(0.5)
        self.assertAlmostEqual(self.mmToCm * 0.5, self.inchesToCm * ufeCamera.verticalApertureOffset())
    
    def _TestFStop(self, ufeCamera, usdCamera):
        usdAttr = usdCamera.GetAttribute('fStop')
        fStop = usdAttr.Get()

        # precision error here from converting units so use a less precise comparison, 6 digits instead of 7
        self.assertAlmostEqual(fStop, self.mmToCm * ufeCamera.fStop(), 6)

        # set the f-stop offset using UFE
        ufeCamera.fStop(8.)
        self.assertAlmostEqual(8., ufeCamera.fStop())

        # set the f-stop offset using USD
        usdAttr.Set(0.5)
        self.assertAlmostEqual(0.5, self.mmToCm * ufeCamera.fStop(), 6)

    def _TestFocalLength(self, ufeCamera, usdCamera):
        usdAttr = usdCamera.GetAttribute('focalLength')
        focalLength = usdAttr.Get()

        self.assertAlmostEqual(self.mmToCm * focalLength, self.mmToCm * ufeCamera.focalLength())

        # set the focal length offset using UFE
        ufeCamera.focalLength(12.)
        self.assertAlmostEqual(12., ufeCamera.focalLength())

        # set the focal length offset using USD
        usdAttr.Set(0.5)
        self.assertAlmostEqual(self.mmToCm * 0.5, self.mmToCm * ufeCamera.focalLength())

    def _TestFocusDistance(self, ufeCamera, usdCamera):
        usdAttr = usdCamera.GetAttribute('focusDistance')
        focusDistance = usdAttr.Get()

        self.assertAlmostEqual(self.mmToCm * focusDistance, self.mmToCm * ufeCamera.focusDistance())

        # set the focus distance offset using UFE
        ufeCamera.focusDistance(9.)
        self.assertAlmostEqual(9., ufeCamera.focusDistance())

        # set the focus distance offset using USD
        usdAttr.Set(0.5)
        self.assertAlmostEqual(self.mmToCm * 0.5, self.mmToCm * ufeCamera.focusDistance())

    def _TestProjection(self, ufeCamera, usdCamera):
        usdAttr = usdCamera.GetAttribute('projection')
        projection = usdAttr.Get()

        self.assertEqual(ufe.Camera.Perspective, ufeCamera.projection())

        # set the projection offset using UFE
        ufeCamera.projection(ufe.Camera.Orthographic)
        self.assertAlmostEqual(ufe.Camera.Orthographic, ufeCamera.projection())

        # set the projection offset using USD
        usdAttr.Set("perspective")
        self.assertAlmostEqual(ufe.Camera.Perspective, ufeCamera.projection())
        
    def _TestRenderable(self, ufeCamera, usdCamera):
        cam = UsdGeom.Imageable(usdCamera)
        usdPurposeAttr = cam.GetPurposeAttr()
        camAttributes = ufe.Attributes.attributes(ufeCamera.sceneItem())
        purposeAttr = camAttributes.attribute('purpose')

        self.assertTrue(ufeCamera.renderable())

        # set the purpose to "guide" using UFE
        purposeAttr.set("guide")
        self.assertFalse(ufeCamera.renderable())

        # set the purpose to "default" using USD
        usdPurposeAttr.Set("default")
        self.assertTrue(ufeCamera.renderable())
        
        # set the purpose to "render" using USD
        usdPurposeAttr.Set("render")
        self.assertTrue(ufeCamera.renderable())

    @unittest.skipUnless(mayaUtils.mayaMajorVersion() >= 2023, 'Requires Python API only available in Maya 2023 or greater.')
    def testUsdCamera(self):
        cam2Item = ufeUtils.createItem(self.camera2PathString)
        cam2Camera = ufe.Camera.camera(cam2Item)
        cameraPrim = usdUtils.getPrimFromSceneItem(cam2Item)
        self._TestCameraAttributes(cam2Camera, cameraPrim)

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 4, 'Requires Ufe v4.')
    def testUsdCameraHandler(self):
        # Test that the camera handlers can find USD cameras in a scene segment
        camera2Path = ufe.PathString.path(self.camera2PathString)
        camera1Path = ufe.PathString.path('|stage|stageShape,/cameras/cam1/camera1')

        proxyShapePath = ufe.PathString.path('|stage|stageShape')
        proxyShapeParentPath = ufe.PathString.path('|stage')
        camerasParentPath = ufe.PathString.path('|stage|stageShape,/cameras')
        camera2ParentPath = ufe.PathString.path('|stage|stageShape,/cameras/cam2')
        geoPath = ufe.PathString.path('|stage|stageShape,/GEO')

        # searching on a gateway item should give all cameras in the scene segment
        # this will test ProxyShapeCameraHandler
        result = ufe.CameraHandler.findAll(proxyShapePath)
        self.assertTrue(result.contains(camera1Path))
        self.assertTrue(result.contains(camera2Path))
        self.assertEqual(len(result), 2)

        # searching the the parent of a gateway item searches the Maya scene segment
        # for cameras and searches nested scene segments. This doesn't find any Maya
        # cameras because the Maya camera handler is not implemented, and there are no
        # Maya cameras which are children of proxyShapeParentPath.
        result = ufe.CameraHandler.findAll(proxyShapeParentPath)
        self.assertTrue(result.contains(camera1Path))
        self.assertTrue(result.contains(camera2Path))
        self.assertEqual(len(result), 2)

        # searching for the USD parent of both cameras should find both cameras.
        # this will test UsdCameraHandler
        result = ufe.CameraHandler.findAll(camerasParentPath)
        self.assertTrue(result.contains(camera1Path))
        self.assertTrue(result.contains(camera2Path))
        self.assertEqual(len(result), 2)
        
        # searching for the parent of only camera2 should find just camera2
        result = ufe.CameraHandler.findAll(camera2ParentPath)
        self.assertFalse(result.contains(camera1Path))
        self.assertTrue(result.contains(camera2Path))
        self.assertEqual(len(result), 1)

        # search for the parent of neither camera should find no cameras
        result = ufe.CameraHandler.findAll(geoPath)
        self.assertTrue(result.empty())

    @unittest.skipUnless(mayaUtils.mayaMajorMinorVersions() >= (2025, 2), 'Maya USD camera manipulator is only supported in 2025.2 or greater.')
    def testUsdCameraManipulators(self):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")
        testFile = testUtils.getTestScene("camera", 'cameraManipulator.usda')
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()

        # create some strings
        cameraManPathString = '|stage|stageShape,/Camera1'

        camItem = ufeUtils.createItem(cameraManPathString)
        cameraPrim = usdUtils.getPrimFromSceneItem(camItem)
    
        translateAttr = cameraPrim.GetAttribute("xformOp:translate")
        self.assertIsNotNone(translateAttr)
        tranlateBeforeEdit = translateAttr.Get()
        print(tranlateBeforeEdit)

        # dolly
        cmds.dolly( cameraManPathString, d = -3 )
        translateAttr = cameraPrim.GetAttribute("xformOp:translate")
        self.assertIsNotNone(translateAttr)
        self.assertEqual(translateAttr.Get(), Gf.Vec3d(0.0, 0.0, -3.0))

        #track
        cmds.track( cameraManPathString, d = 10 )
        translateAttr = cameraPrim.GetAttribute("xformOp:translate")
        self.assertIsNotNone(translateAttr)
        self.assertEqual(translateAttr.Get(), Gf.Vec3d(0.0, -10.0, 0.0))

if __name__ == '__main__':
    unittest.main(verbosity=2)
