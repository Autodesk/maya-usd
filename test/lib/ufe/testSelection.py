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

import mayaUtils, usdUtils, ufeUtils

import ufe

import maya.cmds as cmds

try:
    from maya.internal.ufeSupport import ufeSelectCmd
except ImportError:
    # Maya 2019 and 2020 don't have ufeSupport plugin, so use fallback.
    from ufeScripts import ufeSelectCmd

import unittest
import os

class SelectTestCase(unittest.TestCase):
    '''Verify UFE selection on a USD scene.'''

    pluginsLoaded = False
    
    @classmethod
    def setUpClass(cls):
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()
    
    @classmethod
    def tearDownClass(cls):
        cmds.file(new=True, force=True)

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

    def runTestSelection(self, selectCmd):
        '''Run the replace selection test, using the argument command to
        replace the selection with a single scene item.'''

        # Clear the selection.
        globalSn = ufe.GlobalSelection.get()
        globalSn.clear()
        self.assertTrue(globalSn.empty())

        # Select all items in turn.
        for item in self.items:
            selectCmd(item)

        # Item in the selection should be the last item in our list.
        self.assertEqual(len(globalSn), 1)
        def snFront(sn):
            return next(iter(sn)) if \
                ufe.VersionInfo.getMajorVersion() == 1 else sn.front()

        self.assertEqual(snFront(globalSn), self.items[-1])

        # Check undo.  For this purpose, re-create the list of items in reverse
        # order.  Because we're already at the last item, we skip the last one
        # (i.e. last item is -1, so start at -2, and increment by -1).
        rItems = self.items[-2::-1]

        # Undo until the first element, checking the selection as we go.
        for i in rItems:
            cmds.undo()
            self.assertEqual(len(globalSn), 1)
            self.assertEqual(snFront(globalSn), i)

        # Check redo.
        fItems = self.items[1:]

        # Redo until the last element, checking the selection as we go.
        for i in fItems:
            cmds.redo()
            self.assertEqual(len(globalSn), 1)
            self.assertEqual(snFront(globalSn), i)

    def testUfeSelect(self):
        def selectCmd(item):
            sn = ufe.Selection()
            sn.append(item)
            ufeSelectCmd.replaceWith(sn)
        self.runTestSelection(selectCmd)

    @unittest.skipUnless(((ufeUtils.ufeFeatureSetVersion() >= 2) and (mayaUtils.previewReleaseVersion() >= 121)), 'testMayaSelect only available in UFE v2 or greater and Maya Preview Release 121 or later.')
    def testMayaSelect(self):
        # Maya PR 121 now has support for UFE path string in select command.
        def selectCmd(item):
            cmds.select(ufe.PathString.string(item.path()))
        self.runTestSelection(selectCmd)

    @unittest.skipUnless(((ufeUtils.ufeFeatureSetVersion() >= 2) and (mayaUtils.previewReleaseVersion() >= 121)), 'testMayaSelectFlags only available in UFE v2 or greater and Maya Preview Release 121 or later.')
    def testMayaSelectFlags(self):
        # Maya PR 121 now has support for UFE path string in select command.

        # Clear the selection.
        globalSn = ufe.GlobalSelection.get()
        globalSn.clear()
        self.assertTrue(globalSn.empty())

        # Incrementally add to the selection all items in turn.
        # Also testing undo/redo along the way.
        cnt = 0
        for item in self.items:
            cnt += 1
            cmds.select(ufe.PathString.string(item.path()), add=True)
            self.assertEqual(cnt, len(globalSn))
            self.assertTrue(globalSn.contains(item.path()))

            cmds.undo()
            self.assertEqual(cnt-1, len(globalSn))
            self.assertFalse(globalSn.contains(item.path()))

            cmds.redo()
            self.assertEqual(cnt, len(globalSn))
            self.assertTrue(globalSn.contains(item.path()))

        # Since we added all the items to the global selection, it
        # should have all of them.
        self.assertEqual(len(globalSn), len(self.items))

        # Ensure the global selection order is the same as our item order.
        itemIt = iter(self.items)
        for selIt in globalSn:
            self.assertEqual(selIt, next(itemIt))

        # Incrementally remove from the selection all items in turn.
        # Also testing undo/redo along the way.
        for item in self.items:
            cnt -= 1
            cmds.select(ufe.PathString.string(item.path()), deselect=True)
            self.assertEqual(cnt, len(globalSn))
            self.assertFalse(globalSn.contains(item.path()))

            cmds.undo()
            self.assertEqual(cnt+1, len(globalSn))
            self.assertTrue(globalSn.contains(item.path()))

            cmds.redo()
            self.assertEqual(cnt, len(globalSn))
            self.assertFalse(globalSn.contains(item.path()))

        # Since we removed all items from global selection it
        # should be empty now.
        self.assertTrue(globalSn.empty())

        # Incrementally toggle selection state of all items in turn.
        # Since they all start unselected, they will toggle to selected.
        # Also testing undo/redo along the way.
        globalSn.clear()
        self.assertTrue(globalSn.empty())
        cnt = 0
        for item in self.items:
            cnt += 1
            cmds.select(ufe.PathString.string(item.path()), toggle=True)
            self.assertEqual(cnt, len(globalSn))
            self.assertTrue(globalSn.contains(item.path()))

            cmds.undo()
            self.assertEqual(cnt-1, len(globalSn))
            self.assertFalse(globalSn.contains(item.path()))

            cmds.redo()
            self.assertEqual(cnt, len(globalSn))
            self.assertTrue(globalSn.contains(item.path()))

        # Since we toggled each item to selected, we should have all
        # of them on the global selection.
        self.assertEqual(len(globalSn), len(self.items))

        # Incrementally toggle selection state of all items in turn.
        # Since they all start selected, they will toggle to unselected.
        # Also testing undo/redo along the way.
        for item in self.items:
            cnt -= 1
            cmds.select(ufe.PathString.string(item.path()), toggle=True)
            self.assertEqual(cnt, len(globalSn))
            self.assertFalse(globalSn.contains(item.path()))

            cmds.undo()
            self.assertEqual(cnt+1, len(globalSn))
            self.assertTrue(globalSn.contains(item.path()))

            cmds.redo()
            self.assertEqual(cnt, len(globalSn))
            self.assertFalse(globalSn.contains(item.path()))

        # Since we toggled all items to unselected, the global selection
        # should be empty now.
        self.assertTrue(globalSn.empty())

        # Select all the items at once, replacing the existing selection.
        # Also testing undo/redo.
        pathStrings = [ufe.PathString.string(i.path()) for i in self.items]
        cmds.select(*pathStrings, replace=True)
        self.assertEqual(len(globalSn), len(self.items))

        # Ensure the global selection order is the same as our item order.
        itemIt = iter(self.items)
        for selIt in globalSn:
            self.assertEqual(selIt, next(itemIt))

        cmds.undo()
        self.assertEqual(0, len(globalSn))
        self.assertTrue(globalSn.empty())

        cmds.redo()
        self.assertEqual(len(globalSn), len(self.items))

        # Ensure the global selection order is the same as our item order.
        itemIt = iter(self.items)
        for selIt in globalSn:
            self.assertEqual(selIt, next(itemIt))

        # With all items selected (and in same order as item order)
        # "select -add" the first item which will move it to the end.
        first = self.items[0]
        self.assertEqual(globalSn.front(), first)
        cmds.select(ufe.PathString.string(first.path()), add=True)
        self.assertTrue(globalSn.contains(first.path()))
        self.assertEqual(globalSn.back(), first)
