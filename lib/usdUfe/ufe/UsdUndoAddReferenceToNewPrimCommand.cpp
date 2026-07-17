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

#include <usdUfe/ufe/UsdUndoAddPayloadCommand.h>
#include <usdUfe/ufe/UsdUndoAddReferenceCommand.h>
#include <usdUfe/ufe/Utils.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>

namespace USDUFE_NS_DEF {

PXR_NAMESPACE_USING_DIRECTIVE

USDUFE_VERIFY_CLASS_SETUP(
    UsdUndoableCommand<Ufe::UndoableCommand>,
    UsdUndoAddReferenceToNewPrimCommand);

UsdUndoAddReferenceToNewPrimCommand::UsdUndoAddReferenceToNewPrimCommand(
    const UsdPrim&     parentPrim,
    const std::string& newPrimName,
    const std::string& filePath,
    const std::string& primPath,
    bool               prepend,
    bool               isPayload,
    bool               preload)
    : _parentPrim(parentPrim)
    , _newPrimName(newPrimName)
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

    // Create a typeless "def". The prim's type is composed through the reference (or payload)
    // arc added below, so it automatically takes on the referenced default prim's type.
    UsdPrim newPrim = stage->DefinePrim(childPath);

    if (!newPrim.IsValid()) {
        TF_RUNTIME_ERROR(
            "UsdUndoAddReferenceToNewPrimCommand: failed to create prim '%s'",
            childPath.GetText());
        return;
    }

    if (_isPayload) {
        UsdUndoAddPayloadCommand payloadCmd(newPrim, _filePath, _primPath, _prepend);
        payloadCmd.execute();

        if (_preload) {
            stage->LoadAndUnload(
                SdfPathSet { newPrim.GetPath() }, SdfPathSet {}, UsdLoadWithDescendants);
        } else {
            stage->LoadAndUnload(SdfPathSet {}, SdfPathSet { newPrim.GetPath() });
        }
    } else {
        UsdUndoAddReferenceCommand refCmd(newPrim, _filePath, _primPath, _prepend);
        refCmd.execute();
    }
}

} // namespace USDUFE_NS_DEF
