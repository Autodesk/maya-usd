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

import maya.api.OpenMaya as om
from pxr import Sdf, Tf
import mayaUsd
import ufe
import re

def createPrimAndAttributes(stage, primPath, mayaReferencePath, mayaNamespace):
    prim = stage.DefinePrim(primPath.path, 'MayaReference')

    mayaReferenceAttr = prim.CreateAttribute('mayaReference', Sdf.ValueTypeNames.Asset)
    mayaReferenceAttr.Set(mayaReferencePath)

    mayaNamespaceAttr = prim.CreateAttribute('mayaNamespace', Sdf.ValueTypeNames.String)
    mayaNamespaceAttr.Set(mayaNamespace)

    return prim

# Adapted from testMayaUsdSchemasMayaReference.py.
def createMayaReferencePrim(ufePathStr, mayaReferencePath, mayaNamespace, variantSetName, variantName):
    '''Create a Maya reference prim parented to the argument path.'''

    # Extract the USD path segment from the UFE path and append the Maya
    # reference prim to it.
    ufePath = ufe.PathString.path(ufePathStr)
    parentPath = str(ufePath.segments[1]) if ufePath.nbSegments() > 1 else ''
    primPath = Sdf.AssetPath(parentPath+'/MayaReference')

    stage = mayaUsd.ufe.getStage(ufePathStr)

    variantSetName = Tf.MakeValidIdentifier(variantSetName)
    variantName = Tf.MakeValidIdentifier(variantName)
    if variantSetName and variantName:
        ufePathPrim = mayaUsd.ufe.ufePathToPrim(ufePathStr)
        vset = ufePathPrim.GetVariantSet(variantSetName)
        vset.AddVariant(variantName)
        vset.SetVariantSelection(variantName)
        with vset.GetVariantEditContext():
            # Now all of our subsequent edits will go "inside" the
            # 'variantName' variant of 'variantSetName'.
            prim = createPrimAndAttributes(stage, primPath, mayaReferencePath, mayaNamespace)
    else:
        prim = createPrimAndAttributes(stage, primPath, mayaReferencePath, mayaNamespace)
    if not prim.IsValid():
        om.MGlobal.displayError("Could not create MayaReference prim under %s" % ufePathStr)
        return

def getVariantSetNames(ufePathStr):
    prim = mayaUsd.ufe.ufePathToPrim(ufePathStr)
    if prim.IsValid():
        vs = prim.GetVariantSets()
        return vs.GetNames()
    return []

def getVariantNames(ufePathStr, variantSetName):
    prim = mayaUsd.ufe.ufePathToPrim(ufePathStr)
    if prim.IsValid():
        vn = prim.GetVariantSet(variantSetName).GetVariantNames()
        return vn
    return []

def getPrimPath(ufePathStr):
    ufePath = ufe.PathString.path(ufePathStr)
    parentPath = ufePath.segments[1]
    return str(parentPath)
