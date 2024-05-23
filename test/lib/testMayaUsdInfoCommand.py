#!/usr/bin/env python

#
# Copyright 2024 Autodesk
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

import unittest

import maya.cmds as cmds

class MayaUsdInfoCommandTestCase(unittest.TestCase):
    """ Test the MayaUsd Info command. """

    @classmethod
    def setUpClass(cls):
        loaded = cmds.loadPlugin('mayaUsdPlugin', quiet=True)
        if loaded != ['mayaUsdPlugin']:
            raise RuntimeError('mayaUsd plugin load failed.')

    def testVersionInfo(self):
        # MayaUsd version is 0.x.0
        self.assertGreaterEqual(cmds.mayaUsdInfo(majorVersion=True), 0)
        self.assertEqual(cmds.mayaUsdInfo(mjv=True), cmds.mayaUsdInfo(majorVersion=True))

        self.assertGreater(cmds.mayaUsdInfo(minorVersion=True), 0)
        self.assertEqual(cmds.mayaUsdInfo(mnv=True), cmds.mayaUsdInfo(minorVersion=True))

        self.assertGreaterEqual(cmds.mayaUsdInfo(patchVersion=True), 0)
        self.assertEqual(cmds.mayaUsdInfo(pv=True), cmds.mayaUsdInfo(patchVersion=True))

        # The version flag returns a string of the form "0.x.0"
        self.assertNotEqual(cmds.mayaUsdInfo(version=True), '')
        self.assertEqual(cmds.mayaUsdInfo(v=True), cmds.mayaUsdInfo(version=True))
        self.assertEqual(cmds.mayaUsdInfo(v=True), '%s.%s.%s' % (cmds.mayaUsdInfo(mjv=True), cmds.mayaUsdInfo(mnv=True), cmds.mayaUsdInfo(pv=True)))

    def testBuildInfo(self):
        self.assertGreaterEqual(cmds.mayaUsdInfo(buildNumber=True), 0)
        self.assertEqual(cmds.mayaUsdInfo(bn=True), cmds.mayaUsdInfo(buildNumber=True))

        self.assertNotEqual(cmds.mayaUsdInfo(gitCommit=True), '')
        self.assertEqual(cmds.mayaUsdInfo(gc=True), cmds.mayaUsdInfo(gitCommit=True))

        self.assertNotEqual(cmds.mayaUsdInfo(gitBranch=True), '')
        self.assertEqual(cmds.mayaUsdInfo(gb=True), cmds.mayaUsdInfo(gitBranch=True))

        self.assertNotEqual(cmds.mayaUsdInfo(cutIdentifier=True), '')
        self.assertEqual(cmds.mayaUsdInfo(c=True), cmds.mayaUsdInfo(cutIdentifier=True))

        self.assertNotEqual(cmds.mayaUsdInfo(buildDate=True), '')
        self.assertEqual(cmds.mayaUsdInfo(bd=True), cmds.mayaUsdInfo(buildDate=True))
