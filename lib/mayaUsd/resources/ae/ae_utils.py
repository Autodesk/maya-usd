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

import ufe
import collections
import mayaUsd.ufe as mayaUsdUfe

from pxr import UsdShade

def getConnectedShaders(port):
    """Dig down across NodeGraph boundaries until a surface Shader is found"""
    shaders = []
    for sourceInfo in port.GetConnectedSources()[0]:
        if sourceInfo.source.GetPrim().GetTypeName() != "Shader":
            if sourceInfo.sourceType == UsdShade.AttributeType.Output:
                shaders = shaders + getConnectedShaders(sourceInfo.source.GetOutput(sourceInfo.sourceName))
            else:
                shaders = shaders + getConnectedShaders(sourceInfo.source.GetInput(sourceInfo.sourceName))
        else:
            shaders.append(sourceInfo.source.GetPath())
    return shaders

def getAEBoundMaterials(pathStr):
    """Return the bound materials."""
    ret = []
    try:
        ufePath = ufe.PathString.path(pathStr)
        prim = mayaUsdUfe.ufePathToPrim(pathStr)
        frontPath = ufePath.popSegment()
        matAPI = UsdShade.MaterialBindingAPI(prim)

        if matAPI is None:
            return ret

        # Preserve order of discovery:
        shaders = collections.OrderedDict()

        for purpose in matAPI.GetMaterialPurposes():
            boundInfo = matAPI.ComputeBoundMaterial(materialPurpose=purpose)
            if not boundInfo or not boundInfo[0]:
                continue
            boundMaterial = boundInfo[0]
            for output in boundMaterial.GetSurfaceOutputs():
                for shader in getConnectedShaders(output):
                    usdSeg = ufe.PathSegment(str(shader), mayaUsdUfe.getUsdRunTimeId(), '/')
                    newPath = (frontPath + usdSeg)
                    newPathStr = ufe.PathString.string(newPath)
                    if newPathStr not in shaders:
                        shaders[newPathStr] = 1

        ret = list(shaders.keys())
    except Exception as exp:
        print("Error getting bound materials: {exp}".format(exp=exp))

    return ret

def getAERelatedNodes(pathStr):
    """Return a string array of paths that are related to pathStr and should be shown in extra AE
    tabs."""
    ret = []

    # Add bound materials.
    ret = ret + getAEBoundMaterials(pathStr)

    # Add other related nodes, as required (relationships might be of interest).

    return ret