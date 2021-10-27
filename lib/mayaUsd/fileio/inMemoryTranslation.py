import maya.cmds as cmds
import maya.api.OpenMaya as om
import maya.api.OpenMayaAnim as oma
import mayaUsd.lib as mayaUsdLib
from mayaUsd.lib import proxyAccessor as pa
import ufe
import re

from pxr import Usd, Sdf

# --------------------------------- UTILITIES ----------------------------------------------
def isAnimated(dagPath):
    if cmds.evaluationManager( query=True, invalidate=True ):
        upstream = cmds.evaluationManager(ust=dagPath)
        countNonProxyShape = len(upstream)
        for node in upstream:
            if cmds.nodeType(node) == 'mayaUsdProxyShape':
                countNonProxyShape -= 1
        print(countNonProxyShape)
        return countNonProxyShape > 0
    else:
        sel_list = om.MSelectionList()
        sel_list.add(dagPath)
        dep_node = sel_list.getDependNode(0)
        return oma.MAnimUtil.isAnimated(dep_node)

def pullSetName():
    return 'pullStateSet'
    
def pullPrimMetadataKey():
    return 'Maya:Pull:DagPath'
    
def pullDGMetadataKey():
    return 'Pull_UfePath'

def dagPathToSdfPath(longDagPath):
    path = re.sub(r'[|]',r'/',longDagPath)
    sdfPath = Sdf.Path(re.sub(r'[^a-zA-Z0-9_/]',r'_',path))
    return sdfPath

def sdfPathToDagPath(sdfPath):
    dagPath = re.sub(r'/',r'|',sdfPath)
    return dagPath

def persistPullInformation(srcUfePath,dstDagPath):
    setName = pullSetName()
    
    if setName not in cmds.listSets(allSets=True):
        cmds.sets(em=True,n=setName)
        
    cmds.sets([dstDagPath],add=setName)
    
    # Store on src prim as custom metadata
    shapePath, srcSdfPath = pa.getDagAndPrimFromUfe(srcUfePath)
    stage = mayaUsdLib.GetPrim(shapePath).GetStage()
    srcPrim = stage.GetPrimAtPath(srcSdfPath)
    srcPrim.SetCustomDataByKey(pullPrimMetadataKey(), dstDagPath)
    # Store on dst dg node as metadata
    if not cmds.attributeQuery(pullDGMetadataKey(), node=dstDagPath, exists=True):
        cmds.addAttr(dstDagPath,longName=pullDGMetadataKey(), dataType='string')
    cmds.setAttr('{}.{}'.format(dstDagPath, pullDGMetadataKey()), ufe.PathString.string(srcUfePath.path()), type='string')

def addExcludeFromRendering(shapePath, sdfPath):
    excludedPrimsString = cmds.getAttr('{}.excludePrimPaths'.format(shapePath)) or ''
    excludedPrims = excludedPrimsString.split(',')
    excludedPrims.insert(0,sdfPath)
    excludedPrimsString = ','.join(set(excludedPrims))
    cmds.setAttr('{}.excludePrimPaths'.format(shapePath), excludedPrimsString, type='string')

def removeExcludeFromRendering(shapePath, sdfPath):
    excludedPrimsString = cmds.getAttr('{}.excludePrimPaths'.format(shapePath)) or ''
    excludedPrims = excludedPrimsString.split(',')
    if sdfPath in excludedPrims:
        excludedPrims.remove(sdfPath)
        excludedPrimsString = ','.join(excludedPrims)
        cmds.setAttr('{}.excludePrimPaths'.format(shapePath), excludedPrimsString, type='string')

def getPullInformationFromDag(dagPath):
    setName = pullSetName()
    
    if setName not in cmds.listSets(allSets=True):
        return None
        
    if not cmds.sets([dagPath],im=setName):
        return None
        
    if not cmds.attributeQuery(pullDGMetadataKey(), node=dagPath, exists=True):
        print('Error - DAG path is in pulled set but does not have pulled path stored')
        return None
        
    pulledUfePathString = cmds.getAttr('{}.{}'.format(dagPath,pullDGMetadataKey())) or ''
    
    pulledUfePath = ufe.PathString.path(pulledUfePathString)
    pulledUfeItem = ufe.Hierarchy.createItem(pulledUfePath)
    
    return pulledUfeItem

def getPullInformationFromUsd(ufePath):
    shapePath, sdfPath = pa.getDagAndPrimFromUfe(ufePath)
    if not (shapePath and sdfPath):
        return None
        
    stage = mayaUsdLib.GetPrim(shapePath).GetStage()
    srcPrim = stage.GetPrimAtPath(sdfPath)
    pulledDagPath = srcPrim.GetCustomDataByKey(pullPrimMetadataKey())
    
    return pulledDagPath
    
def getPullInformationFromPrim(usdPrim):
    pulledDagPath = usdPrim.GetCustomDataByKey(pullPrimMetadataKey())
    
    return pulledDagPath

