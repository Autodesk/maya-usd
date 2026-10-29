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
from testUtils import getTestScene

from maya import cmds
from maya import standalone

import mayaUsd.lib
import mayaUsd.ufe
import mayaUsd_createStageWithNewLayer

from pxr import Sdf, UsdGeom, UsdRender

import ufe

import fnmatch
import itertools
import os
import unittest

hasUfeFind = os.getenv('UFE_HIERARCHY_HAS_FIND', 'FALSE') == 'TRUE'


def nameMatcher(pattern, caseSensitive=False):
    '''Node name predicate: a shell glob ('*', '?') on the whole name, or a
    substring if the pattern has no wildcard.'''
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
    '''Verify searching USD prims by name, through UFE Hierarchy.find.
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

        runTimeMgr = ufe.RunTimeMgr.instance()
        self.mayaRtid = runTimeMgr.getId('Maya-DG')
        self.usdRtid = runTimeMgr.getId('USD')
        self.usdHandler = runTimeMgr.hierarchyHandler(self.usdRtid)

        # The Maya world root path, i.e. the single 'world' path component.
        # ufe.PathString.path('|world') would be '|world|world', as Maya
        # path strings are implicitly rooted at the world.
        self.worldPath = ufe.Path(ufe.PathSegment('|world', self.mayaRtid, '|'))

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

    def allChildFilters(self):
        '''Every combination of the USD child filter flags, and the empty filter.'''
        filters = [self.childFilter(InactivePrims=inactive, ClassPrims=classes, RenderPrims=render)
                   for inactive, classes, render in itertools.product([False, True], repeat=3)]
        filters.append([])
        return filters

    @staticmethod
    def filterName(childFilter):
        return ', '.join('%s=%s' % (f.name, f.value) for f in childFilter) or 'children()'

    def toPath(self, searchRoot):
        if searchRoot is None:
            return self.worldPath
        if isinstance(searchRoot, str):
            return ufe.PathString.path(searchRoot)
        return searchRoot

    def findMatching(self, matches, searchRoot=None, childFilter=None):
        if childFilter is None:
            childFilter = self.childFilter()
        return self.pathStrings(ufe.Hierarchy.find(
            self.toPath(searchRoot), matches, childFilter,
            ufe.Hierarchy.FindScope.NestedRuntimesOnly))

    def find(self, pattern, caseSensitive=False, searchRoot=None, childFilter=None):
        return self.findMatching(nameMatcher(pattern, caseSensitive), searchRoot, childFilter)

    def flattenFind(self, pattern, childFilter=None, searchRoot=None):
        '''Case-insensitive search done like the Outliner without find(): walk
        the hierarchy with filteredChildren(), then match the names of all the
        visited items. A USD searchRoot is included if it matches; under a
        Maya searchRoot, by default the world, its proxy shapes are walked.'''
        if childFilter is None:
            childFilter = self.childFilter()
        matches = nameMatcher(pattern)
        searchRoot = self.toPath(searchRoot)

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

        if searchRoot.runTimeId() == self.usdRtid:
            rootItem = ufe.Hierarchy.createItem(searchRoot)
            if rootItem.nodeName() and matches(rootItem.nodeName()):
                found.add(ufe.PathString.string(searchRoot))
            visit(rootItem)
            return found

        for proxyShape in cmds.ls(type='mayaUsdProxyShapeBase', long=True):
            proxyShapePath = ufe.PathString.path(proxyShape)
            if proxyShapePath.startsWith(searchRoot):
                visit(ufe.Hierarchy.createItem(proxyShapePath))
        return found

    def assertMatchesReference(self, searchRoot=None, filters=None, pattern=''):
        '''Compare find() with flattenFind(), by default for the filters that affect pulled prims.'''
        if filters is None:
            filters = [self.childFilter(), self.childFilter(InactivePrims=False), []]
        for childFilter in filters:
            with self.subTest(searchRoot=str(searchRoot), childFilter=self.filterName(childFilter)):
                self.assertEqual(self.find(pattern, searchRoot=searchRoot, childFilter=childFilter),
                                 self.flattenFind(pattern, childFilter, searchRoot))

    def pull(self, pathStr):
        '''Edit the prim as Maya, and return the path string of its Maya object.'''
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(pathStr))
        prim = mayaUsd.ufe.ufePathToPrim(pathStr)
        dagPathStr = mayaUsd.lib.PrimUpdaterManager.readPullInformation(prim)
        self.assertTrue(dagPathStr)
        return ufe.PathString.string(ufe.PathString.path(dagPathStr))

    def createRichStage(self):
        '''A stage with nested instancing, inactive, class and render prims, payloads and a variant set.'''
        ps = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(ps).GetStage()

        def addInstance(primPath, prototypePath):
            prim = stage.DefinePrim(primPath, 'Xform')
            prim.GetReferences().AddInternalReference(prototypePath)
            prim.SetInstanceable(True)

        #   /Protos/Inner          (prototype of /Protos/Outer/innerInst)
        #   /Protos/Outer          (prototype of /World/InstA and /World/InstB)
        stage.DefinePrim('/Protos/Inner/innerLeaf', 'Sphere')
        stage.DefinePrim('/Protos/Inner/innerInactive').SetActive(False)
        stage.DefinePrim('/Protos/Inner/innerClass').SetSpecifier(Sdf.SpecifierClass)
        stage.DefinePrim('/Protos/Outer/outerLeaf', 'Cube')
        stage.DefinePrim('/Protos/Outer/outerInactive').SetActive(False)
        addInstance('/Protos/Outer/innerInst', '/Protos/Inner')
        addInstance('/World/InstA', '/Protos/Outer')
        addInstance('/World/InstB', '/Protos/Outer')

        stage.DefinePrim('/World/inactiveGeom').SetActive(False)
        stage.CreateClassPrim('/_class_rich')
        stage.DefinePrim('/_class_rich/classChild')

        UsdGeom.Scope.Define(stage, '/Render')
        UsdRender.Settings.Define(stage, '/Render/Settings')
        UsdRender.Pass.Define(stage, '/Render/Pass')
        UsdGeom.Scope.Define(stage, '/Render/Products')
        UsdRender.Product.Define(stage, '/Render/Products/Product')
        UsdGeom.Scope.Define(stage, '/Mixed')
        UsdGeom.Mesh.Define(stage, '/Mixed/Mesh')
        UsdRender.Var.Define(stage, '/Mixed/Var')
        UsdGeom.Scope.Define(stage, '/RenderWithInactive')
        UsdRender.Settings.Define(stage, '/RenderWithInactive/Settings')
        UsdGeom.Mesh.Define(stage, '/RenderWithInactive/Mesh').GetPrim().SetActive(False)

        stage.DefinePrim('/PayloadSrc/payloadLeaf', 'Sphere')
        for name in ['Loaded', 'Unloaded']:
            stage.DefinePrim('/World/' + name, 'Xform').GetPayloads().AddInternalPayload('/PayloadSrc')

        variantSet = stage.DefinePrim('/World/Variant', 'Xform').GetVariantSets().AddVariantSet('shape')
        for name in ['ball', 'box']:
            variantSet.AddVariant(name)
            variantSet.SetVariantSelection(name)
            with variantSet.GetVariantEditContext():
                stage.DefinePrim('/World/Variant/' + name)
        variantSet.SetVariantSelection('ball')

        stage.Unload('/World/Unloaded')
        return ps, stage

    def createLargeStage(self, count=3000):
        '''A stage with count prims under /Big.'''
        ps = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        layer = mayaUsd.lib.GetPrim(ps).GetStage().GetEditTarget().GetLayer()
        with Sdf.ChangeBlock():
            big = Sdf.PrimSpec(layer, 'Big', Sdf.SpecifierDef, 'Xform')
            for i in range(count):
                Sdf.PrimSpec(big, 'prim%d' % i, Sdf.SpecifierDef, 'Xform')
        return ps

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

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testMatchesReferenceForAllFiltersAndRoots(self):
        ps, _ = self.createRichStage()
        psPath = ufe.PathString.path(ps)
        roots = [
            None,
            psPath,
            psPath.pop(),
            ps + ',/World',
            ps + ',/World/InstA',
            ps + ',/World/InstA/innerInst',
        ]
        for searchRoot in roots:
            for pattern in ['', 'in']:
                self.assertMatchesReference(searchRoot, self.allChildFilters(), pattern)

        self.assertIn(ps + ',/World/InstB/innerInst/innerLeaf', self.find(''))

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testPrototypesNeverFound(self):
        self.createRichStage()
        for childFilter in self.allChildFilters():
            with self.subTest(childFilter=self.filterName(childFilter)):
                self.assertEqual(self.findMatching(lambda name: name.startswith('__Prototype'),
                                                   childFilter=childFilter), set())

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testPulledPrimsInTwoStages(self):
        bananaMaya = self.pull(self.p1('/Root/Banana'))
        pearMaya = self.pull(self.p2('/World/pear'))

        found = self.find('')
        self.assertIn(bananaMaya, found)
        self.assertIn(pearMaya, found)
        self.assertNotIn(self.p1('/Root/Banana'), found)
        self.assertNotIn(self.p2('/World/pear'), found)

        for searchRoot in [None, self.ps1, self.ps2]:
            self.assertMatchesReference(searchRoot)

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testPulledObjectFoundOnceInAllRuntimes(self):
        pearMaya = self.pull(self.p2('/World/pear'))
        found = ufe.Hierarchy.find(self.worldPath, lambda name: True, self.childFilter(),
                                   ufe.Hierarchy.FindScope.AllRuntimes)
        paths = [ufe.PathString.string(item.path()) for item in found]
        self.assertEqual(paths.count(pearMaya), 1)

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testOrphanedPulledNodeParentDeactivated(self):
        stage2 = mayaUsd.lib.GetPrim(self.ps2).GetStage()
        stage2.DefinePrim('/World/grapeGroup/grape', 'Xform')
        grapeMaya = self.pull(self.p2('/World/grapeGroup/grape'))
        self.assertIn(grapeMaya, self.find(''))

        # Deactivating the parent of the pulled prim orphans its Maya node.
        stage2.GetPrimAtPath('/World/grapeGroup').SetActive(False)
        self.assertNotIn(grapeMaya, self.find(''))
        self.assertMatchesReference()

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testOrphanedPulledNodeVariantSwitch(self):
        ps, stage = mayaUtils.createProxyFromFile(
            getTestScene('hideOrphanedNodes', 'hideOrphanedNodes.usda'))
        cMaya = self.pull(ps + ',/A/B/C')
        self.assertIn(cMaya, self.find(''))
        self.assertMatchesReference()

        # Switching the variant of B orphans the pulled node of C.
        stage.GetPrimAtPath('/A/B').GetVariantSet('cdVariant').SetVariantSelection('d')
        self.assertMatchesReference()

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testPulledPrimOutsidePrimPath(self):
        appleMaya = self.pull(self.p2('/World/APPLE_big'))
        cmds.setAttr(self.ps2 + '.primPath', '/World/pear', type='string')

        self.assertNotIn(appleMaya, self.find(''))
        self.assertMatchesReference()

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testSearchRootAtPulledPrim(self):
        # The hierarchy shows a pulled prim as its Maya object, so searching
        # from its USD path finds that object and its descendants.
        pearPath = self.p2('/World/pear')
        pearMaya = self.pull(pearPath)
        for childFilter in [self.childFilter(), self.childFilter(InactivePrims=False), []]:
            with self.subTest(childFilter=self.filterName(childFilter)):
                shown = set(p for p in self.flattenFind('', childFilter)
                            if p == pearMaya or p.startswith(pearMaya + '|'))
                self.assertEqual(self.find('', searchRoot=pearPath, childFilter=childFilter), shown)

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testInterruptMidTraversal(self):
        bigPs = [self.createLargeStage(), self.createLargeStage()]
        childFilter = self.childFilter()

        def find(searchRoot, interrupt=None):
            return self.pathStrings(self.usdHandler.find(
                self.usdRtid, searchRoot, lambda name: True, childFilter, interrupt))

        def interruptFromSecondPoll():
            polls = [0]
            def interrupt():
                polls[0] += 1
                return polls[0] >= 2
            return interrupt

        for searchRoot in [self.worldPath, ufe.PathString.path(bigPs[0] + ',/Big')]:
            with self.subTest(searchRoot=str(searchRoot)):
                found = find(searchRoot, interruptFromSecondPoll())
                self.assertTrue(found)
                self.assertLess(found, find(searchRoot))

        # The search stopped in the first large stage searched, or before it.
        found = find(self.worldPath, interruptFromSecondPoll())
        inBigStages = [any(p.startswith(ps + ',') for p in found) for ps in bigPs]
        self.assertFalse(all(inBigStages))

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testInterruptInPulledObjectSearch(self):
        '''An interrupt raised while searching a pulled Maya object stops the USD traversal.'''
        stage2 = mayaUsd.lib.GetPrim(self.ps2).GetStage()
        stage2.DefinePrim('/World/grape', 'Xform')
        stage2.DefinePrim('/World/afterGrape', 'Xform')
        grapeMaya = self.pull(self.p2('/World/grape'))
        grapeName = str(ufe.PathString.path(grapeMaya).back())
        childFilter = self.childFilter()

        # Found items are in search order.
        ordered = [ufe.PathString.string(item.path()) for item in self.usdHandler.find(
            self.usdRtid, self.worldPath, lambda name: True, childFilter)]
        self.assertIn(grapeMaya, ordered)
        self.assertIn(self.p2('/World/afterGrape'), ordered)

        stop = [False]
        def matchUntilGrape(name):
            if name == grapeName:
                stop[0] = True
            return True
        found = self.pathStrings(self.usdHandler.find(
            self.usdRtid, self.worldPath, matchUntilGrape, childFilter, lambda: stop[0]))
        self.assertEqual(found, set(ordered[:ordered.index(grapeMaya) + 1]))

    def testFindAvailabilityMatchesBuild(self):
        self.assertEqual(hasattr(ufe.Hierarchy, 'find'), hasUfeFind)

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testNativeFindPollsSparingly(self):
        # The native USD find polls the interrupt every so many prims, where
        # the default UFE walk polls it once per item.
        ps = self.createLargeStage(300)
        polls = [0]
        def countPolls():
            polls[0] += 1
            return False
        found = self.usdHandler.find(
            self.usdRtid, ufe.PathString.path(ps), lambda name: False, self.childFilter(), countPolls)
        self.assertTrue(found.empty())
        self.assertGreater(polls[0], 0)
        self.assertLess(polls[0], 5)

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testFindAllRuntimesFromWorld(self):
        childFilter = self.childFilter()
        found = ufe.Hierarchy.find(self.worldPath, lambda name: True, childFilter,
                                   ufe.Hierarchy.FindScope.AllRuntimes)
        rtids = dict((ufe.PathString.string(item.path()), item.runTimeId()) for item in found)

        ps1Transform = ufe.PathString.string(ufe.PathString.path(self.ps1).pop())
        self.assertEqual(rtids.get(ps1Transform), self.mayaRtid)
        self.assertEqual(rtids.get(self.p1('/Root/Banana')), self.usdRtid)
        self.assertEqual(set(p for p, rtid in rtids.items() if rtid == self.usdRtid),
                         self.find(''))

        mayaHandler = ufe.RunTimeMgr.instance().hierarchyHandler(self.mayaRtid)
        mayaFound = self.pathStrings(
            mayaHandler.find(self.mayaRtid, self.worldPath, lambda name: True, childFilter))
        self.assertIn(ps1Transform, mayaFound)

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testSearchRootPointInstance(self):
        mayaUtils.openPointInstancesGrid14Scene()
        instancer = '|UsdProxy|UsdProxyShape,/PointInstancerGrid/PointInstancer'
        pointInstance = instancer + '/3'

        # A point instance has no children.
        self.assertEqual(self.flattenFind('', searchRoot=pointInstance), {pointInstance})
        self.assertEqual(self.find('', searchRoot=pointInstance), {pointInstance})
        self.assertEqual(self.findMatching(lambda name: name != '3', pointInstance), set())

        # The prototypes are found under the PointInstancer.
        self.assertIn(instancer + '/prototypes/RedCube', self.find('', searchRoot=instancer))
        self.assertMatchesReference(instancer, [self.childFilter(), []])
        self.assertMatchesReference(None, [self.childFilter(), []])

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testPayloadAndVariantChanges(self):
        ps, stage = self.createRichStage()
        loadedLeaf = ps + ',/World/Loaded/payloadLeaf'
        unloadedLeaf = ps + ',/World/Unloaded/payloadLeaf'

        found = self.find('', searchRoot=ps)
        self.assertIn(loadedLeaf, found)
        self.assertIn(ps + ',/World/Unloaded', found)
        self.assertNotIn(unloadedLeaf, found)

        stage.Load('/World/Unloaded')
        stage.Unload('/World/Loaded')
        found = self.find('', searchRoot=ps)
        self.assertIn(unloadedLeaf, found)
        self.assertNotIn(loadedLeaf, found)
        self.assertMatchesReference(ps)

        ball, box = ps + ',/World/Variant/ball', ps + ',/World/Variant/box'
        self.assertIn(ball, found)
        self.assertNotIn(box, found)
        stage.GetPrimAtPath('/World/Variant').GetVariantSet('shape').SetVariantSelection('box')
        found = self.find('', searchRoot=ps)
        self.assertIn(box, found)
        self.assertNotIn(ball, found)
        self.assertMatchesReference(ps)

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testProxyShapePrimPathEdgeCases(self):
        def stage1Found():
            return set(p for p in self.find('') if p.startswith(self.ps1 + ','))

        # An invalid primPath shows nothing of the stage.
        cmds.setAttr(self.ps1 + '.primPath', 'notAbsolute', type='string')
        self.assertEqual(stage1Found(), set())
        self.assertMatchesReference()

        # A primPath to a missing prim shows its existing ancestors.
        cmds.setAttr(self.ps1 + '.primPath', '/Root/Missing/deeper', type='string')
        self.assertEqual(stage1Found(), {self.p1('/Root')})
        self.assertMatchesReference()

        cmds.setAttr(self.ps1 + '.primPath', '/Root/Banana', type='string')
        self.assertEqual(stage1Found(), {self.p1('/Root'), self.p1('/Root/Banana')})
        self.assertMatchesReference()

        # A search root outside the primPath finds nothing.
        self.assertEqual(self.find('', searchRoot=self.p1('/Root/AppleTree')), set())

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testRenameProxyShape(self):
        transform = cmds.rename(cmds.listRelatives(self.ps1, parent=True, fullPath=True)[0],
                                'renamedStage')
        shape = cmds.rename(cmds.listRelatives(transform, shapes=True, fullPath=True)[0],
                            'renamedStageShape')
        newPs1 = cmds.ls(shape, long=True)[0]
        self.assertNotEqual(newPs1, self.ps1)

        found = self.find('')
        self.assertIn(newPs1 + ',/Root/Banana', found)
        self.assertFalse(any(p.startswith(self.ps1 + ',') for p in found))
        self.assertMatchesReference()

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testDeleteAndUndoProxyShape(self):
        ps, _ = mayaUtils.createProxyFromFile(
            getTestScene('hideOrphanedNodes', 'hideOrphanedNodes.usda'))
        ePath = ps + ',/D/E'
        self.assertIn(ePath, self.find(''))

        cmds.delete(cmds.listRelatives(ps, parent=True, fullPath=True)[0])
        found = self.find('')
        self.assertFalse(any(p.startswith(ps + ',') for p in found))
        self.assertIn(self.p2('/World/pear'), found)

        cmds.undo()
        self.assertIn(ePath, self.find(''))

    @unittest.skipUnless(hasUfeFind, 'Requires UFE Hierarchy.find.')
    def testSearchRootPrefixSharingNames(self):
        shapes = []
        for name in ['prefixStage1', 'prefixStage10']:
            ps = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
            mayaUsd.lib.GetPrim(ps).GetStage().DefinePrim('/' + name + 'Prim')
            transform = cmds.rename(cmds.listRelatives(ps, parent=True, fullPath=True)[0], name)
            shapes.append(cmds.listRelatives(transform, shapes=True, fullPath=True)[0])

        self.assertEqual(self.find('', searchRoot='|prefixStage1'),
                         {shapes[0] + ',/prefixStage1Prim'})
        self.assertMatchesReference('|prefixStage1')


if __name__ == '__main__':
    unittest.main(verbosity=2)
