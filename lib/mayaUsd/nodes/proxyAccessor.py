#
# Copyright 2020 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
This file contains a set of reference helper functions which can be used to test and discover
new capabilities provided by proxy accessor on a proxy shape.

The end goal is to leverage UFE to provide integration of mixed data model compute with
workflows like parenting (USD prim to Maya DAG object or vice versa), keying, or constraining.
For the time being, this set of helper functions can be used as a reference to understand
the concepts necessary to build tooling on top of the proxy accessor.
"""

from pxr import Sdf, Usd, UsdGeom, Vt
from mayaUsd import lib as mayaUsdLib
import maya.cmds as cmds
import maya.OpenMaya as om
import ufe

def GetUfeSelection():
    try:
        return next(iter(ufe.GlobalSelection.get()))
    except:
        return None

def GetDagAndPrimFromUfe(ufeObject):
    if ufeObject == None:
        return None, None
        
    segmentCount = len(ufeObject.path().segments)
    if segmentCount == 0:
        return None, None
        
    dagPath = str(ufeObject.path().segments[0])[6:]
    
    if segmentCount == 1:
        return dagPath, None
    
    primPath = str(ufeObject.path().segments[1])
    
    return dagPath, primPath

def GetSelectedDagAndPrim():    
    return GetDagAndPrimFromUfe(GetUfeSelection())

def GetAccessPlugName(sdfPath):
    plugNameValueAttr = 'AccessValue_' + str(sdfPath.__hash__())
    
    return plugNameValueAttr

def IsUfeUsdPath(ufeObject):
    segmentCount = len(ufeObject.path().segments)
    if segmentCount < 2:
        return False
    
    lastSegment = ufeObject.path().segments[segmentCount-1]
    return Sdf.Path.IsValidPathString(str(lastSegment))

def CreateXformOps(ufeObject):
    selDag, selPrim = GetDagAndPrimFromUfe(ufeObject)
    stage = mayaUsdLib.GetPrim(selDag).GetStage()

    primPath = Sdf.Path(selPrim)
    prim = stage.GetPrimAtPath(primPath)

    xformAPI = UsdGeom.XformCommonAPI(prim)
    xformAPI.CreateXformOps(
                UsdGeom.XformCommonAPI.OpTranslate,
                UsdGeom.XformCommonAPI.OpRotate,
                UsdGeom.XformCommonAPI.OpScale)

def CreateAccessPlug(proxyDagPath, sdfPath, sdfValueType):
    savedSelection = ufe.Selection()
    savedSelection.replaceWith(ufe.GlobalSelection.get())

    cmds.select(proxyDagPath);
    
    plugNameValueAttr = GetAccessPlugName(sdfPath)
    plugNameValueAttrNiceName = str(sdfPath)
    
    plug = mayaUsdLib.ReadUtil.FindOrCreateMayaAttr(
        sdfValueType,
        Sdf.VariabilityUniform,
        proxyDagPath,
        plugNameValueAttr,
        plugNameValueAttrNiceName
        )

    ufe.GlobalSelection.get().replaceWith(savedSelection)

def CreateWorldSpaceAccessPlug(proxyDagPath, sdfPath):
    savedSelection = ufe.Selection()
    savedSelection.replaceWith(ufe.GlobalSelection.get())
    
    plugNameValueAttr = GetAccessPlugName(sdfPath)
    plugNameValueAttrNiceName = str(sdfPath)
    
    cmds.addAttr(proxyDagPath,
        dt='matrix',
        longName=plugNameValueAttr,
        niceName=plugNameValueAttrNiceName,
        multi=True,
        storable=True)
        
    # Set worldspace flag!
    obj = om.MObject()
    selList = om.MSelectionList()
    selList.add( proxyDagPath )
    selList.getDependNode(0, obj)

    nodeFn = om.MFnDependencyNode(obj)
    attrObj = nodeFn.attribute(plugNameValueAttr)
    attrFn = om.MFnTypedAttribute(attrObj)
    attrFn.setWorldSpace(True)
        
    # Set default value
    cmds.setAttr('{}.{}[0]'.format(proxyDagPath,plugNameValueAttr), [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0], type='matrix')
    
    ufe.GlobalSelection.get().replaceWith(savedSelection)

def GetSdfValueType(ufeObject, usdAttrName):
    proxyDagPath, usdPrimPath = GetDagAndPrimFromUfe(ufeObject)

    stage = mayaUsdLib.GetPrim(proxyDagPath).GetStage()

    primPath = Sdf.Path(usdPrimPath)
    prim = stage.GetPrimAtPath(primPath)

    usdAttribute = prim.GetAttribute(usdAttrName)
    if usdAttribute.IsDefined():
        return usdAttribute.GetTypeName()
    else:
        return None

def GetAccessPlug(ufeObject, usdAttrName, sdfValueType=Sdf.ValueTypeNames.Matrix4d):
    selectedDagPath, selectedPrimPath = GetDagAndPrimFromUfe(ufeObject)

    if selectedDagPath == None or selectedPrimPath == None:
        return None

    sdfPath = Sdf.Path(selectedPrimPath)

    if not usdAttrName == "":
        sdfPath = sdfPath.AppendProperty(usdAttrName)
        sdfValueType = GetSdfValueType(ufeObject,usdAttrName)
    
    plugNameValueAttr = GetAccessPlugName(sdfPath)
    
    exists = cmds.attributeQuery(plugNameValueAttr, node=selectedDagPath, exists=True)
    if not exists:
        return None
        
    return plugNameValueAttr

def GetOrCreateAccessPlug(ufeObject, usdAttrName, sdfValueType=Sdf.ValueTypeNames.Matrix4d):
    # Look for it first
    plugNameValueAttr = GetAccessPlug(ufeObject,usdAttrName,sdfValueType)
    
    # Create if doesn't exist
    if plugNameValueAttr == None:
        selectedDagPath, selectedPrimPath = GetDagAndPrimFromUfe(ufeObject)

        if selectedDagPath == None or selectedPrimPath == None:
            return None

        sdfPath = Sdf.Path(selectedPrimPath)
        if not usdAttrName == "":
            sdfPath = sdfPath.AppendProperty(usdAttrName)
            
        plugNameValueAttr = GetAccessPlugName(sdfPath)
    
        if not usdAttrName == "":
            sdfValueType = GetSdfValueType(ufeObject,usdAttrName)
            CreateAccessPlug(selectedDagPath, sdfPath, sdfValueType)
        else:
            CreateWorldSpaceAccessPlug(selectedDagPath, sdfPath)
        
        exists = cmds.attributeQuery(plugNameValueAttr, node=selectedDagPath, exists=True)
        if not exists:
            print("Unexpected error in creation of attributes")
    
    return plugNameValueAttr

def KeyframeAccessPlug(ufeObject, usdAttrName):
    proxyDagPath, usdPrimPath = GetDagAndPrimFromUfe(ufeObject)
    plugNameValueAttr = GetOrCreateAccessPlug(ufeObject,usdAttrName)
    
    if plugNameValueAttr == None:
        print("Error in keying. No attributes to key")
    else:
        result = cmds.setKeyframe(proxyDagPath, attribute=plugNameValueAttr)
    
def ParentItems(ufeChildren, ufeParent):
    if not IsUfeUsdPath(ufeParent):
        print("Last selected item needs to be USD parent")
        return
    
    parentDagPath, parentUsdPrimPath = GetDagAndPrimFromUfe(ufeParent)
    parentValueAttr = GetOrCreateAccessPlug(ufeParent, '', Sdf.ValueTypeNames.Matrix4d )
    parentConnectionAttr = parentDagPath+'.'+parentValueAttr+'[0]'
    
    for ufeChild in ufeChildren:
        if IsUfeUsdPath(ufeChild):
            print("Parenting of USD to USD is NOT implemented here")
            continue
         
        childDagPath = GetDagAndPrimFromUfe(ufeChild)[0]
        
        print('Parenting "{}" to "{}{}"'.format(childDagPath, parentDagPath,parentUsdPrimPath))
        childConnectionAttr = childDagPath+'.offsetParentMatrix'
        cmds.connectAttr(parentConnectionAttr, childConnectionAttr)

def Parent():
   ufeSelection = iter(ufe.GlobalSelection.get())
   ufeSelectionList = []
   for ufeItem in ufeSelection:
       ufeSelectionList.append(ufeItem)

   # clearing this selection so nobody will try to use it.
   # next steps can result in new selection being made due to potential call to CreateAccessPlug
   ufeSelection = None

   if len(ufeSelectionList) < 2:
       print("Select at least two objects. DAG child/ren and USD parent at the end")
       return
       
   ufeParent = ufeSelectionList[-1]
   ufeChildren = ufeSelectionList[:-1]
   
   ParentItems(ufeChildren, ufeParent)

def ConnectItems(ufeObjectSrc, ufeObjectDst, attrToConnect):
    connectMayaToUsd = IsUfeUsdPath(ufeObjectDst)
    if connectMayaToUsd == IsUfeUsdPath(ufeObjectSrc):
        print('Must provide two objects with mixed paths (DAG and USD)')
        return
    
    if connectMayaToUsd:
        #source is Maya
        #destination is USD
        CreateXformOps(ufeObjectDst)
        srcDagPath = GetDagAndPrimFromUfe(ufeObjectSrc)[0]
        dstDagPath, dstUsdPrimPath = GetDagAndPrimFromUfe(ufeObjectDst)
        for mayaAttr, usdAttr in attrToConnect:
            dstValueAttr = GetOrCreateAccessPlug(ufeObjectDst, usdAttr)
            srcConnection = srcDagPath+'.'+mayaAttr
            dstConnection = dstDagPath+'.'+dstValueAttr
            cmds.connectAttr(srcConnection, dstConnection)
    else:
        #source is USD
        #destination is Maya
        srcDagPath, srcUsdPrimPath = GetDagAndPrimFromUfe(ufeObjectSrc)
        dstDagPath = GetDagAndPrimFromUfe(ufeObjectDst)[0]
        for mayaAttr, usdAttr, valueType in attrToConnect:
            srcValueAttr = GetOrCreateAccessPlug(ufeObjectSrc, usdAttr)
            srcConnection = srcDagPath+'.'+srcValueAttr
            dstConnection = dstDagPath+'.'+mayaAttr
            cmds.connectAttr(srcConnection, dstConnection)
            
def Connect(attrToConnect=[('translate','xformOp:translate'), ('rotate','xformOp:rotateXYZ')]):
    ufeSelection = ufe.GlobalSelection.get()
    if len(ufeSelection) != 2:
        print('Must select exactly two objects to connect')
        return

    ufeSelectionIter = iter(ufeSelection)

    ufeObjectSrc = next(ufeSelectionIter)
    ufeObjectDst = next(ufeSelectionIter)

    ufeSelectionIter = None
    
    ConnectItems(ufeObjectSrc,ufeObjectDst,attrToConnect)
