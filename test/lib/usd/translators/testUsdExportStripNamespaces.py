#!/usr/bin/env mayapy
#
# Copyright 2016 Pixar
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


import os
import sys
import unittest

from maya import cmds
from maya import standalone

from pxr import Usd

import fixturesUtils

class testUsdExportStripNamespaces(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

        # Deprecated since version 3.2: assertRegexpMatches and assertRaisesRegexp
        # have been renamed to assertRegex() and assertRaisesRegex()
        if sys.version_info.major < 3 or sys.version_info.minor < 2:
            cls.assertRegex = cls.assertRegexpMatches
            cls.assertRaisesRegex = cls.assertRaisesRegexp

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportWithClashStripping(self):
        mayaFilePath = os.path.abspath('UsdExportStripNamespaces.ma')
        cmds.file(mayaFilePath, new=True, force=True)

        node1 = cmds.polyCube( sx=5, sy=5, sz=5, name="cube1" )
        cmds.namespace(add="foo")
        cmds.namespace(set="foo");
        node2 = cmds.polyCube( sx=5, sy=5, sz=5, name="cube1" )
        cmds.namespace(set=":");

        usdFilePath = os.path.abspath('UsdExportStripNamespaces_EXPORTED.usda')

        # https://github.com/Autodesk/maya-usd/blob/dev/lib/mayaUsd/fileio/jobs/writeJob.cpp#L814
        # does indeed generate the proper TF_RUNTIME_ERROR originally tested
        # below, and
        # http://graphics.pixar.com/usd/docs/api/group__group__tf___diagnostic.html#ga4abf7754e5dbf161d2a5a4160fd3b891
        # describes that "Generally, an error handling delegate will take action
        # to turn this error into a python exception [...]".
        #
        # Unfortunately, at time of writing, this is not happening correctly,
        # as the Python exception is generated, but the detailed error string
        # is lost.  We are left with the generic 'Maya command error'
        # string, which is far weaker.  PPT, 16-Jun-20.
        #
        # errorRegexp = "Multiple dag nodes map to the same prim path" \
        #     ".+|cube1 - |foo:cube1.*"
        errorRegexp = 'Maya command error'

        with self.assertRaisesRegex(RuntimeError, errorRegexp) as cm:
            cmds.usdExport(mergeTransformAndShape=True,
                           selection=False,
                           stripNamespaces=True,
                           file=usdFilePath,
                           shadingMode='none')

        with self.assertRaisesRegex(RuntimeError,errorRegexp) as cm:
            cmds.usdExport(mergeTransformAndShape=False,
                           selection=False,
                           stripNamespaces=True,
                           file=usdFilePath,
                           shadingMode='none')

    def testExportWithStripAndMerge(self):
        mayaFilePath = os.path.abspath('UsdExportStripNamespaces.ma')
        cmds.file(mayaFilePath, new=True, force=True)

        cmds.namespace(add=":foo")
        cmds.namespace(add=":bar")

        node1 = cmds.polyCube( sx=5, sy=5, sz=5, name="cube1" )
        cmds.namespace(set=":foo");
        node2 = cmds.polyCube( sx=5, sy=5, sz=5, name="cube2" )
        cmds.namespace(set=":bar");
        node3 = cmds.polyCube( sx=5, sy=5, sz=5, name="cube3" )
        cmds.namespace(set=":");

        usdFilePath = os.path.abspath('UsdExportStripNamespaces_EXPORTED.usda')

        cmds.usdExport(mergeTransformAndShape=True,
                       selection=False,
                       stripNamespaces=True,
                       file=usdFilePath,
                       shadingMode='none')

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        expectedPrims = ("/cube1", "/cube2", "/cube3")

        for primPath in expectedPrims:
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim.IsValid(), "Expect " + primPath)

    def testExportWithStrip(self):
        mayaFilePath = os.path.abspath('UsdExportStripNamespaces.ma')
        cmds.file(mayaFilePath, new=True, force=True)

        cmds.namespace(add=":foo")
        cmds.namespace(add=":bar")

        node1 = cmds.polyCube( sx=5, sy=5, sz=5, name="cube1" )
        cmds.namespace(set=":foo");
        node2 = cmds.polyCube( sx=5, sy=5, sz=5, name="cube2" )
        cmds.namespace(set=":bar");
        node3 = cmds.polyCube( sx=5, sy=5, sz=5, name="cube3" )
        cmds.namespace(set=":");

        usdFilePath = os.path.abspath('UsdExportStripNamespaces_EXPORTED.usda')

        cmds.usdExport(mergeTransformAndShape=False,
                       selection=False,
                       stripNamespaces=True,
                       file=usdFilePath,
                       shadingMode='none')

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        expectedPrims = (
            "/cube1",
            "/cube1/cube1Shape",
            "/cube2",
            "/cube2/cube2Shape",
            "/cube3",
            "/cube3/cube3Shape"
        )

        for primPath in expectedPrims:
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim.IsValid(), "Expect " + primPath)

if __name__ == '__main__':
    unittest.main(verbosity=2)
