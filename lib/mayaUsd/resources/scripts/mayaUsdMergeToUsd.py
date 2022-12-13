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

from maya.api import OpenMaya as om

import maya.cmds as cmds

import mayaUsd
import mayaUsdCacheMayaReference
import mayaUsdUtils
from mayaUsdLibRegisterStrings import getMayaUsdLibString

import ufe

from functools import partial

def createDefaultMenuItem(dagPath, precedingItem):
    # This is the final callable in the chain of responsibility, so it must
    # always return a menu item.
    _, _, _, prim = mayaUsdUtils.getPulledInfo(dagPath)
    if prim and prim.IsValid() and mayaUsd.lib.PrimUpdaterManager.readPullInformation(prim):
        enabled = 1
    else:
        enabled = 0

    cmd = 'maya.mel.eval("mayaUsdMenu_pushBackToUSD ' + dagPath + '")'
    mergeLabel = getMayaUsdLibString('kMenuMergeMayaEdits')
    returnedItem = cmds.menuItem(label=mergeLabel, insertAfter=precedingItem, image="merge_to_USD.png", command=cmd, enable=enabled)

    cmd = 'maya.mel.eval("mayaUsdMenu_pushBackToUSDOptions ' + dagPath + '")'
    cmds.menuItem(insertAfter=returnedItem, optionBox=True, command=cmd, enable=enabled)

    return returnedItem

def createMayaReferenceMenuItem(dagPath, precedingItem):
    '''Create a merge to USD menuItem for Maya references.

    If the Maya object at dagPath is a transform corresponding to a Maya
    reference, create a menuItem for it.
    '''

    # The dagPath must have the pull information that brings us to the USD
    # prim.  If this prim is of type Maya reference, create a specific menu
    # item for it, otherwise return an empty string for the chain of
    # responsibility.
    fullDagPath, _, _, prim = mayaUsdUtils.getPulledInfo(dagPath)

    # If the pulled prim doesn't exist anymore, we won't delegate the reponsibility to
    # another creator and handle the menu item in here. We already have all the information
    # available.
    if prim and prim.IsDefined():
        # If the pulled prim isn't a MayaReference, not our responsibility.
        if prim.GetTypeName() != 'MayaReference':
            return ''

        cmd = partial(mayaUsdCacheMayaReference.cacheDialog, fullDagPath, prim)
        cacheLabel = getMayaUsdLibString('kMenuCacheToUsd')
        return cmds.menuItem(label=cacheLabel, insertAfter=precedingItem,
                             image="cache_to_USD.png", command=cmd, enable=1)
    else:
        menuLabel = getMayaUsdLibString('kMenuMergeMayaEdits')
        return cmds.menuItem(label=menuLabel, insertAfter=precedingItem,
                             image="merge_to_USD.png", enable=0)

_menuItemCreators = [createMayaReferenceMenuItem, createDefaultMenuItem]

def prependMenuItem(item):
    _menuItemCreators.insert(0, item)

def createMenuItem(dagPath, precedingItem):
    '''Create a merge to USD menuItem for Maya object at dagPath.

    The menuItem will be placed in the menu after precedingItem, which must be
    a menuItem string name.
    '''

    for creator in _menuItemCreators:
        created = creator(dagPath, precedingItem)
        if created:
            return created

    # The following means that default menu item creation failed.
    errorMsg = getMayaUsdLibString('kErrorMergeToUsdMenuItem')
    cmds.error(errorMsg)
