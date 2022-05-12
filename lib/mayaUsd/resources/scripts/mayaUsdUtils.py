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


def getMonoFormatFileFilterLabels(includeCompressed = True):
    """
    Returns a list of file-format labels for individual USD file formats.
    This list is not directly usable in the dialog command, they would need
    to be join with ';;'. Call getUSDDialogFileFilters() instead as it does
    the joining.
    """
    labelAndFilters = [
        ("kUsdASCIIFiles", "*.usda"),
        ("kUsdBinaryFiles", "*.usdc"),
    ]

    if includeCompressed:
        labelAndFilters.append(
            ("kUsdCompressedFiles", "*.usdz")
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
        ("kUsdFiles", "*.usd"),
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


def getLastUsedUSDDialogFileFilter():
    """
    Retrieves the last-used USD file filter.
    If the option variable doesn't exist, return the default file filter.
    """
    varName = _getLastUsedUSDDialogFileFilterOptionVarName()
    if cmds.optionVar(exists=varName):
        return cmds.optionVar(query=varName)
    else:
        return getMayaUsdLibString("kAllUsdFiles")
    

def setLastUsedUSDDialogFileFilter(fileFilter):
    """
    Sets the last-used USD file filter.
    """
    varName = _getLastUsedUSDDialogFileFilterOptionVarName()
    return cmds.optionVar(stringValue=(varName, fileFilter))
