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

from pxr import Gf, Sdf, UsdGeom

import ufe

import os


class testVP2RenderDelegatePointInstanceSelection(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate when selecting
    instances of a PointInstancer.
    """

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegatePointInstanceSelectionTest', 'baseline')

        cls._testDir = os.path.abspath('.')

        # Store the previous USD point instances pick mode and selection kind
        # (or None if unset) so we can restore the state later.
        cls._pointInstancesPickModeOptionVarName = mayaUsdLib.OptionVarTokens.PointInstancesPickMode
        cls._prevPointInstancesPickMode = cmds.optionVar(
            query=cls._pointInstancesPickModeOptionVarName) or None

        cls._selectionKindOptionVarName = mayaUsdLib.OptionVarTokens.SelectionKind
        cls._prevSelectionKind = cmds.optionVar(
            query=cls._selectionKindOptionVarName) or None

        # Set the USD point instances pick mode to "Instances" so that we pick
        # individual point instances during the test.
        cmds.optionVar(stringValue=(
            cls._pointInstancesPickModeOptionVarName, 'Instances'))

        # Clear any setting for selection kind.
        cmds.optionVar(remove=cls._selectionKindOptionVarName)

    @classmethod
    def tearDownClass(cls):
        # Restore the previous USD point instances pick mode and selection
        # kind, or remove if they were unset.
        if cls._prevPointInstancesPickMode is None:
            cmds.optionVar(remove=cls._pointInstancesPickModeOptionVarName)
        else:
            cmds.optionVar(stringValue=
                (cls._pointInstancesPickModeOptionVarName,
                    cls._prevPointInstancesPickMode))

        if cls._prevSelectionKind is None:
            cmds.optionVar(remove=cls._selectionKindOptionVarName)
        else:
            cmds.optionVar(stringValue=
                (cls._selectionKindOptionVarName, cls._prevSelectionKind))

    @staticmethod
    def _GetUfePath(instanceIndex=-1):
        mayaSegment = mayaUtils.createUfePathSegment('|UsdProxy|UsdProxyShape')
        usdSegmentString = mayaUsdUfe.usdPathToUfePathSegment(
            '/PointInstancerGrid/PointInstancer', instanceIndex)
        usdSegment = usdUtils.createUfePathSegment(usdSegmentString)
        ufePath = ufe.Path([mayaSegment, usdSegment])
        return ufePath

    @staticmethod
    def _GetSceneItem(instanceIndex=-1):
        ufePath = testVP2RenderDelegatePointInstanceSelection._GetUfePath(
            instanceIndex)
        ufeItem = ufe.Hierarchy.createItem(ufePath)
        return ufeItem

    def assertSnapshotClose(self, imageName):
        baselineImage = os.path.join(self._baselineDir, imageName)
        snapshotImage = os.path.join(self._testDir, imageName)
        imageUtils.snapshot(snapshotImage, width=960, height=540)
        return self.assertImagesClose(baselineImage, snapshotImage)

    def _RunTest(self):
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        self.assertSnapshotClose('%s_unselected.png' % self._testName)

        # Select one instance.
        sceneItem = self._GetSceneItem(0)
        globalSelection.append(sceneItem)
        self.assertSnapshotClose('%s_select_one.png' % self._testName)
        globalSelection.clear()

        # We'll populate a new selection and swap that into the global
        # selection to minimize the overhead of modifying the global selection
        # one item at a time.
        newSelection = ufe.Selection()

        # Select the first seven instances. The most recently selected item
        # should get "Lead" highlighting.
        for instanceIndex in range(7):
            sceneItem = self._GetSceneItem(instanceIndex)
            newSelection.append(sceneItem)
        globalSelection.replaceWith(newSelection)
        self.assertSnapshotClose('%s_select_seven.png' % self._testName)
        globalSelection.clear()
        newSelection.clear()

        # Select the back half of the instances.
        for instanceIndex in range(self._numInstances // 2, self._numInstances):
            sceneItem = self._GetSceneItem(instanceIndex)
            newSelection.append(sceneItem)
        globalSelection.replaceWith(newSelection)
        self.assertSnapshotClose('%s_select_half.png' % self._testName)
        globalSelection.clear()
        newSelection.clear()

        # Select all instances
        for instanceIndex in range(self._numInstances):
            sceneItem = self._GetSceneItem(instanceIndex)
            newSelection.append(sceneItem)
        globalSelection.replaceWith(newSelection)
        self.assertSnapshotClose('%s_select_all.png' % self._testName)
        globalSelection.clear()
        newSelection.clear()

        # Select the PointInstancer itself
        sceneItem = self._GetSceneItem()
        globalSelection.append(sceneItem)
        self.assertSnapshotClose('%s_select_PointInstancer.png' % self._testName)
        globalSelection.clear()

    @staticmethod
    def _DefineCube(stage, path, translate=(0, 0, 0)):
        cube = UsdGeom.Cube.Define(stage, path)
        cube.CreateSizeAttr(0.8)
        UsdGeom.XformCommonAPI(cube).SetTranslate(translate)

    @staticmethod
    def _DefinePointInstancer(stage, path, prototypePath, positions):
        instancer = UsdGeom.PointInstancer.Define(stage, path)
        instancer.CreatePrototypesRel().SetTargets([prototypePath])
        instancer.CreateProtoIndicesAttr([0] * len(positions))
        instancer.CreatePositionsAttr([Gf.Vec3f(*position) for position in positions])

    # Three point instances in a row; the tests select the middle one.
    _OUTER_POSITIONS = [(0, 0, 0), (1.5, 0, 0), (3, 0, 0)]
    # Two copies stacked inside each point instance, where a test needs them.
    _INNER_OFFSETS = [(0, 0, 0), (0, 1, 0)]

    def _SnapshotPointInstanceSelection(self, name, defineScene, instancerPath):
        """Draws the scene defineScene builds in a new scene, then snapshots it
        unselected and with point instance 1 of instancerPath selected."""
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin('mayaUsdPlugin')
        shapeNode, stage = mayaUtils.createProxyAndStage()
        defineScene(stage)
        cmds.modelEditor('modelPanel4', edit=True, grid=False)
        cmds.viewPlace('persp', eye=(1.5, 0.5, 6), lookAt=(1.5, 0.5, 0), up=(0, 1, 0))

        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        unselected = os.path.join(self._testDir, '%s_unselected.png' % name)
        imageUtils.snapshot(unselected, width=960, height=540)

        globalSelection.append(ufe.Hierarchy.createItem(ufe.Path([
            mayaUtils.createUfePathSegment(shapeNode),
            usdUtils.createUfePathSegment(
                mayaUsdUfe.usdPathToUfePathSegment(instancerPath, 1))])))
        selected = os.path.join(self._testDir, '%s_selected.png' % name)
        imageUtils.snapshot(selected, width=960, height=540)
        globalSelection.clear()
        return unselected, selected

    def _AssertPointInstanceSelectionMatchesTwin(self, name, defineScene, defineTwin, instancerPath):
        """Selecting point instance 1 must highlight exactly what selecting it
        highlights in a twin that draws the same but is instanced more simply."""
        images = self._SnapshotPointInstanceSelection(name, defineScene, instancerPath)
        twinImages = self._SnapshotPointInstanceSelection(name + 'Twin', defineTwin, instancerPath)

        # Without a visible highlight in the twin the comparison would prove nothing.
        self.assertGreater(imageUtils.imageDiff(*twinImages), self.AVG_CHANNEL_DIFF)
        for twinImage, image in zip(twinImages, images):
            self.assertImagesClose(twinImage, image)

    def testPointInstanceSelectionPrototypesOutsideInstancer(self):
        """A point instancer may target prototypes outside its own namespace."""
        def defineScene(stage):
            self._DefineCube(stage, '/Prototypes/Cube')
            # An over, so the prototype is not also drawn where it is authored.
            stage.GetPrimAtPath('/Prototypes').SetSpecifier(Sdf.SpecifierOver)
            self._DefinePointInstancer(stage, '/Instancer', '/Prototypes/Cube', self._OUTER_POSITIONS)

        def defineTwin(stage):
            self._DefinePointInstancer(
                stage, '/Instancer', '/Instancer/Prototypes/Cube', self._OUTER_POSITIONS)
            self._DefineCube(stage, '/Instancer/Prototypes/Cube')

        self._AssertPointInstanceSelectionMatchesTwin(
            'PrototypesOutsideInstancer', defineScene, defineTwin, '/Instancer')

    def testPointInstanceSelectionNativeInstancesInPrototype(self):
        """Native instances inside a point instancer prototype are drawn with
        nested instance ids; selecting a point instance highlights all of its
        native instances, and only those."""
        def defineScene(stage):
            stage.CreateClassPrim('/Asset')
            self._DefineCube(stage, '/Asset/Cube')
            self._DefinePointInstancer(
                stage, '/Instancer', '/Instancer/Prototypes/Pair', self._OUTER_POSITIONS)
            for index, offset in enumerate(self._INNER_OFFSETS):
                instance = UsdGeom.Xform.Define(stage, '/Instancer/Prototypes/Pair/Instance%d' % index)
                UsdGeom.XformCommonAPI(instance).SetTranslate(offset)
                instance.GetPrim().GetReferences().AddInternalReference('/Asset')
                instance.GetPrim().SetInstanceable(True)

        def defineTwin(stage):
            self._DefinePointInstancer(
                stage, '/Instancer', '/Instancer/Prototypes/Pair', self._OUTER_POSITIONS)
            for index, offset in enumerate(self._INNER_OFFSETS):
                copy = UsdGeom.Xform.Define(stage, '/Instancer/Prototypes/Pair/Instance%d' % index)
                UsdGeom.XformCommonAPI(copy).SetTranslate(offset)
                self._DefineCube(stage, '/Instancer/Prototypes/Pair/Instance%d/Cube' % index)

        self._AssertPointInstanceSelectionMatchesTwin(
            'NativeInstancesInPrototype', defineScene, defineTwin, '/Instancer')

    def testPointInstanceSelectionNestedPointInstancer(self):
        """A point instancer may instance another point instancer; selecting an
        outer point instance highlights all of its inner instances, and only
        those."""
        def defineScene(stage):
            self._DefinePointInstancer(
                stage, '/Instancer', '/Instancer/Prototypes/Inner', self._OUTER_POSITIONS)
            self._DefinePointInstancer(
                stage, '/Instancer/Prototypes/Inner',
                '/Instancer/Prototypes/Inner/Prototypes/Cube', self._INNER_OFFSETS)
            self._DefineCube(stage, '/Instancer/Prototypes/Inner/Prototypes/Cube')

        def defineTwin(stage):
            self._DefinePointInstancer(
                stage, '/Instancer', '/Instancer/Prototypes/Group', self._OUTER_POSITIONS)
            UsdGeom.Xform.Define(stage, '/Instancer/Prototypes/Group')
            for index, offset in enumerate(self._INNER_OFFSETS):
                self._DefineCube(stage, '/Instancer/Prototypes/Group/Cube%d' % index, offset)

        self._AssertPointInstanceSelectionMatchesTwin(
            'NestedPointInstancer', defineScene, defineTwin, '/Instancer')

    def testPointInstancerGrid14(self):
        self._numInstances = 14
        self._testName = 'Grid_14'
        mayaUtils.openPointInstancesGrid14Scene()
        self._RunTest()

    def testPointInstancerGrid7k(self):
        self._numInstances = 7000
        self._testName = 'Grid_7k'
        mayaUtils.openPointInstancesGrid7kScene()
        self._RunTest()

    def testPointInstancerGrid70k(self):
        self._numInstances = 70000
        self._testName = 'Grid_70k'
        mayaUtils.openPointInstancesGrid70kScene()
        self._RunTest()


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
