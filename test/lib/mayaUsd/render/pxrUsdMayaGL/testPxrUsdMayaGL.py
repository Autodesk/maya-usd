#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

from maya import cmds

import os
import sys
import unittest

import fixturesUtils


class testPxrUsdMayaGL(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        inputPath = fixturesUtils.readOnlySetUpClass(__file__,
            initializeStandalone=False)

        cls._testName = 'PxrUsdMayaGLTest'
        cls._inputDir = os.path.join(inputPath, cls._testName)

    def testEmptySceneDraws(self):
        cmds.file(new=True, force=True)

        usdFilePath = os.path.join(self._inputDir, 'blank.usda')
        proxyShape = cmds.createNode('mayaUsdProxyShape', name='blank')
        cmds.setAttr("%s.filePath" % proxyShape, usdFilePath, type='string')
        cmds.refresh()

    def testSimpleSceneDrawsAndReloads(self):
        # a memory issue would sometimes cause maya to crash when opening a new
        # scene.

        for _ in range(20):
            cmds.file(new=True, force=True)

            for i in range(10):
                usdFilePath = os.path.join(self._inputDir, 'plane%d.usda' % (i%2))
                proxyShape = cmds.createNode('mayaUsdProxyShape', name='plane')
                cmds.setAttr("%s.filePath" % proxyShape, usdFilePath, type='string')

            cmds.refresh()

        cmds.file(new=True, force=True)


if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(testPxrUsdMayaGL)
    results = unittest.TextTestRunner(stream=sys.stdout).run(suite)
    if results.wasSuccessful():
        exitCode = 0
    else:
        exitCode = 1
    # maya running interactively often absorbs all the output.  comment out the
    # following to prevent maya from exiting and open the script editor to look
    # at failures.
    cmds.quit(abort=True, exitCode=exitCode)
