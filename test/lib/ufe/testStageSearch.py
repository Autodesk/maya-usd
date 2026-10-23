#!/usr/bin/env python

#
# Copyright 2026 Autodesk
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

from maya import cmds
from maya import standalone

import mayaUsd.lib
import mayaUsd_createStageWithNewLayer

import ufe

import fnmatch
import os
import unittest

hasUfeFind = os.getenv('UFE_HIERARCHY_HAS_FIND', 'FALSE') == 'TRUE'


def nameMatcher(pattern, caseSensitive=False):
    '''Node name predicate matching like the mayaUsdStageSearch command: a
    shell glob ('*', '?') on the whole name, or a substring if the pattern has
    no wildcard.'''
    glob = pattern if ('*' in pattern or '?' in pattern) else '*' + pattern + '*'
    if caseSensitive:
        return lambda name: fnmatch.fnmatchcase(name, glob)
    glob = glob.lower()
    return lambda name: fnmatch.fnmatchcase(name.lower(), glob)


class CountingHierarchyHandler(ufe.HierarchyHandler):
    '''Wraps a hierarchy handler and counts the calls to find().'''

    def __init__(self, wrapped):
        super(CountingHierarchyHandler, self).__init__()
        self._wrapped = wrapped
        self.findCalls = 0

    def hierarchy(self, item):
        return self._wrapped.hierarchy(item)

    def createItem(self, path):
        return self._wrapped.createItem(path)

    def childFilter(self):
        return self._wrapped.childFilter()

    def find(self, runTimeId, searchRoot, matches, childFilter, interrupt):
        self.findCalls += 1
        return self._wrapped.find(runTimeId, searchRoot, matches, childFilter, interrupt)


