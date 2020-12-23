#!/pxrpythonsubst
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

from pxr import Tf
from pxr import Trace

from maya import cmds

import contextlib
import json
import os
import sys
import unittest

import fixturesUtils


class testProxyShapeDrawPerformance(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._inputDir = os.path.join(inputPath,
            'ProxyShapeDrawPerformanceTest')

        cls._testDir = os.path.abspath('.')

        cls._profileScopeMetrics = dict()

        cls._cameraName = 'TurntableCamera'

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

    def setUp(self):
        cmds.file(new=True, force=True)

        # To control where the rendered images are written, we force Maya to
        # use the test directory as the workspace.
        cmds.workspace(self._testDir, o=True)

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

    def _RunTimeToFirstDrawTest(self):
        # We measure the time to first draw by switching to frame
        # (animStartTime - 1.0). We expect the scene to load on animStartTime,
        # and that currentTime() blocks until the draw is complete.
        profileScopeName = '%s Proxy Time to First Draw' % self._testName

        with self._ProfileScope(profileScopeName):
            cmds.currentTime((self.animStartTime - 1.0), edit=True)

    def _WriteViewportImage(self, outputImageName):
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
        # Specify the output image prefix. The path to it is built from the
        # workspace directory.
        cmds.setAttr('defaultRenderGlobals.imageFilePrefix', outputImageName,
            type='string')
        # Apply the viewer's color transform to the rendered image, otherwise
        # it comes out too dark.
        cmds.setAttr("defaultColorMgtGlobals.outputTransformEnabled", 1)

        # Do the render.
        cmds.ogsRender(camera=self._cameraName, currentFrame=True, width=960,
            height=540)

    def _RunPlaybackTest(self):
        numFrames = self.animEndTime - self.animStartTime + 1.0

        profileScopeName = '%s Proxy Orbit Playback Time' % self._testName

        with self._ProfileScope(profileScopeName):
            for frame in range(int(self.animStartTime), int(self.animEndTime + 1.0)):
                cmds.currentTime(frame, edit=True)

        playElapsedTime = self._profileScopeMetrics[profileScopeName]

        Tf.Status("Number of frames: %f" % numFrames)
        Tf.Status("Playback Elapsed Time: %f" % playElapsedTime)
        Tf.Status("Playback FPS: %f" % (numFrames / playElapsedTime))

    def _RunPerfTest(self):
        mayaSceneFile = 'Grid_5_of_CubeGrid%s_10.ma' % self._testName
        mayaSceneFullPath = os.path.join(self._inputDir, mayaSceneFile)
        cmds.file(mayaSceneFullPath, open=True, force=True)

        Tf.Status("Maya Scene File: %s" % mayaSceneFile)

        self.animStartTime = cmds.playbackOptions(query=True,
            animationStartTime=True)
        self.animEndTime = cmds.playbackOptions(query=True,
            animationEndTime=True)

        self._RunTimeToFirstDrawTest()

        self._RunPlaybackTest()

        cmds.currentTime(self.animStartTime, edit=True)
        self._WriteViewportImage(self._testName)

    def testPerfGridOfCubeGridsCombinedMesh(self):
        """
        Tests load speed and playback/tumble performance drawing a grid of
        proxy shape nodes.

        The geometry in this scene is a grid of grids. The top-level grid is
        made up of USD proxy shape nodes. Each of those proxy shape nodes
        references a USD file that contains a single Mesh prim that is a grid
        of cubes. This single cube grid mesh is the result of combining the
        grid of cube asset meshes referenced from the "ModelRefs" test below.

        The camera in the scene is authored so that it orbits the geometry.
        """
        self._testName = 'CombinedMesh'
        self._RunPerfTest()

    def testPerfGridOfCubeGridsModelRefs(self):
        """
        Tests load speed and playback/tumble performance drawing a grid of
        proxy shape nodes.

        The geometry in this scene is a grid of grids. The top-level grid is
        made up of USD proxy shape nodes. Each of those proxy shape nodes
        references a USD file with many references to a "CubeModel" asset USD
        file. This results in equivalent geometry but a higher prim/mesh count
        than the "CombinedMesh" test above.

        The camera in the scene is authored so that it orbits the geometry.
        """
        self._testName = 'ModelRefs'
        self._RunPerfTest()


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
