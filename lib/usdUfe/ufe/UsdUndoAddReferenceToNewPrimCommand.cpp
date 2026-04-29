//
// Copyright 2026 Autodesk
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
#include "UsdUndoAddReferenceToNewPrimCommand.h"

#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/editRouterContext.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/payload.h>
#include <pxr/usd/sdf/reference.h>
#include <pxr/usd/usd/payloads.h>
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usd/stage.h>

namespace USDUFE_NS_DEF {

PXR_NAMESPACE_USING_DIRECTIVE

USDUFE_VERIFY_CLASS_SETUP(
    UsdUndoableCommand<Ufe::UndoableCommand>,
    UsdUndoAddReferenceToNewPrimCommand);

/* static */
UsdListPosition UsdUndoAddReferenceToNewPrimCommand::getListPosition(bool prepend)
{
    return prepend ? UsdListPositionBackOfPrependList : UsdListPositionBackOfAppendList;
}

UsdUndoAddReferenceToNewPrimCommand::UsdUndoAddReferenceToNewPrimCommand(
    const UsdPrim&     parentPrim,
    const std::string& newPrimName,
    const std::string& newPrimType,
    const std::string& filePath,
    const std::string& primPath,
    bool               prepend,
    bool               isPayload,
    bool               preload)
    : _parentPrim(parentPrim)
    , _newPrimName(newPrimName)
    , _newPrimType(newPrimType)
    , _filePath(filePath)
    , _primPath(primPath)
    , _prepend(prepend)
    , _isPayload(isPayload)
    , _preload(preload)
{
}

void UsdUndoAddReferenceToNewPrimCommand::executeImplementation()
{
    if (!_parentPrim.IsValid())
        return;

    auto stage = _parentPrim.GetStage();
    if (!stage)
        return;

    const std::string uniqueName = UsdUfe::uniqueChildName(_parentPrim, _newPrimName);
    const SdfPath     childPath = _parentPrim.GetPath().AppendChild(TfToken(uniqueName));

    UsdPrim newPrim;
    if (_newPrimType == "Class") {
        newPrim = stage->CreateClassPrim(childPath);
    } else {
        TfToken typeToken = _newPrimType.empty() ? TfToken() : TfToken(_newPrimType);
        newPrim = stage->DefinePrim(childPath, typeToken);
    }

    if (!newPrim.IsValid()) {
        TF_RUNTIME_ERROR(
            "UsdUndoAddReferenceToNewPrimCommand: failed to create prim '%s'",
            childPath.GetText());
        return;
    }

    const SdfPath refPrimPath = _primPath.empty() ? SdfPath() : SdfPath(_primPath);
    const auto    listPos = getListPosition(_prepend);

    if (_isPayload) {
        SdfPayload  payload(_filePath, refPrimPath);
        UsdPayloads primPayloads = newPrim.GetPayloads();
        PrimMetadataEditRouterContext ctx(newPrim, SdfFieldKeys->Payload);
        primPayloads.AddPayload(payload, listPos);
        if (_preload) {
            stage->LoadAndUnload(
                SdfPathSet { newPrim.GetPath() }, SdfPathSet {}, UsdLoadWithDescendants);
        } else {
            stage->LoadAndUnload(SdfPathSet {}, SdfPathSet { newPrim.GetPath() });
        }
    } else {
        SdfReference  ref(_filePath, refPrimPath);
        UsdReferences primRefs = newPrim.GetReferences();
        PrimMetadataEditRouterContext ctx(newPrim, SdfFieldKeys->References);
        primRefs.AddReference(ref, listPos);
    }
}

} // namespace USDUFE_NS_DEF
