#!/usr/bin/env mayapy
#
# Copyright 2026 Autodesk
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
import imageUtils
import mayaUtils
import testUtils

from maya import cmds

try:
    from PySide2 import QtCore
    from PySide2.QtWidgets import QWidget
    from shiboken2 import wrapInstance
except Exception:
    from PySide6 import QtCore
    from PySide6.QtWidgets import QWidget
    from shiboken6 import wrapInstance

from maya.api import OpenMayaUI as OMUI
from maya.api import OpenMaya as OM2
import maya.OpenMaya as OM1
import maya.OpenMayaUI as OMUI1

import qtInputHelpers

from pxr import Gf, Sdf, Usd, UsdVol, Vt

import os

# Synthetic splat samples under test/testSamples/3dgs.
SAMPLES = {
    'twoOverlapping': 'twoOverlappingSplats.usda',
    'anisotropic': 'anisotropicSplat.usda',
    'shDegree3': 'shDegree3Splat.usda',
    'noExtent': 'noExtentSplat.usda',
    'empty': 'emptySplat.usda',
}

SPLAT_PRIM_PATH = '/Splat'


def samplePath(key):
    return testUtils.getTestScene('3dgs', SAMPLES[key])


class testVP2RenderDelegateGaussianSplat(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging of UsdVol ParticleField3DGaussianSplat prims using the
    Viewport 2.0 render delegate.
    """

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(
            __file__, initializeStandalone=False, loadPlugin=False)
        cls._baselineDir = os.path.join(
            inputPath, 'VP2RenderDelegateGaussianSplatTest', 'baseline')
        cls._testDir = os.path.abspath('.')

    def setUp(self):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin('mayaUsdPlugin')
        self._extraPanels = []
        self._snapshotCount = 0

    def tearDown(self):
        self.deleteExtraPanes()

    # ------------------------------------------------------------------
    # Scene helpers
    # ------------------------------------------------------------------

    def createProxy(self, filePath, name='stageShape'):
        """Create a proxy shape on a USD file and return its shape name."""
        shape = cmds.createNode('mayaUsdProxyShape', name=name)
        cmds.setAttr('{}.filePath'.format(shape), filePath, type='string')
        cmds.select(clear=True)
        return shape

    def createSplatStage(self, fileName, positions, scales=None,
                         orientations=None, opacities=None, colors=None,
                         shDegree=0, shCoeffs=None, extent=None,
                         sortingModeHint=None):
        """
        Author a splat prim at SPLAT_PRIM_PATH from arrays and save the stage
        in the test directory. Returns the stage file path. Per-splat arrays
        default to unit scale, identity orientation, full opacity and white.
        Pass extent=None to author no extent.
        """
        count = len(positions)
        scales = scales or [(0.3, 0.3, 0.3)] * count
        orientations = orientations or [(1, 0, 0, 0)] * count
        opacities = opacities or [1.0] * count
        if shCoeffs is None:
            shCoeffs = colors or [(1, 1, 1)] * count

        path = os.path.join(self._testDir, fileName)
        stage = Usd.Stage.CreateNew(path)
        prim = stage.DefinePrim(SPLAT_PRIM_PATH, 'ParticleField3DGaussianSplat')
        stage.SetDefaultPrim(prim)

        prim.CreateAttribute('positions', Sdf.ValueTypeNames.Point3fArray).Set(
            Vt.Vec3fArray([Gf.Vec3f(*p) for p in positions]))
        prim.CreateAttribute('orientations', Sdf.ValueTypeNames.QuatfArray).Set(
            Vt.QuatfArray([Gf.Quatf(*q) for q in orientations]))
        prim.CreateAttribute('scales', Sdf.ValueTypeNames.Float3Array).Set(
            Vt.Vec3fArray([Gf.Vec3f(*s) for s in scales]))
        prim.CreateAttribute('opacities', Sdf.ValueTypeNames.FloatArray).Set(
            Vt.FloatArray(opacities))
        prim.CreateAttribute(
            'radiance:sphericalHarmonicsDegree', Sdf.ValueTypeNames.Int).Set(shDegree)
        prim.CreateAttribute(
            'radiance:sphericalHarmonicsCoefficients',
            Sdf.ValueTypeNames.Float3Array).Set(
                Vt.Vec3fArray([Gf.Vec3f(*c) for c in shCoeffs]))
        if extent is not None:
            prim.CreateAttribute('extent', Sdf.ValueTypeNames.Float3Array).Set(
                Vt.Vec3fArray([Gf.Vec3f(*extent[0]), Gf.Vec3f(*extent[1])]))
        if sortingModeHint is not None:
            prim.CreateAttribute(
                'sortingModeHint', Sdf.ValueTypeNames.Token,
                Sdf.VariabilityUniform).Set(sortingModeHint)

        stage.GetRootLayer().Save()
        del stage
        return path

    # ------------------------------------------------------------------
    # Camera, display style
    # ------------------------------------------------------------------

    def setCamera(self, camera='persp', translate=(0, 0, 5), rotate=(0, 0, 0),
                  panel=None):
        """Place a camera and make the panel look through it."""
        cmds.setAttr('{}.translate'.format(camera), *translate, type='float3')
        cmds.setAttr('{}.rotate'.format(camera), *rotate, type='float3')
        cmds.modelEditor(panel or mayaUtils.activeModelPanel(), edit=True,
                         grid=False)
        if panel:
            cmds.modelPanel(panel, edit=True, camera=camera)

    def setDisplayStyle(self, style, panel=None):
        """style: 'wireframe', 'bounds' or 'shaded'."""
        panel = panel or mayaUtils.activeModelPanel()
        cmds.modelEditor(
            panel, edit=True,
            displayAppearance={'shaded': 'smoothShaded'}.get(style, style))

    # ------------------------------------------------------------------
    # Model panes
    # ------------------------------------------------------------------

    def createModelPane(self, camera='persp'):
        """
        Create an additional model panel (floating window) looking through the
        given camera. Returns the panel name. Deleted in tearDown.
        """
        window = cmds.window(widthHeight=(400, 400))
        cmds.paneLayout()
        panel = cmds.modelPanel(camera=camera)
        cmds.showWindow(window)
        # A new panel starts in wireframe, where the splat item (shaded only) is not drawn.
        cmds.modelEditor(panel, edit=True, displayAppearance='smoothShaded',
                         displayTextures=True, grid=False)
        self._extraPanels.append((panel, window))
        return panel

    def createModelPanes(self, count, camera='persp'):
        return [self.createModelPane(camera) for _ in range(count)]

    def deleteExtraPanes(self):
        for panel, window in self._extraPanels:
            if cmds.window(window, exists=True):
                cmds.deleteUI(window, window=True)
        self._extraPanels = []

    def modelPanes(self):
        return cmds.getPanel(type='modelPanel') or []

    # ------------------------------------------------------------------
    # Frames and pixels
    # ------------------------------------------------------------------

    def captureOneFrame(self, panel=None):
        """
        Force exactly one refresh of the panel (or all panes) and return after
        the idle queue has been flushed, so the change under test is drawn
        once.
        """
        cmds.flushIdleQueue()
        if panel:
            cmds.refresh(panel, force=True)
        else:
            cmds.refresh(force=True)

    def snapshotImage(self, name=None, width=256, height=256):
        """Playblast the active pane to a PNG and return a loaded QImage."""
        try:
            from PySide2.QtGui import QImage
        except ImportError:
            from PySide6.QtGui import QImage
        self._snapshotCount += 1
        name = name or 'splat_{}.png'.format(self._snapshotCount)
        outPath = os.path.join(self._testDir, name)
        imageUtils.snapshot(outPath, width=width, height=height)
        image = QImage()
        self.assertTrue(image.load(outPath), 'cannot load ' + outPath)
        return image

    def samplePixels(self, image, points):
        """Return [(r, g, b)] for each (x, y) point, 0-255 per channel."""
        result = []
        for x, y in points:
            c = image.pixelColor(x, y)
            result.append((c.red(), c.green(), c.blue()))
        return result

    def samplePixel(self, image, x, y):
        return self.samplePixels(image, [(x, y)])[0]

    # ------------------------------------------------------------------
    # Placeholder tests: every sample loads. Real tests (testXxx)
    # come in later batches.
    # ------------------------------------------------------------------

    def testSamplesLoad(self):
        for key in sorted(SAMPLES):
            with self.subTest(sample=key):
                cmds.file(force=True, new=True)
                mayaUtils.loadPlugin('mayaUsdPlugin')
                path = samplePath(key)
                self.assertTrue(os.path.exists(path), path)

                shape = self.createProxy(path)
                stage = Usd.Stage.Open(path)
                prim = stage.GetPrimAtPath(SPLAT_PRIM_PATH)
                self.assertTrue(prim.IsValid())
                self.assertEqual(prim.GetTypeName(), 'ParticleField3DGaussianSplat')

                positions = prim.GetAttribute('positions').Get()
                self.assertEqual(len(positions), 0 if key == 'empty' else
                                 {'twoOverlapping': 2, 'noExtent': 2}.get(key, 1))
                self.assertEqual(
                    prim.GetAttribute('extent').HasAuthoredValue(),
                    key not in ('noExtent', 'empty'))

                self.captureOneFrame()
                self.assertTrue(cmds.objExists(shape))

    def testHelpersBuildStage(self):
        path = self.createSplatStage('helperSplat.usda', [(0, 0, 0)])
        shape = self.createProxy(path)
        self.setCamera(translate=(0, 0, 3))
        self.captureOneFrame()
        image = self.snapshotImage(width=64, height=64)
        self.assertEqual(len(self.samplePixels(image, [(32, 32), (0, 0)])), 2)
        self.assertTrue(cmds.objExists(shape))

    # ------------------------------------------------------------------
    # Shape and opacity (batch E+F)
    # ------------------------------------------------------------------

    # Raw SH coefficients that give a pure red splat: color = 0.5 + C0 * coeff,
    # clamped to 0..1.
    RED = (2.0, -2.0, -2.0)

    @staticmethod
    def isRed(pixel):
        r, g, b = pixel
        return r > 150 and g < 90 and b < 90 and r - g > 80

    def redExtents(self, image):
        """Return (width, height) of the red pixels through the image center."""
        cx, cy = image.width() // 2, image.height() // 2
        row = sum(1 for x in range(image.width())
                  if self.isRed(self.samplePixel(image, x, cy)))
        col = sum(1 for y in range(image.height())
                  if self.isRed(self.samplePixel(image, cx, y)))
        return row, col

    def drawSplat(self, fileName, distance=5, remove=(), **kwargs):
        """Author a one-splat stage (removing the given attributes), draw it
        and return the snapshot."""
        kwargs.setdefault('colors', [self.RED])
        path = self.createSplatStage(fileName, [(0, 0, 0)], **kwargs)
        if remove:
            stage = Usd.Stage.Open(path)
            prim = stage.GetPrimAtPath(SPLAT_PRIM_PATH)
            for name in remove:
                prim.RemoveProperty(name)
            stage.GetRootLayer().Save()
            del stage
        self.createProxy(path)
        self.setCamera(translate=(0, 0, distance))
        self.captureOneFrame()
        return self.snapshotImage(name=fileName.replace('.usda', '.png'))

    def testAnisotropicSplatFollowsOrientation(self):
        scales = [(0.4, 0.07, 0.07)]
        # Identity orientation: elongated along x.
        image = self.drawSplat('tp22_identity.usda', scales=scales)
        width, height = self.redExtents(image)
        self.assertGreater(width, 3 * height, (width, height))
        self.assertGreater(height, 0)

        # A quarter turn around z (USD quaternion order w, x, y, z): elongated along y.
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin('mayaUsdPlugin')
        half = 0.7071067811865476
        image = self.drawSplat('tp22_rotated.usda', scales=scales,
                               orientations=[(half, 0, 0, half)])
        width, height = self.redExtents(image)
        self.assertGreater(height, 3 * width, (width, height))
        self.assertGreater(width, 0)

    def testMissingShapeStreamsUseDefaults(self):
        # No scales and no orientations authored: a default isotropic splat is drawn.
        image = self.drawSplat('tp23.usda', distance=20,
                               remove=('scales', 'orientations'))
        width, height = self.redExtents(image)
        self.assertGreater(width, 5, (width, height))
        self.assertLessEqual(abs(width - height), 2, (width, height))

    def testPartialOpacityBlends(self):
        # Color management is off: the pixel expectations are raw (unconverted) colors.
        self.disableColorManagement()
        scales = [(0.5, 0.5, 0.5)]
        opaque = self.drawSplat('tp24_opaque.usda', scales=scales, opacities=[1.0])
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin('mayaUsdPlugin')
        self.disableColorManagement()
        half = self.drawSplat('tp24_half.usda', scales=scales, opacities=[0.5])
        cx = cy = 128
        r1, g1, _ = self.samplePixel(opaque, cx, cy)
        r2, g2, _ = self.samplePixel(half, cx, cy)
        # Opaque center is (almost) pure red, the half opaque one is red mixed with the grey
        # background (92): less red, some green.
        self.assertGreater(r1, 230)
        self.assertLess(g1, 20)
        self.assertTrue(130 < r2 < 215, r2)
        self.assertTrue(20 < g2 < 80, g2)

    def testMissingOpacityIsOpaque(self):
        # Color management is off: the pixel expectations are raw (unconverted) colors.
        self.disableColorManagement()
        image = self.drawSplat('tp25.usda', scales=[(0.5, 0.5, 0.5)],
                               remove=('opacities',))
        r, g, _ = self.samplePixel(image, 128, 128)
        self.assertGreater(r, 230)
        self.assertLess(g, 20)

    # ------------------------------------------------------------------
    # Per-Rprim shader and teardown (batch G1)
    # ------------------------------------------------------------------

    def testEmptyPrimDrawsNothing(self):
        # Empty prim: nothing is drawn in any style, neither splats nor a box (no positions).
        emptyStage = self.createSplatStage(
            'tp39_empty.usda', [], extent=((-1, -1, -1), (1, 1, 1)))
        emptyShape = self.createProxy(emptyStage, name='emptyShape')
        panel = mayaUtils.activeModelPanel()
        self.setCamera(translate=(0, 0, 5))
        for style in ('shaded', 'wireframe', 'boundingBox'):
            self.setStyle(panel, style)
            self.captureOneFrame()
            image = self.snapshotPanel(panel, 'tp39_empty_{}.png'.format(style))
            self.assertEqual(self.countColor(image, self.isRed), 0, style)
            self.assertEqual(self.boxBounds(image), None, style)
        cmds.delete(cmds.listRelatives(emptyShape, parent=True)[0])
        self.setStyle(panel, 'shaded')

        # Smoke part: deleting the node and "file -new" after a draw must not
        # crash (the shader clone is released by a later commit).
        path = self.createSplatStage('tp39.usda', [(0, 0, 0)], colors=[self.RED])
        shape = self.createProxy(path)
        self.captureOneFrame()
        cmds.delete(cmds.listRelatives(shape, parent=True)[0])
        self.captureOneFrame()
        self.captureOneFrame()

        shape = self.createProxy(path, name='stageShape2')
        self.captureOneFrame()
        cmds.file(force=True, new=True)
        self.captureOneFrame()
        self.assertFalse(cmds.objExists(shape))

    # ------------------------------------------------------------------
    # Pre-draw callback: panes, passes, selection (batch G2)
    # ------------------------------------------------------------------

    # Camera on the +z axis looking down -z, and its mirror on the -z axis.
    FRONT = {'translate': (0, 0, 5), 'rotate': (0, 0, 0)}
    BACK = {'translate': (0, 0, -5), 'rotate': (0, 180, 0)}

    def createCamera(self, name, placement):
        """Create a camera with the given placement and return its transform."""
        transform = cmds.camera(name=name)[0]
        cmds.setAttr('{}.translate'.format(transform), *placement['translate'],
                     type='float3')
        cmds.setAttr('{}.rotate'.format(transform), *placement['rotate'],
                     type='float3')
        cmds.select(clear=True)
        return transform

    def snapshotPanel(self, panel, name):
        """Read back the color buffer of a given panel (a playblast would only render the
        active one) and return it as a QImage. The panel is refreshed first."""
        try:
            from PySide2.QtGui import QImage
        except ImportError:
            from PySide6.QtGui import QImage
        cmds.setFocus(panel)
        cmds.modelEditor(panel, edit=True, grid=False)
        cmds.flushIdleQueue()
        view = OMUI1.M3dView()
        OMUI1.M3dView.getM3dViewFromModelEditor(panel, view)
        view.refresh(False, True)
        mimage = OM1.MImage()
        view.readColorBuffer(mimage, True)
        outPath = os.path.join(self._testDir, name)
        mimage.writeToFile(outPath, 'png')
        image = QImage()
        self.assertTrue(image.load(outPath), 'cannot load ' + outPath)
        return image

    def overlapPixel(self, image):
        """Color in the overlap of the two splats."""
        return self.samplePixel(image, image.width() // 2, image.height() * 5 // 8)

    def assertImagesEqual(self, a, b, tolerance=8):
        """Same size and every pixel within `tolerance` per channel."""
        self.assertEqual((a.width(), a.height()), (b.width(), b.height()))
        worst = 0
        where = None
        for y in range(0, a.height(), 2):
            for x in range(0, a.width(), 2):
                pa = self.samplePixel(a, x, y)
                pb = self.samplePixel(b, x, y)
                diff = max(abs(i - j) for i, j in zip(pa, pb))
                if diff > worst:
                    worst, where = diff, (x, y, pa, pb)
        self.assertLessEqual(worst, tolerance, where)

    def drawOverlapping(self):
        path = samplePath('twoOverlapping')
        return self.createProxy(path)

    def drawOverlappingRaw(self):
        """The overlapping sample, with its coefficients declared in the rendering space so that
        no color conversion applies: the pixel expectations of the tests that use it are
        raw colors, and they test sorting and highlight, not color management. (Turning color
        management off changes the pixel balance these thresholds were tuned on.)"""
        stage = Usd.Stage.Open(samplePath('twoOverlapping'))
        stage.GetPrimAtPath(SPLAT_PRIM_PATH).GetAttribute(
            'radiance:sphericalHarmonicsCoefficients').SetColorSpace(self.renderingSpace())
        path = os.path.join(self._testDir, 'twoOverlappingRaw.usda')
        stage.Export(path)
        del stage
        return self.createProxy(path)

    def testTwoPanesMatchSinglePaneReferences(self):
        # Splat 0 (red) is at z=0, splat 1 (blue) at z=0.2: from the front the blue one is
        # on top, from the back the red one.
        self.drawOverlapping()
        mainPanel = mayaUtils.activeModelPanel()
        front = self.createCamera('camFront', self.FRONT)
        back = self.createCamera('camBack', self.BACK)

        # Two panes. A playblast depends on the aspect of its pane, so each reference is taken
        # in the same pane as the image it is compared to, with both panes looking through
        # the same camera (one view only: no other camera can interfere).
        otherPanel = self.createModelPane(camera=front)
        cmds.modelPanel(mainPanel, edit=True, camera=front)
        cmds.modelPanel(otherPanel, edit=True, camera=front)
        self.captureOneFrame()
        refFront = self.snapshotPanel(mainPanel, 'tp26_ref_front.png')
        cmds.modelPanel(mainPanel, edit=True, camera=back)
        cmds.modelPanel(otherPanel, edit=True, camera=back)
        self.captureOneFrame()
        refBack = self.snapshotPanel(otherPanel, 'tp26_ref_back.png')

        # The two cameras must see a different order.
        rf, _, bf = self.overlapPixel(refFront)
        rb, _, bb = self.overlapPixel(refBack)
        self.assertGreater(bf, rf, (rf, bf))
        self.assertGreater(rb, bb, (rb, bb))

        # Two cameras at once, drawn alternately: each pane must still match its reference.
        cmds.modelPanel(mainPanel, edit=True, camera=front)
        cmds.modelPanel(otherPanel, edit=True, camera=back)
        self.captureOneFrame()
        for round_ in range(2):
            paneFront = self.snapshotPanel(mainPanel, 'tp26_pane_front{}.png'.format(round_))
            paneBack = self.snapshotPanel(otherPanel, 'tp26_pane_back{}.png'.format(round_))
            self.assertImagesEqual(paneFront, refFront)
            self.assertImagesEqual(paneBack, refBack)

    def testSelectionPassLeavesBeautyUnchanged(self):
        self.drawOverlapping()
        panel = mayaUtils.activeModelPanel()
        front = self.createCamera('camFront', self.FRONT)
        cmds.modelPanel(panel, edit=True, camera=front)
        before = self.snapshotPanel(panel, 'tp28_before.png')
        r, _, b = self.overlapPixel(before)
        self.assertGreater(b, r, (r, b))

        # A marquee in the corner, away from the splats: a selection pass runs and
        # selects nothing.
        view = OMUI.M3dView.active3dView()
        widget = wrapInstance(int(view.widget()), QWidget)
        widget.update()
        QtCore.QCoreApplication.processEvents(
            QtCore.QEventLoop.ProcessEventsFlag.AllEvents, 10)
        topLeft = widget.rect().topLeft() + QtCore.QPoint(2, 2)
        qtInputHelpers.send_mouse_drag(widget, topLeft, topLeft + QtCore.QPoint(15, 15))
        QtCore.QCoreApplication.processEvents(
            QtCore.QEventLoop.ProcessEventsFlag.AllEvents, 10)
        self.assertFalse(cmds.ls(selection=True))

        after = self.snapshotPanel(panel, 'tp28_after.png')
        self.assertImagesEqual(before, after)

    def testOpaqueHighlightStillSorted(self):
        # A selected prim with an opaque highlight color is still drawn sorted, from both
        # sides, as transparent geometry, and its splats show the highlight tint.
        shape = self.drawOverlappingRaw()
        panel = mayaUtils.activeModelPanel()
        front = self.createCamera('camFront', self.FRONT)
        back = self.createCamera('camBack', self.BACK)

        cmds.modelPanel(panel, edit=True, camera=front)
        plainFront = self.snapshotPanel(panel, 'tp29_plain_front.png')

        cmds.select(cmds.listRelatives(shape, parent=True)[0])
        selFront = self.snapshotPanel(panel, 'tp29_front.png')
        cmds.modelPanel(panel, edit=True, camera=back)
        selBack = self.snapshotPanel(panel, 'tp29_back.png')

        # Sorted from both sides: the red-blue balance flips (the tint moves both splats toward
        # green, so only the difference is compared).
        rf, _, bf = self.overlapPixel(selFront)
        rb, _, bb = self.overlapPixel(selBack)
        self.assertGreater((rb - bb) - (rf - bf), 25, ((rf, bf), (rb, bb)))

        # The highlight color half: the overlap pixel moved toward the highlight color (the
        # default lead color is a light green, so green goes up by a few levels at the 5% tint)
        # while the order is kept.
        self.assertGreater(self.overlapPixel(selFront)[1], self.overlapPixel(plainFront)[1] + 3)

    # ------------------------------------------------------------------
    # Display styles and items (batch H)
    # ------------------------------------------------------------------

    def setStyle(self, panel, style, textures=True, wireOnShaded=False,
                 defaultMaterial=False):
        """style: 'shaded', 'wireframe' or 'boundingBox'."""
        appearance = {'shaded': 'smoothShaded'}.get(style, style)
        cmds.modelEditor(panel, edit=True, displayAppearance=appearance,
                         displayTextures=textures, wireframeOnShaded=wireOnShaded,
                         useDefaultMaterial=defaultMaterial, grid=False)

    @staticmethod
    def isBlue(pixel):
        r, g, b = pixel
        return b > 140 and r < 90 and g < 90

    @staticmethod
    def interior(image):
        """Pixel ranges of the image without the corner widgets (axis tripod, view cube) and
        the camera name at the bottom."""
        return (range(150, image.width() - 150), range(60, image.height() - 70))

    def center(self, image):
        return image.width() // 2, image.height() // 2

    def countColor(self, image, test):
        xr, yr = self.interior(image)
        return sum(1 for y in yr for x in xr if test(self.samplePixel(image, x, y)))

    def boxBounds(self, image, minPixels=20):
        """Bounding rectangle (x0, y0, x1, y1) of the blue box lines, or None."""
        xs, ys = [], []
        xr, yr = self.interior(image)
        for y in yr:
            for x in xr:
                if self.isBlue(self.samplePixel(image, x, y)):
                    xs.append(x)
                    ys.append(y)
        if len(xs) < minPixels:
            return None
        return min(xs), min(ys), max(xs), max(ys)

    BOX = ((-1, -1, -1), (1, 1, 1))

    def drawRedSplat(self, fileName, extent=BOX, positions=((0, 0, 0),), **kwargs):
        """One red splat scene at distance 5 in the active panel; returns the panel."""
        path = self.createSplatStage(fileName, list(positions), colors=[self.RED] * len(positions),
                                     extent=extent, scales=[(0.3, 0.3, 0.3)] * len(positions),
                                     **kwargs)
        self.createProxy(path)
        self.setCamera(translate=(0, 0, 5))
        return mayaUtils.activeModelPanel()

    def centerPixel(self, image):
        return self.samplePixel(image, *self.center(image))

    def shot(self, panel, name):
        self.captureOneFrame()
        return self.snapshotPanel(panel, name)

    def testWireframeAloneDrawsBoxOnly(self):
        panel = self.drawRedSplat('tp33.usda')
        self.setStyle(panel, 'wireframe')
        image = self.shot(panel, 'tp33.png')
        self.assertIsNotNone(self.boxBounds(image))
        self.assertEqual(self.countColor(image, self.isRed), 0)

    def testWireframeOnShadedDrawsBoth(self):
        panel = self.drawRedSplat('tp34.usda')
        self.setStyle(panel, 'shaded', wireOnShaded=True)
        image = self.shot(panel, 'tp34.png')
        self.assertIsNotNone(self.boxBounds(image))
        self.assertGreater(self.countColor(image, self.isRed), 50)

    def testBboxStyleDrawsAuthoredExtentOnly(self):
        # The splat sits at the origin with a small scale, the authored extent is much larger
        # than the splat: the box size shows that the authored extent is used.
        panel = self.drawRedSplat('tp35_small.usda', extent=((-0.5, -0.5, -0.5), (0.5, 0.5, 0.5)))
        self.setStyle(panel, 'boundingBox')
        small = self.shot(panel, 'tp35_small.png')
        self.assertEqual(self.countColor(small, self.isRed), 0)
        boxSmall = self.boxBounds(small)
        self.assertIsNotNone(boxSmall)

        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin('mayaUsdPlugin')
        panel = self.drawRedSplat('tp35_big.usda', extent=self.BOX)
        self.setStyle(panel, 'boundingBox')
        big = self.shot(panel, 'tp35_big.png')
        boxBig = self.boxBounds(big)
        self.assertIsNotNone(boxBig)
        widthSmall = boxSmall[2] - boxSmall[0]
        widthBig = boxBig[2] - boxBig[0]
        self.assertGreater(widthBig, 1.5 * widthSmall, (widthSmall, widthBig))
        self.assertLess(widthBig, 3.0 * widthSmall, (widthSmall, widthBig))
        self.assertGreater(widthSmall, 20, widthSmall)

    def testNoExtentDrawsNoBox(self):
        path = self.createSplatStage('tp36.usda', [(0, 0, 0)], colors=[self.RED], extent=None)
        self.createProxy(path)
        self.setCamera(translate=(0, 0, 5))
        panel = mayaUtils.activeModelPanel()
        self.setStyle(panel, 'wireframe')
        self.assertIsNone(self.boxBounds(self.shot(panel, 'tp36_wire.png')))
        self.setStyle(panel, 'boundingBox')
        self.assertIsNone(self.boxBounds(self.shot(panel, 'tp36_bbox.png')))
        # The splats are still drawn in the shaded style.
        self.setStyle(panel, 'shaded')
        shaded = self.shot(panel, 'tp36_shaded.png')
        self.assertGreater(self.countColor(shaded, self.isRed), 50)

    def testCountChangeKeepsSortedOrder(self):
        # Splat 0 (red) is at z=0, splat 1 (blue) at z=0.2. From the back the red one is on
        # top, which is the reverse of the index order: an index buffer left in its identity
        # order would put the blue one on top.
        self.drawOverlappingRaw()
        panel = mayaUtils.activeModelPanel()
        cmds.modelPanel(panel, edit=True, camera=self.createCamera('camBack', self.BACK))
        self.captureOneFrame()
        r, _, b = self.overlapPixel(self.snapshotPanel(panel, 'tp51_before.png'))
        self.assertGreater(r, b, (r, b))

        # A third, green splat, away from the overlap. The count change resizes the index
        # buffer, which must then be sorted again.
        import mayaUsd.lib
        stage = mayaUsd.lib.GetPrim(cmds.ls('stageShape', long=True)[0]).GetStage()
        prim = stage.GetPrimAtPath(SPLAT_PRIM_PATH)
        with Sdf.ChangeBlock():
            for name, value in (
                    ('positions', Gf.Vec3f(0, 0.8, 0.1)),
                    ('orientations', Gf.Quatf(1, 0, 0, 0)),
                    ('scales', Gf.Vec3f(0.2, 0.2, 0.2)),
                    ('opacities', 0.8),
                    ('radiance:sphericalHarmonicsCoefficients', Gf.Vec3f(0, 1, 0))):
                attr = prim.GetAttribute(name)
                values = list(attr.Get())
                values.append(value)
                attr.Set(values)
        self.captureOneFrame()
        after = self.snapshotPanel(panel, 'tp51_after.png')

        r, _, b = self.overlapPixel(after)
        self.assertGreater(r, b, (r, b))
        # The new splat is drawn. Its blended green is pale, so the margin is small: none of
        # the pixels before the edit has any.
        isGreen = lambda p: p[1] > max(p[0], p[2]) + 10
        self.assertGreater(self.countColor(after, isGreen), 100)

    def testPositionEditKeepsAuthoredBox(self):
        panel = self.drawRedSplat('tp37.usda', positions=((0, 0, 0),))
        self.setStyle(panel, 'shaded', wireOnShaded=True)
        before = self.shot(panel, 'tp37_before.png')
        boxBefore = self.boxBounds(before)
        self.assertIsNotNone(boxBefore)

        # Move the splat far from the center, without touching the extent.
        import mayaUsd.lib
        stage = mayaUsd.lib.GetPrim(cmds.ls('stageShape', long=True)[0]).GetStage()
        prim = stage.GetPrimAtPath('/Splat')
        prim.GetAttribute('positions').Set(Vt.Vec3fArray([Gf.Vec3f(0.7, 0.7, 0)]))
        after = self.shot(panel, 'tp37_after.png')
        self.assertEqual(self.boxBounds(after), boxBefore)

        # The splat did move (its red pixels are no longer centered).
        def centroidX(image):
            xr, yr = self.interior(image)
            xs = [x for y in yr for x in xr
                  if self.isRed(self.samplePixel(image, x, y))]
            return sum(xs) / max(1, len(xs))
        self.assertGreater(centroidX(after), centroidX(before) + 10)

    def testTogglingStylesDrawsOneSet(self):
        # Half opaque splats: a set drawn twice would visibly change the blended color.
        path = self.createSplatStage(
            'tp38.usda', [(0, 0, 0)], colors=[self.RED], opacities=[0.5],
            scales=[(0.5, 0.5, 0.5)], extent=self.BOX)
        self.createProxy(path)
        self.setCamera(translate=(0, 0, 5))
        panel = mayaUtils.activeModelPanel()

        self.setStyle(panel, 'shaded', textures=True)
        reference = self.shot(panel, 'tp38_ref_textured.png')
        centerRef = self.samplePixel(reference, *self.center(reference))

        # Toggle back and forth; several reprs are alive during the next frames.
        for round_ in range(3):
            self.setStyle(panel, 'shaded', textures=False)
            untextured = self.shot(panel, 'tp38_untextured{}.png'.format(round_))
            self.assertImagesEqual(untextured, reference)
            self.setStyle(panel, 'shaded', textures=True)
            textured = self.shot(panel, 'tp38_textured{}.png'.format(round_))
            self.assertImagesEqual(textured, reference)
        # And the blend is the single-draw one (not more red than the first frame).
        self.assertEqual(self.samplePixel(textured, *self.center(textured)), centerRef)

    def testStyleSwitchShowsRightColorNextFrame(self):
        path = self.createSplatStage(
            'tp27.usda', [(0, 0, 0)], colors=[self.RED], scales=[(0.5, 0.5, 0.5)],
            extent=self.BOX)
        self.createProxy(path)
        self.setCamera(translate=(0, 0, 5))
        panel = mayaUtils.activeModelPanel()
        self.setStyle(panel, 'shaded', textures=True)
        first = self.shot(panel, 'tp27_textured.png')
        self.assertTrue(self.isRed(self.samplePixel(first, *self.center(first))))
        # The first frame after each switch draws the new item with the right color.
        self.setStyle(panel, 'shaded', textures=False)
        self.assertTrue(self.isRed(self.centerPixel(self.shot(panel, 'tp27_untextured.png'))))
        self.setStyle(panel, 'wireframe')
        self.shot(panel, 'tp27_wire.png')
        self.setStyle(panel, 'shaded', textures=True)
        self.assertTrue(self.isRed(self.centerPixel(self.shot(panel, 'tp27_back.png'))))
        self.setStyle(panel, 'shaded', textures=True, defaultMaterial=True)
        self.shot(panel, 'tp27_default.png')
        self.setStyle(panel, 'shaded', textures=True)
        self.assertTrue(self.isRed(self.centerPixel(self.shot(panel, 'tp27_back2.png'))))

    def testDefaultMaterialKeepsLastOrder(self):
        self.drawOverlapping()
        panel = mayaUtils.activeModelPanel()
        front = self.createCamera('camFront', self.FRONT)
        cmds.modelPanel(panel, edit=True, camera=front)
        self.setStyle(panel, 'shaded')
        beauty = self.shot(panel, 'tp30_beauty.png')
        r, _, b = self.overlapPixel(beauty)
        self.assertGreater(b, r, (r, b))

        self.setStyle(panel, 'shaded', defaultMaterial=True)
        override = self.shot(panel, 'tp30_default.png')
        background = self.samplePixel(override, 0, 0)
        self.assertGreater(self.countColor(override, lambda p: p != background), 100,
                           'nothing is drawn under the default material')
        # Same blend order as the beauty frame: the blue splat is still in front at the overlap.
        r, _, b = self.overlapPixel(override)
        self.assertGreater(b, r, (r, b))

    def testDefaultMaterialItemIdleWhenOff(self):
        # With the default material off, only the shaded item draws: one set, so the
        # blended color equals the first single frame (a second item drawing would deepen it).
        path = self.createSplatStage(
            'tp32.usda', [(0, 0, 0)], colors=[self.RED], opacities=[0.5],
            scales=[(0.5, 0.5, 0.5)], extent=self.BOX)
        self.createProxy(path)
        self.setCamera(translate=(0, 0, 5))
        panel = mayaUtils.activeModelPanel()
        self.setStyle(panel, 'shaded')
        reference = self.shot(panel, 'tp32_ref.png')
        self.setStyle(panel, 'shaded', defaultMaterial=True)
        self.shot(panel, 'tp32_on.png')
        self.setStyle(panel, 'shaded', defaultMaterial=False)
        off = self.shot(panel, 'tp32_off.png')
        self.assertImagesEqual(off, reference)
        self.assertGreater(self.countColor(off, self.isRed), 0)

    def testForcedItemsStayDisabled(self):
        # Smoke test: a splat instance must keep drawing as a plain splat. A forced box or forced
        # untextured item that is enabled would draw a blue box, or a second half opaque set.
        proto = self.createSplatStage(
            'tp46_proto.usda', [(0, 0, 0)], colors=[self.RED], opacities=[0.5],
            scales=[(0.5, 0.5, 0.5)], extent=self.BOX)
        path = os.path.join(self._testDir, 'tp46.usda')
        stage = Usd.Stage.CreateNew(path)
        stage.DefinePrim('/Proto', 'Xform')
        stage.GetRootLayer().subLayerPaths.append(proto)
        inst = stage.DefinePrim('/Inst', 'Xform')
        inst.GetReferences().AddReference(proto, SPLAT_PRIM_PATH)
        inst.SetInstanceable(True)
        stage.GetRootLayer().Save()
        del stage
        shape = self.createProxy(path)
        proxy = cmds.ls(shape, long=True)[0]
        self.setCamera(translate=(0, 0, 5))
        panel = mayaUtils.activeModelPanel()
        self.setStyle(panel, 'shaded')
        reference = self.shot(panel, 'tp46_reference.png')
        self.assertGreater(self.countColor(reference, self.isRed), 50)

        cmds.createDisplayLayer(name='layer1', noRecurse=True)
        layer = OM2.MFnDisplayLayer(self._mayaNode('layer1'))
        layer.add(proxy + ',/Inst')
        for attr, value in (('levelOfDetail', 1), ('shading', False)):
            cmds.setAttr('layer1.drawInfo.' + attr, value)
            image = self.shot(panel, 'tp46_{}.png'.format(attr))
            self.assertIsNone(self.boxBounds(image), attr)
            self.assertImagesEqual(image, reference)
            cmds.setAttr('layer1.drawInfo.' + attr, 0 if attr == 'levelOfDetail' else True)

    # ------------------------------------------------------------------
    # Selection tint and template mode (task 6.1)
    # ------------------------------------------------------------------

    BLUE = (-2.0, -2.0, 2.0)

    def disableColorManagement(self):
        """The tint is a 50% mix in rendering space; with color management off the pixels are
        directly the rendering space colors, so the mix can be checked numerically."""
        enabled = cmds.colorManagementPrefs(query=True, cmEnabled=True)
        cmds.colorManagementPrefs(edit=True, cmEnabled=False)
        self.addCleanup(cmds.colorManagementPrefs, edit=True, cmEnabled=enabled)

    def twoColorSplats(self, fileName, opacities=None):
        self.disableColorManagement()
        """A red splat on the left and a blue one on the right, apart from each other."""
        path = self.createSplatStage(
            fileName, [(-0.6, 0, 0), (0.6, 0, 0)], colors=[self.RED, self.BLUE],
            opacities=opacities, scales=[(0.2, 0.2, 0.2)] * 2, extent=self.BOX)
        shape = self.createProxy(path)
        self.setCamera(translate=(0, 0, 5))
        panel = mayaUtils.activeModelPanel()
        self.setStyle(panel, 'shaded')
        return shape, panel, cmds.listRelatives(shape, parent=True)[0]

    def splatCenters(self, image):
        """Pixel of the strongest red and of the strongest blue (left and right splat)."""
        xr, yr = self.interior(image)
        best = {}
        for y in yr:
            for x in xr:
                r, g, b = self.samplePixel(image, x, y)
                for key, score in (('red', r - g - b), ('blue', b - r - g)):
                    if key not in best or score > best[key][0]:
                        best[key] = (score, (x, y))
        return best['red'][1], best['blue'][1]

    @staticmethod
    def near(pixel, expected, tolerance=14):
        return all(abs(p - e) <= tolerance for p, e in zip(pixel, expected))

    def leadColor(self):
        return tuple(cmds.displayRGBColor('lead', query=True))

    # Share of the highlight color in a tinted splat: kHighlightStrength in GaussianSplatGeometry.xml.
    HIGHLIGHT_STRENGTH = 0.05

    def expectedMix(self, splat, highlight):
        k = self.HIGHLIGHT_STRENGTH
        return tuple(int(round(255 * ((1 - k) * a + k * b))) for a, b in zip(splat, highlight))

    def testSelectedPrimIsTintedNotReplaced(self):
        shape, panel, transform = self.twoColorSplats('tp40.usda')
        before = self.shot(panel, 'tp40_before.png')
        redAt, blueAt = self.splatCenters(before)
        redBefore = self.samplePixel(before, *redAt)
        blueBefore = self.samplePixel(before, *blueAt)
        self.assertTrue(self.near(redBefore, (255, 0, 0), 20), redBefore)
        self.assertTrue(self.near(blueBefore, (0, 0, 255), 20), blueBefore)

        cmds.select(transform)
        after = self.shot(panel, 'tp40_after.png')
        redAfter = self.samplePixel(after, *redAt)
        blueAfter = self.samplePixel(after, *blueAt)
        # Not replaced: both still differ from each other, and from the before color.
        self.assertGreater(sum(abs(a - b) for a, b in zip(redAfter, blueAfter)), 100)
        self.assertGreater(redAfter[0], redAfter[2] + 30)
        self.assertGreater(blueAfter[2], blueAfter[0] + 30)
        # A fixed 5% mix of the highlight color into the splat color.
        lead = self.leadColor()
        self.assertTrue(self.near(redAfter, self.expectedMix((1, 0, 0), lead)),
                        (redAfter, self.expectedMix((1, 0, 0), lead)))
        self.assertTrue(self.near(blueAfter, self.expectedMix((0, 0, 1), lead)),
                        (blueAfter, self.expectedMix((0, 0, 1), lead)))

    def testNewHighlightColorChangesTint(self):
        shape, panel, transform = self.twoColorSplats('tp41.usda')
        before = self.shot(panel, 'tp41_before.png')
        redAt, _ = self.splatCenters(before)
        cmds.select(transform)
        first = self.samplePixel(self.shot(panel, 'tp41_first.png'), *redAt)
        original = self.leadColor()
        try:
            cmds.displayRGBColor('lead', 0.1, 0.1, 1.0)
            second = self.samplePixel(self.shot(panel, 'tp41_second.png'), *redAt)
        finally:
            cmds.displayRGBColor('lead', *original)
        self.assertGreater(second[2], first[2] + 3)
        self.assertTrue(self.near(second, self.expectedMix((1, 0, 0), (0.1, 0.1, 1.0))), second)

    def testDeselectRestoresColors(self):
        shape, panel, transform = self.twoColorSplats('tp42.usda')
        before = self.shot(panel, 'tp42_before.png')
        cmds.select(transform)
        selected = self.shot(panel, 'tp42_selected.png')
        redAt, _ = self.splatCenters(before)
        self.assertGreater(
            sum(abs(a - b) for a, b in zip(self.samplePixel(before, *redAt),
                                            self.samplePixel(selected, *redAt))), 20)
        cmds.select(clear=True)
        after = self.shot(panel, 'tp42_after.png')
        self.assertImagesEqual(before, after)

    def testTintKeepsOpacity(self):
        # One splat at half opacity: the tint changes the color, not the coverage. The edge of
        # the splat (falloff) must keep the same footprint, and the center the same alpha.
        self.disableColorManagement()
        path = self.createSplatStage(
            'tp43.usda', [(0, 0, 0)], colors=[self.RED], opacities=[0.5],
            scales=[(0.4, 0.4, 0.4)], extent=self.BOX)
        shape = self.createProxy(path)
        self.setCamera(translate=(0, 0, 5))
        panel = mayaUtils.activeModelPanel()
        self.setStyle(panel, 'shaded')
        before = self.shot(panel, 'tp43_before.png')
        cmds.select(cmds.listRelatives(shape, parent=True)[0])
        after = self.shot(panel, 'tp43_after.png')

        # Premultiplied blending: center = opacity * color + (1 - opacity) * background, so the
        # opacity is recovered per channel from the pixel, the color and the background.
        bg = self.samplePixel(before, 160, 70)
        cx, cy = self.center(before)
        cb, ca = self.samplePixel(before, cx, cy), self.samplePixel(after, cx, cy)
        # Unselected: the green channel of the red splat is 0.
        unselected = tuple(int(round(0.5 * c + 0.5 * g)) for c, g in zip((255, 0, 0), bg))
        self.assertTrue(self.near(cb, unselected, 10), (cb, unselected))
        # Half opacity over the background in both: center = 0.5 * color + 0.5 * bg.
        lead = self.leadColor()
        mix = self.expectedMix((1, 0, 0), lead)
        expected = tuple(int(round(0.5 * m + 0.5 * b)) for m, b in zip(mix, bg))
        self.assertTrue(self.near(ca, expected, 18), (ca, expected))

    def testTemplateLayerIsTinted(self):
        shape, panel, transform = self.twoColorSplats('tp44.usda')
        before = self.shot(panel, 'tp44_before.png')
        redAt, blueAt = self.splatCenters(before)
        cmds.createDisplayLayer(name='tmplLayer', noRecurse=True)
        cmds.editDisplayLayerMembers('tmplLayer', cmds.ls(shape, long=True)[0], noRecurse=True)
        cmds.displayRGBColor('templateDormant', 0.2, 0.9, 0.2)
        cmds.setAttr('tmplLayer.displayType', 1)
        image = self.shot(panel, 'tp44_template.png')
        red = self.samplePixel(image, *redAt)
        blue = self.samplePixel(image, *blueAt)
        # Tinted toward the template color, and the two splats still differ.
        self.assertGreater(red[1], 6)
        self.assertGreater(red[0], red[2] + 30)
        self.assertGreater(blue[2], blue[0] + 30)
        self.assertTrue(self.near(red, self.expectedMix((1, 0, 0), (0.2, 0.9, 0.2)), 25), red)

    def testTranslucentHighlightBlendsSorted(self):
        # Maya reports every highlight color (displayRGBColor) with alpha 1, so a translucent
        # highlight color cannot be produced from a test. What is checked: translucent splats
        # that are tinted blend and stay sorted from both sides (the sorted path that the
        # translucent highlight shares with the opaque one).
        shape = self.drawOverlappingRaw()
        panel = mayaUtils.activeModelPanel()
        front = self.createCamera('camFront', self.FRONT)
        back = self.createCamera('camBack', self.BACK)
        cmds.select(cmds.listRelatives(shape, parent=True)[0])
        cmds.modelPanel(panel, edit=True, camera=front)
        selFront = self.snapshotPanel(panel, 'tp45_front.png')
        cmds.modelPanel(panel, edit=True, camera=back)
        selBack = self.snapshotPanel(panel, 'tp45_back.png')
        # The tint moves both splats toward green, so compare the red-blue balance: it flips
        # from the front to the back.
        rf, _, bf = self.overlapPixel(selFront)
        rb, _, bb = self.overlapPixel(selBack)
        self.assertGreater((rb - bb) - (rf - bf), 25, ((rf, bf), (rb, bb)))


    # ------------------------------------------------------------------
    # Color space of the splat colors (converted to the rendering space on the GPU)
    # ------------------------------------------------------------------

    C0 = 0.28209479177387814

    # sRGB encoded 0.5 as a scene-linear value: the neutral gray the conversion must give in any
    # neutral rendering space.
    SRGB_HALF_AS_LINEAR = 0.21404114

    def setColorManagement(self, enabled):
        previous = cmds.colorManagementPrefs(query=True, cmEnabled=True)
        cmds.colorManagementPrefs(edit=True, cmEnabled=enabled)
        self.addCleanup(cmds.colorManagementPrefs, edit=True, cmEnabled=previous)

    def renderingSpace(self):
        return cmds.colorManagementPrefs(query=True, renderingSpaceName=True)

    def drawGraySplat(self, fileName, gray, colorSpace=None):
        """Draw one splat whose band-0 color is `gray` (a color space is declared on the
        coefficients when given), and return its center pixel. The proxy is deleted after."""
        coeff = (gray - 0.5) / self.C0
        path = self.createSplatStage(fileName, [(0, 0, 0)], colors=[(coeff,) * 3],
                                     extent=self.BOX)
        if colorSpace is not None:
            stage = Usd.Stage.Open(path)
            attr = stage.GetPrimAtPath(SPLAT_PRIM_PATH).GetAttribute(
                'radiance:sphericalHarmonicsCoefficients')
            attr.SetColorSpace(colorSpace)
            stage.GetRootLayer().Save()
            del stage
        shape = self.createProxy(path)
        self.setCamera(translate=(0, 0, 5))
        panel = mayaUtils.activeModelPanel()
        self.setStyle(panel, 'shaded')
        pixel = self.centerPixel(self.shot(panel, fileName.replace('.usda', '.png')))
        cmds.delete(cmds.listRelatives(shape, parent=True)[0])
        return pixel

    def testSrgbSplatIsConvertedToRenderingSpace(self):
        self.setColorManagement(True)
        rendering = self.renderingSpace()
        if rendering in ('sRGB', ''):
            self.skipTest('rendering space is sRGB: no conversion to observe')

        converted = self.drawGraySplat('tp48_srgb.usda', 0.5, 'sRGB')
        # The same color as an already converted value, drawn without any transform.
        expected = self.drawGraySplat('tp48_expected.usda', self.SRGB_HALF_AS_LINEAR, rendering)
        # The old, washed out look: the sRGB value taken as scene-linear.
        washedOut = self.drawGraySplat('tp48_washed.usda', 0.5, rendering)

        self.assertTrue(self.near(converted, expected, 6), (converted, expected))
        self.assertGreater(washedOut[0] - converted[0], 20, (converted, washedOut))

    def testColorManagementOffKeepsRawColor(self):
        self.setColorManagement(False)

        declared = self.drawGraySplat('tp49_srgb.usda', 0.5, 'sRGB')
        plain = self.drawGraySplat('tp49_plain.usda', 0.5)

        self.assertTrue(self.near(declared, (128, 128, 128), 10), declared)
        self.assertTrue(self.near(plain, (128, 128, 128), 10), plain)

    def testDeclaredColorSpaceIsUsedUnknownFallsBack(self):
        self.setColorManagement(True)
        rendering = self.renderingSpace()
        if rendering in ('sRGB', ''):
            self.skipTest('rendering space is sRGB: no conversion to observe')

        notAuthored = self.drawGraySplat('tp50_none.usda', 0.5)
        srgb = self.drawGraySplat('tp50_srgb.usda', 0.5, 'sRGB')
        unknown = self.drawGraySplat('tp50_unknown.usda', 0.5, 'NoSuchColorSpace')
        inRendering = self.drawGraySplat('tp50_rendering.usda', 0.5, rendering)

        # Nothing authored and an unknown name both fall back to sRGB.
        self.assertTrue(self.near(notAuthored, srgb, 4), (notAuthored, srgb))
        self.assertTrue(self.near(unknown, srgb, 4), (unknown, srgb))
        # The declared name is read: a splat declared in the rendering space is not converted.
        self.assertGreater(inRendering[0] - srgb[0], 20, (srgb, inRendering))


    def _mayaNode(self, name):
        selection = OM2.MSelectionList()
        selection.add(name)
        return selection.getDependNode(0)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
