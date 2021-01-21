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

from pxr import Tf

from maya import cmds

import os
import sys
import unittest

import fixturesUtils


class testProxyShapeRendererSceneMessages(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._testName = 'ProxyShapeRendererSceneMessagesTest'
        cls._inputDir = os.path.join(inputPath, cls._testName)

    def setUp(self):
        cmds.file(new=True, force=True)

    def testSaveSceneAs(self):
        """
        Tests performing a "Save Scene As..." after having drawn a USD proxy
        shape.

        Previously, the batch renderer listened for kSceneUpdate Maya scene
        messages so that it could reset itself when the scene was changed. This
        was intended to fire for new empty scenes or when opening a different
        scene file. It turns out Maya also emits that message during
        "Save Scene As..." operations, in which case we do *not* want to reset
        the batch renderer, as doing so would lead to a crash, since all of the
        proxy shapes are still in the DAG.

        This test ensures that the batch renderer does not reset and Maya does
        not crash when doing a "Save Scene As..." operation.
        """
        mayaSceneFile = '%s.ma' % self._testName
        mayaSceneFullPath = os.path.join(self._inputDir, mayaSceneFile)
        Tf.Status('Opening Maya Scene File: %s' % mayaSceneFullPath)
        cmds.file(mayaSceneFullPath, open=True, force=True)
        currentTime = cmds.playbackOptions(query=True, animationStartTime=True)

        # Force a draw to complete by switching frames.
        currentTime += 1
        cmds.currentTime(currentTime, edit=True)

        saveAsMayaSceneFile = 'SavedScene.ma'
        saveAsMayaSceneFullPath = os.path.abspath(saveAsMayaSceneFile)
        Tf.Status('Saving Maya Scene File As: %s' % saveAsMayaSceneFullPath)
        cmds.file(rename=saveAsMayaSceneFullPath)
        cmds.file(save=True)

        # Try to select the USD proxy shape. This will cause the proxy's
        # shape adapter's Sync() method to be called, which would fail if the
        # batch renderer had been reset out from under the shape adapter.
        cmds.select('CubeProxyShape')

        # Force a draw to complete by switching frames.
        currentTime += 1
        cmds.currentTime(currentTime, edit=True)

    def testSceneOpenAndReopen(self):
        """
        Tests opening and then re-opening scenes after having drawn a USD proxy
        shape.

        The batch renderer needs to be reset when opening a new scene, but
        kBeforeOpen and kAfterOpen scene messages are not always delivered at
        the right time. With the former, the current scene may still be active
        when the message is received in which case another draw may occur
        before the scene is finally closed. With the latter, the scene may have
        been fully read and an initial draw may have happened by the time the
        message is received. Either case results in the batch renderer being
        reset in the middle of an active scene and a possible crash.
        """
        mayaSceneFile = '%s.ma' % self._testName
        mayaSceneFullPath = os.path.join(self._inputDir, mayaSceneFile)
        Tf.Status('Opening Maya Scene File: %s' % mayaSceneFullPath)
        cmds.file(mayaSceneFullPath, open=True, force=True)
        currentTime = cmds.playbackOptions(query=True, animationStartTime=True)

        # Force a draw to complete by switching frames.
        currentTime += 1
        cmds.currentTime(currentTime, edit=True)

        # Re-open the same scene.
        Tf.Status('Re-opening Maya Scene File: %s' % mayaSceneFullPath)
        cmds.file(mayaSceneFullPath, open=True, force=True)
        currentTime = cmds.playbackOptions(query=True, animationStartTime=True)

        # Force a draw to complete by switching frames.
        currentTime += 1
        cmds.currentTime(currentTime, edit=True)

        # Try to select the proxy shape.
        cmds.select('CubeProxyShape')

        # Force a draw to complete by switching frames.
        currentTime += 1
        cmds.currentTime(currentTime, edit=True)

    def testSceneImportAndReference(self):
        """
        Tests that importing or referencing a Maya scene does not reset the
        batch renderer.

        The batch renderer listens for kBeforeFileRead scene messages, but
        those are also emitted when a scene is imported or referenced into the
        current scene. In that case, we want to make sure that the batch
        renderer does not get reset.
        """
        mayaSceneFile = '%s.ma' % self._testName
        mayaSceneFullPath = os.path.join(self._inputDir, mayaSceneFile)
        Tf.Status('Opening Maya Scene File: %s' % mayaSceneFullPath)
        cmds.file(mayaSceneFullPath, open=True, force=True)
        currentTime = cmds.playbackOptions(query=True, animationStartTime=True)

        # Force a draw to complete by switching frames.
        currentTime += 1
        cmds.currentTime(currentTime, edit=True)

        # Import another scene file into the current scene.
        mayaSceneFile = 'EmptyScene.ma'
        mayaSceneFullPath = os.path.join(self._inputDir, mayaSceneFile)
        Tf.Status('Importing Maya Scene File: %s' % mayaSceneFullPath)
        cmds.file(mayaSceneFullPath, i=True)

        # Force a draw to complete by switching frames.
        currentTime += 1
        cmds.currentTime(currentTime, edit=True)

        # Try to select the proxy shape.
        cmds.select('CubeProxyShape')

        # Force a draw to complete by switching frames.
        currentTime += 1
        cmds.currentTime(currentTime, edit=True)

        # Reference another scene file into the current scene.
        Tf.Status('Referencing Maya Scene File: %s' % mayaSceneFullPath)
        cmds.file(mayaSceneFullPath, reference=True)

        # Force a draw to complete by switching frames.
        currentTime += 1
        cmds.currentTime(currentTime, edit=True)

        # Try to select the proxy shape.
        cmds.select('CubeProxyShape')


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
