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

import maya.api.OpenMaya as om
from pxr import Sdf, Tf, Usd
import mayaUsd
import ufe
import re
import maya.cmds as cmds
from mayaUsdLibRegisterStrings import getMayaUsdLibString
import mayaUsdMayaReferenceUtils as mayaRefUtils

kDefaultEditAsMayaData = False

def createPrimAndAttributes(stage, primPath, mayaReferencePath, mayaNamespace, mayaAutoEdit):
    try:
        prim = stage.DefinePrim(primPath.path, 'MayaReference')
    except (Tf.ErrorException):
        return Usd.Prim()

    mayaReferenceAttr = prim.CreateAttribute('mayaReference', Sdf.ValueTypeNames.Asset)
    mayaReferenceAttr.Set(mayaReferencePath)

    mayaNamespaceAttr = prim.CreateAttribute('mayaNamespace', Sdf.ValueTypeNames.String)
    mayaNamespaceAttr.Set(mayaNamespace)
    
    mayaAutoEditAttr = prim.CreateAttribute('mayaAutoEdit', Sdf.ValueTypeNames.Bool)
    mayaAutoEditAttr.Set(mayaAutoEdit)

    return prim

def getDefaultGroupPrimName(parentPrim, mayaNamespace):
    # "The name of the reference file" + "RN" + "group" for example 'sphereRNgroup'.
    # If a prim with this name already exists, then add a numeric suffix after "RN".
    startName = mayaNamespace + "RNgroup"
    checkName = mayaUsd.ufe.uniqueChildName(parentPrim, startName)
    index = 1
    while (checkName != startName):
        startName = "%sRN%dgroup" % (mayaNamespace, index)
        checkName = mayaUsd.ufe.uniqueChildName(parentPrim, startName)
        index += 1
    return startName

