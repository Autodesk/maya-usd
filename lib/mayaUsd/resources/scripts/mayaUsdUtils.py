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

from re import L
from mayaUsdLibRegisterStrings import getMayaUsdLibString

from maya.api import OpenMaya as om
import maya.cmds as cmds

import mayaUsd

import ufe
import os.path

def getPulledInfo(dagPath):
    """
    Retrieves the full DAG path, UFE hierarchy item, FE path of the USD pulled path and the USD prim.
    The USD pulled path and USD prim may be invalid if the pulled object is orphaned.
    """
    # The dagPath must have the pull information that brings us to the USD
    # prim.  If this prim is of type Maya reference, create a specific menu
    # item for it, otherwise return an empty string for the chain of
    # responsibility.

    # The dagPath may come in as a shortest unique name.  Expand it to full
    # name; ls -l would be an alternate implementation.
    sn = om.MSelectionList()
    sn.add(dagPath)
    fullDagPath = sn.getDagPath(0).fullPathName()
    mayaItem = ufe.Hierarchy.createItem(ufe.PathString.path(fullDagPath))
    pathMapper = ufe.PathMappingHandler.pathMappingHandler(mayaItem)
    pulledPath = pathMapper.fromHost(mayaItem.path())
    prim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(pulledPath))
    return fullDagPath, mayaItem, pulledPath, prim


def getDagPathUsdTypeName(dagPath):
    """
    Retrieves the type name of the USD object that created the node pointed to by the DAG path.
    This type name is kept in the 'USD_typeName' attribute.
    Returns None if the attribute is not found.
    """
    sn = om.MSelectionList()
    sn.add(dagPath)
    obj = sn.getDependNode(0)
    node = om.MFnDependencyNode(obj)
    if not node.hasAttribute('USD_typeName'):
        return None

    plug = node.findPlug('USD_typeName', True)
    if not plug:
        return None
        
    return plug.asString()


def isPulledMayaReference(dagPath):
    """
    Verifies if the DAG path refers to a pulled prim that is a Maya reference.
    """
    if getDagPathUsdTypeName(dagPath) == 'MayaReference':
        return True

    _, _, _, prim = getPulledInfo(dagPath)
    return prim and prim.GetTypeName() == 'MayaReference'


def getCurrentTargetLayerDir(prim):
    """
    Retrieve the edit target of the stage of the given prim and return the
    folder that contains the targeted layer.

    Returns an empty string for invalid prim, unsaved layers, etc.
    """
    stage = prim.GetStage()
    if not stage:
        return ''
    layer = stage.GetEditTarget().GetLayer()
    if not layer:
        return ''
    layerFileName = layer.realPath
    return os.path.dirname(layerFileName)


def getMonoFormatFileFilterLabels(includeCompressed = True):
    """
    Returns a list of file-format labels for individual USD file formats.
    This list is not directly usable in the dialog command, they would need
    to be join with ';;'. Call getUSDDialogFileFilters() instead as it does
    the joining.
    """
    labelAndFilters = [
        ("kUsdASCIIFiles", "(*.usda)"),
        ("kUsdBinaryFiles", "(*.usdc)"),
    ]

    if includeCompressed:
        labelAndFilters.append(
            ("kUsdCompressedFiles", "(*.usdz)")
        )
    
    localizedLabels = [getMayaUsdLibString(labelKey) + ' ' + filter for labelKey, filter in labelAndFilters]
    return localizedLabels


def getMultiFormatsFileFilterLabels(includeCompressed = True):
    """
    Returns a list of file-format labels for multi-formats USD file formats.
    This list is not directly usable in the dialog command, they would need
    to be join with ';;'. Call getUSDDialogFileFilters() instead as it does
    the joining.
    """
    labelAndFilters = [
        ("kAllUsdFiles", "(*.usd *.usda *.usdc)"),
        ("kUsdFiles", "(*.usd)"),
    ]

    if includeCompressed:
        labelAndFilters[0] = ("kAllUsdFiles", "(*.usd *.usda *.usdc *.usdz)")

    localizedLabels = [getMayaUsdLibString(labelKey) + ' ' + filter for labelKey, filter in labelAndFilters]
    return localizedLabels


