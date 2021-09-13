#!/usr/bin/env mayapy
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

from maya import cmds
from maya import standalone
from maya.api import OpenMayaUI as OMUI
from pxr import Usd, Sdf

import fixturesUtils
import mayaUtils
import testUtils

import os
import unittest

import ufe
import mayaUsd.ufe

from PySide2 import QtCore
from PySide2.QtTest import QTest
from PySide2.QtWidgets import QWidget

from shiboken2 import wrapInstance

class testUtilsEditability(unittest.TestCase):
    """
    Tests editability of attributes.
    """

    selectabilityToken = "mayaLock"
    onToken = "on"
    offToken = "off"

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__, initializeStandalone=False)

    @classmethod
    def tearDownClass(cls):
        pass

    def resetScene(self):
        cmds.file(new=True, force=True)

    def createStageWithNewLayer(self):
        shapeNode = cmds.createNode('mayaUsdProxyShape', skipSelect=True, name='stageShape1')
        cmds.connectAttr('time1.outTime', shapeNode+'.time')
        cmds.select(shapeNode, replace=True)
        fullPath = cmds.ls(shapeNode, long=True)
        return fullPath[0]

    def createLayer(self):
        '''
        Helper that creates a stage and layer and return the item of its shape item.
        '''
        proxyShapePath = self.createStageWithNewLayer()
        proxyShapePath = ufe.PathString.path(proxyShapePath)
        proxyShapeItem = ufe.Hierarchy.createItem(proxyShapePath)

        return proxyShapeItem, proxyShapePath

    def createPrim(self, underItem, underPath, primType, expectedPath):
        '''
        Helper that creates a prim of the given type under the given item, and return its item.
        '''
        proxyShapeContextOps = ufe.ContextOps.contextOps(underItem)
        proxyShapeContextOps.doOp(['Add New Prim', primType])

        primPath = ufe.Path([
            underPath.segments[0],
            ufe.PathSegment(expectedPath, mayaUsd.ufe.getUsdRunTimeId(), "/")
            ])
        print(f'primPath = {primPath}')
        primItem = ufe.Hierarchy.createItem(primPath)
        prim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(primPath))

        return prim, primItem, primPath

    def prepareProperty(self):
        self.resetScene()
        shapesItem, shapesPath = self.createLayer()
        prim, primItem, primPath = self.createPrim(shapesItem, shapesPath, 'Cone', 'Cone1')

        prop = prim.GetProperty('height')
        ufeProp = ufe.Attributes.attributes(primItem).attribute('height')
        return prop, ufeProp

    def testEditabilityViaUfe(self):
        '''
        Verify that a an attribute can be made non-editable when accessed via UFE.
        '''
        prop, ufeProp = self.prepareProperty()

        # Make sure it is editable by default.
        value = ufeProp.get()
        ufeProp.set(value + 1.0)
        self.assertEqual(ufeProp.get(), value + 1.0)

        # Make sure it is editable when lock is explicitely off.
        prop.SetMetadata('mayaLock', 'off') 
        value = ufeProp.get()
        ufeProp.set(value + 1.0)
        self.assertEqual(ufeProp.get(), value + 1.0)

        # Make sure non-editable attribute with lock "on" cannot be changed.
        prop.SetMetadata('mayaLock', 'on')
        value = ufeProp.get()
        with self.assertRaises(RuntimeError):
            ufeProp.set(value + 1.0)
        self.assertEqual(ufeProp.get(), value)

    def testEditabilityViaUfeCmd(self):
        '''
        Verify that a an attribute can be made non-editable when accessed via UFE commands.
        '''
        prop, ufeProp = self.prepareProperty()

        # Make sure it is editable by default.
        value = ufeProp.get()
        ufeProp.setCmd(value + 1.0).execute()
        self.assertEqual(ufeProp.get(), value + 1.0)

        # Make sure it is editable when lock is explicitely off.
        prop.SetMetadata('mayaLock', 'off') 
        value = ufeProp.get()
        ufeProp.setCmd(value + 1.0).execute()
        self.assertEqual(ufeProp.get(), value + 1.0)

        # Make sure non-editable attribute with lock "on" cannot be changed.
        prop.SetMetadata('mayaLock', 'on')
        value = ufeProp.get()
        with self.assertRaises(RuntimeError):
            ufeProp.setCmd(value + 1.0).execute()
        self.assertEqual(ufeProp.get(), value)

    def testEditabilityMetadatUndo(self):
        '''
        Verify that setting the metadata on a an attribute can be undone when using undo blocks.
        '''
        prop, ufeProp = self.prepareProperty()

        with mayaUsd.lib.UsdUndoBlock():
            prop.SetMetadata('mayaLock', 'off')
        self.assertEqual(prop.GetMetadata('mayaLock'), 'off')

        with mayaUsd.lib.UsdUndoBlock():
            prop.SetMetadata('mayaLock', 'on')
        self.assertEqual(prop.GetMetadata('mayaLock'), 'on')

        cmds.undo()
        self.assertEqual(prop.GetMetadata('mayaLock'), 'off')

        cmds.undo()
        self.assertEqual(prop.GetMetadata('mayaLock'), None)


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
