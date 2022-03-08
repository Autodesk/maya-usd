#!/usr/bin/env python

#
# Copyright 2021 Autodesk
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
import mayaUsd_createStageWithNewLayer

from usdUtils import createSimpleXformScene

import mayaUsd.lib

import mayaUtils
import mayaUsd.ufe

from pxr import UsdGeom, Gf

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as om

import ufe

import unittest

from testUtils import assertVectorAlmostEqual

import os

class DuplicateAsTestCase(unittest.TestCase):
    '''Test duplicate as: duplicate USD data into Maya and vice versa.
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
        cmds.file(new=True, force=True)

    def testDuplicateAsMaya(self):
        '''Duplicate a USD transform hierarchy to Maya.'''

        (_, _, aXlation, aUsdUfePathStr, aUsdUfePath, _, _, 
               bXlation, bUsdUfePathStr, bUsdUfePath, _) = \
            createSimpleXformScene()

        # Duplicate USD data as Maya data, placing it under the root.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.duplicate(
                aUsdUfePathStr, '|world'))

        # Should now have two transform nodes in the Maya scene: the path
        # components in the second segment of the aUsdItem and bUsdItem will
        # now be under the Maya world.
        aMayaPathStr = str(aUsdUfePath.segments[1]).replace('/', '|')
        bMayaPathStr = str(bUsdUfePath.segments[1]).replace('/', '|')
        self.assertEqual(cmds.ls(aMayaPathStr, long=True)[0], aMayaPathStr)
        self.assertEqual(cmds.ls(bMayaPathStr, long=True)[0], bMayaPathStr)

        # Translation should have been copied over to the Maya data model.
        self.assertEqual(cmds.getAttr(aMayaPathStr + '.translate')[0],
                         aXlation)
        self.assertEqual(cmds.getAttr(bMayaPathStr + '.translate')[0],
                         bXlation)

    def testDuplicateAsNonRootMaya(self):
        '''Duplicate a USD transform hierarchy to Maya.'''

        (_, _, aXlation, aUsdUfePathStr, aUsdUfePath, _, _, 
               bXlation, bUsdUfePathStr, bUsdUfePath, _) = \
            createSimpleXformScene()

        xform = cmds.createNode('transform')
        xformNames = cmds.ls(xform)
        self.assertEqual(1, len(xformNames))
        xformName = xformNames[0]

        # Duplicate USD data as Maya data, placing it under the transform we created.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.duplicate(
                aUsdUfePathStr, '|world|'+ xformName))

        # Should now have two transform nodes in the Maya scene: the path
        # components in the second segment of the aUsdItem and bUsdItem will
        # now be under the transform.
        aMayaPathStr = '|' + xformName + str(aUsdUfePath.segments[1]).replace('/', '|')
        bMayaPathStr = '|' + xformName + str(bUsdUfePath.segments[1]).replace('/', '|')
        self.assertEqual(1, len(cmds.ls(aMayaPathStr, long=True)))
        self.assertEqual(1, len(cmds.ls(bMayaPathStr, long=True)))
        self.assertEqual(cmds.ls(aMayaPathStr, long=True)[0], aMayaPathStr)
        self.assertEqual(cmds.ls(bMayaPathStr, long=True)[0], bMayaPathStr)

        # Translation should have been copied over to the Maya data model.
        self.assertEqual(cmds.getAttr(aMayaPathStr + '.translate')[0],
                         aXlation)
        self.assertEqual(cmds.getAttr(bMayaPathStr + '.translate')[0],
                         bXlation)

    def testDuplicateAsMayaUndoRedo(self):
        '''Duplicate a USD transform hierarchy to Maya and then undo and redo the command.'''

        (_, _, aXlation, aUsdUfePathStr, aUsdUfePath, _, _, 
               bXlation, bUsdUfePathStr, bUsdUfePath, _) = \
            createSimpleXformScene()

        # Capture selection before duplicate.
        previousSn = cmds.ls(sl=True, ufe=True, long=True)

        # Duplicate USD data as Maya data, placing it under the root.
        cmds.mayaUsdDuplicate(aUsdUfePathStr, '|world')

        def verifyDuplicate():
            # Should now have two transform nodes in the Maya scene: the path
            # components in the second segment of the aUsdItem and bUsdItem will
            # now be under the Maya world.
            aMayaPathStr = str(aUsdUfePath.segments[1]).replace('/', '|')
            bMayaPathStr = str(bUsdUfePath.segments[1]).replace('/', '|')
            self.assertEqual(cmds.ls(aMayaPathStr, long=True)[0], aMayaPathStr)
            self.assertEqual(cmds.ls(bMayaPathStr, long=True)[0], bMayaPathStr)

            # Translation should have been copied over to the Maya data model.
            self.assertEqual(cmds.getAttr(aMayaPathStr + '.translate')[0],
                            aXlation)
            self.assertEqual(cmds.getAttr(bMayaPathStr + '.translate')[0],
                            bXlation)

            # Selection is on the duplicate.
            sn = cmds.ls(sl=True, ufe=True, long=True)
            self.assertEqual(len(sn), 1)
            self.assertEqual(sn[0], aMayaPathStr)

        verifyDuplicate()

        cmds.undo()

        def verifyDuplicateIsGone():
            bMayaPathStr = str(bUsdUfePath.segments[1]).replace('/', '|')
            self.assertListEqual(cmds.ls(bMayaPathStr, long=True), [])
            self.assertEqual(cmds.ls(sl=True, ufe=True, long=True), previousSn)

        verifyDuplicateIsGone()

        cmds.redo()

        verifyDuplicate()

    def testDuplicateAsUsd(self):
        '''Duplicate a Maya transform hierarchy to USD.'''

        # Create a hierarchy.  Because group1 is selected upon creation, group2
        # will be its parent.
        group1 = cmds.createNode('transform')
        group2 = cmds.group()
        self.assertEqual(cmds.listRelatives(group1, parent=True)[0], group2)

        cmds.setAttr(group1 + '.translate', 1, 2, 3)
        cmds.setAttr(group2 + '.translate', -4, -5, -6)

        # Create a stage to receive the USD duplicate.
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        
        # Duplicate Maya data as USD data.  As of 17-Nov-2021 no single-segment
        # path handler registered to UFE for Maya path strings, so use absolute
        # path.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.duplicate(
                cmds.ls(group2, long=True)[0], psPathStr))

        # Maya hierarchy should be duplicated in USD.
        usdGroup2PathStr = psPathStr + ',/' + group2
        usdGroup1PathStr = usdGroup2PathStr + '/' + group1
        usdGroup2Path = ufe.PathString.path(usdGroup2PathStr)
        usdGroup1Path = ufe.PathString.path(usdGroup1PathStr)

        # group1 is the child of group2
        usdGroup1 = ufe.Hierarchy.createItem(usdGroup1Path)
        usdGroup2 = ufe.Hierarchy.createItem(usdGroup2Path)
        usdGroup1Hier = ufe.Hierarchy.hierarchy(usdGroup1)
        usdGroup2Hier = ufe.Hierarchy.hierarchy(usdGroup2)
        self.assertEqual(usdGroup2, usdGroup1Hier.parent())
        self.assertEqual(len(usdGroup2Hier.children()), 1)
        self.assertEqual(usdGroup1, usdGroup2Hier.children()[0])

        # Translations have been preserved.
        usdGroup1T3d = ufe.Transform3d.transform3d(usdGroup1)
        usdGroup2T3d = ufe.Transform3d.transform3d(usdGroup2)
        self.assertEqual([1, 2, 3], usdGroup1T3d.translation().vector)
        self.assertEqual([-4, -5, -6], usdGroup2T3d.translation().vector)

    def testDuplicateAsUsdSameName(self):
        '''Duplicate a Maya transform to USD when USD already has a prim with that name.'''

        # Create a Maya transform named 'A'.
        mayaA = cmds.createNode('transform', name='A')
        cmds.setAttr(mayaA + '.translate', 1, 2, 3)

        # Create a stage to receive the USD duplicate, with a prim of the same name.
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        aPrim = stage.DefinePrim('/A', 'Xform')
        
        # Duplicate Maya data as USD data.  As of 17-Nov-2021 no single-segment
        # path handler registered to UFE for Maya path strings, so use absolute
        # path.
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.duplicate(
                cmds.ls(mayaA, long=True)[0], psPathStr))

        # Maya hierarchy should be duplicated in USD, but with a numeric suffix due to the collision.
        usdNewAPathStr = psPathStr + ',/' + mayaA + '1'
        usdNewAPath = ufe.PathString.path(usdNewAPathStr)

        # Translations have been preserved.
        usdNewA = ufe.Hierarchy.createItem(usdNewAPath)
        usdNewAT3d = ufe.Transform3d.transform3d(usdNewA)
        self.assertEqual([1, 2, 3], usdNewAT3d.translation().vector)

    def testDuplicateAsUsdUndoRedo(self):
        '''Duplicate a Maya transform hierarchy to USD and then undo and redo the command.'''

        # Create a hierarchy.  Because group1 is selected upon creation, group2
        # will be its parent.
        group1 = cmds.createNode('transform')
        group2 = cmds.group()
        self.assertEqual(cmds.listRelatives(group1, parent=True)[0], group2)

        cmds.setAttr(group1 + '.translate', 1, 2, 3)
        cmds.setAttr(group2 + '.translate', -4, -5, -6)

        # Create a stage to receive the USD duplicate.
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        
        # Capture selection before duplicate.
        previousSn = cmds.ls(sl=True, ufe=True, long=True)

        # Duplicate Maya data as USD data.  As of 17-Nov-2021 no single-segment
        # path handler registered to UFE for Maya path strings, so use absolute
        # path.
        cmds.mayaUsdDuplicate(cmds.ls(group2, long=True)[0], psPathStr)

        def verifyDuplicate():
            # Maya hierarchy should be duplicated in USD.
            usdGroup2PathStr = psPathStr + ',/' + group2
            usdGroup1PathStr = usdGroup2PathStr + '/' + group1
            usdGroup2Path = ufe.PathString.path(usdGroup2PathStr)
            usdGroup1Path = ufe.PathString.path(usdGroup1PathStr)

            # group1 is the child of group2
            usdGroup1 = ufe.Hierarchy.createItem(usdGroup1Path)
            usdGroup2 = ufe.Hierarchy.createItem(usdGroup2Path)
            usdGroup1Hier = ufe.Hierarchy.hierarchy(usdGroup1)
            usdGroup2Hier = ufe.Hierarchy.hierarchy(usdGroup2)
            self.assertEqual(usdGroup2, usdGroup1Hier.parent())
            self.assertEqual(len(usdGroup2Hier.children()), 1)
            self.assertEqual(usdGroup1, usdGroup2Hier.children()[0])

            # Translations have been preserved.
            usdGroup1T3d = ufe.Transform3d.transform3d(usdGroup1)
            usdGroup2T3d = ufe.Transform3d.transform3d(usdGroup2)
            self.assertEqual([1, 2, 3], usdGroup1T3d.translation().vector)
            self.assertEqual([-4, -5, -6], usdGroup2T3d.translation().vector)

            # Selection is on duplicate.
            sn = cmds.ls(sl=True, ufe=True, long=True)
            self.assertEqual(len(sn), 1)
            self.assertEqual(sn[0], usdGroup2PathStr)
            
        verifyDuplicate()

        cmds.undo()

        def verifyDuplicateIsGone():
            # Maya hierarchy should no longer be duplicated in USD.
            usdGroup2PathStr = psPathStr + ',/' + group2
            usdGroup2Path = ufe.PathString.path(usdGroup2PathStr)
            usdGroup2 = ufe.Hierarchy.createItem(usdGroup2Path)
            self.assertIsNone(usdGroup2)
            self.assertEqual(cmds.ls(sl=True, ufe=True, long=True), previousSn)

        verifyDuplicateIsGone()

        cmds.redo()

        verifyDuplicate()

    def testDuplicateWithoutMaterials(self):
        '''Duplicate a Maya sphere without merging the materials.'''

        # Create a sphere.
        sphere = cmds.polySphere(r=1)

        # Create a stage to receive the USD duplicate.
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        
        # Duplicate Maya data as USD data with materials
        cmds.mayaUsdDuplicate(cmds.ls(sphere, long=True)[0], psPathStr)

        # Verify that the copied sphere has a look (material) prim.
        looksPrim = stage.GetPrimAtPath("/pSphere1/Looks")
        self.assertTrue(looksPrim.IsValid())

        # Undo duplicate to USD.
        cmds.undo()

        # Duplicate Maya data as USD data without materials
        cmds.mayaUsdDuplicate(cmds.ls(sphere, long=True)[0], psPathStr, exportOptions='shadingMode=none')

        # Verify that the copied sphere does not have a look (material) prim.
        looksPrim = stage.GetPrimAtPath("/pSphere1/Looks")
        self.assertFalse(looksPrim.IsValid())

if __name__ == '__main__':
    unittest.main(verbosity=2)
