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

#include "UsdUndoToggleActiveCommand.h"

#include "private/UfeNotifGuard.h"

#include <usdUfe/ufe/Utils.h>

namespace USDUFE_NS_DEF {

UsdUndoToggleActiveCommand::UsdUndoToggleActiveCommand(const PXR_NS::UsdPrim& prim)
    : _stage(prim.GetStage())
    , _primPath(prim.GetPath())
{
}

void UsdUndoToggleActiveCommand::executeImplementation()
{
    if (!_stage)
        return;

    PXR_NS::UsdPrim prim = _stage->GetPrimAtPath(_primPath);
    if (!prim.IsValid())
        return;

    std::string errMsg;
    if (!UsdUfe::isPrimMetadataEditAllowed(
            prim, PXR_NS::SdfFieldKeys->Active, PXR_NS::TfToken(), &errMsg)) {
        // Note: we don't throw an exception because this would break bulk actions.
        TF_RUNTIME_ERROR(errMsg);
        return;
    }

    UsdUfe::InAddOrDeleteOperation ad;
    prim.SetActive(!prim.IsActive());
}

} // namespace USDUFE_NS_DEF
