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

import mayaUsd.lib as mayaUsdLib

from pxr import Tf

from maya import cmds
from maya import standalone
from maya import OpenMaya as OM

import fixturesUtils

import sys
import unittest


class testDiagnosticDelegate(unittest.TestCase):

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

    def setUp(self):
        self.messageLog = []
        self.callback = None
        cmds.loadPlugin('mayaUsdPlugin', quiet=True)

        # assertCountEqual in python 3 is equivalent to assertItemsEqual
        if sys.version_info[0] >= 3:
            self.assertItemsEqual = self.assertCountEqual

    def _OnCommandOutput(self, message, messageType, _):
        if (messageType == OM.MCommandMessage.kInfo
                or messageType == OM.MCommandMessage.kWarning
                or messageType == OM.MCommandMessage.kError):
            self.messageLog.append((message, messageType))

    def _StartRecording(self):
        self.callback = OM.MCommandMessage.addCommandOutputCallback(
                self._OnCommandOutput)
        self.messageLog = []

    def _StopRecording(self):
        OM.MMessage.removeCallback(self.callback)
        self.callback = None
        return list(self.messageLog)

    def testStatus(self):
        """Statuses should become Maya statuses."""
        self._StartRecording()
        Tf.Status("test status 1")
        Tf.Status("another test status")
        Tf.Status("yay for statuses!")
        log = self._StopRecording()
        self.assertListEqual(log, [
            ("test status 1", OM.MCommandMessage.kInfo),
            ("another test status", OM.MCommandMessage.kInfo),
            ("yay for statuses!", OM.MCommandMessage.kInfo)
        ])

    def testWarning(self):
        """Warnings should become Maya warnings."""
        self._StartRecording()
        Tf.Warn("spooky warning")
        Tf.Warn("so scary")
        Tf.Warn("something bad maybe")
        log = self._StopRecording()
        self.assertListEqual(log, [
            ("spooky warning", OM.MCommandMessage.kWarning),
            ("so scary", OM.MCommandMessage.kWarning),
            ("something bad maybe", OM.MCommandMessage.kWarning)
        ])

    def testError(self):
        """Simulate error in non-Python-invoked code by using
        Tf.RepostErrors."""
        self._StartRecording()
        try:
            Tf.RaiseCodingError("blah")
        except Tf.ErrorException as e:
            Tf.RepostErrors(e)
        log = self._StopRecording()
        self.assertEqual(len(log), 1)
        logText, logCode = log[0]
        self.assertRegex(logText,
            "^Python coding error: blah -- Coding Error in "
            "testDiagnosticDelegate\.testError at line [0-9]+ of ")
        self.assertEqual(logCode, OM.MCommandMessage.kError)

    def testError_Python(self):
        """Errors in Python-invoked code should still propagate to Python
        normally."""
        with self.assertRaises(Tf.ErrorException):
            Tf.RaiseCodingError("coding error!")
        with self.assertRaises(Tf.ErrorException):
            Tf.RaiseRuntimeError("runtime error!")

    def testBatching(self):
        self._StartRecording()
        with mayaUsdLib.DiagnosticBatchContext():
            Tf.Warn("spooky warning")
            Tf.Status("informative status")

            for i in range(5):
                Tf.Status("repeated status %d" % i)

            for i in range(3):
                Tf.Warn("spam warning %d" % i)

            try:
                Tf.RaiseCodingError("coding error!")
            except:
                pass
        log = self._StopRecording()

        # Note: we use assertItemsEqual because coalescing may re-order the
        # diagnostic messages.
        self.assertItemsEqual(log, [
            ("spooky warning", OM.MCommandMessage.kWarning),
            ("informative status", OM.MCommandMessage.kInfo),
            ("repeated status 0 -- and 4 similar", OM.MCommandMessage.kInfo),
            ("spam warning 0 -- and 2 similar", OM.MCommandMessage.kWarning)
        ])

    def testBatching_DelegateRemoved(self):
        """Tests removing the diagnostic delegate when the batch context is
        still open."""
        self._StartRecording()
        with mayaUsdLib.DiagnosticBatchContext():
            Tf.Warn("this warning won't be lost")
            Tf.Status("this status won't be lost")

            cmds.unloadPlugin('mayaUsdPlugin', force=True)

            for i in range(5):
                Tf.Status("no delegate, this will be lost %d" % i)
        log = self._StopRecording()

        # Note: we use assertItemsEqual because coalescing may re-order the
        # diagnostic messages.
        self.assertItemsEqual(log, [
            ("this warning won't be lost", OM.MCommandMessage.kWarning),
            ("this status won't be lost", OM.MCommandMessage.kInfo),
        ])

    def testBatching_BatchCount(self):
        """Tests the GetBatchCount() debugging function."""
        count = -1
        with mayaUsdLib.DiagnosticBatchContext():
            with mayaUsdLib.DiagnosticBatchContext():
                count = mayaUsdLib.DiagnosticDelegate.GetBatchCount()
        self.assertEqual(count, 2)
        count = mayaUsdLib.DiagnosticDelegate.GetBatchCount()
        self.assertEqual(count, 0)


if __name__ == '__main__':
    unittest.main(verbosity=2)