# Adapted from testMayaUsdSchemasMayaReference.py.
def createMayaReferencePrim(ufePathStr, mayaReferencePath, mayaNamespace, 
                            mayaReferencePrimName = mayaRefUtils.defaultMayaReferencePrimName(),
                            groupPrim = None, variantSet = None,
                            mayaAutoEdit = kDefaultEditAsMayaData):
    '''Create a Maya reference prim and optional group prim parented to the argument path.
    Optionally create a variant set and name and placed the edits inside that variant.

    Naming of Maya Reference prim is supported, otherwise default name is used.

    The group prim is optional.

    The variant set and name are optional

    Parameters:
    -----------
    ufePathStr : str : Ufe PathString of parent prim to add Maya Reference
    mayaReferencePath : str : File path of Maya Reference (for attribute)
    mayaNamespace : str : Namespace (for attribute)
    mayaReferencePrimName : str [optional] : Name for the Maya Reference prim
    groupPrim : tuple(str,str,str) [optional] : The Group prim Name, Type & Kind to create
                                                Note: the name is optional and will be auto-computed
                                                      if empty or not provided.
                                                Note: Type and Kind are both mandatory, but Kind is
                                                      allowed to be empty string.
    variantSet : tuple(str,str) [optional] : The Variant Set Name and Variant Name to create

    Return:
    -------
    The Usd prim of the newly created Maya Reference or an invalid prim if there is an error.
    '''

    # Make sure the prim name is valid and doesn't already exist.
    parentPrim = mayaUsd.ufe.ufePathToPrim(ufePathStr)

    # There are special conditions when we are given the ProxyShape gateway node.
    ufePath = ufe.PathString.path(ufePathStr)
    isGateway = (ufePath.nbSegments() == 1)

    # Were we given a Group prim to create?
    groupPrimName = None
    groupPrimType = None
    groupPrimKind = None
    if groupPrim:
        if (len(groupPrim) == 2):
            groupPrimType, groupPrimKind = groupPrim
        elif (len(groupPrim) == 3):
            groupPrimName, groupPrimType, groupPrimKind = groupPrim

            # Make sure the input Group prim name doesn't exist already
            # and validate the input name.
            # Note: it is allowed to be input as empty in which case a default is used.
            if groupPrimName:
                checkGroupPrimName = mayaUsd.ufe.uniqueChildName(parentPrim, groupPrimName)
                if checkGroupPrimName != groupPrimName:
                    errorMsgFormat = getMayaUsdLibString('kErrorGroupPrimExists')
                    errorMsg = cmds.format(errorMsgFormat, stringArg=(groupPrimName, ufePathStr))
                    om.MGlobal.displayError(errorMsg)
                    return Usd.Prim()
                groupPrimName = Tf.MakeValidIdentifier(checkGroupPrimName)

        # If the group prim was either not provided or empty we use a default name.
        if not groupPrimName:
            groupPrimName = getDefaultGroupPrimName(parentPrim, mayaNamespace)

    # When the input is a gateway we cannot have in variant unless group is also given.
    if isGateway and variantSet and not groupPrimName:
        errorMsg = getMayaUsdLibString('kErrorCannotAddToProxyShape')
        om.MGlobal.displayError(errorMsg)
        return Usd.Prim()

    # Make sure the input Maya Reference prim name doesn't exist already
    # and validate the input name.
    # Note: if we are given a group prim to create, then we know that the
    #       Maya Reference prim name will be unique since it will be the
    #       only child (of the newly created group prim).
    checkName = mayaUsd.ufe.uniqueChildName(parentPrim, mayaReferencePrimName) if groupPrim is None else mayaReferencePrimName
    if checkName != mayaReferencePrimName:
        errorMsgFormat = getMayaUsdLibString('kErrorMayaRefPrimExists')
        errorMsg = cmds.format(errorMsgFormat, stringArg=(mayaReferencePrimName, ufePathStr))
        om.MGlobal.displayError(errorMsg)
        return Usd.Prim()
    validatedPrimName = Tf.MakeValidIdentifier(checkName)

    # Extract the USD path segment from the UFE path and append the Maya
    # reference prim to it.
    parentPath = str(ufePath.segments[1]) if ufePath.nbSegments() > 1 else ''

    stage = mayaUsd.ufe.getStage(ufePathStr)

    with mayaUsd.lib.UsdUndoBlock():
        # Optionally insert a Group prim as a parent of the Maya reference prim.
        groupPrim = None
        if groupPrimName:
            groupPath = Sdf.AssetPath(parentPath + '/' + groupPrimName)
            try:
                groupPrim = stage.DefinePrim(groupPath.path, groupPrimType)
            except (Tf.ErrorException):
                groupPrim = Usd.Prim()
            if not groupPrim.IsValid():
                errorMsgFormat = getMayaUsdLibString('kErrorCreatingGroupPrim')
                errorMsg = cmds.format(errorMsgFormat, stringArg=(ufePathStr))
                om.MGlobal.displayError(errorMsg)
                return Usd.Prim()
            if groupPrimKind:
                model = Usd.ModelAPI(groupPrim)
                model.SetKind(groupPrimKind)

        if groupPrim:
            primPath = Sdf.AssetPath(groupPrim.GetPath().pathString + '/' + validatedPrimName)
        else:
            primPath = Sdf.AssetPath(parentPath + '/' + validatedPrimName)

        # Were we given a Variant Set to create?
        variantSetName = None
        variantName = None
        if variantSet and (len(variantSet) == 2):
            variantSetName, variantName = variantSet
        if variantSetName and variantName:
            validatedVariantSetName = Tf.MakeValidIdentifier(variantSetName)
            validatedVariantName = Tf.MakeValidIdentifier(variantName)

            # If we created a group prim add the variant set there, otherwise add it
            # to the prim that corresponds to the input ufe path.
            variantPrim = groupPrim if groupPrim else mayaUsd.ufe.ufePathToPrim(ufePathStr)
            vset = variantPrim.GetVariantSet(validatedVariantSetName)
            vset.AddVariant(validatedVariantName)
            vset.SetVariantSelection(validatedVariantName)
            with vset.GetVariantEditContext():
                # Now all of our subsequent edits will go "inside" the
                # 'variantName' variant of 'variantSetName'.
                prim = createPrimAndAttributes(stage, primPath, mayaReferencePath, mayaNamespace, mayaAutoEdit)
        else:
            prim = createPrimAndAttributes(stage, primPath, mayaReferencePath, mayaNamespace, mayaAutoEdit)
            
    if prim is None or not prim.IsValid():
        errorMsgFormat = getMayaUsdLibString('kErrorCreatingMayaRefPrim')
        errorMsg = cmds.format(errorMsgFormat, stringArg=(ufePathStr))
        om.MGlobal.displayError(errorMsg)
        return Usd.Prim()

    return prim

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
    return '' if ufePath.nbSegments() == 1 else str(ufePath.segments[1])

def getUniqueMayaReferencePrimName(ufePathStr, startName=None):
    '''Helper function to get a unique name for the Maya Reference prim.'''
    newPrimName = mayaRefUtils.defaultMayaReferencePrimName() if not startName else startName
    prim = mayaUsd.ufe.ufePathToPrim(ufePathStr)
    if prim.IsValid():
        return mayaUsd.ufe.uniqueChildName(prim, newPrimName)
    return newPrimName
