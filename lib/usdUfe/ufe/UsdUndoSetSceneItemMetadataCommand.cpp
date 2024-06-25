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

#include "UsdUndoSetSceneItemMetadataCommand.h"

#include <usdUfe/base/tokens.h>
#include <usdUfe/ufe/Utils.h>

#include <pxr/base/tf/token.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/usd/editContext.h>

#include <ufe/scene.h>
#include <ufe/sceneNotification.h>
#include <ufe/undoableCommand.h>

#include <exception>

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(
    UsdUfe::UsdUndoableCommand<Ufe::UndoableCommand>,
    SetSceneItemMetadataCommand);

SetSceneItemMetadataCommand::SetSceneItemMetadataCommand(
    const PXR_NS::UsdPrim& prim,
    const std::string&     group,
    const std::string&     key,
    const Ufe::Value&      value)
    : _stage(prim.GetStage())
    , _primPath(prim.GetPath())
    , _group(group)
    , _key(key)
    , _value(value)
{
}

SetSceneItemMetadataCommand::SetSceneItemMetadataCommand(
    const PXR_NS::UsdPrim& prim,
    const std::string&     key,
    const Ufe::Value&      value)
    : _stage(prim.GetStage())
    , _primPath(prim.GetPath())
    , _group("")
    , _key(key)
    , _value(value)
{
}

void SetSceneItemMetadataCommand::setKeyMetadata()
{
    const PXR_NS::UsdPrim prim = _stage->GetPrimAtPath(_primPath);

    // If this is not a grouped metadata, set the _value directly on the _key
    prim.SetCustomDataByKey(TfToken(_key), ufeValueToVtValue(_value));
}

void SetSceneItemMetadataCommand::setGroupMetadata()
{
    const PXR_NS::UsdPrim prim = _stage->GetPrimAtPath(_primPath);

    PXR_NS::VtDictionary newDict;
    {
        PXR_NS::VtValue data = prim.GetCustomDataByKey(_group);
        if (!data.IsEmpty()) {
            if (!data.IsHolding<PXR_NS::VtDictionary>()) {
                std::string errMsg = TfStringPrintf(
                    "Metadata [%s] on prim [%s] is not a dictionary.",
                    _group.GetString().c_str(),
                    prim.GetName().GetString().c_str());
                throw std::runtime_error(errMsg);
            }
            newDict = data.UncheckedGet<PXR_NS::VtDictionary>();
        }
    }

    // When targeting the private "Autodesk" metadata group, we always
    // write in the session layer.
    PXR_NS::UsdEditTarget target = _stage->GetEditTarget();
    if (_group == MetadataTokens->Autodesk)
        target = PXR_NS::UsdEditTarget(_stage->GetSessionLayer());
    PXR_NS::UsdEditContext editCtx(_stage, target);

    newDict[_key] = ufeValueToVtValue(_value);
    prim.SetCustomDataByKey(_group, PXR_NS::VtValue(newDict));
}

void SetSceneItemMetadataCommand::executeImplementation()
{
    if (!_stage)
        return;

    if (_group.GetString().empty())
        setKeyMetadata();
    else
        setGroupMetadata();
}

} // namespace USDUFE_NS_DEF
