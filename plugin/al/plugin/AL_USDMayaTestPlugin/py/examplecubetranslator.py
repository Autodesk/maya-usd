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

from AL import usd, usdmaya

from pxr import Tf, UsdGeom, Gf, Vt

import maya.api.OpenMaya as om


class BoxNodeTranslator(usdmaya.TranslatorBase):

    def __init__(self):
        usdmaya.TranslatorBase.__init__(self)
    
    # extract handles to the MObjects for the attributes we care about
    def initialize(self):
        pcnc = om.MNodeClass('renderBox')
        type(self)._width = pcnc.attribute('szx')
        type(self)._height = pcnc.attribute('szy')
        type(self)._depth = pcnc.attribute('szz')
        return True

    # Currently we are unable to pass MObjects/MDagPaths between C++/python,
    # so we need a util to convert a string to an MObject
    def objectFromString(self, objStr):
        sl = om.MSelectionList()
        sl.add(objStr)
        return sl.getDependNode(0)

    # return the scheam type this class translates
    def getTranslatedType(self):
        return Tf.Type.FindByName('AL_usd_ExamplePolyCubeNode')
    
    # in this case we are importing a DAG node, which will require a transform parent
    def needsTransformParent(self):
        return True
    
    # we support update
    def supportsUpdate(self):
        return True
        
    # we will import by default
    def importableByDefault(self):
        return True

    # create the maya nodes from the USD description
    def importObject(self, prim, par):
        # create the polyCube creator node (and MPlugs for attrs we will change)
        fnCubeNode = om.MFnDagNode()
        nodePolyCube = fnCubeNode.create('renderBox', 'myrender', self.objectFromString(par))
        # store referenced to the nodes to remove during a variant switch
        self.insertItem(prim, fnCubeNode.name());
        # extract values from the USD prim, and set on the maya node
        self.update(prim)
        return True
    
    # override the exportObject method to create a new prim, and 
    # copy the values from maya to USD. 
    def exportObject(self, stage, path, usdPath, params):
        mayaObj = self.objectFromString(path)
        pc = UsdGeom.Cube.Define(stage, usdPath)
        self.writeEdits(pc, mayaObj)

    # Utility method to copy the attribute values from Maya onto the USD prim
    def writeEdits(self, pc, mayaObj):
        pWidth = om.MPlug(mayaObj, type(self)._width)
        pHeight = om.MPlug(mayaObj, type(self)._height)
        pDepth = om.MPlug(mayaObj, type(self)._depth)
        halfExtents = Gf.Vec3f(0.5 * pWidth.asFloat(), 0.5 * pHeight.asFloat(), 0.5 * pDepth.asFloat())
        minMax = Vt.Vec3fArray(2, [-halfExtents, halfExtents])
        pc.GetExtentAttr().Set(minMax)

    # If we needed to make any attribute connections, you should do so here
    def postImport(self, prim):
        return True

    # A method that serves two purposes. 
    # 1. If the prim remains valid a variant switch, we'll update the values in Maya 
    #    to catch cases where a variant *may* have changed some attr values. 
    # 2. When exporting data from Maya to USD, we'll re-use this method to write
    #    the USD parameters
    def update(self, prim):
        # grab the array of maya object names we registered with self.insertItem
        objects = self.getMObjects(prim)
        nodePolyCube = self.objectFromString(objects[0])
        # grab the plugs for the maya attributes
        pWidth = om.MPlug(nodePolyCube, type(self)._width)
        pHeight = om.MPlug(nodePolyCube, type(self)._height)
        pDepth = om.MPlug(nodePolyCube, type(self)._depth)
        # copy values from USD, and set maya values
        pc = usd.schemas.mayatest.ExamplePolyCubeNode(prim)
        pWidth.setFloat(pc.GetWidthAttr().Get())
        pHeight.setFloat(pc.GetHeightAttr().Get())
        pDepth.setFloat(pc.GetDepthAttr().Get())
        return True

    # called prior to tearing down a prim. 
    # Use this to store any edits back into the USD file
    def preTearDown(self, prim):
        objects = self.getMObjects(prim)
        nodePolyCube = self.objectFromString(objects[0])
        pc = UsdGeom.Cube(prim)
        self.writeEdits(pc, nodePolyCube)
        return True
    
    # override to delete any Maya objects you created in import
    def tearDown(self, path):
        self.removeItems(path)
        return True
    
    # Are we able to export the specified maya object?
    def canExport(self, mayaObjectName):
        obj = self.objectFromString(mayaObjectName)
        if obj.apiType() == om.MFn.kRenderBox:
            return usdmaya.ExportFlag.kSupported
        return usdmaya.ExportFlag.kNotSupported

bt = BoxNodeTranslator()
usdmaya.TranslatorBase.registerTranslator(bt)