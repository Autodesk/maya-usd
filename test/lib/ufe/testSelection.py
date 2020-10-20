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
        filePath = os.path.join(os.path.dirname(os.path.realpath(__file__)), "../..", "testSamples", "parentCmd", "simpleSceneMayaPlusUSD_TRS.ma" )
        cmds.file(filePath, force=True, open=True)

        # Clear selection to start off
        cmds.select(clear=True)

    def runTestSelection(self, selectCmd):
        '''Run the replace selection test, using the argument command to
        replace the selection with a single scene item.'''

        # Create multiple scene items.  We will alternate between selecting a
        # Maya item and a USD item, 3 items each data model, one item at a
        # time, so we select 6 different items, one at a time.
        shapeSegment = mayaUtils.createUfePathSegment(
            "|world|mayaUsdProxy1|mayaUsdProxyShape1")
        names = ["pCube1", "pCylinder1", "pSphere1"]
        usdPaths = []
        for n in ["/" + o for o in names]:
            usdPaths.append(ufe.Path(
                [shapeSegment, usdUtils.createUfePathSegment(n)]))
        mayaPaths = []
        for n in ["|world|" + o for o in names]:
            mayaPaths.append(ufe.Path(mayaUtils.createUfePathSegment(n)))

        # Create a list of paths by alternating USD objects and Maya objects
        # Flatten zipped tuples using list comprehension double loop.
        zipped = zip(usdPaths, mayaPaths)
        paths = [j for i in zipped for j in i]

        # Create items for all paths.
        items = [ufe.Hierarchy.createItem(p) for p in paths]

        # Clear the selection.
        globalSn = ufe.GlobalSelection.get()
        globalSn.clear()
        self.assertTrue(globalSn.empty())

        # Select all items in turn.
        for item in items:
            selectCmd(item)

        # Item in the selection should be the last item in our list.
        self.assertEqual(len(globalSn), 1)
        def snFront(sn):
            return next(iter(sn)) if \
                ufe.VersionInfo.getMajorVersion() == 1 else sn.front()

        self.assertEqual(snFront(globalSn), items[-1])

        # Check undo.  For this purpose, re-create the list of items in reverse
        # order.  Because we're already at the last item, we skip the last one
        # (i.e. last item is -1, so start at -2, and increment by -1).
        rItems = items[-2::-1]

        # Undo until the first element, checking the selection as we go.
        for i in rItems:
            cmds.undo()
            self.assertEqual(len(globalSn), 1)
            self.assertEqual(snFront(globalSn), i)

        # Check redo.
        fItems = items[1:]

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

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testMayaSelect only available in UFE v2 or greater.')
    def testMayaSelect(self):
        # At time of writing (17-May-2020), Maya select command does not
        # accept UFE path strings.  Use Maya select for Maya scene items,
        # and UFE select for non-Maya scene items.
        def selectCmd(item):
            if item.runTimeId() == 1:
                # Single path segment.  Simply pop the |world head of path.
                p = str(item.path().popHead())
                cmds.select(p)
            else:
                sn = ufe.Selection()
                sn.append(item)
                ufeSelectCmd.replaceWith(sn)
        self.runTestSelection(selectCmd)
