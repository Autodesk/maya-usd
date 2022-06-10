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

import unittest
import mayaUtils
import ufeUtils
import fixturesUtils
from testUtils import assertVectorAlmostEqual, assertVectorNotAlmostEqual

import mayaUsd.lib

from maya import cmds
from maya import standalone

import ufe

@unittest.skipUnless(mayaUtils.ufeSupportFixLevel() > 0, "Requires pixelMove UFE support.")
class PixelMoveCmdTestCase(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def setUp(self):
        cmds.file(new=True, force=True)
        cmds.select(clear=True)
    
    def testPixelMove(self):
        '''Pixel move command must move non-Maya UFE objects.'''

        # pixelMove has different behavior for orthographic and perspective
        # views.  Only perspective views are affected by the selected objects'
        # bounding boxes.  pixelMove sets the view plane origin to be the
        # centroid of the bounding box of the selected objects.
        
        # Save the current camera.
        currentCamera = cmds.lookThru(q=True)

        # Use default perspective camera
        cmds.lookThru('persp')

        # Create a simple USD scene.  Also create a Maya object: this will be
        # our reference.
        import mayaUsd_createStageWithNewLayer

        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/A', 'Xform')
        xformItem = ufeUtils.createItem('|stage1|stageShape1,/A')

        mayaGroup = cmds.group(empty=True)
        mayaGroupXlate = cmds.getAttr(mayaGroup+'.translate')[0]
        assertVectorAlmostEqual(self, mayaGroupXlate, [0, 0, 0])

        # Run pixelMove on the Maya object.  This will be our reference.
        cmds.pixelMove(0, 1)
        mayaGroupXlate = cmds.getAttr(mayaGroup+'.translate')[0]
        assertVectorNotAlmostEqual(self, mayaGroupXlate, [0, 0, 0])

        # pixelMove operates only on the selection, not on a commmand line
        # argument.
        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(xformItem)

        # Initially the object translation is the identity.
        t3d = ufe.Transform3d.transform3d(xformItem)

        self.assertEqual(t3d.translation().vector, [0, 0, 0])

        # Run pixelMove on the USD object.  It must match the Maya values.
        cmds.pixelMove(0, 1)

        assertVectorAlmostEqual(self, t3d.translation().vector, mayaGroupXlate)

        # Restore previous camera.
        cmds.lookThru(currentCamera)

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