class StageSearchTestCase(unittest.TestCase):
    '''Verify searching USD prims by name, through UFE Hierarchy.find and the
    mayaUsdStageSearch command.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        self.assertTrue(self.pluginsLoaded)
        cmds.file(new=True, force=True)

        # Stage 1:
        #   /Root
        #     /AppleTree
        #       /apple
        #     /Banana
        #     /Inactive_apple      (inactive: searched)
        #   /_class_appleClass     (abstract: not searched)
        #   /Proto
        #     /protoApple
        #   /Inst                  (instance of /Proto: /Inst/protoApple is an instance proxy)
        self.ps1 = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage1 = mayaUsd.lib.GetPrim(self.ps1).GetStage()
        stage1.DefinePrim('/Root', 'Xform')
        stage1.DefinePrim('/Root/AppleTree', 'Xform')
        stage1.DefinePrim('/Root/AppleTree/apple', 'Sphere')
        stage1.DefinePrim('/Root/Banana', 'Xform')
        stage1.DefinePrim('/Root/Inactive_apple', 'Xform').SetActive(False)
        stage1.CreateClassPrim('/_class_appleClass')
        stage1.DefinePrim('/Proto', 'Xform')
        stage1.DefinePrim('/Proto/protoApple', 'Sphere')
        inst = stage1.DefinePrim('/Inst', 'Xform')
        inst.GetReferences().AddInternalReference('/Proto')
        inst.SetInstanceable(True)

        # Stage 2:
        #   /World
        #     /APPLE_big
        #     /pear
        self.ps2 = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage2 = mayaUsd.lib.GetPrim(self.ps2).GetStage()
        stage2.DefinePrim('/World', 'Xform')
        stage2.DefinePrim('/World/APPLE_big', 'Xform')
        stage2.DefinePrim('/World/pear', 'Xform')

        # The Maya world root path, i.e. the single 'world' path component.
        # ufe.PathString.path('|world') would be '|world|world', as Maya
        # path strings are implicitly rooted at the world.
        mayaRtid = ufe.RunTimeMgr.instance().getId('Maya-DG')
        self.worldPath = ufe.Path(ufe.PathSegment('|world', mayaRtid, '|'))

    def p1(self, primPath):
        return self.ps1 + ',' + primPath

    def p2(self, primPath):
        return self.ps2 + ',' + primPath

    @staticmethod
    def pathStrings(selection):
        return set(ufe.PathString.string(item.path()) for item in selection)

    @staticmethod
    def childFilter(**values):
        '''The USD runtime child filter, with the argument flag values.'''
        runTimeMgr = ufe.RunTimeMgr.instance()
        childFilter = runTimeMgr.hierarchyHandler(runTimeMgr.getId('USD')).childFilter()
        for flag in childFilter:
            if flag.name in values:
                flag.value = values[flag.name]
        return childFilter

    def findMatching(self, matches, searchRoot=None, childFilter=None):
        if searchRoot is None:
            searchRoot = self.worldPath
        elif isinstance(searchRoot, str):
            searchRoot = ufe.PathString.path(searchRoot)
        if childFilter is None:
            childFilter = self.childFilter()
        return self.pathStrings(ufe.Hierarchy.find(
            searchRoot, matches, childFilter,
            ufe.Hierarchy.FindScope.NestedRuntimesOnly))

    def find(self, pattern, caseSensitive=False, searchRoot=None, childFilter=None):
        return self.findMatching(nameMatcher(pattern, caseSensitive), searchRoot, childFilter)

    def flattenFind(self, pattern, childFilter=None):
        '''Case-insensitive search done like the Outliner without find(): walk
        the hierarchy of every proxy shape with filteredChildren(), then match
        the names of all the visited items.'''
        if childFilter is None:
            childFilter = self.childFilter()
        matches = nameMatcher(pattern)

        found = set()
        def visit(item):
            hierarchy = ufe.Hierarchy.hierarchy(item)
            if hierarchy is None:
                return
            children = hierarchy.filteredChildren(childFilter) if childFilter else hierarchy.children()
            for child in children:
                if matches(child.nodeName()):
                    found.add(ufe.PathString.string(child.path()))
                visit(child)

        for proxyShape in cmds.ls(type='mayaUsdProxyShapeBase', long=True):
            visit(ufe.Hierarchy.createItem(ufe.PathString.path(proxyShape)))
        return found

    def assertFindMatchesFlatten(self, pattern, childFilter=None):
        found = self.find(pattern, childFilter=childFilter)
        self.assertEqual(found, self.flattenFind(pattern, childFilter))
        return found

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testBasicMatch(self):
        self.assertEqual(self.find('Banana'), {self.p1('/Root/Banana')})

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testPredicate(self):
        # The predicate alone decides what matches, and is given prim names.
        names = set()
        def recordName(name):
            names.add(name)
            return name.endswith('apple')
        self.assertEqual(self.findMatching(recordName), {
            self.p1('/Root/AppleTree/apple'),
            self.p1('/Root/Inactive_apple'),
        })
        self.assertIn('Banana', names)
        self.assertIn('pear', names)
        self.assertNotIn(self.p1('/Root/Banana'), names)

        self.assertEqual(self.findMatching(lambda name: name.lower() == 'pear'),
                         {self.p2('/World/pear')})

        # A null predicate finds nothing.
        self.assertEqual(self.findMatching(None), set())

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testCaseSensitivity(self):
        # Case-insensitive by default. Inactive prims and instance proxies are
        # included, abstract (class) prims are not.
        self.assertEqual(self.find('apple'), {
            self.p1('/Root/AppleTree'),
            self.p1('/Root/AppleTree/apple'),
            self.p1('/Root/Inactive_apple'),
            self.p1('/Proto/protoApple'),
            self.p1('/Inst/protoApple'),
            self.p2('/World/APPLE_big'),
        })

        self.assertEqual(self.find('apple', caseSensitive=True), {
            self.p1('/Root/AppleTree/apple'),
            self.p1('/Root/Inactive_apple'),
        })

        self.assertEqual(self.find('Apple', caseSensitive=True), {
            self.p1('/Root/AppleTree'),
            self.p1('/Proto/protoApple'),
            self.p1('/Inst/protoApple'),
        })

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testWildcards(self):
        # A wildcard pattern matches the whole name.
        self.assertEqual(self.find('apple*'), {
            self.p1('/Root/AppleTree'),
            self.p1('/Root/AppleTree/apple'),
            self.p2('/World/APPLE_big'),
        })
        self.assertEqual(self.find('?anana'), {self.p1('/Root/Banana')})
        self.assertEqual(self.find('*_apple'), {self.p1('/Root/Inactive_apple')})

        # A plain pattern matches as a substring.
        self.assertEqual(self.find('ea'), {self.p2('/World/pear')})

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testNoMatch(self):
        self.assertEqual(self.find('zzz'), set())
        self.assertEqual(self.find('APPLE_BIG', caseSensitive=True), set())

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testInstanceProxies(self):
        self.assertIn(self.p1('/Inst/protoApple'), self.find('protoApple'))

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testAbstractPrimsExcluded(self):
        self.assertEqual(self.find('_class_'), set())

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testMultipleStagesInOneCall(self):
        '''Searching from the Maya world root with the nested-runtimes scope
        calls the USD hierarchy handler find once, for all stages.'''
        runTimeMgr = ufe.RunTimeMgr.instance()
        usdRtid = runTimeMgr.getId('USD')
        usdHandler = runTimeMgr.hierarchyHandler(usdRtid)
        countingHandler = CountingHierarchyHandler(usdHandler)
        runTimeMgr.setHierarchyHandler(usdRtid, countingHandler)
        try:
            found = self.find('a')
        finally:
            runTimeMgr.setHierarchyHandler(usdRtid, usdHandler)

        self.assertEqual(countingHandler.findCalls, 1)
        self.assertIn(self.p1('/Root/Banana'), found)
        self.assertIn(self.p2('/World/pear'), found)

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testSearchRootStage(self):
        # Scoping to a proxy shape searches only its stage.
        self.assertEqual(self.find('apple*', searchRoot=self.ps1), {
            self.p1('/Root/AppleTree'),
            self.p1('/Root/AppleTree/apple'),
        })
        self.assertEqual(self.find('apple*', searchRoot=self.ps2), {
            self.p2('/World/APPLE_big'),
        })

        # Scoping to a Maya ancestor of a proxy shape.
        ps2Parent = ufe.PathString.path(self.ps2).pop()
        self.assertEqual(self.find('apple*', searchRoot=ps2Parent), {
            self.p2('/World/APPLE_big'),
        })

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testSearchRootPrim(self):
        # Scoping to a prim searches its subtree, including itself.
        self.assertEqual(self.find('apple', searchRoot=self.p1('/Root/AppleTree')), {
            self.p1('/Root/AppleTree'),
            self.p1('/Root/AppleTree/apple'),
        })
        self.assertEqual(self.find('apple', searchRoot=self.p1('/Inst')), {
            self.p1('/Inst/protoApple'),
        })

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testMatchesFlatten(self):
        for pattern in ['a', 'apple', 'Proto*', '?oot', '']:
            self.assertFindMatchesFlatten(pattern)

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testInactiveAndClassPrimsFilters(self):
        # Inactive prims are shown by default, class prims are not.
        self.assertIn(self.p1('/Root/Inactive_apple'), self.find('apple'))
        self.assertNotIn(self.p1('/_class_appleClass'), self.find('apple'))

        noInactive = self.childFilter(InactivePrims=False)
        self.assertNotIn(self.p1('/Root/Inactive_apple'),
                         self.assertFindMatchesFlatten('apple', noInactive))

        withClass = self.childFilter(ClassPrims=True)
        self.assertIn(self.p1('/_class_appleClass'),
                      self.assertFindMatchesFlatten('apple', withClass))

        # Without a filter, the unfiltered children are searched, which
        # exclude inactive and class prims.
        found = self.assertFindMatchesFlatten('apple', [])
        self.assertNotIn(self.p1('/Root/Inactive_apple'), found)
        self.assertNotIn(self.p1('/_class_appleClass'), found)
        self.assertIn(self.p1('/Root/AppleTree/apple'), found)

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testRenderPrimsFilter(self):
        ps = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(ps).GetStage()
        stage.DefinePrim('/RenderScope_kiwi', 'Scope')
        stage.DefinePrim('/RenderScope_kiwi/kiwiSettings', 'RenderSettings')
        stage.DefinePrim('/kiwiGeom', 'Xform')
        renderScope = ps + ',/RenderScope_kiwi'
        renderSettings = ps + ',/RenderScope_kiwi/kiwiSettings'
        geom = ps + ',/kiwiGeom'

        # Render prims and render-only scopes are hidden by default.
        self.assertEqual(self.assertFindMatchesFlatten('kiwi'), {geom})

        withRender = self.childFilter(RenderPrims=True)
        self.assertEqual(self.assertFindMatchesFlatten('kiwi', withRender),
                         {renderScope, renderSettings, geom})

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testProxyShapePrimPath(self):
        cmds.setAttr(self.ps1 + '.primPath', '/Root/AppleTree', type='string')

        # Only the ancestors of the primPath and its descendants are searched.
        self.assertEqual(self.assertFindMatchesFlatten('apple'), {
            self.p1('/Root/AppleTree'),
            self.p1('/Root/AppleTree/apple'),
            self.p2('/World/APPLE_big'),
        })
        self.assertEqual(self.assertFindMatchesFlatten('Root'), {self.p1('/Root')})
        self.assertEqual(self.find('Banana'), set())
        self.assertEqual(self.find('apple', searchRoot=self.ps1), {
            self.p1('/Root/AppleTree'),
            self.p1('/Root/AppleTree/apple'),
        })

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testPulledPrim(self):
        grapePath = self.p2('/World/grape')
        stage2 = mayaUsd.lib.GetPrim(self.ps2).GetStage()
        stage2.DefinePrim('/World/grape', 'Xform')
        stage2.DefinePrim('/World/grape/grapeSeed', 'Xform')

        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(grapePath))

        # The pulled prim is replaced by its Maya object, and its USD
        # descendants are not searched.
        found = self.assertFindMatchesFlatten('grape')
        self.assertNotIn(grapePath, found)
        self.assertNotIn(self.p2('/World/grape/grapeSeed'), found)
        mayaRtid = ufe.RunTimeMgr.instance().getId('Maya-DG')
        foundItems = [ufe.PathString.path(p) for p in found]
        self.assertTrue(any(p.runTimeId() == mayaRtid for p in foundItems))

        # The pulled prim is inactive, so hiding inactive prims hides it.
        noInactive = self.childFilter(InactivePrims=False)
        self.assertEqual(self.assertFindMatchesFlatten('grape', noInactive), set())

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testReparentProxyShapeAncestor(self):
        self.assertIn(self.p1('/Root/Banana'), self.find('Banana'))

        group = cmds.group(empty=True, name='searchGroup')
        ps1Transform = cmds.listRelatives(self.ps1, parent=True, fullPath=True)[0]
        cmds.parent(ps1Transform, group)
        newPs1 = cmds.ls(cmds.listRelatives(group, allDescendents=True, type='mayaUsdProxyShape'),
                         long=True)[0]
        self.assertNotEqual(newPs1, self.ps1)

        self.assertEqual(self.assertFindMatchesFlatten('Banana'), {newPs1 + ',/Root/Banana'})

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testInterrupt(self):
        runTimeMgr = ufe.RunTimeMgr.instance()
        usdRtid = runTimeMgr.getId('USD')
        usdHandler = runTimeMgr.hierarchyHandler(usdRtid)
        childFilter = self.childFilter()

        polls = [0]
        def countPolls():
            polls[0] += 1
            return False
        matches = nameMatcher('a')
        found = usdHandler.find(usdRtid, self.worldPath, matches, childFilter, countPolls)
        self.assertGreater(len(found), 0)
        self.assertGreater(polls[0], 0)

        self.assertTrue(usdHandler.find(
            usdRtid, self.worldPath, matches, childFilter, lambda: True).empty())
        self.assertTrue(ufe.Hierarchy.find(
            self.worldPath, matches, childFilter,
            ufe.Hierarchy.FindScope.NestedRuntimesOnly, lambda: True).empty())

    def testStageSearchCommand(self):
        result = set(cmds.mayaUsdStageSearch(pattern='apple*') or [])
        self.assertEqual(result, {
            self.p1('/Root/AppleTree'),
            self.p1('/Root/AppleTree/apple'),
            self.p2('/World/APPLE_big'),
        })

        result = set(cmds.mayaUsdStageSearch(pattern='apple', caseSensitive=True) or [])
        self.assertEqual(result, {
            self.p1('/Root/AppleTree/apple'),
            self.p1('/Root/Inactive_apple'),
        })

        self.assertEqual(cmds.mayaUsdStageSearch(pattern='zzz') or [], [])

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testStageSearchCommandMatchesFind(self):
        for pattern, caseSensitive in [('a', False), ('Apple', True), ('?anana', False)]:
            cmdResult = set(cmds.mayaUsdStageSearch(
                pattern=pattern, caseSensitive=caseSensitive) or [])
            self.assertEqual(cmdResult, self.find(pattern, caseSensitive))


if __name__ == '__main__':
    unittest.main(verbosity=2)
