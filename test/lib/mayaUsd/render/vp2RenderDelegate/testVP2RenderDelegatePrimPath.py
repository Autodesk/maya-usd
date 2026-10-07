#!/usr/bin/env mayapy
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
import imageUtils
import mayaUtils
import usdUtils

from mayaUsd import lib as mayaUsdLib
from mayaUsd import ufe as mayaUsdUfe

from maya import cmds

from pxr import Gf, Sdf, Usd, UsdGeom, UsdShade

import ufe

import os


class testVP2RenderDelegatePrimPath(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate when using a primPath.
    """

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Y-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='y')

        inputPath = fixturesUtils.setUpClass(__file__, initializeStandalone=False, loadPlugin=False)
        cls._baselineDir = os.path.join(inputPath,'VP2RenderDelegatePrimPathTest', 'baseline')
        cls._testDir = os.path.abspath('.')

    def assertSnapshotClose(self, imageName):
        baselineImage = os.path.join(self._baselineDir, imageName)
        snapshotImage = os.path.join(self._testDir, imageName)
        imageUtils.snapshot(snapshotImage, width=960, height=540)
        return self.assertImagesClose(baselineImage, snapshotImage)

    @staticmethod
    def _GetUfePath(name):
        mayaSegment = mayaUtils.createUfePathSegment('|stage1|stageShape1')
        usdSegmentString = mayaUsdUfe.usdPathToUfePathSegment(name, -1)
        usdSegment = usdUtils.createUfePathSegment(usdSegmentString)
        ufePath = ufe.Path([mayaSegment, usdSegment])
        return ufePath

    @staticmethod
    def _GetSceneItem(name):
        ufePath = testVP2RenderDelegatePrimPath._GetUfePath(name)
        ufeItem = ufe.Hierarchy.createItem(ufePath)
        return ufeItem

    def _SnapshotScopedAsset(self, name, binding):
        """Draws /Asset/Geo/Cube with primPath /Asset/Geo and snapshots it. The
        red material lives outside primPath, in /Asset/Looks, and binding is
        'collection' (a collection on /Asset/Looks, bound on /Asset), 'direct'
        (bound on the cube) or None."""
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin('mayaUsdPlugin')
        shapeNode, stage = mayaUtils.createProxyAndStage()

        UsdGeom.Xform.Define(stage, '/Asset')
        UsdGeom.Xform.Define(stage, '/Asset/Geo')
        UsdGeom.Cube.Define(stage, '/Asset/Geo/Cube')
        UsdGeom.Scope.Define(stage, '/Asset/Looks')
        material = UsdShade.Material.Define(stage, '/Asset/Looks/Red')
        shader = UsdShade.Shader.Define(stage, '/Asset/Looks/Red/Surface')
        shader.CreateIdAttr('UsdPreviewSurface')
        shader.CreateInput('diffuseColor', Sdf.ValueTypeNames.Color3f).Set(Gf.Vec3f(1, 0, 0))
        material.CreateSurfaceOutput().ConnectToSource(shader.ConnectableAPI(), 'surface')

        if binding == 'collection':
            collection = Usd.CollectionAPI.Apply(stage.GetPrimAtPath('/Asset/Looks'), 'redGeo')
            collection.CreateIncludesRel().AddTarget('/Asset/Geo/Cube')
            UsdShade.MaterialBindingAPI.Apply(stage.GetPrimAtPath('/Asset')).Bind(
                collection, material, 'redGeo')
        elif binding == 'direct':
            UsdShade.MaterialBindingAPI.Apply(stage.GetPrimAtPath('/Asset/Geo/Cube')).Bind(material)

        cmds.setAttr(shapeNode + '.primPath', '/Asset/Geo', type='string')
        cmds.modelEditor('modelPanel4', edit=True, grid=False)
        cmds.viewPlace('persp', eye=(5, 4, 7), lookAt=(0, 0, 0), up=(0, 1, 0))
        snapshot = os.path.join(self._testDir, '%s.png' % name)
        imageUtils.snapshot(snapshot, width=960, height=540)
        return snapshot

    def testCollectionBindingOutsidePrimPath(self):
        """A collection binding that applies to a prim inside primPath must
        resolve even when the binding is authored on an ancestor of primPath
        and the collection on a prim outside it."""
        collection = self._SnapshotScopedAsset('collectionBinding', 'collection')
        direct = self._SnapshotScopedAsset('directBinding', 'direct')
        unbound = self._SnapshotScopedAsset('noBinding', None)

        # Without a visible material the comparison would prove nothing.
        self.assertGreater(imageUtils.imageDiff(direct, unbound), self.AVG_CHANNEL_DIFF)
        self.assertImagesClose(direct, collection)

    def testPrimPath(self):
        # Start off with nothing, whatever an earlier test left loaded.
        cmds.file(force=True, new=True)
        self.assertSnapshotClose('empty.png')

        def testSinglePrim(primPath, imageName):
            cmds.setAttr( 'stageShape1.primPath', primPath, type="string")
            self.assertSnapshotClose('%s.png' % imageName)

            globalSelection = ufe.GlobalSelection.get()
            globalSelection.clear()
            sceneItem = testVP2RenderDelegatePrimPath._GetSceneItem(primPath)
            globalSelection.append(sceneItem)

            cmds.move(-5,0,0, relative=True)
            self.assertSnapshotClose('%s_moved.png' % imageName)

        mayaUtils.openPrimPathScene()

        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        self.assertSnapshotClose('initial.png')

        testSinglePrim("/Cube1", "Cube1")
        testSinglePrim("/Cube1/Cube2", "Cube2")
        testSinglePrim("/Cube1/Cube2/Cube3", "Cube3")

        globalSelection.clear()
        cmds.setAttr( 'stageShape1.primPath', "", type="string")
        self.assertSnapshotClose('final.png')

        # Test with a invalid prim path. We should get nothing
        globalSelection.clear()
        cmds.setAttr( 'stageShape1.primPath', "/invalidPrim", type="string")
        self.assertSnapshotClose('empty.png')


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
