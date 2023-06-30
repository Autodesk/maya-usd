//
// Copyright 2023 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "UsdUndoAddRefOrPayloadCommand.h"

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/payloads.h>
#include <pxr/usd/usd/references.h>

namespace USDUFE_NS_DEF {

using namespace PXR_NS;

static std::string validatePrimSpec(const UsdPrim& prim, const SdfPrimSpecHandle& primSpec)
{
    if (!primSpec)
        return "is not valid.";

    // A common error is to reference a prim that is not the same type as the prim
    // that contains the reference. Since only the type of the prim that contains
    // the reference is used, the referenced prim might not show up.
    //
    // This happens a lot when trying to reference geometry (mesh) instead of the
    // prim containing the geometry. Of vis-versa, referencing a prim inside a mesh.
    const std::string& primType = prim.GetTypeName();
    const std::string& targetType = primSpec->GetTypeName();
    if (primType != targetType)
        return TfStringPrintf(
            "does not have the same type as the targeted prim: [%s] vs [%s].",
            primType.c_str(),
            targetType.c_str());

    return "";
}

static PXR_NS::SdfPath
getPrimPath(const UsdPrim& prim, const std::string& filePath, const std::string& primPath)
{
    // When an explicit prim path was given, use that.
    if (!primPath.empty())
        return SdfPath(primPath);

    // If no prim path were specified and we are referencing a MaterialX file
    // then use the MaterialX prim as the target for the reference.
    //
    // TODO: should we force this even when the referenced file has a default prim?
    if (TfStringEndsWith(filePath, ".mtlx"))
        return SdfPath("/MaterialX");

    // Retrieve the layer for analysis.
    //
    // Note: we don't print any warning if the layer cannot be found as we assume
    //       the load itself will also fail and print an error.
    SdfLayerRefPtr layerRef = SdfLayer::FindOrOpen(filePath);
    if (!layerRef)
        return SdfPath();

    // If the referenced file has a default prim, leave the prim path empty.
    if (layerRef->HasDefaultPrim()) {
        TfToken           primName = layerRef->GetDefaultPrim();
        SdfPrimSpecHandle primSpec = layerRef->GetPrimAtPath(SdfPath(primName.GetText()));
        const std::string errorMessage = validatePrimSpec(prim, primSpec);
        if (!errorMessage.empty())
            TF_WARN(
                (std::string("The default prim in file [%s] ") + errorMessage).c_str(),
                filePath.c_str());
        return SdfPath();
    }

    // If the referenced file has no default prim, return the path to the first
    // valid root prim we find.
    std::string errorMessage = " is absent.";
    for (const SdfPrimSpecHandle primSpec : layerRef->GetRootPrims()) {
        if (!primSpec)
            continue;

        errorMessage = validatePrimSpec(prim, primSpec);
        if (errorMessage.empty())
            return primSpec->GetPath();
    }

    TF_WARN(
        (std::string("The file [%s] does not contain a default prim and the root prim ")
         + errorMessage)
            .c_str(),
        filePath.c_str());

    return SdfPath();
}

/* static */ UsdListPosition UsdUndoAddRefOrPayloadCommand::getListPosition(bool prepend)
{
    return prepend ? UsdListPositionBackOfPrependList : UsdListPositionBackOfAppendList;
}

UsdUndoAddRefOrPayloadCommand::UsdUndoAddRefOrPayloadCommand(
    const UsdPrim&     prim,
    const std::string& filePath,
    const std::string& primPath,
    UsdListPosition    listPos,
    bool               isPayload)
    : _prim(prim)
    , _filePath(filePath)
    , _primPath(primPath)
    , _listPos(listPos)
    , _isPayload(isPayload)
{
}

void UsdUndoAddRefOrPayloadCommand::executeImplementation()
{
    if (!_prim.IsValid())
        return;

    SdfPath primPath = getPrimPath(_prim, _filePath, _primPath);
    if (_isPayload) {
        SdfPayload  payload(_filePath, primPath);
        UsdPayloads primPayloads = _prim.GetPayloads();
        primPayloads.AddPayload(payload, _listPos);
    } else {
        SdfReference  ref(_filePath, primPath);
        UsdReferences primRefs = _prim.GetReferences();
        primRefs.AddReference(ref, _listPos);
    }
}

} // namespace USDUFE_NS_DEF
