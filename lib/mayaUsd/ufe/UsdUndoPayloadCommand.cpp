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
#include "UsdUndoPayloadCommand.h"

#include <mayaUsd/nodes/proxyShapeStageExtraData.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoLoadUnloadBaseCommand::UsdUndoLoadUnloadBaseCommand(
    const PXR_NS::UsdPrim& prim,
    PXR_NS::UsdLoadPolicy  policy)
    : _stage(prim.GetStage())
    , _primPath(prim.GetPath())
    , _policy(policy)
{
}

UsdUndoLoadUnloadBaseCommand::UsdUndoLoadUnloadBaseCommand(const PXR_NS::UsdPrim& prim)
    : _stage(prim.GetStage())
    , _primPath(prim.GetPath())
    , _policy(PXR_NS::UsdLoadPolicy::UsdLoadWithoutDescendants)
{
    if (!_stage)
        return;

    // When not provided with the load policy, we need to figure out
    // what the current policy is.
    PXR_NS::UsdStageLoadRules loadRules = _stage->GetLoadRules();
    _policy
        = loadRules.GetEffectiveRuleForPath(_primPath) == PXR_NS::UsdStageLoadRules::Rule::AllRule
        ? PXR_NS::UsdLoadPolicy::UsdLoadWithDescendants
        : PXR_NS::UsdLoadPolicy::UsdLoadWithoutDescendants;
}

void UsdUndoLoadUnloadBaseCommand::doLoad() const
{
    if (!_stage)
        return;

    _stage->Load(_primPath, _policy);
    saveModifiedLoadRules();
}

void UsdUndoLoadUnloadBaseCommand::doUnload() const
{
    if (!_stage)
        return;

    _stage->Unload(_primPath);
    saveModifiedLoadRules();
}

void UsdUndoLoadUnloadBaseCommand::saveModifiedLoadRules() const
{
    // Save the load rules so that switching the stage settings will be able to preserve the
    // load rules.
    MayaUsd::MayaUsdProxyShapeStageExtraData::saveLoadRules(_stage);
}

UsdUndoLoadPayloadCommand::UsdUndoLoadPayloadCommand(
    const PXR_NS::UsdPrim& prim,
    PXR_NS::UsdLoadPolicy  policy)
    : UsdUndoLoadUnloadBaseCommand(prim, policy)
{
}

void UsdUndoLoadPayloadCommand::redo() { doLoad(); }
void UsdUndoLoadPayloadCommand::undo() { doUnload(); }

UsdUndoUnloadPayloadCommand::UsdUndoUnloadPayloadCommand(const PXR_NS::UsdPrim& prim)
    : UsdUndoLoadUnloadBaseCommand(prim)
{
}

void UsdUndoUnloadPayloadCommand::redo() { doUnload(); }
void UsdUndoUnloadPayloadCommand::undo() { doLoad(); }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
