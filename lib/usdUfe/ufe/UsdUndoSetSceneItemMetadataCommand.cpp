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
#include <usdUfe/utils/editRouter.h>
#include <usdUfe/utils/editRouterContext.h>

#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/usd/editContext.h>

#include <ufe/scene.h>
#include <ufe/sceneNotification.h>
#include <ufe/undoableCommand.h>

#include <exception>

namespace USDUFE_NS_DEF {

namespace {

void setSceneItemCustomDataByKey(
    const PXR_NS::UsdPrim& prim,
    const PXR_NS::TfToken& key,
    const Ufe::Value&      value)
{
    std::string errMsg;
    if (!UsdUfe::isPrimMetadataEditAllowed(prim, PXR_NS::SdfFieldKeys->CustomData, key, &errMsg)) {
        // Note: we don't throw an exception because this would break bulk actions.
        TF_RUNTIME_ERROR(errMsg);
    } else {
        prim.SetCustomDataByKey(key, ufeValueToVtValue(value));
    }
}

} // namespace

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
    const PXR_NS::TfToken key(_key);

    // If this is not a grouped metadata, set the _value directly on the _key
    PrimMetadataEditRouterContext ctx(prim, PXR_NS::SdfFieldKeys->CustomData, key);
    setSceneItemCustomDataByKey(prim, key, _value);
}

void SetSceneItemMetadataCommand::setGroupMetadata()
{
    const PXR_NS::UsdPrim prim = _stage->GetPrimAtPath(_primPath);

    // When the group name starts with "SessionLayer-", remove that prefix
    // and write in the session layer if the operation is not editRouted.
    std::string prefixlessGroupName;
    if (isSessionLayerGroupMetadata(_group, &prefixlessGroupName)) {
        PXR_NS::TfToken fullKey(prefixlessGroupName + std::string(":") + _key);

        PrimMetadataEditRouterContext ctx(
            prim,
            PXR_NS::SdfFieldKeys->CustomData,
            fullKey,
            /*fallbackLayer=*/_stage->GetSessionLayer());

        setSceneItemCustomDataByKey(prim, fullKey, _value);
    } else {
        PXR_NS::TfToken               fullKey(_group + std::string(":") + _key);
        PrimMetadataEditRouterContext ctx(prim, PXR_NS::SdfFieldKeys->CustomData, fullKey);
        setSceneItemCustomDataByKey(prim, fullKey, _value);
    }
}

void SetSceneItemMetadataCommand::executeImplementation()
{
    if (!_stage)
        return;

    if (_group.empty())
        setKeyMetadata();
    else
        setGroupMetadata();
}

} // namespace USDUFE_NS_DEF
