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

"""
    Helper functions regarding UFE that will be used throughout the test.
"""

import ufe
from maya import cmds
try:
    from maya.internal.ufeSupport import ufeSelectCmd
except ImportError:
    # Maya 2019 and 2020 don't have ufeSupport plugin, so use fallback.
    from ufeScripts import ufeSelectCmd

def getUfeGlobalSelectionList():
    """ 
        Returns the current UFE selection in a list
        Returns:
            A list(str) containing all selected UFE items
    """
    selection = []
    for item in ufe.GlobalSelection.get():
        selection.append(str(item.path().back()))
    return selection

def selectPath(path, replace=False):
    """ 
        Select a path in the UFE Global selection
        Args:
            path (ufe.Path): The UFE path to select
            replace (bool=False): Replace the selection with
                                    given UFE path
    """
    sceneItem = ufe.Hierarchy.createItem(path)
    if replace:
        selection = ufe.Selection()
        selection.append(sceneItem)
        ufeSelectCmd.replaceWith(selection)
    else:
        ufeSelectCmd.append(sceneItem)

def createUfeSceneItem(dagPath, sdfPath=None):
    """
    Make ufe item out of dag path and sdfpath
    """
    ufePath = ufe.PathString.path('{},{}'.format(dagPath,sdfPath) if sdfPath != None else '{}'.format(dagPath))
    ufeItem = ufe.Hierarchy.createItem(ufePath)
    return ufeItem

def selectUfeItems(selectItems):
    """
    Add given UFE item or list of items to a UFE global selection list
    """
    ufeSelectionList = ufe.Selection()
    
    realListToSelect = selectItems if type(selectItems) is list else [selectItems]
    for item in realListToSelect:
        ufeSelectionList.append(item)
    
    ufe.GlobalSelection.get().replaceWith(ufeSelectionList)

def ufeFeatureSetVersion():
    '''Return the UFE feature set version taking into account rollback to using
    0 for unreleased version after we ship a UFE.

    If you need to test for a specific UFE preview release version then check
    the cmake variable UFE_PREVIEW_VERSION_NUM. This is also set as an env
    for the python tests.
    '''

    # Examples:
    #   v2.0.0 (released version 2).
    #   v0.2.20 (unreleased preview version 2).
    major = ufe.VersionInfo.getMajorVersion()
    return ufe.VersionInfo.getMinorVersion() if major == 0 else major
