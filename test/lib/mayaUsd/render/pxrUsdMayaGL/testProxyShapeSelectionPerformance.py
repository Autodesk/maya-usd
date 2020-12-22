#!/usr/bin/env mayapy
#
# Copyright 2018 Pixar
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

from pxr import Gf
from pxr import Tf
from pxr import Trace

from maya import cmds
from maya.api import OpenMayaUI as OMUI

import ufe

from PySide2 import QtCore
from PySide2.QtTest import QTest
from PySide2.QtWidgets import QApplication
from PySide2.QtWidgets import QWidget

from shiboken2 import wrapInstance

import contextlib
import json
import os
import sys
import unittest

import fixturesUtils


class testProxyShapeSelectionPerformance(unittest.TestCase):

    @staticmethod
    def _IsViewportRendererViewport20():
        m3dView = OMUI.M3dView.active3dView()
        if m3dView.getRendererName() == OMUI.M3dView.kViewport2Renderer:
            return True

        return False

    @staticmethod
    def _ClickInView(viewWidget, clickPosition,
            keyboardModifier=QtCore.Qt.NoModifier):
        clickPoint = QtCore.QPoint(clickPosition[0], clickPosition[1])
        QTest.mouseClick(viewWidget, QtCore.Qt.LeftButton, keyboardModifier,
            clickPoint)

    @staticmethod
    def _SelectAreaInView(viewWidget, selectAreaRange,
            keyboardModifier=QtCore.Qt.NoModifier):
        pressPoint = QtCore.QPoint(selectAreaRange.min[0], selectAreaRange.min[1])
        releasePoint = QtCore.QPoint(selectAreaRange.max[0], selectAreaRange.max[1])
        QTest.mousePress(viewWidget, QtCore.Qt.LeftButton, keyboardModifier,
            pressPoint)
        QTest.mouseRelease(viewWidget, QtCore.Qt.LeftButton, keyboardModifier,
            releasePoint)

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._inputDir = os.path.join(inputPath,
            'ProxyShapeSelectionPerformanceTest')

        cls._testDir = os.path.abspath('.')

        cls._profileScopeMetrics = dict()

        cls._cameraName = 'SelectionCamera'

        # These are the dimensions we want for the viewport we're drawing into.
        cls._viewportWidth = 960
        cls._viewportHeight = 540

        # To get those viewport dimensions, we add padding to the window size
        # to account for the window frame and toolbar.
        cls._viewWindowWidth = cls._viewportWidth + 4
        cls._viewWindowHeight = cls._viewportHeight + 23

        # Store the previous USD selection kind (or None if there wasn't one)
        # so we can restore the state later.
        cls._selKindOptionVarName = 'UsdSelectionKind'
        cls._prevSelKind = cmds.optionVar(query=cls._selKindOptionVarName) or None

        # Set the USD selection kind to "assembly" so that we select entire
        # "assets" during the test.
        cmds.optionVar(stringValue=(cls._selKindOptionVarName, 'assembly'))

    @classmethod
    def tearDownClass(cls):
        statsOutputLines = []
        for profileScopeName in cls._profileScopeMetrics.keys():
            elapsedTime = cls._profileScopeMetrics[profileScopeName]
            statsDict = {
                'profile': profileScopeName,
                'metric': 'time',
                'value': elapsedTime,
                'samples': 1
            }
            statsOutputLines.append(json.dumps(statsDict))

        statsOutput = os.linesep.join(statsOutputLines)
        perfStatsFilePath = os.path.join(cls._testDir, 'perfStats.raw')
        with open(perfStatsFilePath, 'w') as perfStatsFile:
            perfStatsFile.write(statsOutput)

        # Restore the previous USD selection kind, or remove it if there wasn't
        # one.
        if cls._prevSelKind is None:
            cmds.optionVar(remove=cls._selKindOptionVarName)
        else:
            cmds.optionVar(stringValue=
                (cls._selKindOptionVarName, cls._prevSelKind))

    def setUp(self):
        cmds.file(new=True, force=True)

        # To control where the rendered images are written, we force Maya to
        # use the test directory as the workspace.
        cmds.workspace(self._testDir, o=True)

    def tearDown(self):
        self._viewWidget = None
        self._m3dView = None
        cmds.deleteUI(self._testWindow)

    def _GetViewportWidget(self, cameraName, rendererName):
        self._testWindow = cmds.window('SelectionTestWindow',
            widthHeight=(self._viewWindowWidth, self._viewWindowHeight))
        cmds.paneLayout()
        testModelPanel = cmds.modelPanel(menuBarVisible=False)
        testModelEditor = cmds.modelPanel(testModelPanel, q=True, modelEditor=True)
        cmds.modelEditor(testModelEditor, edit=True, camera=cameraName,
            displayAppearance='smoothShaded', rendererName=rendererName)
        cmds.showWindow(self._testWindow)

        self._m3dView = OMUI.M3dView.getM3dViewFromModelPanel(testModelPanel)
        viewWidget = wrapInstance(long(self._m3dView.widget()), QWidget)

        return viewWidget

    def _GetViewWidgetCenter(self, viewWidget):
        width = viewWidget.width()
        height = viewWidget.height()
        viewSize = Gf.Vec2f(width, height)
        Tf.Status("Maya Viewport Widget Dimensions: %s" % viewSize)

        viewCenter = viewSize / 2.0

        return viewCenter

    def _WriteViewportImage(self, outputImageName, suffix):
        # Make sure the hardware renderer is available
        MAYA_RENDERER_NAME = 'mayaHardware2'
        mayaRenderers = cmds.renderer(query=True, namesOfAvailableRenderers=True)
        self.assertIn(MAYA_RENDERER_NAME, mayaRenderers)

        # Make it the current renderer.
        cmds.setAttr('defaultRenderGlobals.currentRenderer', MAYA_RENDERER_NAME,
            type='string')
        # Set the image format to PNG.
        cmds.setAttr('defaultRenderGlobals.imageFormat', 32)
        # Set the render mode to shaded and textured.
        cmds.setAttr('hardwareRenderingGlobals.renderMode', 4)
        # Enable the UI object filter so that the rendered image includes
        # selection highlighting when using the Viewport 2.0 render delegate.
        filterNames = cmds.getAttr(
            'hardwareRenderingGlobals.objectTypeFilterNameArray')
        filterIndex = filterNames.index('UI')
        filterValues = cmds.getAttr(
            'hardwareRenderingGlobals.objectTypeFilterValueArray')
        filterValues[filterIndex] = 1
        cmds.setAttr('hardwareRenderingGlobals.objectTypeFilterValueArray',
            filterValues, type='Int32Array')
        # Specify the output image prefix. The path to it is built from the
        # workspace directory.
        cmds.setAttr('defaultRenderGlobals.imageFilePrefix',
            '%s_%s' % (outputImageName, suffix),
            type='string')
        # Apply the viewer's color transform to the rendered image, otherwise
        # it comes out too dark.
        cmds.setAttr("defaultColorMgtGlobals.outputTransformEnabled", 1)

        # Do the render.
        cmds.ogsRender(camera=self._cameraName, currentFrame=True,
            width=self._viewportWidth, height=self._viewportHeight)

    @contextlib.contextmanager
    def _ProfileScope(self, profileScopeName):
        """
        A context manager that measures the execution time between enter and
        exit and stores the elapsed time in the class' metrics dictionary.
        """
        stopwatch = Tf.Stopwatch()
        collector = Trace.Collector()

        try:
            stopwatch.Start()
            collector.enabled = True
            collector.BeginEvent(profileScopeName)
            yield
        finally:
            collector.EndEvent(profileScopeName)
            collector.enabled = False
            stopwatch.Stop()
            elapsedTime = stopwatch.seconds
            self._profileScopeMetrics[profileScopeName] = elapsedTime
            Tf.Status('%s: %f' % (profileScopeName, elapsedTime))

            traceFilePath = os.path.join(self._testDir,
                '%s.trace' % profileScopeName)
            Trace.Reporter.globalReporter.Report(traceFilePath)
            collector.Clear()
            Trace.Reporter.globalReporter.ClearTree()

    def _TestSelectCenterSingle(self):
        """
        Tests a single-click selection in the center of the view.
        """
        clickPosition = self._GetViewWidgetCenter(self._viewWidget)

        profileScopeName = '%s Proxy Select Center Time' % self._testName

        with self._ProfileScope(profileScopeName):
            self._ClickInView(self._viewWidget, clickPosition)

        self._WriteViewportImage(self._testName, 'select_center')

    def _TestSelectCenterArea(self):
        """
        Tests an area selection in the center of the view.
        """
        viewCenter = self._GetViewWidgetCenter(self._viewWidget)

        centerRange = Gf.Range2f(viewCenter, viewCenter)
        unitArea = Gf.Range2f(Gf.Vec2f(-0.5, -0.5), Gf.Vec2f(0.5, 0.5))

        profileScopeName = '%s Proxy Select Center Area Time' % self._testName

        selectAreaRange = centerRange + unitArea * 10.0

        with self._ProfileScope(profileScopeName):
            self._SelectAreaInView(self._viewWidget, selectAreaRange)

        self._WriteViewportImage(self._testName, 'select_center_area')

    def _TestUnselect(self):
        """
        Tests "un-selecting" by doing a single-click selection in an empty area
        of the view.
        """
        profileScopeName = '%s Proxy Unselect Time' % self._testName

        clickPosition = Gf.Vec2f(10.0, 10.0)

        with self._ProfileScope(profileScopeName):
            self._ClickInView(self._viewWidget, clickPosition)

        self._WriteViewportImage(self._testName, 'unselect')

    def _TestSelectionAppend(self):
        """
        Tests selecting multiple objects in the view by appending to the
        selection an object at a time. This simulates selecting the objects in
        the corners, beginning with the top left and proceeding clockwise,
        while holding down the shift key.
        """
        viewCenter = self._GetViewWidgetCenter(self._viewWidget)

        horizontalOffset = self._viewWidget.width() * 0.198
        verticalOffset = self._viewWidget.height() * 0.352

        # TOP LEFT
        clickPosition = viewCenter + Gf.Vec2f(-horizontalOffset, -verticalOffset)

        profileScopeName = '%s Proxy Selection Append 1 Time' % self._testName

        with self._ProfileScope(profileScopeName):
            self._ClickInView(self._viewWidget, clickPosition,
                keyboardModifier=QtCore.Qt.ShiftModifier)

        self._WriteViewportImage(self._testName, 'selection_append_1')

        # TOP RIGHT
        clickPosition = viewCenter + Gf.Vec2f(horizontalOffset, -verticalOffset)

        profileScopeName = '%s Proxy Selection Append 2 Time' % self._testName

        with self._ProfileScope(profileScopeName):
            self._ClickInView(self._viewWidget, clickPosition,
                keyboardModifier=QtCore.Qt.ShiftModifier)

        self._WriteViewportImage(self._testName, 'selection_append_2')

        # BOTTOM RIGHT
        clickPosition = viewCenter + Gf.Vec2f(horizontalOffset, verticalOffset)

        profileScopeName = '%s Proxy Selection Append 3 Time' % self._testName

        with self._ProfileScope(profileScopeName):
            self._ClickInView(self._viewWidget, clickPosition,
                keyboardModifier=QtCore.Qt.ShiftModifier)

        self._WriteViewportImage(self._testName, 'selection_append_3')

        # BOTTOM LEFT
        clickPosition = viewCenter + Gf.Vec2f(-horizontalOffset, verticalOffset)

        profileScopeName = '%s Proxy Selection Append 4 Time' % self._testName

        with self._ProfileScope(profileScopeName):
            self._ClickInView(self._viewWidget, clickPosition,
                keyboardModifier=QtCore.Qt.ShiftModifier)

        self._WriteViewportImage(self._testName, 'selection_append_4')

    def _ValidateSelection(self, expectedSelectionSet):
        if Tf.GetEnvSetting('VP2_RENDER_DELEGATE_PROXY'):
            # When the Viewport 2.0 render delegate is being used, we will have
            # selected USD prims rather than proxy shape nodes or their
            # transform nodes, so we query UFE for the selection and manipulate
            # the paths of the selected scene items to yield the Maya-side
            # selection.
            ufeSelection = ufe.GlobalSelection.get()
            self.assertEqual(len(ufeSelection), len(expectedSelectionSet))

            # Pop the USD root prim name, and then the proxy shape name off of
            # the UFE path to leave the name of the proxy shape's transform
            # node at the end of the path.
            ufePaths = [ufeItem.path().pop().pop() for ufeItem in ufeSelection]
            actualSelectionSet = {str(ufePath.back()) for ufePath in ufePaths}
        else:
            actualSelectionSet = set(cmds.ls(selection=True) or [])

        self.assertEqual(actualSelectionSet, expectedSelectionSet)

    def _RunPerfTest(self):
        mayaSceneFile = 'Grid_5_of_CubeGrid%s_10.ma' % self._testName
        mayaSceneFullPath = os.path.join(self._inputDir, mayaSceneFile)
        cmds.file(mayaSceneFullPath, open=True, force=True)

        Tf.Status("Maya Scene File: %s" % mayaSceneFile)

        # Get the QWidget for the viewport window.
        self.assertTrue(self._IsViewportRendererViewport20())
        self._viewWidget = self._GetViewportWidget(self._cameraName,
            'vp2Renderer')
        self.assertTrue(self._viewWidget)

        # Force the initial view to draw so that the viewport size stabilizes.
        animStartTime = cmds.playbackOptions(query=True,
            animationStartTime=True)
        cmds.currentTime(animStartTime, edit=True)
        QApplication.processEvents()

        # Render an image and validate that nothing is selected to start.
        self._WriteViewportImage(self._testName, 'before_selection')
        expectedSelectionSet = set()
        self._ValidateSelection(expectedSelectionSet)


        self._TestSelectCenterSingle()
        expectedSelectionSet = set(['AssetRef_2_0_2'])
        self._ValidateSelection(expectedSelectionSet)


        self._TestSelectCenterArea()
        expectedSelectionSet = set([
            'AssetRef_2_0_2',
            'AssetRef_2_1_2',
            'AssetRef_2_2_2',
            'AssetRef_2_3_2',
            'AssetRef_2_4_2'])
        self._ValidateSelection(expectedSelectionSet)


        self._TestUnselect()
        expectedSelectionSet = set()
        self._ValidateSelection(expectedSelectionSet)


        self._TestSelectionAppend()
        expectedSelectionSet = set([
            'AssetRef_0_0_4',
            'AssetRef_4_0_4',
            'AssetRef_4_0_0',
            'AssetRef_0_0_0'])
        self._ValidateSelection(expectedSelectionSet)

    def testPerfGridOfCubeGridsCombinedMesh(self):
        """
        Tests selection correctness and performance with a grid of proxy shape
        nodes.

        The geometry in this scene is a grid of grids. The top-level grid is
        made up of USD proxy shape nodes. Each of those proxy shape nodes
        references a USD file that contains a single Mesh prim that is a grid
        of cubes. This single cube grid mesh is the result of combining the
        grid of cube asset meshes referenced from the "ModelRefs" test below.

        The camera in the scene is positioned in a side view.
        """
        self._testName = 'CombinedMesh'
        self._RunPerfTest()

    def testPerfGridOfCubeGridsModelRefs(self):
        """
        Tests selection correctness and performance with a grid of proxy shape
        nodes.

        The geometry in this scene is a grid of grids. The top-level grid is
        made up of USD proxy shape nodes. Each of those proxy shape nodes
        references a USD file with many references to a "CubeModel" asset USD
        file. This results in equivalent geometry but a higher prim/mesh count
        than the "CombinedMesh" test above.

        The camera in the scene is positioned in a side view.
        """
        self._testName = 'ModelRefs'
        self._RunPerfTest()


if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(
        testProxyShapeSelectionPerformance)

    results = unittest.TextTestRunner(stream=sys.stdout).run(suite)
    if results.wasSuccessful():
        exitCode = 0
    else:
        exitCode = 1
    # maya running interactively often absorbs all the output.  comment out the
    # following to prevent maya from exiting and open the script editor to look
    # at failures.
    cmds.quit(abort=True, exitCode=exitCode)
