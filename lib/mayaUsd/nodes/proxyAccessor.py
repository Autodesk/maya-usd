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

from pxr import Gf, Sdf, Usd, UsdGeom, Vt
from mayaUsd import lib as mayaUsdLib
import maya.cmds as cmds
import maya.OpenMaya as om
import ufe
import re

def isGatewayNode(dagPath):
    baseProxyShape = 'mayaUsdProxyShapeBase'
    return baseProxyShape in cmds.nodeType(dagPath, inherited=True)

def getUfeSelection():
    try:
        return ufe.GlobalSelection.get().front()
    except:
        return None

def getDagAndPrimFromUfe(ufeObject):
    if ufeObject == None:
        return None, None
        
    segmentCount = len(ufeObject.path().segments)
    if segmentCount == 0:
        return None, None
        
    dagPath = str(ufeObject.path().segments[0])[6:]
    
    if segmentCount == 1:
        if isGatewayNode(dagPath):
            return dagPath, Sdf.Path.absoluteRootPath
        else:
            return dagPath, None
    
    primPath = str(ufeObject.path().segments[1])
    
    return dagPath, primPath

def getSelectedDagAndPrim():    
    return getDagAndPrimFromUfe(getUfeSelection())

def getAccessPlugName(sdfPath):
    plugNameValueAttr = 'AP_' + re.sub(r'[^a-zA-Z0-9_]',r'_',str(sdfPath))
    
    return plugNameValueAttr

def isUfeUsdPath(ufeObject):
    segmentCount = ufeObject.path().nbSegments()
    if segmentCount == 0:
        return False
        
    if segmentCount == 1:
        dagPath = str(ufeObject.path().segments[0])[6:]
        return isGatewayNode(dagPath)
    
    lastSegment = ufeObject.path().segments[segmentCount-1]
    return Sdf.Path.IsValidPathString(str(lastSegment))

def createUfeSceneItem(dagPath, sdfPath=None):
    ufePath = ufe.PathString.path('{},{}'.format(dagPath,sdfPath) if sdfPath != None else '{}'.format(dagPath))
    ufeItem = ufe.Hierarchy.createItem(ufePath)
    return ufeItem

def createXformOps(ufeObject):
    selDag, selPrim = getDagAndPrimFromUfe(ufeObject)
    stage = mayaUsdLib.GetPrim(selDag).GetStage()

    primPath = Sdf.Path(selPrim)
    prim = stage.GetPrimAtPath(primPath)

    xformAPI = UsdGeom.XformCommonAPI(prim)
    t, p, r, s, pInv = xformAPI.CreateXformOps(
                UsdGeom.XformCommonAPI.OpTranslate,
                UsdGeom.XformCommonAPI.OpRotate,
                UsdGeom.XformCommonAPI.OpScale)
          
    if not t.GetAttr().HasAuthoredValue():
        t.GetAttr().Set(Gf.Vec3d(0,0,0))
    if not r.GetAttr().HasAuthoredValue():
        r.GetAttr().Set(Gf.Vec3f(0,0,0))
    if not s.GetAttr().HasAuthoredValue():
        s.GetAttr().Set(Gf.Vec3f(1,1,1))

def createAccessPlug(proxyDagPath, sdfPath, sdfValueType):
    plugNameValueAttr = getAccessPlugName(sdfPath)
    plugNameValueAttrNiceName = str(sdfPath)
    
    plug = mayaUsdLib.ReadUtil.FindOrCreateMayaAttr(
        sdfValueType,
        Sdf.VariabilityUniform,
        proxyDagPath,
        plugNameValueAttr,
        plugNameValueAttrNiceName
        )

def createWorldSpaceAccessPlug(proxyDagPath, sdfPath):
    plugNameValueAttr = getAccessPlugName(sdfPath)
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

def getSdfValueType(ufeObject, usdAttrName):
    proxyDagPath, usdPrimPath = getDagAndPrimFromUfe(ufeObject)

    stage = mayaUsdLib.GetPrim(proxyDagPath).GetStage()

    primPath = Sdf.Path(usdPrimPath)
    prim = stage.GetPrimAtPath(primPath)

    usdAttribute = prim.GetAttribute(usdAttrName)
    if usdAttribute.IsDefined():
        return usdAttribute.GetTypeName()
    else:
        return None

def getAccessPlug(ufeObject, usdAttrName, sdfValueType=Sdf.ValueTypeNames.Matrix4d):
    selectedDagPath, selectedPrimPath = getDagAndPrimFromUfe(ufeObject)

    if selectedDagPath == None or selectedPrimPath == None:
        return None

    sdfPath = Sdf.Path(selectedPrimPath)

    if not usdAttrName == "":
        sdfPath = sdfPath.AppendProperty(usdAttrName)
        sdfValueType = getSdfValueType(ufeObject,usdAttrName)
    
    plugNameValueAttr = getAccessPlugName(sdfPath)
    
    exists = cmds.attributeQuery(plugNameValueAttr, node=selectedDagPath, exists=True)
    if not exists:
        return None
        
    return plugNameValueAttr

