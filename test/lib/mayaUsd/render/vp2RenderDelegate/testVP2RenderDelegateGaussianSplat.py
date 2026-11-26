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
import splatTestUtils
from splatTestUtils import quatAboutAxis, shDcForColor

from maya import cmds
import maya.api.OpenMayaRender as omr

import math
import os
import unittest

import maya.api.OpenMaya as om
from mayaUsd import lib as mayaUsdLib


# Splat fragments registered by the plugin. Each fragment added by a later
# batch must be listed here, so that this test checks its registration.
SPLAT_FRAGMENT_NAMES = ['SplatGeometry', 'SplatGeometryInstanced', 'SplatPixel',
                        'SplatFallbackShader']


# Large optional data, used by the integration-large tests (TP-53 to TP-55), which skip
# when a file is missing. The paths are fixed locations, see the test plan.
HOME = os.path.expanduser('~')
EAGLE_PLY = os.path.join(HOME, 'Downloads', 'Eagle', 'Eagle.ply')
EAGLE_INSTANCED_USDA = os.path.join(HOME, 'Downloads', 'eagle_instanced.usda')
# The prototype of eagle_instanced.usda, referenced by a relative path.
EAGLE_PROTOTYPE_USD = os.path.join(HOME, 'Downloads', 'eagle_yup.usd')
FLY_XXL_PLY = os.path.join(HOME, 'Gaussian Splats', 'Fly', 'cluster fly XXL.ply')


def _requireFiles(*paths):
    missing = [p for p in paths if not os.path.isfile(p)]
    return unittest.skipIf(bool(missing), 'Large data file missing: %s' % ', '.join(missing))


@unittest.skipUnless(splatTestUtils.splatSchemaAvailable(),
                     'pxr.UsdVol has no ParticleField3DGaussianSplat')
