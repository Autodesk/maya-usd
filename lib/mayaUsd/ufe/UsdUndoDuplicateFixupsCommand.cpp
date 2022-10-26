//
// Copyright 2022 Autodesk
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
#include "UsdUndoDuplicateFixupsCommand.h"

#include "private/Utils.h"

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/UsdUndoBlock.h>
#include <mayaUsdUtils/util.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/property.h>
#include <pxr/usd/usd/relationship.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/path.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoDuplicateFixupsCommand::UsdUndoDuplicateFixupsCommand()
    : Ufe::UndoableCommand()
{
}

UsdUndoDuplicateFixupsCommand::~UsdUndoDuplicateFixupsCommand() { }

UsdUndoDuplicateFixupsCommand::Ptr UsdUndoDuplicateFixupsCommand::create()
{
    return std::make_shared<UsdUndoDuplicateFixupsCommand>();
}

void UsdUndoDuplicateFixupsCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    // Fixups were grouped by stage.
    for (const auto& stageData : _duplicatesMap) {
        UsdStageWeakPtr stage(getStage(stageData.first));
        if (!stage) {
            continue;
        }
        for (const auto& duplicatePair : stageData.second) {
            // Cleanup relationships and connections on the duplicate.
            for (auto p : UsdPrimRange(stage->GetPrimAtPath(duplicatePair.second))) {
                for (auto& prop : p.GetProperties()) {
                    if (prop.Is<PXR_NS::UsdAttribute>()) {
                        PXR_NS::UsdAttribute attr = prop.As<PXR_NS::UsdAttribute>();
                        SdfPathVector        sources;
                        attr.GetConnections(&sources);
                        if (_updateSdfPathVector(sources, duplicatePair, stageData.second)) {
                            attr.SetConnections(sources);
                        }
                    } else if (prop.Is<UsdRelationship>()) {
                        UsdRelationship rel = prop.As<UsdRelationship>();
                        SdfPathVector   targets;
                        rel.GetTargets(&targets);
                        if (_updateSdfPathVector(targets, duplicatePair, stageData.second)) {
                            rel.SetTargets(targets);
                        }
                    }
                }
            }
        }
    }
}

bool UsdUndoDuplicateFixupsCommand::_updateSdfPathVector(
    SdfPathVector&                        pathVec,
    const TDuplicatePathsMap::value_type& duplicatePair,
    const TDuplicatePathsMap&             otherPairs)
{
    bool hasChanged = false;
    for (size_t i = 0; i < pathVec.size(); ++i) {
        const SdfPath& path = pathVec[i];
        SdfPath        finalPath = path;
        // Paths are lexicographically ordered, this means we can search quickly for bounds of
        // candidate paths.
        auto itPath = otherPairs.lower_bound(finalPath);
        if (itPath != otherPairs.begin()) {
            --itPath;
        }
        const auto endPath = otherPairs.upper_bound(finalPath);
        for (; itPath != endPath; ++itPath) {
            if (*itPath == duplicatePair) {
                // That one was correctly processed by USD when duplicating.
                continue;
            }
            finalPath = finalPath.ReplacePrefix(itPath->first, itPath->second);
            if (path != finalPath) {
                pathVec[i] = finalPath;
                hasChanged = true;
                break;
            }
        }
    }
    return hasChanged;
}

void UsdUndoDuplicateFixupsCommand::trackDuplicates(const UsdPrim& srcPrim, const UsdPrim& dstPrim)
{
    Ufe::Path path = stagePath(dstPrim.GetStage());
    auto      stageIt = _duplicatesMap.find(path);
    if (stageIt == _duplicatesMap.end()) {
        stageIt = _duplicatesMap.insert({ path, TDuplicatePathsMap() }).first;
    }
    // Make sure we are not tracking more than one duplicate per source.
    TF_VERIFY(stageIt->second.count(srcPrim.GetPath()) == 0);
    stageIt->second.insert({ srcPrim.GetPath(), dstPrim.GetPath() });
}

void UsdUndoDuplicateFixupsCommand::undo() { _undoableItem.undo(); }

void UsdUndoDuplicateFixupsCommand::redo() { _undoableItem.redo(); }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