def getUSDDialogFileFilters(includeCompressed = True):
    """
    Returns a text string of all USD file formats, ';;'-separated.
    Directly usable in the Maya SDK dialog commands.
    """
    localizedLabels = getMultiFormatsFileFilterLabels(includeCompressed) + getMonoFormatFileFilterLabels(includeCompressed)
    fileFilters = ';;'.join(localizedLabels)
    return fileFilters


def _getLastUsedUSDDialogFileFilterOptionVarName():
    return "mayaUsd_LastUsedUSDDialogFileFilter"


def loadLastUsedUSDDialogFileFilter():
    """
    Loads the last-used USD file filter.
    If the option variable doesn't exist, return the default file filter.
    """
    varName = _getLastUsedUSDDialogFileFilterOptionVarName()
    if cmds.optionVar(exists=varName):
        return cmds.optionVar(query=varName)
    else:
        return getMayaUsdLibString("kAllUsdFiles")
    

def saveLastUsedUSDDialogFileFilter(fileFilter):
    """
    Saves the last-used USD file filter.
    """
    varName = _getLastUsedUSDDialogFileFilterOptionVarName()
    return cmds.optionVar(stringValue=(varName, fileFilter))


_userSelectedUSDDialogFileFilter = None

def getUserSelectedUSDDialogFileFilter():
    """
    Gets the USD file filter last selected by the user, but not yet saved in optionvar.
    """
    global _userSelectedUSDDialogFileFilter
    return _userSelectedUSDDialogFileFilter
    

def setUserSelectedUSDDialogFileFilter(fileFilter):
    """
    Sets the USD file filter last selected by the user, but not yet saved in optionvar.
    """
    global _userSelectedUSDDialogFileFilter
    _userSelectedUSDDialogFileFilter = fileFilter
    
    
def wantReferenceCompositionArc():
    opVarName = "mayaUsd_WantReferenceCompositionArc"
    return not cmds.optionVar(exists=opVarName) or cmds.optionVar(query=opVarName)

def saveWantReferenceCompositionArc(want):
    opVarName = "mayaUsd_WantReferenceCompositionArc"
    cmds.optionVar(iv=(opVarName, want))

def wantPrependCompositionArc():
    opVarName = "mayaUsd_WantPrependCompositionArc"
    return not cmds.optionVar(exists=opVarName) or cmds.optionVar(query=opVarName)

def saveWantPrependCompositionArc(want):
    opVarName = "mayaUsd_WantPrependCompositionArc"
    cmds.optionVar(iv=(opVarName, want))

def wantPayloadLoaded():
    opVarName = "mayaUsd_WantPayloadLoaded"
    return not cmds.optionVar(exists=opVarName) or cmds.optionVar(query=opVarName)

def saveWantPayloadLoaded(want):
    opVarName = "mayaUsd_WantPayloadLoaded"
    cmds.optionVar(iv=(opVarName, want))

def getReferencedPrimPath():
    opVarName = "mayaUsd_ReferencedPrimPath"
    if not cmds.optionVar(exists=opVarName):
        return ''
    return cmds.optionVar(query=opVarName)

def saveReferencedPrimPath(primPath):
    opVarName = "mayaUsd_ReferencedPrimPath"
    cmds.optionVar(sv=(opVarName, primPath))

def showHelpMayaUSD(contentId):
    """
    Helper method to display help content.

    Note that this is a wrapper around Maya's showHelp() method and showHelpMayaUSD()
    should be used for all help contents in Maya USD.

    Example usage of this method:
    
    - In Python scripts:
    from mayaUsdUtils import showHelpMayaUSD
    showHelpMayaUSD("someContentId");

    - In MEL scripts:
    python(\"from mayaUsdUtils import showHelpMayaUSD; showHelpMayaUSD('someContentId');\")
    
    - In C++:
    MGlobal::executePythonCommand(
    "from mayaUsdUtils import showHelpMayaUSD; showHelpMayaUSD(\"someContentId\");");

    Input contentId refers to the contentId that is registered in helpTableMayaUSD
    file which is used to open help pages.
    """
    import os
    
    try:
        # Finding the path to helpTableMayaUSD file.
        helpTablePath = os.path.join(os.environ['MAYAUSD_LIB_LOCATION'], 'helpTable/helpTableMayaUSD');
        # Setting the default helpTable to helpTableMayaUSD
        cmds.showHelp(helpTablePath, helpTable=True)
        # Showing the help content
        cmds.showHelp(contentId)
    finally:
        # Restoring Maya's default helpTable
        cmds.showHelp('helpTable', helpTable=True)    
