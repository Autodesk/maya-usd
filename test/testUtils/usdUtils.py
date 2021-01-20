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
    Helper functions regarding USD that will be used throughout the test.
"""

import mayaUsd.ufe

import ufe

from pxr import Usd
from pxr import UsdGeom

usdSeparator = '/'


def createUfePathSegment(usdPath):
    """
        Create an UFE path from a given usd path.
        Args:
            usdPath (str): The usd path to use
        Returns :
            PathSegment of the given usdPath
    """
    return ufe.PathSegment(usdPath, mayaUsd.ufe.getUsdRunTimeId(), usdSeparator)

def getPrimFromSceneItem(item):
    if ufeUtils.ufeFeatureSetVersion() >= 2:
        rawItem = item.getRawAddress()
        prim = mayaUsd.ufe.getPrimFromRawItem(rawItem)
        return prim
    else:
        return Usd.Prim()

def createAnimatedHierarchy(stage):
    """
    Create simple hierarchy in the stage:
    /ParentA
        /Sphere
        /Cube
    /ParenB
    
    Entire ParentA hierarchy will receive time samples on translate for time 1 and 100
    """
    parentA = "/ParentA"
    parentB = "/ParentB"
    childSphere = "/ParentA/Sphere"
    childCube = "/ParentA/Cube"
    
    parentPrimA = stage.DefinePrim(parentA, 'Xform')
    parentPrimB = stage.DefinePrim(parentB, 'Xform')
    childPrimSphere = stage.DefinePrim(childSphere, 'Sphere')
    childPrimCube = stage.DefinePrim(childCube, 'Cube')
    
    UsdGeom.XformCommonAPI(parentPrimA).SetRotate((0,0,0))
    UsdGeom.XformCommonAPI(parentPrimB).SetTranslate((1,10,0))
    
    time1 = Usd.TimeCode(1.)
    UsdGeom.XformCommonAPI(parentPrimA).SetTranslate((0,0,0),time1)
    UsdGeom.XformCommonAPI(childPrimSphere).SetTranslate((5,0,0),time1)
    UsdGeom.XformCommonAPI(childPrimCube).SetTranslate((0,0,5),time1)
    
    time2 = Usd.TimeCode(100.)
    UsdGeom.XformCommonAPI(parentPrimA).SetTranslate((0,5,0),time2)
    UsdGeom.XformCommonAPI(childPrimSphere).SetTranslate((-5,0,0),time2)
    UsdGeom.XformCommonAPI(childPrimCube).SetTranslate((0,0,-5),time2)
