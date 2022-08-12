#!/usr/bin/env python

#
# Copyright 2022 Autodesk
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

import mayaUsd.lib
import mayaUsd.ufe

import mayaUtils

from pxr import UsdGeom
# Usd, Gf, Sdf
from maya import cmds
from maya import standalone
from maya.api import OpenMaya as om
 
import ufe

import unittest

from testUtils import getTestScene

def visibilityPlug(pathStr):
    dagPath = om.MSelectionList().add(pathStr).getDagPath(0)
    return om.MFnDagNode(dagPath).findPlug('visibility', True)

class HideOrphanedNodesTestCase(unittest.TestCase):
    '''Maya pulled nodes must be hidden when USD scene structure requires it.
    '''

    # We will use the following scene in our tests:
    #
    # A is unloadable, has variant set abVariant (a, b)
    # B has variant set cdVariant (c, d)
    #
    # A variant selection (a), B variant selection (c):
    #
    # stage1
    #       |stageShape1
    #                   /A (a)
    #                     /B (c)
    #                       /C (xformOp:translate = (1, 2, 3))
    #                   /D
    #                     /E   
    #
    # A variant selection (a), B variant selection (d):
    #
    # stage1
    #       |stageShape1
    #                   /A (a)
    #                     /B (d)
    #                       /C (xformOp:translate = (4, 5, 6))
    #                   /D
    #                     /E   
    #
    # A variant selection (b):
    #
    # stage1
    #       |stageShape1
    #                   /A (b)
    #                     /F
    #                   /D
    #                     /E   
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

        testFile = getTestScene("hideOrphanedNodes", "hideOrphanedNodes.usda")
        self.ps, stage = mayaUtils.createProxyFromFile(testFile)

        self.cPathStr = self.ps + ',/A/B/C'
        self.ePathStr = self.ps + ',/D/E'

    def pullAndGetParentVisibility(self, pathStrings):

        visibilityPlugs = {}

        for pathStr in pathStrings:
            # See testEditAsMaya.py comments: PathMappingHandler toHost()
            # unimplemented, use selection to get pull host path.
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(pathStr))
            mayaItem = ufe.GlobalSelection.get().front()
            mayaPath = mayaItem.path()
            self.assertEqual(mayaPath.nbSegments(), 1)
    
            # Get the pull parent from the path.  Pulled node is visible.
            visibility = visibilityPlug(ufe.PathString.string(mayaPath.pop()))
            visibilityPlugs[pathStr] = visibility
            self.assertTrue(visibility.asBool())
        
        return visibilityPlugs

    def testHideOnDelete(self):
        # Pull on C and E.
        pullParentVisibilityPlug = self.pullAndGetParentVisibility(
            [self.cPathStr, self.ePathStr])

        # Delete the proxy shape.  Both pulled nodes should be orphaned and
        # hidden.
        cmds.delete(self.ps)

        for pathStr in pullParentVisibilityPlug:
            self.assertFalse(pullParentVisibilityPlug[pathStr].asBool())

        # Undo: pulled nodes now back as visible.
        cmds.undo()

        for pathStr in pullParentVisibilityPlug:
            self.assertTrue(pullParentVisibilityPlug[pathStr].asBool())

    def testHideOnInactivate(self):
        # Pull on C and E.
        pullParentVisibilityPlug = self.pullAndGetParentVisibility(
            [self.cPathStr, self.ePathStr])

        # Inactivate B, C's parent.
        bPath = ufe.PathString.path(self.cPathStr).pop()
        bPrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(bPath))
        bPrim.SetActive(False)

        # Pulled node for C is hidden
        self.assertFalse(pullParentVisibilityPlug[self.cPathStr].asBool())
        self.assertTrue(pullParentVisibilityPlug[self.ePathStr].asBool())

        bPrim.SetActive(True)

        self.assertTrue(pullParentVisibilityPlug[self.cPathStr].asBool())
        self.assertTrue(pullParentVisibilityPlug[self.ePathStr].asBool())

    def testHideOnPayloadUnload(self):
        # Pull on C and E.
        pullParentVisibilityPlug = self.pullAndGetParentVisibility(
            [self.cPathStr, self.ePathStr])

        # Unload A, C's grandparent.
        aPath = ufe.PathString.path(self.cPathStr).pop().pop()
        aPrim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(aPath))

        aPrim.Unload()

        # Pulled node for C is hidden
        self.assertFalse(pullParentVisibilityPlug[self.cPathStr].asBool())
        self.assertTrue(pullParentVisibilityPlug[self.ePathStr].asBool())

        aPrim.Load()

        self.assertTrue(pullParentVisibilityPlug[self.cPathStr].asBool())
        self.assertTrue(pullParentVisibilityPlug[self.ePathStr].asBool())

    def testHideOnNestedVariantSwitch(self):
        # Pull on C and E.
        pullParentVisibilityPlug = self.pullAndGetParentVisibility(
            [self.cPathStr, self.ePathStr])

        # B's variant set cdVariant is set to variant selection c, so Maya
        # version of pulled node C has translation (1, 2, 3).
        variantCXlation = (1, 2, 3)
        cPrim = mayaUsd.ufe.ufePathToPrim(self.cPathStr)
        cMayaPathStr = mayaUsd.lib.PrimUpdaterManager.readPullInformation(cPrim)
        cDagPath = om.MSelectionList().add(cMayaPathStr).getDagPath(0)
        cFn= om.MFnTransform(cDagPath)
        self.assertEqual(cFn.translation(om.MSpace.kObject),
                         om.MVector(*variantCXlation))

        bPrim = cPrim.GetParent()
        cdVariant = bPrim.GetVariantSet('cdVariant')
        self.assertEqual(cdVariant.GetVariantSelection(), 'c')

        # Switch B's variant set cdVariant to variant selection d.  The prim in
        # that variant is also called C, but it is a different prim, with a
        # different translation, so the pulled node is hidden.
        cdVariant.SetVariantSelection('d')

        cPrim = mayaUsd.ufe.ufePathToPrim(self.cPathStr)
        cXformable = UsdGeom.Xformable(cPrim)
        self.assertEqual(cXformable.GetLocalTransformation().GetRow3(3), [4, 5, 6])

        self.assertFalse(pullParentVisibilityPlug[self.cPathStr].asBool())
        self.assertTrue(pullParentVisibilityPlug[self.ePathStr].asBool())

        # Revert back to variant selection c, pulled node is shown.
        cdVariant.SetVariantSelection('c')

        self.assertTrue(pullParentVisibilityPlug[self.cPathStr].asBool())
        self.assertTrue(pullParentVisibilityPlug[self.ePathStr].asBool())

    def testHideOnNestingVariantSwitch(self):
        # Pull on C and E.
        pullParentVisibilityPlug = self.pullAndGetParentVisibility(
            [self.cPathStr, self.ePathStr])

        # A's variant set abVariant is set to variant selection a, so Maya
        # version of pulled node C is present and has translation (1, 2, 3).
        variantCXlation = (1, 2, 3)
        cPrim = mayaUsd.ufe.ufePathToPrim(self.cPathStr)
        cMayaPathStr = mayaUsd.lib.PrimUpdaterManager.readPullInformation(cPrim)
        cDagPath = om.MSelectionList().add(cMayaPathStr).getDagPath(0)
        cFn= om.MFnTransform(cDagPath)
        self.assertEqual(cFn.translation(om.MSpace.kObject),
                         om.MVector(*variantCXlation))

        aPrim = cPrim.GetParent().GetParent()
        abVariant = aPrim.GetVariantSet('abVariant')
        self.assertEqual(abVariant.GetVariantSelection(), 'a')
        self.assertEqual(aPrim.GetChildrenNames()[0], 'B')

        # Switch A's variant set abVariant to variant selection b.  Pulled node
        # C will be hidden.
        abVariant.SetVariantSelection('b')
        self.assertEqual(aPrim.GetChildrenNames()[0], 'F')

        self.assertFalse(pullParentVisibilityPlug[self.cPathStr].asBool())
        self.assertTrue(pullParentVisibilityPlug[self.ePathStr].asBool())

        # Revert back to variant selection a, pulled node is shown.
        abVariant.SetVariantSelection('a')

        self.assertTrue(pullParentVisibilityPlug[self.cPathStr].asBool())
        self.assertTrue(pullParentVisibilityPlug[self.ePathStr].asBool())

if __name__ == '__main__':
    unittest.main(verbosity=2)
