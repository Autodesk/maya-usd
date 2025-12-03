import maya.cmds as cmds
import mayaUsd.lib
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

def updateDefaultPrimCandidates(excludeMesh, excludeLight, excludeCamera, excludeStage):
    allItems = cmds.ls(assemblies=True)
    excludeList = []

    def add_excluded(items):
        for m in items:
            rel = cmds.listRelatives(m, parent=True, fullPath=True)[0]
            excludeList.append(m)
            excludeList.append(rel)
            if rel.startswith("|"):
                excludeList.append((rel.removeprefix("|")))

    if excludeMesh == "1":
        add_excluded(cmds.ls(type=('mesh'), l=True))

    if excludeLight == "1":
        add_excluded(cmds.ls(type=('light'), l=True))
    
    if excludeCamera == "1":
        add_excluded(cmds.ls(type=('camera'), l=True))
    else:
        # Note: we still want to exclude the startup cameras even if
        # excludeCamera is not set, to avoid exporting them by mistake.
        cameras = cmds.ls(type=('camera'), l=True)
        startup_cameras = []
        for c in cameras:
            camRelatives = cmds.listRelatives(c, parent=True, fullPath=True)
            if cmds.camera(camRelatives[0], startupCamera=True, q=True):
                startup_cameras.append(camRelatives[0])

        startup_cameras.extend([c.removeprefix("|") for c in startup_cameras])
        allItems = list(set(allItems) - set(startup_cameras))
    
    if excludeStage == "1":
        add_excluded(cmds.ls(type=('mayaUsdProxyShape'), l=True))
    
    allItems = removeHiddenInOutliner(list(set(allItems) - set(excludeList)))
    allItems.sort(key=natural_key)
    return allItems

def updateDefaultPrimCandidatesFromSelection(excludeMesh, excludeLight, excludeCamera, excludeStage):
    def _getRelatives(item):
        listInfo = cmds.listRelatives(item, fullPath=True)
        if not listInfo:
            return None
        return listInfo[0]
    
    allSelectedItems = cmds.ls(selection=True)
    allItems = set()
    for i in allSelectedItems:
        path = _getRelatives(i)
        if not path:
            continue
        root = path.split("|")[1]
        allItems.add(root)

    excludeSet = set()

    def _addExclusions(nodeType, excludeParent=False):
        nodes = cmds.ls(type=(nodeType), l=True)
        for n in nodes:
            if excludeParent:
                excludeSet.add(cmds.listRelatives(n, parent=True, fullPath=True)[0])
            path = _getRelatives(n)
            if not path:
                continue
            excludeSet.add(path)

    if excludeMesh == "1":
        _addExclusions('mesh')

    if excludeLight == "1":
        _addExclusions('light')
    
    if excludeCamera == "1":
        _addExclusions('camera')

    if excludeStage == "1":
        _addExclusions('mayaUsdProxyShape', excludeParent=True)

    allItems = removeHiddenInOutliner(list(allItems - excludeSet))
    allItems.sort(key=natural_key)
    return allItems

def getDefaultMaterialScopeName():
    '''
    Retrieve the default material scope name.
    '''
    return  mayaUsd.lib.JobExportArgs.GetDefaultMaterialsScopeName()