def getOrCreateAccessPlug(ufeObject, usdAttrName, sdfValueType=Sdf.ValueTypeNames.Matrix4d):
    # Look for it first
    plugNameValueAttr = getAccessPlug(ufeObject,usdAttrName,sdfValueType)
    
    # Create if doesn't exist
    if plugNameValueAttr == None:
        selectedDagPath, selectedPrimPath = getDagAndPrimFromUfe(ufeObject)

        if selectedDagPath == None or selectedPrimPath == None:
            return None

        sdfPath = Sdf.Path(selectedPrimPath)
        if not usdAttrName == "":
            sdfPath = sdfPath.AppendProperty(usdAttrName)
            
        plugNameValueAttr = getAccessPlugName(sdfPath)
    
        if not usdAttrName == "":
            sdfValueType = getSdfValueType(ufeObject,usdAttrName)
            createAccessPlug(selectedDagPath, sdfPath, sdfValueType)
        else:
            createWorldSpaceAccessPlug(selectedDagPath, sdfPath)
        
        exists = cmds.attributeQuery(plugNameValueAttr, node=selectedDagPath, exists=True)
        if not exists:
            print("Unexpected error in creation of attributes")
    
    return plugNameValueAttr

def keyframeAccessPlug(ufeObject, usdAttrName):
    proxyDagPath, usdPrimPath = getDagAndPrimFromUfe(ufeObject)
    plugNameValueAttr = getOrCreateAccessPlug(ufeObject,usdAttrName)
    
    if plugNameValueAttr == None:
        print("Error in keying. No attributes to key")
    else:
        result = cmds.setKeyframe(proxyDagPath, attribute=plugNameValueAttr)
    
def parentItems(ufeChildren, ufeParent):
    if not isUfeUsdPath(ufeParent):
        print("This method implements parenting under USD prim. Please provide UFE-USD item for ufeParent")
        return
    
    parentDagPath, parentUsdPrimPath = getDagAndPrimFromUfe(ufeParent)
    parentValueAttr = getOrCreateAccessPlug(ufeParent, '', Sdf.ValueTypeNames.Matrix4d )
    parentConnectionAttr = parentDagPath+'.'+parentValueAttr+'[0]'
    
    for ufeChild in ufeChildren:
        if isUfeUsdPath(ufeChild):
            print("Parenting of USD to USD is NOT implemented here")
            continue
         
        childDagPath = getDagAndPrimFromUfe(ufeChild)[0]
        
        print('Parenting "{}" to "{}{}"'.format(childDagPath, parentDagPath,parentUsdPrimPath))
        childConnectionAttr = childDagPath+'.offsetParentMatrix'
        cmds.connectAttr(parentConnectionAttr, childConnectionAttr)

def parent():
   ufeSelection = iter(ufe.GlobalSelection.get())
   ufeSelectionList = []
   for ufeItem in ufeSelection:
       ufeSelectionList.append(ufeItem)

   # clearing this selection so nobody will try to use it.
   # next steps can result in new selection being made due to potential call to createAccessPlug
   ufeSelection = None

   if len(ufeSelectionList) < 2:
       print("Select at least two objects. DAG child/ren and USD parent at the end")
       return
       
   ufeParent = ufeSelectionList[-1]
   ufeChildren = ufeSelectionList[:-1]
   
   parentItems(ufeChildren, ufeParent)

def connectItems(ufeObjectSrc, ufeObjectDst, attrToConnect):
    connectMayaToUsd = isUfeUsdPath(ufeObjectDst)
    if connectMayaToUsd == isUfeUsdPath(ufeObjectSrc):
        print('Must provide two objects with mixed paths (DAG and USD)')
        return
    
    if connectMayaToUsd:
        #source is Maya
        #destination is USD
        createXformOps(ufeObjectDst)
        srcDagPath = getDagAndPrimFromUfe(ufeObjectSrc)[0]
        dstDagPath, dstUsdPrimPath = getDagAndPrimFromUfe(ufeObjectDst)
        for mayaAttr, usdAttr in attrToConnect:
            dstValueAttr = getOrCreateAccessPlug(ufeObjectDst, usdAttr)
            srcConnection = srcDagPath+'.'+mayaAttr
            dstConnection = dstDagPath+'.'+dstValueAttr
            cmds.connectAttr(srcConnection, dstConnection)
    else:
        #source is USD
        #destination is Maya
        srcDagPath, srcUsdPrimPath = getDagAndPrimFromUfe(ufeObjectSrc)
        dstDagPath = getDagAndPrimFromUfe(ufeObjectDst)[0]
        for mayaAttr, usdAttr, valueType in attrToConnect:
            srcValueAttr = getOrCreateAccessPlug(ufeObjectSrc, usdAttr)
            srcConnection = srcDagPath+'.'+srcValueAttr
            dstConnection = dstDagPath+'.'+mayaAttr
            cmds.connectAttr(srcConnection, dstConnection)
            
