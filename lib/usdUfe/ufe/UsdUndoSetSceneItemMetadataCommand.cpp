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

#include "Utils.h"
#include "private/Utils.h"

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/value.h>

#include <ufe/scene.h>
#include <ufe/sceneNotification.h>
#include <ufe/undoableCommand.h>

namespace USDUFE_NS_DEF {

SetSceneItemMetadataCommand::SetSceneItemMetadataCommand(
    const PXR_NS::UsdPrim& prim,
    const std::string&     group, // NOLINT
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

void SetSceneItemMetadataCommand::executeImplementation()
{
    if (_stage) {
        const UsdPrim prim = _stage->GetPrimAtPath(_primPath);

        if (_group.GetString().empty()) {
            // If this is not a grouped metadata, set the _value directly on the _key
            prim.SetCustomDataByKey(TfToken(_key), ufeValueToVtValue(_value));
        } else {
            PXR_NS::VtValue data = prim.GetCustomDataByKey(_group);

            PXR_NS::VtDictionary newDict;
            if (!data.IsEmpty()) {
                if (data.IsHolding<PXR_NS::VtDictionary>()) {
                    newDict = data.UncheckedGet<PXR_NS::VtDictionary>();
                } else {
                    return;
                }
            }

            newDict[_key] = ufeValueToVtValue(_value);
            prim.SetCustomDataByKey(_group, PXR_NS::VtValue(newDict));
        }
    }
}

} // namespace USDUFE_NS_DEF
