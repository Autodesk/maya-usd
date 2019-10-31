#!/usr/bin/env python

#
# Copyright 2019 Autodesk
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

from ufeTestUtils import mayaUtils
from ufeTestUtils import usdUtils
from ufeTestUtils.testUtils import assertVectorAlmostEqual

import ufe

from pxr import Usd, UsdGeom

import maya.cmds as cmds
import maya.api.OpenMaya as OpenMaya

import unittest
import os

def nameToPlug(nodeName):
    selection = OpenMaya.MSelectionList()
    selection.add(nodeName)
    return selection.getPlug(0)

class Object3dTestCase(unittest.TestCase):
    '''Verify the Object3d UFE interface, for the USD runtime.
    '''

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    def setUp(self):
        ''' Called initially to set up the Maya test environment '''
        self.assertTrue(self.pluginsLoaded)

    def testBoundingBox(self):
        '''Test the Object3d bounding box interface.'''

        # Create a simple USD scene.  Default sphere radius is 1, so extents
        # are from (-1, -1, -1) to (1, 1, 1).
        usdFilePath = cmds.internalVar(utd=1) + '/testObject3d.usda'
        stage = Usd.Stage.CreateNew(usdFilePath)
        xform = stage.DefinePrim('/parent', 'Xform')
        sphere = stage.DefinePrim('/parent/sphere', 'Sphere')
        extentAttr = sphere.GetAttribute('extent')
        extent = extentAttr.Get()
        
        assertVectorAlmostEqual(self, extent[0], [-1]*3)
        assertVectorAlmostEqual(self, extent[1], [1]*3)

        # Move the sphere.  Its UFE bounding box should not be affected by
        # transformation hierarchy.
        UsdGeom.XformCommonAPI(xform).SetTranslate((7, 8, 9))

        # Save out the file, and bring it back into Maya under a proxy shape.
        stage.GetRootLayer().Save()

        proxyShape = cmds.createNode('mayaUsdProxyShape')
        cmds.setAttr('mayaUsdProxyShape1.filePath', usdFilePath, type='string')

        # MAYA-101766: loading a stage is done by the proxy shape compute.
        # Because we're a script, no redraw is done, so need to pull explicitly
        # on the outStageData (and simply discard the returned data), to get
        # the proxy shape compute to run, and thus the stage to load.  This
        # should be improved.  Without a loaded stage, UFE item creation
        # fails.  PPT, 31-10-2019.
        outStageData = nameToPlug('mayaUsdProxyShape1.outStageData')
        outStageData.asMDataHandle()

        proxyShapeMayaPath = cmds.ls(proxyShape, long=True)[0]
        proxyShapePathSegment = mayaUtils.createUfePathSegment(
            proxyShapeMayaPath)
        
        # Create a UFE scene item from the sphere prim.
        spherePathSegment = usdUtils.createUfePathSegment('/parent/sphere')
        spherePath = ufe.Path([proxyShapePathSegment, spherePathSegment])
        sphereItem = ufe.Hierarchy.createItem(spherePath)

        # Get its Object3d interface.
        object3d = ufe.Object3d.object3d(sphereItem)

        # Get its bounding box.
        ufeBBox = object3d.boundingBox()

        # Compare it to known extents.
        assertVectorAlmostEqual(self, ufeBBox.min.vector, [-1]*3)
        assertVectorAlmostEqual(self, ufeBBox.max.vector, [1]*3)

        # Remove the test file.
        os.remove(usdFilePath)
