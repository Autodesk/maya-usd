#!/pxrpythonsubst
#
# Copyright 2020 Pixar
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

from pxr import UsdMaya

from pxr import Tf

from maya import cmds

import contextlib
import json
import os
import sys
import unittest


class testProxyShapeDuplicatePerformance(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        cls._testDir = os.path.abspath('.')

        cls._profileScopeMetrics = dict()

        cls._cameraName = 'TurntableCamera'

    @classmethod
    def tearDownClass(cls):
        statsOutputLines = []
        for profileScopeName, elapsedTime in cls._profileScopeMetrics.iteritems():
            statsDict = {
                'profile': profileScopeName,
                'metric': 'time',
                'value': elapsedTime,
                'samples': 1
            }
            statsOutputLines.append(json.dumps(statsDict))

        statsOutput = '\n'.join(statsOutputLines)
        perfStatsFilePath = '%s/perfStats.raw' % cls._testDir
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

        try:
            stopwatch.Start()
            yield
        finally:
            stopwatch.Stop()
            elapsedTime = stopwatch.seconds
            self._profileScopeMetrics[profileScopeName] = elapsedTime
            Tf.Status("%s: %f" % (profileScopeName, elapsedTime))

    def _RunLoadTest(self):
        profileScopeName = '%s Assemblies Load Time' % self._testName

        with self._ProfileScope(profileScopeName):
            UsdMaya.LoadReferenceAssemblies()

    def _RunDuplicateTest(self):
        profileScopeName = '%s Assembly Duplicate' % self._testName

        cmds.select('AssetRef_0_0_0')
        cmds.refresh(force=True)

        with self._ProfileScope(profileScopeName):
            cmds.duplicate()
            cmds.refresh(force=True)

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
        # Specify the output image prefix. The path to it is built from the
        # workspace directory.
        cmds.setAttr('defaultRenderGlobals.imageFilePrefix',
            '%s_%s' % (outputImageName, suffix),
            type='string')
        # Apply the viewer's color transform to the rendered image, otherwise
        # it comes out too dark.
        cmds.setAttr("defaultColorMgtGlobals.outputTransformEnabled", 1)

        # Do the render.
        cmds.ogsRender(camera=self._cameraName, currentFrame=True, width=960,
            height=540)

    def testPerfGridOfCubeModelDuplicate(self):
        """
        Tests speed of duplicating a model reference assembly/proxy when many
        such models are present.

        The geometry in this scene is a grid of grids. The top-level grid is
        made up of USD reference assembly nodes. Each of those assembly nodes
        references a USD file with many references to a "CubeModel" asset USD
        file.
        """
        self._testName = 'ModelRefs'
        mayaSceneFile = 'Grid_5_of_CubeGrid%s_10.ma' % self._testName
        mayaSceneFullPath = os.path.abspath(mayaSceneFile)
        cmds.file(mayaSceneFullPath, open=True, force=True)

        Tf.Status("Maya Scene File: %s" % mayaSceneFile)

        self._RunLoadTest()

        self._WriteViewportImage(self._testName, 'initial')

        self._RunDuplicateTest()


if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(
        testProxyShapeDuplicatePerformance)

    results = unittest.TextTestRunner(stream=sys.stdout).run(suite)
    if results.wasSuccessful():
        exitCode = 0
    else:
        exitCode = 1
    # maya running interactively often absorbs all the output.  comment out the
    # following to prevent maya from exiting and open the script editor to look
    # at failures.
    cmds.quit(abort=True, exitCode=exitCode)