# --------------------------------- PUSH & PULL ----------------------------------------------
def pull(ufePath, srcLayer, asCopy=False):
    shapePath, sdfPath = pa.getDagAndPrimFromUfe(ufePath)
    
    if shapePath == None or sdfPath == None:
        return
        
    if srcLayer == None:
        srcLayer = mayaUsdLib.GetPrim(shapePath).GetStage().GetRootLayer()
    
    if sdfPath == None:
        sdfPath = Sdf.Path.absoluteRootPath
    
    cmds.mayaUSDImport(f=srcLayer.identifier, primPath=sdfPath, readAnimData=True)
    
    # put objects in the right space
    parentPath = Sdf.Path(sdfPath).GetParentPath()
    childDagPath = sdfPathToDagPath('|'+Sdf.Path(sdfPath).name)
    ufeChildren = [pa.createUfeSceneItem(childDagPath)]
    if parentPath != Sdf.Path.absoluteRootPath:
        ufeParent = pa.createUfeSceneItem(shapePath,parentPath.pathString)
    else:
        ufeParent = pa.createUfeSceneItem(shapePath)
    pa.parentItems(ufeChildren, ufeParent)
    
    if not asCopy:
        #store the linkage between prim and dag in metadata
        persistPullInformation(ufePath,childDagPath)
        
        #prevent doouble rendering by excluding pulled prim from rendering
        addExcludeFromRendering(shapePath, sdfPath)

    return childDagPath

def pushClear(ufePush):
    pulledShapePath = None
    pulledPrim = None
    pushDagPath = None

    if pa.isUfeUsdPath(ufePush):
        pulledShapePath, pulledSdfPathString = pa.getDagAndPrimFromUfe(ufePush)
        pulledStage = mayaUsdLib.GetPrim(pulledShapePath).GetStage()
        pulledPrim = pulledStage.GetPrimAtPath(pulledSdfPathString)
        pushDagPath = pulledPrim.GetCustomDataByKey(pullPrimMetadataKey())
    else:
        pushDagPath, pushSdfPath = pa.getDagAndPrimFromUfe(ufePush)
        pulledUfePath = getPullInformationFromDag(pushDagPath)
        pulledShapePath, pulledSdfPathString = pa.getDagAndPrimFromUfe(pulledUfePath)
        
        pulledStage = mayaUsdLib.GetPrim(pulledShapePath).GetStage()
        pulledPrim = pulledStage.GetPrimAtPath(pulledSdfPathString)
        
    # Cleanup pull infromation and re-enable rendering from stage
    removeExcludeFromRendering(pulledShapePath,pulledPrim.GetPath().pathString)
    cmds.delete(pushDagPath)
    pulledPrim.ClearCustomDataByKey(pullPrimMetadataKey())
    
def push(dagPath, dstLayer, dstSdfPath=None, withAnimation=True):
    if dstSdfPath == Sdf.Path.absoluteRootPath:
        print('Pushing state over pseudo root is not allowed')
        return False
    
    srcLayer = Sdf.Layer.CreateAnonymous()
    cmds.select(dagPath);
    if withAnimation and isAnimated(dagPath):
        cmds.mayaUSDExport(f=srcLayer.identifier, sl=True, frameSample=1.0, frameRange=[cmds.playbackOptions(q=True, min=True),cmds.playbackOptions(q=True, max=True)])
    else:
        cmds.mayaUSDExport(f=srcLayer.identifier, sl=True)

    stage = Usd.Stage.Open(srcLayer)
    srcPrim = stage.GetPrimAtPath(dagPathToSdfPath(dagPath))
    if dstSdfPath:
        Sdf.CopySpec(srcLayer, srcPrim.GetPath(), dstLayer, dstSdfPath)
    else:
        Sdf.CopySpec(srcLayer, srcPrim.GetPath(), dstLayer, srcPrim.GetPath())
    
    # Copy everything (not used anymore...left here simply because this script is not yet under source control)
    #root = stage.GetPrimAtPath(Sdf.Path.absoluteRootPath)
    #for child in root.GetAllChildren():
    #    Sdf.CopySpec(srcLayer, child.GetPath(), dstLayer, child.GetPath())
    
    return True

def pushBack(ufePush):
    pushDagPath, pushSdfPath = pa.getDagAndPrimFromUfe(ufePush)
    if pushSdfPath:
        print('Not implemented pull from usd selection - but we have everything stored to make it')
    else:
        pulledUfePath = getPullInformationFromDag(pushDagPath)
        pulledShapePath, pulledSdfPathString = pa.getDagAndPrimFromUfe(pulledUfePath)
        
        pulledStage = mayaUsdLib.GetPrim(pulledShapePath).GetStage()
        
        dstLayer = pulledStage.GetRootLayer()
        
        if push(pushDagPath,dstLayer,Sdf.Path(pulledSdfPathString)):
            pulledPrim = pulledStage.GetPrimAtPath(pulledSdfPathString)
            # Cleanup pull infromation and re-enable rendering from stage
            removeExcludeFromRendering(pulledShapePath,pulledSdfPathString)
            cmds.delete(pushDagPath)
            # Since we pushed over existing prim, the custom metadata should be already cleared
            pulledPrim.ClearCustomDataByKey(pullPrimMetadataKey())