class testVP2RenderDelegateGaussianSplat(unittest.TestCase):
    """
    Tests Gaussian splat drawing with the Viewport 2.0 render delegate.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__, initializeStandalone=False, loadPlugin=False)

    def setUp(self):
        # Exact colors: no color management on the viewport.
        cmds.colorManagementPrefs(edit=True, cmEnabled=False)
        splatTestUtils.setViewportMode(textured=False)

    def tearDown(self):
        splatTestUtils.setViewportMode(textured=False, defaultMaterial=False)
        for layer in ('splatTemplateLayer', 'splatWireLayer'):
            if cmds.objExists(layer):
                cmds.delete(layer)

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    RED = (1.0, 0.0, 0.0)
    GREEN = (0.0, 1.0, 0.0)

    @staticmethod
    def _splat(stage, path, position=(0, 0, 0), scale=(1, 1, 1), opacity=1.0,
               color=RED, quat=(1, 0, 0, 0)):
        """A prim of one splat with every input authored."""
        return splatTestUtils.defineSplat(
            stage, path, [position], scales=[scale], opacities=[opacity],
            orientations=[quat], shCoeffs=[shDcForColor(color)], shDegree=0)

    def _render(self, build, name, camera=None):
        """Render a scene built by build(stage) and return the snapshot."""
        splatTestUtils.newScene()
        shapeNode, stage = splatTestUtils.createStage()
        self._shapeNode = shapeNode
        build(stage)
        splatTestUtils.frameScene()
        if camera:
            camera()
        return splatTestUtils.snapshotImage('%s.png' % name)

    def _renderView(self, build):
        """Render a scene built by build(stage) and read the view color buffer."""
        splatTestUtils.newScene()
        self._shapeNode, self._stage = splatTestUtils.createStage()
        build(self._stage)
        splatTestUtils.frameScene()
        return splatTestUtils.viewImage()

    def _axes(self, image, region=None):
        mask = splatTestUtils.largestCluster(splatTestUtils.foregroundMask(image, region=region))
        return splatTestUtils.ellipseAxes(mask)

    def test_TP_24_splat_fragments_registered_and_splat_draws(self):
        """
        With a USD build that has the splat schema, every splat fragment is
        registered after the plugin loads, and a splat prim draws.
        """
        with splatTestUtils.LogCapture() as log:
            splatTestUtils.newScene()
            _, stage = splatTestUtils.createStage()
            splatTestUtils.defineGridSplat(stage, '/splat')
            splatTestUtils.frameScene()
            image = splatTestUtils.snapshotImage('TP_24_splat.png')

        fragmentManager = omr.MRenderer.getFragmentManager()
        for name in SPLAT_FRAGMENT_NAMES:
            self.assertTrue(fragmentManager.hasFragment(name),
                            'Splat fragment %s is not registered' % name)

        # The splat prim drew something, and no splat warning or error was reported.
        self.assertGreater(splatTestUtils.coverage(image), 0.03)
        self.assertEqual(log.warnings(containing='plat'), [])

    # ------------------------------------------------------------------
    # Quads, opacity and noise
    # ------------------------------------------------------------------

    def test_TP_01_elongated_splat_is_an_oriented_ellipse(self):
        """An elongated, rotated splat appears as an ellipse with its axis ratio and angle."""
        image = self._render(
            lambda stage: self._splat(
                stage, '/splat', scale=(0.6, 0.2, 0.2),
                quat=quatAboutAxis((0, 0, 1), 30)), 'TP_01')
        major, minor, angle = self._axes(image)
        self.assertAlmostEqual(major / minor, 3.0, delta=0.4)
        # Counter-clockwise 30 degrees on screen, and the image y axis points down.
        self.assertAlmostEqual(angle, -math.radians(30), delta=0.1)

    def test_TP_02_ellipse_follows_the_view(self):
        """A flat splat is round from the front and a foreshortened ellipse from the side."""
        def build(stage):
            self._splat(stage, '/splat', scale=(0.5, 0.5, 0.02))

        front = self._render(build, 'TP_02_front')
        major, minor, _ = self._axes(front)
        self.assertLess(major / minor, 1.15)

        side = self._render(build, 'TP_02_side', camera=lambda: splatTestUtils.orbitCamera(60))
        major, minor, _ = self._axes(side)
        self.assertAlmostEqual(major / minor, 2.0, delta=0.3)

    def test_TP_03_half_opaque_splat_covers_half(self):
        """
        A large splat with opacity 0.5 draws about half of its fragments, and the other half
        shows the opaque green prim behind it.
        """
        def build(stage):
            self._splat(stage, '/behind', position=(0, 0, -1), scale=(2.5, 2.5, 2.5),
                        color=self.GREEN)
            self._splat(stage, '/splat', scale=(1.5, 1.5, 1.5), opacity=0.5)

        image = self._render(build, 'TP_03')
        x0, y0, x1, y1 = splatTestUtils.centerRegion(40)
        pixels = splatTestUtils.imageArray(image)[y0:y1, x0:x1].astype(int)
        red = (pixels[:, :, 0] > 200) & (pixels[:, :, 1] < 60) & (pixels[:, :, 2] < 60)
        green = (pixels[:, :, 1] > 200) & (pixels[:, :, 0] < 60) & (pixels[:, :, 2] < 60)
        area = float(red.size)
        self.assertAlmostEqual(red.sum() / area, 0.5, delta=0.08)
        self.assertAlmostEqual(green.sum() / area, 0.5, delta=0.08)
        # Every pixel is one of the two prims: nothing is blended and no background shows.
        self.assertGreater((red | green).sum() / area, 0.99)

    def test_TP_04_opaque_splat_covers_everything(self):
        """A splat with opacity 1 draws every fragment inside its shape."""
        image = self._render(
            lambda stage: self._splat(stage, '/splat', scale=(1.5, 1.5, 1.5), opacity=1.0),
            'TP_04')
        fraction = splatTestUtils.coverage(image, region=splatTestUtils.centerRegion(40))
        self.assertGreater(fraction, 0.999)

    def test_TP_05_overlapping_splats_do_not_depend_on_order(self):
        """Two overlapping splats of one prim give the same image in either data order, nearer one visible."""
        near = dict(position=(0, 0, 0.5), color=self.RED)
        far = dict(position=(0.3, 0, -0.5), color=self.GREEN)

        def build(order):
            def build(stage):
                splatTestUtils.defineSplat(
                    stage, '/splats', [s['position'] for s in order],
                    scales=[(1, 1, 1)] * 2, opacities=[0.7] * 2,
                    orientations=[(1, 0, 0, 0)] * 2,
                    shCoeffs=[shDcForColor(s['color']) for s in order], shDegree=0)
            return build

        first = self._render(build([near, far]), 'TP_05_first')
        second = self._render(build([far, near]), 'TP_05_second')
        self.assertTrue(splatTestUtils.imagesEqual(first, second, maxDifferingFraction=0.0005))
        # Where both are kept the nearer red one wins, so red dominates the left of the overlap.
        red, green, _ = splatTestUtils.dominantColor(
            first, region=(480 - 100, 270 - 20, 480 - 20, 270 + 20))
        self.assertGreater(red, green)

    def test_TP_06_template_layer_keeps_splats_opaque(self):
        """A template display layer does not blend overlapping splats."""
        def build(stage):
            self._splat(stage, '/near', position=(0, 0, 0.5), color=self.RED)
            self._splat(stage, '/far', position=(0.5, 0, -0.5), color=self.GREEN)

        def template():
            transform = cmds.listRelatives(self._shapeNode, parent=True, fullPath=True)[0]
            cmds.select(transform, replace=True)
            cmds.createDisplayLayer(name='splatTemplateLayer', noRecurse=True)
            cmds.setAttr('splatTemplateLayer.displayType', 1)
            cmds.select(clear=True)

        image = self._render(build, 'TP_06', camera=template)
        self.assertGreater(splatTestUtils.coverage(image), 0.01)
        # The overlap is entirely the nearer splat, never a blend of the two.
        colors = splatTestUtils.distinctColors(
            image, (480 - 45, 270 - 30, 480 + 100, 270 + 30))
        self.assertLessEqual(len(colors), 2, colors)

    def test_TP_07_redraw_is_identical(self):
        """Two redraws of an unchanged scene give identical images."""
        first = self._renderView(
            lambda stage: self._splat(stage, '/splat', scale=(1.5, 1.5, 1.5), opacity=0.5))
        second = splatTestUtils.viewImage()
        self.assertTrue(splatTestUtils.imagesEqual(first, second))
        self.assertGreater(splatTestUtils.coverage(first), 0.02)

    def test_TP_08_orbit_does_no_splat_sync_or_commit(self):
        """No splat sync or buffer commit happens when the camera orbits."""
        scopes = ['HdVP2GaussianSplat::Sync', 'CommitPositions']

        splatTestUtils.newScene()
        _, stage = splatTestUtils.createStage()
        prim = splatTestUtils.defineGridSplat(stage, '/splat', count=100, spacing=0.05,
                                              scale=0.02, opacity=0.8)
        splatTestUtils.frameScene()
        splatTestUtils.snapshotImage('TP_08_first.png')
        splatTestUtils.refreshFrames(3)

        def orbit():
            for degrees in (10, 20, 30, 40, 50, 60):
                splatTestUtils.orbitCamera(degrees)
                cmds.refresh(force=True)

        counts = splatTestUtils.profileEvents(orbit, scopes)
        self.assertEqual(counts, {'HdVP2GaussianSplat::Sync': 0, 'CommitPositions': 0})

        # Positive control: editing the positions syncs the prim once.
        def edit():
            attr = prim.GetAttribute(splatTestUtils.ATTR_POSITIONS)
            positions = attr.Get()
            positions[0] = splatTestUtils.Gf.Vec3f(0.1, 0.1, 0.0)
            attr.Set(positions)

        counts = splatTestUtils.profileEvents(edit, scopes)
        self.assertEqual(counts['HdVP2GaussianSplat::Sync'], 1)

    # ------------------------------------------------------------------
    # Per-splat frustum culling
    # ------------------------------------------------------------------

    def test_TP_09_splat_with_center_off_screen_draws_nothing(self):
        """A large splat whose center is outside the view draws no fragment."""
        outside = self._render(
            lambda stage: self._splat(stage, '/splat', position=(-5.0, 0, 0), scale=(1.2, 1.2, 1.2)),
            'TP_09_outside')
        self.assertEqual(splatTestUtils.coverage(outside), 0.0)

        # Control: the same splat with its center inside the view draws.
        inside = self._render(
            lambda stage: self._splat(stage, '/splat', position=(-3.5, 0, 0), scale=(1.2, 1.2, 1.2)),
            'TP_09_inside')
        self.assertGreater(splatTestUtils.coverage(inside), 0.01)

    def test_TP_10_splat_partly_in_view_draws_the_visible_part(self):
        """A splat with its center inside the view and its quad outside draws the visible part."""
        image = self._render(
            lambda stage: self._splat(stage, '/splat', position=(-3.8, 0, 0), scale=(1.0, 1.0, 1.0)),
            'TP_10')
        self.assertGreater(splatTestUtils.coverage(image), 0.005)
        # The drawn part touches the left edge of the image.
        self.assertGreater(
            splatTestUtils.coverage(image, region=(0, 0, 3, splatTestUtils.SNAPSHOT_HEIGHT)), 0.1)

    def test_TP_11_prim_off_screen_draws_nothing(self):
        """A prim whose splat centers are all outside the view draws nothing."""
        positions = ([(-9.0 + 0.5 * i, 0.0, 0.0) for i in range(6)]
                     + [(0.0, 0.0, 20.0), (1.0, 0.0, 30.0)])
        n = len(positions)

        def build(stage):
            splatTestUtils.defineSplat(
                stage, '/splat', positions, scales=[(0.5, 0.5, 0.5)] * n, opacities=[1.0] * n,
                orientations=[(1, 0, 0, 0)] * n, shCoeffs=[shDcForColor(self.RED)] * n,
                shDegree=0)

        with splatTestUtils.LogCapture() as log:
            image = self._render(build, 'TP_11')
        self.assertEqual(splatTestUtils.coverage(image), 0.0)
        self.assertEqual(log.warnings(containing='splat'), [])

    # ------------------------------------------------------------------
    # Shaded reprs
    # ------------------------------------------------------------------

    def _halfOpaqueScene(self, stage):
        self._splat(stage, '/splat', scale=(1.5, 1.5, 1.5), opacity=0.5)

    def test_TP_12_smooth_shaded_draws_each_splat_once(self):
        """
        In smooth shaded mode a splat prim draws one render item, also after a switch from the
        textured mode, which leaves Hydra holding both shaded repr tokens. Its opacity 0.5
        splat covers about 50%, not about 75%.
        """
        image = self._renderView(self._halfOpaqueScene)
        region = splatTestUtils.centerRegion(40, image)
        self.assertAlmostEqual(splatTestUtils.coverage(image, region=region), 0.5, delta=0.08)
        self.assertEqual(splatTestUtils.drawnItemCounts().get('Opaque'), 1)

        splatTestUtils.setViewportMode(textured=True)
        splatTestUtils.refreshFrames(2)
        splatTestUtils.setViewportMode(textured=False)
        again = splatTestUtils.viewImage()
        self.assertEqual(splatTestUtils.drawnItemCounts().get('Opaque'), 1)
        self.assertAlmostEqual(splatTestUtils.coverage(again, region=region), 0.5, delta=0.08)

        # Control: the count follows the drawn splat items, a second prim draws a second item.
        self._splat(self._stage, '/other', position=(2.5, 0, 0), scale=(0.5, 0.5, 0.5))
        self.assertEqual(splatTestUtils.drawnItemCounts().get('Opaque'), 2)

    def test_TP_13_switching_textured_and_shaded_draws_once(self):
        """Switching between textured and shaded draws each splat once in both modes."""
        splatTestUtils.setViewportMode(textured=True)
        textured = self._renderView(self._halfOpaqueScene)
        region = splatTestUtils.centerRegion(40, textured)
        self.assertAlmostEqual(splatTestUtils.coverage(textured, region=region), 0.5, delta=0.08)

        splatTestUtils.setViewportMode(textured=False)
        shaded = splatTestUtils.viewImage()
        self.assertAlmostEqual(splatTestUtils.coverage(shaded, region=region), 0.5, delta=0.08)

        splatTestUtils.setViewportMode(textured=True)
        again = splatTestUtils.viewImage()
        self.assertAlmostEqual(splatTestUtils.coverage(again, region=region), 0.5, delta=0.08)

    def test_TP_14_default_material_mode_keeps_the_splat_look(self):
        """With default material mode on, splats keep their shape, opacity and color."""
        splatTestUtils.setViewportMode(textured=False, defaultMaterial=True)
        image = self._render(self._halfOpaqueScene, 'TP_14')
        region = splatTestUtils.centerRegion(40)
        self.assertAlmostEqual(splatTestUtils.coverage(image, region=region), 0.5, delta=0.08)
        red, green, blue = splatTestUtils.dominantColor(image, region=region)
        self.assertGreater(red, 150)
        self.assertLess(green, 60)
        self.assertLess(blue, 60)

    # ------------------------------------------------------------------
    # Wireframe
    # ------------------------------------------------------------------

    def test_TP_15_wireframe_viewport_draws_fixed_size_quads(self):
        """In a wireframe viewport each splat is a fixed-size quad with its band 0 color."""
        def build(stage):
            self._splat(stage, '/small', position=(-2, 0, 0), scale=(0.1, 0.1, 0.1))
            self._splat(stage, '/large', position=(2, 0, 0), scale=(1.2, 1.2, 1.2),
                        color=self.GREEN)

        splatTestUtils.newScene()
        _, stage = splatTestUtils.createStage()
        build(stage)
        splatTestUtils.frameScene()
        splatTestUtils.setWireframeViewport()
        image = splatTestUtils.viewImage()
        width, height = image.width(), image.height()
        sizes, colors = [], []
        for region in ((0, height // 4, width // 2, height * 3 // 4),
                       (width // 2, height // 4, width, height * 3 // 4)):
            mask = splatTestUtils.largestCluster(splatTestUtils.foregroundMask(image, region=region))
            sizes.append(splatTestUtils.boundingSize(mask))
            colors.append(splatTestUtils.dominantColor(image, region=region))
        # The two quads have the same size, and the authored scales do not matter.
        self.assertAlmostEqual(sizes[0][0], sizes[1][0], delta=1)
        self.assertAlmostEqual(sizes[0][1], sizes[1][1], delta=1)
        self.assertGreater(sizes[0][0], 3)
        self.assertLess(sizes[0][0], 20)
        self._assertColor(colors[0], self.RED, msg='left quad')
        self._assertColor(colors[1], self.GREEN, msg='right quad')

    def test_TP_16_wireframe_on_shaded_draws_quads_over_the_splats(self):
        """With wireframe on shaded, the fixed-size quads draw over the shaded splats."""
        def build(stage):
            self._splat(stage, '/splat', scale=(1.5, 1.5, 1.5), opacity=0.5)

        splatTestUtils.setWireframeOnShaded(False)
        shaded = self._renderView(build)
        small = splatTestUtils.centerRegion(2, shaded)
        large = splatTestUtils.centerRegion(40, shaded)
        self.assertAlmostEqual(splatTestUtils.coverage(shaded, region=large), 0.5, delta=0.08)
        self.assertLess(splatTestUtils.coverage(shaded, region=small), 0.9)

        splatTestUtils.setWireframeOnShaded(True)
        both = splatTestUtils.viewImage()
        # The shaded splat is still there, and the opaque quad fills the holes at its center.
        self.assertAlmostEqual(splatTestUtils.coverage(both, region=large), 0.5, delta=0.1)
        self.assertGreater(splatTestUtils.coverage(both, region=small), 0.99)

    def test_TP_17_wireframe_layer_draws_only_the_quads(self):
        """A display layer forcing wireframe draws the fixed-size quads and not the shaded splat."""
        def build(stage):
            self._splat(stage, '/splat', scale=(1.5, 1.5, 1.5))

        shaded = self._renderView(build)
        large = splatTestUtils.centerRegion(40, shaded)
        self.assertGreater(splatTestUtils.coverage(shaded, region=large), 0.999)

        transform = cmds.listRelatives(self._shapeNode, parent=True, fullPath=True)[0]
        cmds.select(transform, replace=True)
        cmds.createDisplayLayer(name='splatWireLayer', noRecurse=True)
        cmds.setAttr('splatWireLayer.shading', 0)
        cmds.select(clear=True)

        wire = splatTestUtils.viewImage()
        width, height = splatTestUtils.boundingSize(
            splatTestUtils.largestCluster(splatTestUtils.foregroundMask(
                wire, region=(0, wire.height() // 4, wire.width(), wire.height() * 3 // 4))))
        # Only the small quad is drawn, centered, and it has the splat color.
        self.assertGreater(width, 3)
        self.assertLess(width, 20)
        self.assertLess(splatTestUtils.coverage(wire, region=large), 0.1)
        self.assertGreater(splatTestUtils.coverage(wire, region=splatTestUtils.centerRegion(2, wire)),
                           0.99)
        self._assertColor(splatTestUtils.centerDominantColor(wire, 2), self.RED)

        # Removing the override draws the shaded splat again.
        cmds.setAttr('splatWireLayer.shading', 1)
        back = splatTestUtils.viewImage()
        self.assertGreater(splatTestUtils.coverage(back, region=large), 0.999)

    # ------------------------------------------------------------------
    # Opacity, scale and rotation inputs
    # ------------------------------------------------------------------

    def test_TP_18_all_inputs_authored(self):
        """Each splat draws with its own scale, rotation and opacity."""
        def build(stage):
            # A: elongated and rotated so that its long axis is vertical.
            self._splat(stage, '/a', position=(-2.2, 0, 0), scale=(0.8, 0.3, 0.3),
                        quat=quatAboutAxis((0, 0, 1), 90))
            # B: round, a different scale.
            self._splat(stage, '/b', position=(2.2, 0, 0), scale=(0.4, 0.4, 0.4))
            # C: a different opacity.
            self._splat(stage, '/c', position=(0, 0, 0), scale=(0.6, 0.6, 0.6), opacity=0.25)

        image = self._render(build, 'TP_18')
        width, height = splatTestUtils.SNAPSHOT_WIDTH, splatTestUtils.SNAPSHOT_HEIGHT
        majorA, minorA, angleA = self._axes(image, region=(0, 0, 320, height))
        self.assertAlmostEqual(majorA / minorA, 8.0 / 3.0, delta=0.4)
        self.assertGreater(abs(angleA), math.radians(80))

        majorB, minorB, _ = self._axes(image, region=(640, 0, width, height))
        self.assertAlmostEqual(majorB / minorB, 1.0, delta=0.15)
        self.assertAlmostEqual(majorB / minorA, 4.0 / 3.0, delta=0.25)

        fraction = splatTestUtils.coverage(image, region=splatTestUtils.centerRegion(40))
        self.assertAlmostEqual(fraction, 0.25, delta=0.06)

    def test_TP_19_missing_opacity_is_fully_opaque(self):
        """A prim without opacities draws every splat with opacity 1."""
        def build(stage):
            splatTestUtils.defineSplat(
                stage, '/splat', [(0, 0, 0)], scales=[(1.5, 1.5, 1.5)],
                orientations=[(1, 0, 0, 0)], shCoeffs=[shDcForColor(self.RED)], shDegree=0)

        with splatTestUtils.LogCapture() as log:
            image = self._render(build, 'TP_19')
        fraction = splatTestUtils.coverage(image, region=splatTestUtils.centerRegion(40))
        self.assertGreater(fraction, 0.999)
        self.assertEqual(log.warnings(containing='opacities'), [])

    def test_TP_20_mismatched_array_warns_and_uses_defaults(self):
        """An input array of the wrong size gives a warning and the defaults."""
        def build(stage):
            splatTestUtils.defineSplat(
                stage, '/splat', [(-1, 0, 0), (1, 0, 0)],
                scales=[(1, 1, 1)] * 3, opacities=[0.2],
                orientations=[(1, 0, 0, 0)] * 2,
                shCoeffs=[shDcForColor(self.RED)] * 2, shDegree=0)

        with splatTestUtils.LogCapture() as log:
            image = self._render(build, 'TP_20')

        self.assertTrue(log.warnings(containing='scales'), log.messages)
        self.assertTrue(log.warnings(containing='opacities'), log.messages)

        # Defaults: opacity 1 and the default scale, which is a small fully drawn ellipse.
        major, minor, _ = self._axes(image)
        expected = 2.0 * splatTestUtils.DEFAULT_SCALE * 117.0
        self.assertAlmostEqual(major, expected, delta=0.35 * expected)
        self.assertGreater(
            splatTestUtils.coverage(image, region=(480 - 160, 270 - 6, 480 - 80, 270 + 6)), 0.0)

    # ------------------------------------------------------------------
    # Selection highlight
    # ------------------------------------------------------------------

    def _selectionScene(self, stage):
        """Two red splats side by side, each far larger than the 5% tint measurement needs."""
        self._splat(stage, '/a', position=(-2, 0, 0), scale=(1.2, 1.2, 1.2))
        self._splat(stage, '/b', position=(2, 0, 0), scale=(1.2, 1.2, 1.2))

    def _colorShifts(self, before, after, regionColors):
        """Per region of regionColors(image), the mean rgb change in 0..255 from before to after."""
        result = []
        for b, a in zip(regionColors(before), regionColors(after)):
            self.assertIsNotNone(b)
            self.assertIsNotNone(a)
            result.append(tuple(y - x for x, y in zip(b, a)))
        return result

    def _shifts(self, before, after):
        """Per half, the mean rgb change in 0..255 from before to after."""
        return self._colorShifts(before, after, splatTestUtils.halfColors)

    @staticmethod
    def _expectedShift(target):
        """The rgb change in 0..255 of a red splat tinted by the 0..1 selection color target."""
        # rgb = mix(red, target, 0.05)
        return (255.0 * 0.05 * (target[0] - 1.0), 255.0 * 0.05 * target[1],
                255.0 * 0.05 * target[2])

    def _assertShift(self, shift, target, msg=''):
        """shift is a rgb change in 0..255 of a red splat, target the 0..1 selection color."""
        expected = self._expectedShift(target)
        for s, e in zip(shift, expected):
            self.assertAlmostEqual(s, e, delta=3.0, msg='%s: %s vs %s' % (msg, shift, expected))

    def _highlightColors(self):
        """The active (white) and lead selection colors of the viewport, 0..1."""
        active = tuple(cmds.displayRGBColor('polymeshActive', query=True))
        lead = tuple(cmds.displayRGBColor('lead', query=True))
        return active, lead

    def test_TP_21_selected_prim_shifts_five_percent_toward_the_active_color(self):
        """A selected splat prim that is not the lead shifts 5% toward the active color."""
        before = self._renderView(self._selectionScene)
        splatTestUtils.select(self._shapeNode, ['/a', '/b'])
        self.assertEqual(len(splatTestUtils.selectedPathStrings()), 2)
        after = splatTestUtils.viewImage()
        active, lead = self._highlightColors()
        shifts = self._shifts(before, after)
        # The last selected prim is the lead, /a is active (left half).
        self._assertShift(shifts[0], active, 'active')
        self._assertShift(shifts[1], lead, 'lead')

    def test_TP_22_lead_selection_uses_the_lead_color(self):
        """The prim that becomes the lead selection blends the lead color, the other the active."""
        before = self._renderView(self._selectionScene)
        splatTestUtils.select(self._shapeNode, ['/a', '/b'])
        first = splatTestUtils.viewImage()
        splatTestUtils.select(self._shapeNode, ['/b', '/a'])
        second = splatTestUtils.viewImage()
        active, lead = self._highlightColors()
        self.assertGreater(max(abs(l - a) for l, a in zip(lead, active)), 0.1)
        firstShifts = self._shifts(before, first)
        secondShifts = self._shifts(before, second)
        self._assertShift(firstShifts[0], active, 'a active')
        self._assertShift(firstShifts[1], lead, 'b lead')
        self._assertShift(secondShifts[0], lead, 'a lead')
        self._assertShift(secondShifts[1], active, 'b active')

    def test_TP_23_deselect_restores_the_splat_color(self):
        """After the prim is deselected its splats return to their normal color."""
        before = self._renderView(self._selectionScene)
        splatTestUtils.select(self._shapeNode, ['/a', '/b'])
        selected = splatTestUtils.viewImage()
        self.assertGreater(max(abs(v) for shift in self._shifts(before, selected) for v in shift), 8)
        splatTestUtils.clearSelection()
        after = splatTestUtils.viewImage()
        self.assertTrue(splatTestUtils.imagesEqual(before, after))

    # ------------------------------------------------------------------
    # Spherical harmonics
    # ------------------------------------------------------------------

    def _assertColor(self, actual, expected, delta=0.03, msg=None):
        """actual is an 0..255 rgb triple (or None), expected a 0..1 rgb triple."""
        self.assertIsNotNone(actual, msg or 'nothing was drawn')
        for a, e in zip(actual, expected):
            self.assertAlmostEqual(a / 255.0, e, delta=delta, msg='%s vs %s %s' % (
                actual, tuple(round(255 * c) for c in expected), msg or ''))

    def _bigSplat(self, stage, path, coeffs, position=(0, 0, 0), xform=None, degree=None):
        """One large opaque splat with the given SH coefficients (one splat, all inputs)."""
        return splatTestUtils.defineSplat(
            stage, path, [position], scales=[(1.5, 1.5, 1.5)], opacities=[1.0],
            orientations=[(1, 0, 0, 0)], shCoeffs=coeffs,
            shDegree=degree, xform=xform)

    def _splatColorFromCamera(self, degrees):
        """The center color of the view with the camera orbited by degrees."""
        splatTestUtils.orbitCamera(degrees)
        return splatTestUtils.centerDominantColor(splatTestUtils.viewImage())

    def test_TP_25_degree_zero_is_constant_from_any_view(self):
        """A prim with only band 0 coefficients shows one color from any view."""
        color = (0.2, 0.7, 0.4)
        with splatTestUtils.LogCapture() as log:
            splatTestUtils.newScene()
            _, stage = splatTestUtils.createStage()
            self._bigSplat(stage, '/splat', [shDcForColor(color)], degree=0)
            splatTestUtils.frameScene()
            first = splatTestUtils.centerDominantColor(splatTestUtils.viewImage())
            second = self._splatColorFromCamera(50)
            third = self._splatColorFromCamera(-120)

        for drawn in (first, second, third):
            self._assertColor(drawn, color)
        self.assertEqual(log.errors(), [])
        self.assertEqual(log.warnings(containing='plat'), [])

    def test_TP_26_degree_three_matches_cpu_evaluation_from_two_cameras(self):
        """The center pixel of a degree 3 splat equals the CPU evaluation, from two cameras."""
        coeffs = splatTestUtils.sampleCoefficients(16, seed=1.0)
        splatTestUtils.newScene()
        _, stage = splatTestUtils.createStage()
        self._bigSplat(stage, '/splat', coeffs, degree=3)
        splatTestUtils.frameScene()

        drawn = {}
        for degrees in (0, 55):
            drawn[degrees] = self._splatColorFromCamera(degrees)
            direction = splatTestUtils.localViewDirection(
                (0, 0, 0), splatTestUtils.cameraPosition(degrees))
            self._assertColor(drawn[degrees],
                              splatTestUtils.shColor(coeffs, direction, 3),
                              msg='camera %s' % degrees)

        # The two views must differ, otherwise the check cannot tell view-dependent from constant.
        difference = max(abs(a - b) for a, b in zip(drawn[0], drawn[55]))
        self.assertGreater(difference, 12)

    def test_TP_27_out_of_range_result_is_clamped(self):
        """SH results below 0 or above 1 are drawn as 0 or 1."""
        # Camera on +Z looks along -Z: the degree 1 z term is -C1 * coefficient.
        coeffs = [(0.0, 0.0, 0.0), (0.0, 0.0, 0.0), (3.0, -3.0, 0.0), (0.0, 0.0, 0.0)]
        direction = splatTestUtils.localViewDirection((0, 0, 0), splatTestUtils.cameraPosition(0))
        raw = splatTestUtils.shColorRaw(coeffs, direction, 1)
        self.assertLess(raw[0], 0.0)
        self.assertGreater(raw[1], 1.0)

        image = self._renderView(lambda stage: self._bigSplat(stage, '/splat', coeffs, degree=1))
        color = splatTestUtils.centerDominantColor(image)
        self._assertColor(color, (0.0, 1.0, 0.5))

    def test_TP_28_usable_degree_follows_the_complete_bands(self):
        """With degree 0 and 1 and only part of degree 2 authored, degree 1 colors the splat."""
        # 6 coefficients: band 0, band 1 and two of the five band 2 coefficients, which are
        # large so that reading them would change the color. The prim is tilted so that the
        # local view direction has x, y and z components.
        coeffs = splatTestUtils.sampleCoefficients(4, seed=2.0)
        partial = coeffs + [(0.8, -0.8, 0.8), (-0.8, 0.8, 0.8)]
        tilt = splatTestUtils.Gf.Matrix4d(1.0)
        tilt.SetRotate(splatTestUtils.Gf.Rotation((1, 1, 0), 50))
        direction = splatTestUtils.localViewDirection(
            (0, 0, 0), splatTestUtils.cameraPosition(0), tilt)
        expected = splatTestUtils.shColor(coeffs, direction, 1)
        wrong = splatTestUtils.shColor(partial + [(0.0, 0.0, 0.0)] * 3, direction, 2)
        self.assertGreater(max(abs(a - b) for a, b in zip(expected, wrong)), 0.1)

        splatTestUtils.newScene()
        _, stage = splatTestUtils.createStage()
        self._bigSplat(stage, '/splat', partial, xform=tilt)
        splatTestUtils.frameScene()
        self._assertColor(splatTestUtils.centerDominantColor(splatTestUtils.viewImage()), expected)

    def test_TP_29_rotating_the_prim_changes_the_color_like_moving_the_camera(self):
        """Rotating a prim by 90 degrees acts as moving the camera by the opposite rotation."""
        coeffs = splatTestUtils.sampleCoefficients(4, seed=3.0, amplitude=0.7)
        rotation = splatTestUtils.Gf.Matrix4d(1.0)
        rotation.SetRotate(splatTestUtils.Gf.Rotation((0, 1, 0), 90))

        splatTestUtils.newScene()
        _, stage = splatTestUtils.createStage()
        self._bigSplat(stage, '/still', coeffs, degree=1)
        self._bigSplat(stage, '/rotated', coeffs, degree=1, xform=rotation)
        stage.GetPrimAtPath('/rotated').SetActive(False)
        splatTestUtils.frameScene()
        unrotated = splatTestUtils.centerDominantColor(splatTestUtils.viewImage())

        stage.GetPrimAtPath('/still').SetActive(False)
        stage.GetPrimAtPath('/rotated').SetActive(True)
        rotated = splatTestUtils.centerDominantColor(splatTestUtils.viewImage())

        camera = splatTestUtils.cameraPosition(0)
        local = splatTestUtils.localViewDirection((0, 0, 0), camera, rotation)
        self._assertColor(rotated, splatTestUtils.shColor(coeffs, local, 1), msg='rotated')
        straight = splatTestUtils.localViewDirection((0, 0, 0), camera)
        self._assertColor(unrotated, splatTestUtils.shColor(coeffs, straight, 1), msg='unrotated')
        self.assertGreater(max(abs(a - b) for a, b in zip(rotated, unrotated)), 12)

        # The same color as the unrotated prim seen by the camera moved by the opposite rotation.
        stage.GetPrimAtPath('/still').SetActive(True)
        stage.GetPrimAtPath('/rotated').SetActive(False)
        moved = self._splatColorFromCamera(-90)
        self._assertColor(moved, tuple(c / 255.0 for c in rotated), delta=0.03)

    def test_TP_30_edited_coefficients_show_on_the_next_draw(self):
        """Editing the SH coefficients changes the drawn colors."""
        first = splatTestUtils.sampleCoefficients(4, seed=4.0)
        second = splatTestUtils.sampleCoefficients(4, seed=9.0)
        direction = splatTestUtils.localViewDirection((0, 0, 0), splatTestUtils.cameraPosition(25))

        splatTestUtils.newScene()
        _, stage = splatTestUtils.createStage()
        prim = self._bigSplat(stage, '/splat', first, degree=1)
        splatTestUtils.frameScene()
        before = self._splatColorFromCamera(25)
        self._assertColor(before, splatTestUtils.shColor(first, direction, 1), msg='before')

        splatTestUtils.setSplatArrays(
            prim, [(0, 0, 0)], [(1.5, 1.5, 1.5)], [1.0], [(1, 0, 0, 0)], second)
        after = splatTestUtils.centerDominantColor(splatTestUtils.viewImage())
        self._assertColor(after, splatTestUtils.shColor(second, direction, 1), msg='after')
        self.assertGreater(max(abs(a - b) for a, b in zip(before, after)), 12)

    def test_TP_31_splat_count_change_shows_matching_colors(self):
        """A changed splat count draws the new set of splats with their own SH colors."""
        left, right = (-2.2, 0, 0), (2.2, 0, 0)
        coeffsA = splatTestUtils.sampleCoefficients(9, seed=5.0)
        coeffsB = splatTestUtils.sampleCoefficients(9, seed=6.0)
        coeffsC = splatTestUtils.sampleCoefficients(9, seed=7.0)

        def authored(positions, coeffs):
            n = len(positions)
            return dict(positions=positions, scales=[(0.7, 0.7, 0.7)] * n, opacities=[1.0] * n,
                        orientations=[(1, 0, 0, 0)] * n, shCoeffs=sum(coeffs, []))

        splatTestUtils.newScene()
        _, stage = splatTestUtils.createStage()
        prim = splatTestUtils.defineSplat(
            stage, '/splat', **dict(authored([left], [coeffsA]), shDegree=2))
        splatTestUtils.frameScene()
        camera = splatTestUtils.cameraPosition(0)

        image = splatTestUtils.viewImage()
        leftColor, rightColor = splatTestUtils.halfColors(image)
        self._assertColor(leftColor, splatTestUtils.shColor(
            coeffsA, splatTestUtils.localViewDirection(left, camera), 2), msg='one splat')
        self.assertIsNone(rightColor)

        # Two splats, with other coefficients.
        splatTestUtils.setSplatArrays(prim, **authored([left, right], [coeffsB, coeffsC]))
        image = splatTestUtils.viewImage()
        leftColor, rightColor = splatTestUtils.halfColors(image)
        self._assertColor(leftColor, splatTestUtils.shColor(
            coeffsB, splatTestUtils.localViewDirection(left, camera), 2), msg='two splats, left')
        self._assertColor(rightColor, splatTestUtils.shColor(
            coeffsC, splatTestUtils.localViewDirection(right, camera), 2), msg='two splats, right')

        # Back to one splat.
        splatTestUtils.setSplatArrays(prim, **authored([right], [coeffsA]))
        image = splatTestUtils.viewImage()
        leftColor, rightColor = splatTestUtils.halfColors(image)
        self.assertIsNone(leftColor)
        self._assertColor(rightColor, splatTestUtils.shColor(
            coeffsA, splatTestUtils.localViewDirection(right, camera), 2), msg='one splat again')

    def test_TP_32_wrong_sized_sh_array_warns_and_uses_default_color(self):
        """An SH array that does not match the splat count warns and draws the default color."""
        coeffs = splatTestUtils.sampleCoefficients(4, seed=8.0)

        def build(stage):
            # Two splats, but 7 coefficients: not a whole number per splat.
            splatTestUtils.defineSplat(
                stage, '/splat', [(-0.3, 0, 0), (0.3, 0, 0)],
                scales=[(1.5, 1.5, 1.5)] * 2, opacities=[1.0, 1.0],
                orientations=[(1, 0, 0, 0)] * 2, shCoeffs=coeffs + coeffs[:3], shDegree=1)

        with splatTestUtils.LogCapture() as log:
            image = self._renderView(build)

        self.assertTrue(log.warnings(containing='SH coefficients'), log.messages)
        self.assertEqual(log.errors(containing='plat'), [])
        self._assertColor(splatTestUtils.centerDominantColor(image),
                          splatTestUtils.defaultPointsColor(), delta=0.04)

    def test_TP_33_no_sh_data_uses_default_color_without_error(self):
        """A prim without SH coefficients draws the default color and reports no error."""
        def build(stage):
            splatTestUtils.defineSplat(
                stage, '/splat', [(0, 0, 0)], scales=[(1.5, 1.5, 1.5)], opacities=[1.0],
                orientations=[(1, 0, 0, 0)])

        with splatTestUtils.LogCapture() as log:
            image = self._renderView(build)

        self.assertEqual(log.errors(), [])
        self._assertColor(splatTestUtils.centerDominantColor(image),
                          splatTestUtils.defaultPointsColor(), delta=0.04)

    def test_TP_34_two_prims_use_their_own_sh_data(self):
        """Two prims with different SH, in one scene, each draw with their own colors."""
        left, right = (-2.2, 0, 0), (2.2, 0, 0)
        coeffsA = splatTestUtils.sampleCoefficients(16, seed=10.0)
        coeffsB = splatTestUtils.sampleCoefficients(4, seed=11.0)

        splatTestUtils.newScene()
        _, stage = splatTestUtils.createStage()
        splatTestUtils.defineSplat(
            stage, '/a', [left], scales=[(0.7, 0.7, 0.7)], opacities=[1.0],
            orientations=[(1, 0, 0, 0)], shCoeffs=coeffsA, shDegree=3)
        splatTestUtils.defineSplat(
            stage, '/b', [right], scales=[(0.7, 0.7, 0.7)], opacities=[1.0],
            orientations=[(1, 0, 0, 0)], shCoeffs=coeffsB, shDegree=1)
        splatTestUtils.frameScene()

        for degrees in (0, 40):
            splatTestUtils.orbitCamera(degrees)
            camera = splatTestUtils.cameraPosition(degrees)
            leftColor, rightColor = splatTestUtils.halfColors(splatTestUtils.viewImage())
            self._assertColor(leftColor, splatTestUtils.shColor(
                coeffsA, splatTestUtils.localViewDirection(left, camera), 3),
                msg='prim a, camera %s' % degrees)
            self._assertColor(rightColor, splatTestUtils.shColor(
                coeffsB, splatTestUtils.localViewDirection(right, camera), 1),
                msg='prim b, camera %s' % degrees)

    # ------------------------------------------------------------------
    # Instancing
    # ------------------------------------------------------------------

    @staticmethod
    def _elongated(stage, path):
        """The prototype of most instancing tests: a red horizontal ellipse."""
        return testVP2RenderDelegateGaussianSplat._splat(stage, path, scale=(0.5, 0.2, 0.2))

    def _xforms(self):
        """Three instance transforms: plain, rotated by 90 degrees and scaled, shifted."""
        xform = splatTestUtils.instanceXform
        return [xform((-3, 0, 0)), xform((0, 0, 0), 90, scale=1.5), xform((3, 0.6, 0))]

    def _assertSameImage(self, instanced, twin, msg, minCoverage=0.002):
        self.assertGreater(splatTestUtils.coverage(twin, region=splatTestUtils.middleBand(twin)),
                           minCoverage, msg + ': the twin drew nothing')
        fraction = splatTestUtils.differingFraction(instanced, twin)
        self.assertLess(fraction, 0.001, '%s: %.4f of the pixels differ from the baked twin'
                        % (msg, fraction))

    def _pointInstancerAndTwin(self, protoBuilder, xforms, instancerSetup=None, twinXforms=None):
        """Render a point instancer scene and its baked twin."""
        def instanced(stage):
            instancer = splatTestUtils.defineInstancedSplat(stage, '/PI', protoBuilder, xforms)
            if instancerSetup:
                instancerSetup(instancer)

        def twin(stage):
            splatTestUtils.defineBakedTwin(stage, '/twin', protoBuilder,
                                           xforms if twinXforms is None else twinXforms)

        return splatTestUtils.viewInstancedAndTwin(instanced, twin)

    def test_TP_35_point_instancer_draws_each_instance_like_a_baked_prim(self):
        """Three instances of a splat prototype match three baked prims at the same transforms."""
        instanced, twin = self._pointInstancerAndTwin(self._elongated, self._xforms())
        self._assertSameImage(instanced, twin, 'point instancer')
        # The three instances are all there.
        self.assertTrue(all(c is not None for c in splatTestUtils.thirdColors(instanced)))

    def test_TP_36_nested_instancers_compose_their_transforms(self):
        """Every instance of an inner instancer draws at the transform composed with the outer."""
        xform = splatTestUtils.instanceXform
        inner = [xform((-1.2, 0, 0)), xform((1.2, 0, 0), 90)]
        outer = [xform((0, 1.2, 0)), xform((0, -1.2, 0), 90)]

        def nestedPrototype(stage, path):
            splatTestUtils.defineInstancedSplat(stage, path, self._elongated, inner)

        def instanced(stage):
            splatTestUtils.defineInstancedSplat(stage, '/outer', nestedPrototype, outer)

        def twin(stage):
            splatTestUtils.defineBakedTwin(stage, '/twin', self._elongated,
                                           [i * o for o in outer for i in inner])

        images = splatTestUtils.viewInstancedAndTwin(instanced, twin)
        self._assertSameImage(images[0], images[1], 'nested instancers')

    def test_TP_37_native_instances_draw_at_their_own_transform(self):
        """Native instances of a splat prim match baked prims at the instance transforms."""
        xforms = self._xforms()

        def instanced(stage):
            splatTestUtils.defineNativeInstances(stage, self._elongated, xforms)

        def twin(stage):
            splatTestUtils.defineBakedTwin(stage, '/twin', self._elongated, xforms)

        images = splatTestUtils.viewInstancedAndTwin(instanced, twin)
        self._assertSameImage(images[0], images[1], 'native instances')
        self.assertTrue(all(c is not None for c in splatTestUtils.thirdColors(images[0])))

    def test_TP_38_rotated_instance_rotates_the_ellipse(self):
        """An instance rotated by 90 degrees about Z has a vertical ellipse of the same shape."""
        xform = splatTestUtils.instanceXform

        def build(stage):
            splatTestUtils.defineInstancedSplat(
                stage, '/PI', lambda s, p: self._splat(s, p, scale=(0.6, 0.2, 0.2)),
                [xform((0, 0, 0), 90)])

        splatTestUtils.newScene()
        _, stage = splatTestUtils.createStage()
        build(stage)
        splatTestUtils.frameScene()
        image = splatTestUtils.viewImage()
        major, minor, angle = self._axes(image, region=splatTestUtils.middleBand(image))
        self.assertAlmostEqual(major / minor, 3.0, delta=0.4)
        self.assertGreater(abs(angle), math.radians(80))

    def test_TP_39_each_instance_evaluates_sh_in_its_own_local_space(self):
        """Two instances rotated differently show the colors of the baked prims from one camera."""
        coeffs = splatTestUtils.sampleCoefficients(4, seed=3.0, amplitude=0.7)
        xform = splatTestUtils.instanceXform
        xforms = [xform((-2.2, 0, 0)), xform((2.2, 0, 0), 90, axis=(0, 1, 0))]

        def proto(stage, path):
            return splatTestUtils.defineSplat(
                stage, path, [(0, 0, 0)], scales=[(0.7, 0.7, 0.7)], opacities=[1.0],
                orientations=[(1, 0, 0, 0)], shCoeffs=coeffs, shDegree=1)

        instanced, twin = self._pointInstancerAndTwin(proto, xforms)
        camera = splatTestUtils.cameraPosition(0)
        left, right = splatTestUtils.halfColors(instanced)
        for name, color, matrix in (('left', left, xforms[0]), ('right', right, xforms[1])):
            local = splatTestUtils.localViewDirection((0, 0, 0), camera, matrix)
            self._assertColor(color, splatTestUtils.shColor(coeffs, local, 1), msg=name)
        self.assertGreater(max(abs(a - b) for a, b in zip(left, right)), 12)
        self._assertSameImage(instanced, twin, 'sh instances')

    def test_TP_40_added_instance_shows_one_more_copy(self):
        """Adding an instance to the instancer draws one more copy at its transform."""
        xforms = self._xforms()
        splatTestUtils.newScene()
        _, stage = splatTestUtils.createStage()
        instancer = splatTestUtils.defineInstancedSplat(stage, '/PI', self._elongated, xforms[:2])
        splatTestUtils.frameScene()
        before = splatTestUtils.viewImage()
        self.assertIsNone(splatTestUtils.thirdColors(before)[2])

        splatTestUtils.setInstanceXforms(instancer, xforms)
        after = splatTestUtils.viewImage()

        _, twin = self._pointInstancerAndTwin(self._elongated, xforms)
        self._assertSameImage(after, twin, 'after adding an instance')
        self.assertIsNotNone(splatTestUtils.thirdColors(after)[2])

    def test_TP_41_instancer_without_instances_draws_nothing(self):
        """An instancer with no instances draws nothing, and instances come back when added."""
        xforms = self._xforms()
        splatTestUtils.newScene()
        _, stage = splatTestUtils.createStage()
        instancer = splatTestUtils.defineInstancedSplat(stage, '/PI', self._elongated, xforms)
        splatTestUtils.frameScene()
        full = splatTestUtils.viewImage()
        band = splatTestUtils.middleBand(full)
        self.assertGreater(splatTestUtils.coverage(full, region=band), 0.002)

        splatTestUtils.setInstanceXforms(instancer, [])
        empty = splatTestUtils.viewImage()
        self.assertEqual(splatTestUtils.coverage(empty, region=band), 0.0)

        splatTestUtils.setInstanceXforms(instancer, xforms)
        again = splatTestUtils.viewImage()
        self.assertLess(splatTestUtils.differingFraction(full, again), 0.0005)

    def test_TP_42_prim_that_stops_being_instanced_draws_once_with_the_same_look(self):
        """
        A splat prim that stops being instanced draws once at its own transform with the same
        look, and Hydra draws it through a new plain Rprim.

        No USD edit turns a live instanced Rprim into a plain one: the instanced Rprims have
        their own ids (point instancer prototypes and native instance prototypes), Hydra
        removes them and inserts plain Rprims under the prim paths. The splat therefore has no
        instanced-to-plain switch, and this test checks the replacement it relies on: the Rprims
        synced after the edit are new ids, never the instanced ones.
        """
        sync = 'HdVP2GaussianSplat::Sync'
        xform = splatTestUtils.instanceXform

        def assertReplaced(before, after, msg):
            self.assertTrue(before, '%s: no instanced Rprim synced' % msg)
            self.assertTrue(after, '%s: no Rprim synced after the edit' % msg)
            self.assertEqual(before & after, set(),
                             '%s: an instanced Rprim synced again: %s' % (msg, before & after))

        # A point instancer whose only instance is the identity: it draws the prototype at the
        # prototype's own transform. Retyping the instancer stops the instancing, and the
        # prototype prim draws once by itself, with the same look.
        splatTestUtils.newScene()
        _, stage = splatTestUtils.createStage()
        splatTestUtils.frameScene()
        def prototype(stage, path):
            prim = self._elongated(stage, path)
            splatTestUtils.UsdGeom.Xformable(prim).AddTransformOp().Set(
                xform((1.5, 0.5, 0), 30))
            return prim

        instancedIds = splatTestUtils.profileEventNames(
            lambda: splatTestUtils.defineInstancedSplat(stage, '/PI', prototype, [xform()]), sync)
        instanced = splatTestUtils.viewImage()
        band = splatTestUtils.middleBand(instanced)
        self.assertGreater(splatTestUtils.coverage(instanced, region=band), 0.002)

        plainIds = splatTestUtils.profileEventNames(
            lambda: stage.GetPrimAtPath('/PI').SetTypeName('Xform'), sync)
        plain = splatTestUtils.viewImage()
        assertReplaced(instancedIds, plainIds, 'point instancer')
        self.assertLess(splatTestUtils.differingFraction(instanced, plain), 0.001)

        # Native instances that stop being instanced, and are instanced again.
        xforms = self._xforms()
        splatTestUtils.newScene()
        _, stage = splatTestUtils.createStage()
        splatTestUtils.frameScene()
        prims = []
        instancedIds = splatTestUtils.profileEventNames(
            lambda: prims.extend(
                splatTestUtils.defineNativeInstances(stage, self._elongated, xforms)), sync)
        instanced = splatTestUtils.viewImage()
        self.assertGreater(splatTestUtils.coverage(instanced, region=band), 0.002)
        self.assertTrue(all(c is not None for c in splatTestUtils.thirdColors(instanced)))

        def setInstanceable(value):
            for prim in prims:
                prim.SetInstanceable(value)

        plainIds = splatTestUtils.profileEventNames(lambda: setInstanceable(False), sync)
        plain = splatTestUtils.viewImage()
        assertReplaced(instancedIds, plainIds, 'native instances')
        self.assertLess(splatTestUtils.differingFraction(instanced, plain), 0.001)

        againIds = splatTestUtils.profileEventNames(lambda: setInstanceable(True), sync)
        again = splatTestUtils.viewImage()
        assertReplaced(plainIds, againIds, 'native instances again')
        self.assertLess(splatTestUtils.differingFraction(instanced, again), 0.001)

    def test_TP_43_invisible_instance_is_not_drawn(self):
        """An instance in the invisible ids does not draw and the others do."""
        xforms = self._xforms()

        def hide(instancer):
            instancer.CreateInvisibleIdsAttr().Set(splatTestUtils.Vt.Int64Array([1]))

        instanced, twin = self._pointInstancerAndTwin(
            self._elongated, xforms, instancerSetup=hide, twinXforms=[xforms[0], xforms[2]])
        self._assertSameImage(instanced, twin, 'invisible instance')
        thirds = splatTestUtils.thirdColors(instanced)
        self.assertIsNone(thirds[1])
        self.assertIsNotNone(thirds[0])
        self.assertIsNotNone(thirds[2])

    def _instanceLayer(self, shapeNode, instanceNames, name='splatWireLayer'):
        """A display layer that holds the given native instances."""
        cmds.createDisplayLayer(name=name, empty=True, noRecurse=True)
        selection = om.MSelectionList()
        selection.add(name)
        layer = om.MFnDisplayLayer(selection.getDependNode(0))
        shapePath = cmds.ls(shapeNode, long=True)[0]
        for instanceName in instanceNames:
            layer.add('%s,/Scene/%s' % (shapePath, instanceName))
        return name

    def test_TP_44_instance_in_a_wireframe_layer_is_not_drawn(self):
        """An instance whose display layer forces wireframe is not drawn, the others are."""
        xforms = self._xforms()
        splatTestUtils.newScene()
        shapeNode, stage = splatTestUtils.createStage()
        splatTestUtils.defineNativeInstances(stage, self._elongated, xforms)
        splatTestUtils.frameScene()
        before = splatTestUtils.viewImage()
        self.assertIsNotNone(splatTestUtils.thirdColors(before)[1])

        layer = self._instanceLayer(shapeNode, ['Instance1'])
        cmds.setAttr(layer + '.shading', 0)
        after = splatTestUtils.viewImage()
        thirds = splatTestUtils.thirdColors(after)
        self.assertIsNone(thirds[1])
        self.assertIsNotNone(thirds[0])
        self.assertIsNotNone(thirds[2])

        # Without the override all three draw again.
        cmds.setAttr(layer + '.shading', 1)
        back = splatTestUtils.viewImage()
        self.assertLess(splatTestUtils.differingFraction(before, back), 0.001)

        # The two other instances look like two baked prims (a new scene).
        _, twin = self._pointInstancerAndTwin(self._elongated, xforms,
                                              twinXforms=[xforms[0], xforms[2]])
        self._assertSameImage(after, twin, 'instance in a wireframe layer')

    # Per-instance highlight: three big red instances, side by side.

    @staticmethod
    def _bigRound(stage, path):
        """The prototype of the highlight and picking tests: a big red round splat."""
        return testVP2RenderDelegateGaussianSplat._splat(stage, path, scale=(0.8, 0.8, 0.8))

    @staticmethod
    def _rowXforms():
        """Three instance transforms side by side along X."""
        xform = splatTestUtils.instanceXform
        return [xform((-2.5, 0, 0)), xform((0, 0, 0)), xform((2.5, 0, 0))]

    def _pointInstancerRow(self, stage):
        """A point instancer /PI of three big red instances, side by side."""
        splatTestUtils.defineInstancedSplat(stage, '/PI', self._bigRound, self._rowXforms())

    def _highlightScene(self, stage):
        splatTestUtils.defineNativeInstances(stage, self._bigRound, self._rowXforms())

    def _thirdShifts(self, before, after):
        """Per third, the mean rgb change in 0..255 from before to after."""
        return self._colorShifts(before, after, splatTestUtils.thirdColors)

    def _assertNoShift(self, shift, msg):
        for s in shift:
            self.assertAlmostEqual(s, 0.0, delta=1.5, msg='%s: %s' % (msg, shift))

    def _selectInstances(self, names):
        splatTestUtils.select(self._shapeNode, ['/Scene/' + n for n in names])

    def test_TP_45_selected_instance_shifts_toward_the_selection_color_alone(self):
        """Only the selected instance shifts, by 5% toward the selection color."""
        before = self._renderView(self._highlightScene)
        self._selectInstances(['Instance1'])
        after = splatTestUtils.viewImage()
        active, lead = self._highlightColors()
        left, middle, right = self._thirdShifts(before, after)
        self._assertShift(middle, lead, 'selected instance')
        self._assertNoShift(left, 'left instance')
        self._assertNoShift(right, 'right instance')

    def test_TP_46_selected_instancer_shifts_every_instance(self):
        """Selecting the whole instancer shifts all its instances toward the selection color."""
        before = self._renderView(self._pointInstancerRow)
        splatTestUtils.select(self._shapeNode, '/PI')
        after = splatTestUtils.viewImage()
        active, lead = self._highlightColors()
        for shift in self._thirdShifts(before, after):
            self._assertShift(shift, lead, 'instancer selected')

    def test_TP_47_lead_and_active_instances_use_their_own_colors(self):
        """With two selected instances, the lead one blends the lead color and the other the active."""
        before = self._renderView(self._highlightScene)
        active, lead = self._highlightColors()
        self.assertGreater(max(abs(l - a) for l, a in zip(lead, active)), 0.1)
        self._selectInstances(['Instance0', 'Instance2'])
        after = splatTestUtils.viewImage()
        shifts = self._thirdShifts(before, after)
        self._assertNoShift(shifts[1], 'unselected instance')
        # Which of the two is the lead follows the selection order, whatever the instance order.
        kinds = []
        for shift in (shifts[0], shifts[2]):
            for name, target in (('active', active), ('lead', lead)):
                expected = self._expectedShift(target)
                if all(abs(a - b) < 3.0 for a, b in zip(shift, expected)):
                    kinds.append(name)
        self.assertEqual(sorted(kinds), ['active', 'lead'], shifts)

        self._selectInstances(['Instance2', 'Instance0'])
        swapped = self._thirdShifts(before, splatTestUtils.viewImage())
        self.assertNotEqual([tuple(round(v) for v in s) for s in swapped[0::2]],
                            [tuple(round(v) for v in s) for s in shifts[0::2]])

    def test_TP_48_deselected_instance_returns_to_its_color(self):
        """A deselected instance goes back to its normal color and the others do not change."""
        before = self._renderView(self._highlightScene)
        self._selectInstances(['Instance0', 'Instance1'])
        both = splatTestUtils.viewImage()
        self.assertEqual(
            sum(1 for s in self._thirdShifts(before, both)
                if max(abs(v) for v in s) > 4), 2)

        self._selectInstances(['Instance0'])
        one = splatTestUtils.viewImage()
        shifts = self._thirdShifts(before, one)
        self.assertEqual(sum(1 for s in shifts if max(abs(v) for v in s) > 4), 1)

        splatTestUtils.clearSelection()
        none = splatTestUtils.viewImage()
        self.assertLess(splatTestUtils.differingFraction(before, none, tolerance=2), 0.0005)

    # Picking: Maya picks a splat by its center point.

    def _pickInstances(self):
        """Pick one instance of a point instancer with a click, in the Instances pick mode."""
        name = mayaUsdLib.OptionVarTokens.PointInstancesPickMode
        previous = cmds.optionVar(query=name) or None
        cmds.optionVar(stringValue=(name, 'Instances'))
        self.addCleanup(lambda: cmds.optionVar(remove=name) if previous is None
                        else cmds.optionVar(stringValue=(name, previous)))

    def test_TP_49_click_on_an_instance_center_picks_that_instance(self):
        """Clicking the center of an instance's splat picks that instance of the prim."""
        self._pickInstances()
        self._renderView(self._pointInstancerRow)
        splatTestUtils.clearSelection()
        splatTestUtils.clickInView(splatTestUtils.widgetPosition((2.5, 0, 0)))
        picked = splatTestUtils.selectedPathStrings()
        self.assertEqual(len(picked), 1, picked)
        self.assertTrue(picked[0].endswith(',/PI/2'), picked)

        splatTestUtils.clickInView(splatTestUtils.widgetPosition((-2.5, 0, 0)))
        picked = splatTestUtils.selectedPathStrings()
        self.assertEqual(len(picked), 1, picked)
        self.assertTrue(picked[0].endswith(',/PI/0'), picked)

    def test_TP_50_click_inside_a_quad_away_from_the_center_picks_nothing(self):
        """Clicking inside an ellipse, away from any splat center, picks nothing."""
        self._pickInstances()
        self._renderView(self._pointInstancerRow)
        splatTestUtils.clearSelection()
        center = splatTestUtils.widgetPosition((2.5, 0, 0))
        # The ellipse of a splat of scale 0.8 is far wider than this offset.
        offset = splatTestUtils.QtCore.QPoint(center.x() + 60, center.y())
        splatTestUtils.clickInView(offset)
        self.assertEqual(splatTestUtils.selectedPathStrings(), [])

    def test_TP_51_many_instances_do_not_thicken_the_splats(self):
        """100,000 instances of a small half-opaque splat cover as much, per splat, as 100 do."""
        np = splatTestUtils.np
        columns, rows, spacing = 500, 200, 0.0125

        def build(count):
            def builder(stage):
                proto = lambda s, p: self._splat(s, p, scale=(0.004, 0.004, 0.004), opacity=0.5)
                instancer = splatTestUtils.UsdGeom.PointInstancer.Define(stage, '/PI')
                stage.DefinePrim('/PI/Prototypes', 'Scope')
                proto(stage, '/PI/Prototypes/splat')
                instancer.CreatePrototypesRel().SetTargets([splatTestUtils.Sdf.Path('/PI/Prototypes/splat')])
                xs, ys = np.meshgrid(np.arange(columns) - columns / 2.0,
                                     np.arange(rows) - rows / 2.0)
                positions = np.stack([xs.ravel() * spacing, ys.ravel() * spacing,
                                      np.zeros(xs.size)], axis=1).astype(np.float32)
                if count < positions.shape[0]:
                    # The first 'count' lattice points of a block of the grid.
                    block = (np.abs(xs.ravel()) < 5) & (np.abs(ys.ravel()) < 5)
                    positions = positions[block][:count]
                instancer.CreatePositionsAttr().Set(
                    splatTestUtils.Vt.Vec3fArray.FromNumpy(positions))
                instancer.CreateProtoIndicesAttr().Set(
                    splatTestUtils.Vt.IntArray.FromNumpy(np.zeros(positions.shape[0], dtype=np.int32)))
            return builder

        results = {}
        for count in (100, columns * rows):
            splatTestUtils.newScene()
            _, stage = splatTestUtils.createStage()
            build(count)(stage)
            splatTestUtils.frameScene()
            image = splatTestUtils.viewImage()
            region = (0, image.height() // 5, image.width(), image.height() * 4 // 5)
            area = (region[2] - region[0]) * (region[3] - region[1])
            results[count] = splatTestUtils.coverage(image, region=region) * area / count
        self.assertGreater(results[columns * rows], 0.0)
        self.assertAlmostEqual(results[columns * rows] / results[100], 1.0, delta=0.2,
                               msg=str(results))

    def test_TP_52_wireframe_viewport_draws_quads_for_every_instance(self):
        """In a wireframe viewport every splat of every instance is a fixed-size quad."""
        xforms = self._xforms()

        def proto(stage, path):
            return splatTestUtils.defineSplat(
                stage, path, [(-0.4, 0, 0), (0.4, 0.2, 0)], scales=[(0.3, 0.3, 0.3)] * 2,
                opacities=[1.0] * 2, orientations=[(1, 0, 0, 0)] * 2,
                shCoeffs=[shDcForColor(self.RED), shDcForColor(self.GREEN)], shDegree=0)

        def instanced(stage):
            splatTestUtils.defineInstancedSplat(stage, '/PI', proto, xforms)

        def twin(stage):
            splatTestUtils.defineBakedTwin(stage, '/twin', proto, xforms)

        images = splatTestUtils.viewInstancedAndTwin(
            instanced, twin, viewSetup=splatTestUtils.setWireframeViewport)
        self._assertSameImage(images[0], images[1], 'wireframe instances', minCoverage=0.0001)
        # Six quads, all with the same size.
        quads = splatTestUtils.clusters(
            splatTestUtils.foregroundMask(images[0], region=splatTestUtils.middleBand(images[0])))
        self.assertEqual(len(quads), 6, [len(q) for q in quads])
        sizes = [splatTestUtils.boundingSize(q) for q in quads]
        for axis in (0, 1):
            extent = [size[axis] for size in sizes]
            self.assertLessEqual(max(extent) - min(extent), 1, sizes)
        self.assertGreater(min(size[0] for size in sizes), 3, sizes)


    # ------------------------------------------------------------------
    # Large scenes (skipped when the data files are missing)
    # ------------------------------------------------------------------

    @staticmethod
    def _recordResult(name, lines):
        """Write measurements of a large scene next to the test output, for the record."""
        with open(os.path.abspath('%s.txt' % name), 'w') as out:
            out.write('\n'.join(lines) + '\n')

    @_requireFiles(EAGLE_PLY)
    def test_TP_53_eagle_degree_three_draws_without_error_from_two_cameras(self):
        """Eagle (253,172 splats, degree 3) draws with no error or fallback, and changes with the camera."""
        with splatTestUtils.LogCapture() as log:
            splatTestUtils.newScene()
            _, stage = splatTestUtils.createStage()
            prim, splats = splatTestUtils.definePlySplat(stage, '/eagle', EAGLE_PLY)
            self.assertEqual(splats['positions'].shape[0], 253172)
            self.assertEqual(splats['shDegree'], 3)
            center, radius = splatTestUtils.boundingSphere(splats['positions'])

            images = []
            for degrees in (0, 70):
                splatTestUtils.orbitAround(center, radius, degrees)
                splatTestUtils.refreshFrames(2)
                images.append(splatTestUtils.viewImage())

        self.assertEqual(log.errors(), [])
        self.assertEqual(log.warnings(containing='plat'), [])
        self.assertEqual(log.lines(containing='band 0'), [])
        for image in images:
            self.assertGreater(splatTestUtils.coverage(image), 0.01)
        self.assertGreater(splatTestUtils.differingFraction(images[0], images[1]), 0.01)

    @_requireFiles(EAGLE_INSTANCED_USDA, EAGLE_PROTOTYPE_USD)
    def test_TP_54_eagle_instanced_draws_every_instance_like_the_baked_twin(self):
        """eagle_instanced (12 instances, about 3.0 million splats) draws like 12 baked eagles."""
        source = splatTestUtils.Usd.Stage.Open(EAGLE_INSTANCED_USDA)
        instancerPrim = source.GetDefaultPrim()
        instancer = splatTestUtils.UsdGeom.PointInstancer(instancerPrim)
        default = splatTestUtils.Usd.TimeCode.Default()
        # Without the prototype transform: the baked eagle keeps its own transform below.
        xforms = instancer.ComputeInstanceTransformsAtTime(
            default, default, splatTestUtils.UsdGeom.PointInstancer.ExcludeProtoXform)
        self.assertEqual(len(xforms), 12)
        bounds = splatTestUtils.UsdGeom.BBoxCache(
            splatTestUtils.Usd.TimeCode.Default(),
            [splatTestUtils.UsdGeom.Tokens.default_]).ComputeWorldBound(instancerPrim)
        box = bounds.ComputeAlignedRange()
        center = tuple((box.GetMin()[i] + box.GetMax()[i]) / 2.0 for i in range(3))
        radius = (box.GetMax() - box.GetMin()).GetLength() / 2.0

        def instanced(stage):
            stage.DefinePrim('/scene').GetReferences().AddReference(EAGLE_INSTANCED_USDA)

        def twin(stage):
            stage.DefinePrim('/twin', 'Scope')
            for i, xform in enumerate(xforms):
                splatTestUtils.UsdGeom.Xform.Define(stage, '/twin/i%d' % i).AddTransformOp().Set(
                    splatTestUtils.Gf.Matrix4d(xform))
                eagle = stage.DefinePrim('/twin/i%d/eagle' % i)
                eagle.GetReferences().AddReference(EAGLE_PROTOTYPE_USD)

        def measure(builder, name):
            with splatTestUtils.LogCapture() as log:
                splatTestUtils.newScene()
                _, stage = splatTestUtils.createStage()
                builder(stage)
                splatTestUtils.orbitAround(center, radius, 0, distanceFactor=1.6)
                splatTestUtils.refreshFrames(2)
                image = splatTestUtils.viewImage()
                image.save(os.path.abspath('TP_54_%s.png' % name))
            self.assertEqual(log.errors(), [], name)
            self.assertEqual(log.warnings(containing='plat'), [], name)
            return image

        instancedImage = measure(instanced, 'instanced')
        twinImage = measure(twin, 'twin')
        instancedCoverage = splatTestUtils.coverage(instancedImage)
        twinCoverage = splatTestUtils.coverage(twinImage)
        self._recordResult('TP_54', ['instanced coverage %f' % instancedCoverage,
                                     'twin coverage %f' % twinCoverage,
                                     'differing fraction %f' % splatTestUtils.differingFraction(
                                         instancedImage, twinImage)])
        self.assertGreater(instancedCoverage, 0.02)
        self.assertAlmostEqual(instancedCoverage / twinCoverage, 1.0, delta=0.05)
        self.assertLess(splatTestUtils.differingFraction(instancedImage, twinImage), 0.02)

    @_requireFiles(FLY_XXL_PLY)
    def test_TP_55_fly_xxl_orbit_does_no_sync_or_commit_and_reports_at_most_one_error(self):
        """Fly XXL (3,506,799 splats) draws, or reports one error and draws band 0, and orbits without sync."""
        scopes = ['HdVP2GaussianSplat::Sync', 'CommitPositions']
        with splatTestUtils.LogCapture() as log:
            splatTestUtils.newScene()
            _, stage = splatTestUtils.createStage()
            prim, splats = splatTestUtils.definePlySplat(stage, '/fly', FLY_XXL_PLY)
            self.assertEqual(splats['positions'].shape[0], 3506799)
            center, radius = splatTestUtils.boundingSphere(splats['positions'])
            del splats

            splatTestUtils.orbitAround(center, radius, 0)
            first = splatTestUtils.viewImage()
            splatTestUtils.refreshFrames(3)

            def orbit():
                for degrees in (10, 20, 30, 40, 50, 60):
                    splatTestUtils.orbitAround(center, radius, degrees)
                    cmds.refresh(force=True)

            counts = splatTestUtils.profileEvents(orbit, scopes)
            last = splatTestUtils.viewImage()

        # Record any Maya per-buffer limit message or splat error for the notes.
        reported = log.errors() + log.warnings(containing='plat') + log.lines(containing='limit')
        self._recordResult('TP_55', ['radius %f' % radius, 'sync/commit %s' % counts,
                                     'coverage first %f last %f' % (
                                         splatTestUtils.coverage(first),
                                         splatTestUtils.coverage(last))] + reported)
        self.assertEqual(counts, {'HdVP2GaussianSplat::Sync': 0, 'CommitPositions': 0})
        # At most one splat error, and then it is the band 0 fallback.
        splatErrors = log.errors(containing='Gaussian splat')
        self.assertLessEqual(len(splatErrors), 1, splatErrors)
        if splatErrors:
            self.assertIn('band 0', splatErrors[0])
        self.assertEqual([e for e in log.errors() if e not in splatErrors], [])
        self.assertGreater(splatTestUtils.coverage(first), 0.001)
        self.assertGreater(splatTestUtils.coverage(last), 0.001)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