def connect(attrToConnect=[('translate','xformOp:translate'), ('rotate','xformOp:rotateXYZ')]):
    ufeSelection = ufe.GlobalSelection.get()
    if len(ufeSelection) != 2:
        print('Must select exactly two objects to connect')
        return

    ufeSelectionIter = iter(ufeSelection)

    ufeObjectSrc = next(ufeSelectionIter)
    ufeObjectDst = next(ufeSelectionIter)

    ufeSelectionIter = None
    
    connectItems(ufeObjectSrc,ufeObjectDst,attrToConnect)

def parentConstraintItems(ufeItem, ufeTarget):
    itemDagPath, itemPrimPath = getDagAndPrimFromUfe(ufeItem)
    targetDagPath, targetPrimPath = getDagAndPrimFromUfe(ufeTarget)
    
    itemParentPath = Sdf.Path(itemPrimPath).GetParentPath()
    targetParentPath = Sdf.Path(targetPrimPath).GetParentPath()
    
    createXformOps(ufeItem)
    createXformOps(ufeTarget)
    
    itemTranslatePlug = getOrCreateAccessPlug(ufeItem, 'xformOp:translate')
    itemRotatePlug = getOrCreateAccessPlug(ufeItem, 'xformOp:rotateXYZ')
    itemScalePlug = getOrCreateAccessPlug(ufeItem, 'xformOp:scale')
    
    targetTranslatePlug = getOrCreateAccessPlug(ufeTarget, 'xformOp:translate')
    targetRotatePlug = getOrCreateAccessPlug(ufeTarget, 'xformOp:rotateXYZ')
    targetScalePlug = getOrCreateAccessPlug(ufeTarget, 'xformOp:scale')
    
    constraintNode = cmds.createNode('parentConstraint')

    # Connect the target to the constraint
    cmds.connectAttr('{}.{}'.format(targetDagPath,targetTranslatePlug), '{}.target[0].targetTranslate'.format(constraintNode), f=True)
    cmds.connectAttr('{}.{}'.format(targetDagPath,targetRotatePlug), '{}.target[0].targetRotate'.format(constraintNode), f=True)
    cmds.connectAttr('{}.{}'.format(targetDagPath,targetScalePlug), '{}.target[0].targetScale'.format(constraintNode), f=True)

    # Connect target parent and constraint parent inverse matrix
    if targetParentPath != Sdf.Path.absoluteRootPath:
        ufeTargetParent = createUfeSceneItem(targetDagPath,targetParentPath)
        targetParentPlug = getOrCreateAccessPlug(ufeTargetParent, '', Sdf.ValueTypeNames.Matrix4d )
        cmds.connectAttr('{}.{}[0]'.format(targetDagPath,targetParentPlug), '{}.target[0].targetParentMatrix'.format(constraintNode), f=True)
    
    if itemParentPath != Sdf.Path.absoluteRootPath:
        ufeItemParent = createUfeSceneItem(itemDagPath,itemParentPath)
        itemParentPlug = getOrCreateAccessPlug(ufeItemParent, '', Sdf.ValueTypeNames.Matrix4d )
        inverseNode = cmds.createNode('inverseMatrix')
        cmds.connectAttr('{}.{}[0]'.format(itemDagPath,itemParentPlug), '{}.inputMatrix'.format(inverseNode), f=True)
        cmds.connectAttr('{}.outputMatrix'.format(inverseNode), '{}.constraintParentInverseMatrix'.format(constraintNode), f=True)
   
    # Connect outputs of the constraint to drive the item
    cmds.connectAttr('{}.constraintTranslate'.format(constraintNode), '{}.{}'.format(itemDagPath,itemTranslatePlug), f=True)
    cmds.connectAttr('{}.constraintRotate'.format(constraintNode), '{}.{}'.format(itemDagPath,itemRotatePlug), f=True)
    
def parentConstraint():
    ufeSelection = ufe.GlobalSelection.get()
    if len(ufeSelection) != 2:
        print('Must select exactly two objects to parentConstraint')
        return

    ufeSelectionIter = iter(ufeSelection)

    ufeObjectSrc = next(ufeSelectionIter)
    ufeObjectDst = next(ufeSelectionIter)
    
    ufeSelectionIter = None
    
    parentConstraintItems(ufeObjectSrc, ufeObjectDst)
