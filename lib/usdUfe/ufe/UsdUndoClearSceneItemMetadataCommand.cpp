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

#include "UsdUndoClearSceneItemMetadataCommand.h"

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/value.h>

#include <ufe/scene.h>
#include <ufe/sceneNotification.h>

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(
    UsdUfe::UsdUndoableCommand<Ufe::UndoableCommand>,
    ClearSceneItemMetadataCommand);

ClearSceneItemMetadataCommand::ClearSceneItemMetadataCommand(
    const PXR_NS::UsdPrim& prim,
    const std::string&     group,
    const std::string&     key)
    : _stage(prim.GetStage())
    , _primPath(prim.GetPath())
    , _group(group)
    , _key(key)
{
}

void ClearSceneItemMetadataCommand::executeImplementation()
{
    if (_stage) {
        const PXR_NS::UsdPrim prim = _stage->GetPrimAtPath(_primPath);
        if (_group.GetString().empty()) {
            // If this is not a grouped meta data, remove the key
            prim.ClearCustomDataByKey(TfToken(_key));
        } else {

            const PXR_NS::VtValue data = prim.GetCustomDataByKey(_group);

            if (!data.IsEmpty() && data.IsHolding<PXR_NS::VtDictionary>()) {
                // Remove the key and its value.
                if (!_key.empty()) {
                    PXR_NS::VtDictionary newDict = data.UncheckedGet<PXR_NS::VtDictionary>();
                    if (newDict.find(_key) != newDict.end()) {
                        newDict.erase(_key);

                        // Set the new data.
                        prim.SetCustomDataByKey(_group, PXR_NS::VtValue(newDict));
                    }
                }
                // Remove the group.
                else {
                    prim.ClearCustomDataByKey(_group);
                }
            }
        }
    }
}

} // namespace USDUFE_NS_DEF