def pushBetween(ufeSrc,ufeDst):
    if pa.isUfeUsdPath(ufeSrc):
        print('Must select first Dag object')
        return
        
    srcDagPath, srcSdfPath = pa.getDagAndPrimFromUfe(ufeSrc)
    shapePath, dstSdfPath = pa.getDagAndPrimFromUfe(ufeDst)
    
    dstLayer = mayaUsdLib.GetPrim(shapePath).GetStage().GetRootLayer()
    
    if dstSdfPath and dstSdfPath != Sdf.Path.absoluteRootPath:
        push(srcDagPath, dstLayer, Sdf.Path(dstSdfPath))
    else:
        push(srcDagPath, dstLayer)

def pushToVariant(srcDagPath, dstSdfPath, dstLayer, dstVariantSetName, dstVariantName):
    srcLayer = Sdf.Layer.CreateAnonymous()
    cmds.mayaUSDExport(f=srcLayer.identifier, sl=True)
    
    dstStage = Usd.Stage.Open(dstLayer)
    dstPrim = dstStage.GetPrimAtPath(dstSdfPath)
    dstVariantSet = dstPrim.GetVariantSets().AddVariantSet(dstVariantSetName)
    dstVariantSet.AddVariant(dstVariantName)
    dstVariantSet.SetVariantSelection(dstVariantName)
    with dstVariantSet.GetVariantEditContext():
        stage = Usd.Stage.Open(srcLayer)
        root = stage.GetPrimAtPath(Sdf.Path.absoluteRootPath)
        for child in root.GetAllChildren():
            #targetPrimPath = dstPrim.GetPath().AppendPath(dstVariantName)
            #dstStage.DefinePrim(targetPrimPath, 'Scope')
            dstPrimPathWithVariant = dstPrim.GetPath().AppendVariantSelection(dstVariantSetName, dstVariantName)
            dstPrimPathWithVariant = child.GetPath().ReplacePrefix(Sdf.Path.absoluteRootPath,dstPrimPathWithVariant)
            print(dstPrimPathWithVariant)
            Sdf.CopySpec(srcLayer, child.GetPath(), dstLayer, dstPrimPathWithVariant)
            
# -------------------------------------- WORKFLOWS ------------------------------------------

def pullSelection():
    pulledDagPath = pull(pa.getUfeSelection(),None) or ''
    cmds.select(pulledDagPath)

def pushSelection():
    ufeSelection = ufe.GlobalSelection.get()
    if len(ufeSelection) == 2:
        ufeSelectionIter = iter(ufeSelection)
    
        ufeSrc = next(ufeSelectionIter)
        ufeDst = next(ufeSelectionIter)
        
        pushBetween(ufeSrc,ufeDst)
    elif len(ufeSelection) == 1:
        ufePush = next(iter(ufeSelection))
        pushBack(ufePush)
    else:
        print('Unhandled push from selection')
        
def pushClearSelection():
    ufeSelection = ufe.GlobalSelection.get()
    if len(ufeSelection) == 1:
        ufePush = next(iter(ufeSelection))
        pushClear(ufePush)
    else:
        print('Unhandled push clear from selection')

def pushSelectionToVariant(variantSetName, variantName):
    ufeSelection = ufe.GlobalSelection.get()
    if len(ufeSelection) != 2:
        print('Must select exactly two objects')
        return

    ufeSelectionIter = iter(ufeSelection)

    ufeSrc = next(ufeSelectionIter)
    ufeDst = next(ufeSelectionIter)
    
    ufeSelectionIter = None
    
    if pa.isUfeUsdPath(ufeSrc):
        print('Must select first Dag object')
        return
        
    if not pa.isUfeUsdPath(ufeDst):
        print('Must select second ufe-usd object')
        return
        
    srcDagPath, srcSdfPath = pa.getDagAndPrimFromUfe(ufeSrc)
    shapePath, dstSdfPath = pa.getDagAndPrimFromUfe(ufeDst)
    
    dstLayer = mayaUsdLib.GetPrim(shapePath).GetStage().GetRootLayer()
    
    pushToVariant(srcDagPath, dstSdfPath, dstLayer, variantSetName, variantName)

def assignMaterialToSelection(material):
    ufeItem = pa.getUfeSelection()
    shapePath, sdfPath = pa.getDagAndPrimFromUfe(ufeItem)
    
    tmpDagPath = sdfPathToDagPath(sdfPath)
    
    pull(ufeItem,None)
    
    cmds.select(tmpDagPath);
    cmds.sets(e=True,forceElement=material)
    cmds.select(cl=True)
    
    dstLayer = mayaUsdLib.GetPrim(shapePath).GetStage().GetRootLayer()
    push(tmpDagPath, dstLayer)
    
    cmds.delete(tmpDagPath)
    
