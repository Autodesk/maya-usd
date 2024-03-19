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
        self.assertGreaterEqual(cmds.mayaUsd(majorVersion=True), 0)
        self.assertEqual(cmds.mayaUsd(mjv=True), cmds.mayaUsd(majorVersion=True))

        self.assertGreater(cmds.mayaUsd(minorVersion=True), 0)
        self.assertEqual(cmds.mayaUsd(mnv=True), cmds.mayaUsd(minorVersion=True))

        self.assertGreaterEqual(cmds.mayaUsd(patchVersion=True), 0)
        self.assertEqual(cmds.mayaUsd(pv=True), cmds.mayaUsd(patchVersion=True))

        # The version flag returns a string of the form "0.x.0"
        self.assertNotEqual(cmds.mayaUsd(version=True), '')
        self.assertEqual(cmds.mayaUsd(v=True), cmds.mayaUsd(version=True))
        self.assertEqual(cmds.mayaUsd(v=True), '%s.%s.%s' % (cmds.mayaUsd(mjv=True), cmds.mayaUsd(mnv=True), cmds.mayaUsd(pv=True)))

    def testBuildInfo(self):
        self.assertGreaterEqual(cmds.mayaUsd(buildNumber=True), 0)
        self.assertEqual(cmds.mayaUsd(bn=True), cmds.mayaUsd(buildNumber=True))

        self.assertNotEqual(cmds.mayaUsd(gitCommit=True), '')
        self.assertEqual(cmds.mayaUsd(gc=True), cmds.mayaUsd(gitCommit=True))

        self.assertNotEqual(cmds.mayaUsd(gitBranch=True), '')
        self.assertEqual(cmds.mayaUsd(gb=True), cmds.mayaUsd(gitBranch=True))

        self.assertNotEqual(cmds.mayaUsd(cutIdentifier=True), '')
        self.assertEqual(cmds.mayaUsd(c=True), cmds.mayaUsd(cutIdentifier=True))

        self.assertNotEqual(cmds.mayaUsd(buildDate=True), '')
        self.assertEqual(cmds.mayaUsd(bd=True), cmds.mayaUsd(buildDate=True))
