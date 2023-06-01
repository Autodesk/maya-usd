#!/usr/bin/env python

#
# Copyright 2020 Autodesk
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
import mayaUtils
import usdUtils

from maya import cmds
from maya import standalone
from maya import OpenMaya as om

import ufe

try:
    from maya.internal.ufeSupport import ufeSelectCmd
except ImportError:
    # Maya 2019 and 2020 don't have ufeSupport plugin, so use fallback.
    from ufeScripts import ufeSelectCmd

import unittest


class SelectNotificationTestCase(unittest.TestCase):
    '''Verify UFE selection notification.'''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()
    
    @classmethod
    def tearDownClass(cls):
        cmds.file(new=True, force=True)

        standalone.uninitialize()

    def setUp(self):
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

        # Load a file that has the same scene in both the Maya Dag
        # hierarchy and the USD hierarchy.
        mayaUtils.openTestScene("parentCmd", "simpleSceneMayaPlusUSD_TRS.ma")

        # Create multiple scene items.  We will alternate between selecting a
        # Maya item and a USD item, 3 items each data model, one item at a
        # time, so we select 6 different items, one at a time.
        shapeSegment = mayaUtils.createUfePathSegment(
            "|mayaUsdProxy1|mayaUsdProxyShape1")
        ufeNames = ["cubeXform", "cylinderXform", "sphereXform"]
        mayaNames = ["pCube1", "pCylinder1", "pSphere1"]
        usdPaths = []
        for n in ["/" + o for o in ufeNames]:
            usdPaths.append(ufe.Path(
                [shapeSegment, usdUtils.createUfePathSegment(n)]))
        mayaPaths = []
        for n in ["|" + o for o in mayaNames]:
            mayaPaths.append(ufe.Path(mayaUtils.createUfePathSegment(n)))

        # Create a list of paths by alternating USD objects and Maya objects
        # Flatten zipped tuples using list comprehension double loop.
        zipped = zip(usdPaths, mayaPaths)
        paths = [j for i in zipped for j in i]

        # Create items for all paths.
        self.items = [ufe.Hierarchy.createItem(p) for p in paths]

        # Clear selection to start off
        cmds.select(clear=True)

    def runSelectionNotificationTest(self, selectCmd, getSelection, notificationName):
        selected = []

        def selectionCallback(params):
            nonlocal selected
            selected = getSelection()

        def clearSelection():
            globalSn = ufe.GlobalSelection.get()
            globalSn.clear()
            self.assertTrue(globalSn.empty())

        ufeSelectionCallbackID = om.MEventMessage.addEventCallback(notificationName, selectionCallback)
        try:
            # Select each item in turn.
            for item in self.items:
                clearSelection()
                selectCmd(item)
                expected = ufe.PathString.string(item.path())
                self.assertListEqual(selected, [expected])
        finally:
            om.MEventMessage.removeCallback(ufeSelectionCallbackID)

    @staticmethod
    def selectWithUfe(item):
        sn = ufe.Selection()
        sn.append(item)
        ufeSelectCmd.replaceWith(sn)

    @staticmethod
    def selectWithMaya(item):
        cmds.select(ufe.PathString.string(item.path()))

    @staticmethod
    def getSelectionWithUfe():
        items = ufe.GlobalSelection.get()
        return [ufe.PathString.string(item.path()) for item in items]

    @staticmethod
    def getSelectionWithMaya():
        items = cmds.ls(sl=True, ufe=True)
        return [item if ',' in item else '|' + item for item in items ]

    # Maya Notification tests
    #
    # These all fail. See MAYA-129071

    # def testMayaNotificationWithUfeSelectAndUfeGet(self):
    #     self.runSelectionNotificationTest(self.selectWithUfe, self.getSelectionWithUfe, "SelectionChanged")

    # def testMayaNotificationWithUfeSelectAndMayaGet(self):
    #     self.runSelectionNotificationTest(self.selectWithUfe, self.getSelectionWithMaya, "SelectionChanged")

    # def testMayaNotificationWithMayaSelectAndUfeGet(self):
    #     self.runSelectionNotificationTest(self.selectWithMaya, self.getSelectionWithUfe, "SelectionChanged")

    # def testMayaNotificationWithMayaSelectAndMayaGet(self):
    #     self.runSelectionNotificationTest(self.selectWithMaya, self.getSelectionWithMaya, "SelectionChanged")

    # UFE Notification tests

    def testUfeNotificationWithUfeSelectAndUfeGet(self):
        self.runSelectionNotificationTest(self.selectWithUfe, self.getSelectionWithUfe, "UFESelectionChanged")

    def testUfeNotificationWithUfeSelectAndMayaGet(self):
        self.runSelectionNotificationTest(self.selectWithUfe, self.getSelectionWithMaya, "UFESelectionChanged")

    def testUfeNotificationWithMayaSelectAndUfeGet(self):
        self.runSelectionNotificationTest(self.selectWithMaya, self.getSelectionWithUfe, "UFESelectionChanged")

    # This fails. See MAYA-129071
    #
    # def testUfeNotificationWithMayaSelectAndMayaGet(self):
    #     self.runSelectionNotificationTest(self.selectWithMaya, self.getSelectionWithMaya, "UFESelectionChanged")

if __name__ == '__main__':
    unittest.main(verbosity=0)
