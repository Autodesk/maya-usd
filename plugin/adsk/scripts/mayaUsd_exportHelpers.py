import maya.cmds as cmds
import re
from mayaUSDRegisterStrings import getMayaUsdString


__naturalOrderRE = re.compile(r'([0-9]+)')
def natural_key(item):
    return [int(s) if s.isdigit() else s.lower() for s in __naturalOrderRE.split(item)]

def isHiddenInOutliner(item):
    '''Verify if the Maya item is hidden in the outliner.'''
    return bool(cmds.getAttr(item + ".hiddenInOutliner"))

def removeHiddenInOutliner(allItems):
    '''Filter and return a list of Maya items to remove those hidden in the outliner.'''
    return [item for item in allItems if not isHiddenInOutliner(item)]

def updateDefaultPrimCandidates(excludeMesh, excludeLight, excludeCamera):
    allItems = cmds.ls(assemblies=True)
    excludeList = []
    if excludeMesh == "1":
        meshes = cmds.ls(type=('mesh'), l=True)
        for m in meshes:
            excludeList.append(cmds.listRelatives(m, parent=True)[0])

    if excludeLight == "1":
        lights = cmds.ls(type=('light'), l=True)
        for l in lights:
            excludeList.append(cmds.listRelatives(l, parent=True)[0])
    
    if excludeCamera == "1":
        cameras = cmds.ls(type=('camera'), l=True)
        for c in cameras:
            excludeList.append(cmds.listRelatives(c, parent=True)[0])
    else:
        cameras = cmds.ls(type=('camera'), l=True)
        startup_cameras = []
        for c in cameras:
            if cmds.camera(cmds.listRelatives(c, parent=True)[0], startupCamera=True, q=True):
                startup_cameras.append(cmds.listRelatives(c, parent=True)[0])

        allItems = list(set(allItems) - set(startup_cameras))
    
    allItems = removeHiddenInOutliner(list(set(allItems) - set(excludeList)))
    allItems.sort(key=natural_key)
    return allItems

def updateDefaultPrimCandidatesFromSelection(excludeMesh, excludeLight, excludeCamera):
    allSelectedItems = cmds.ls(selection=True)
    allItems = set()
    for i in allSelectedItems:
        path = cmds.listRelatives(i, fullPath=True)[0]
        root = path.split("|")[1]
        allItems.add(root)

    excludeSet = set()
    if excludeMesh == "1":
        meshes = cmds.ls(type=('mesh'), l=True)
        for m in meshes:
            excludeSet.add(cmds.listRelatives(m, parent=True)[0])

    if excludeLight == "1":
        lights = cmds.ls(type=('light'), l=True)
        for l in lights:
            excludeSet.add(cmds.listRelatives(l, parent=True)[0])
    
    if excludeCamera == "1":
        cameras = cmds.ls(type=('camera'), l=True)
        for c in cameras:
            excludeSet.add(cmds.listRelatives(c, parent=True)[0])

    allItems = removeHiddenInOutliner(list(allItems - excludeSet))
    allItems.sort(key=natural_key)
    return allItems
