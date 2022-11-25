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
#include "UsdUndoDuplicateSelectionCommand.h"

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

namespace MAYAUSD_NS_DEF {
namespace ufe {

namespace {

bool shouldConnectExternalInputs(const Ufe::ValueDictionary& duplicateOptions)
{
    const auto itInputConnections = duplicateOptions.find("inputConnections");
    return itInputConnections != duplicateOptions.end() && itInputConnections->second.get<bool>();
}

} // namespace

UsdUndoDuplicateSelectionCommand::UsdUndoDuplicateSelectionCommand(
    const Ufe::Selection&       selection,
    const Ufe::ValueDictionary& duplicateOptions)
    : Ufe::SelectionUndoableCommand()
    , _copyExternalInputs(shouldConnectExternalInputs(duplicateOptions))
    , _sourceSelection(selection)
{
}

UsdUndoDuplicateSelectionCommand::~UsdUndoDuplicateSelectionCommand() { }

UsdUndoDuplicateSelectionCommand::Ptr UsdUndoDuplicateSelectionCommand::create(
    const Ufe::Selection&       selection,
    const Ufe::ValueDictionary& duplicateOptions)
{
    bool canDuplicate = false;
    for (auto&& item : selection) {
        UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
        if (usdItem) {
            canDuplicate = true;
            break;
        }
    }
    if (canDuplicate) {
        return std::make_shared<UsdUndoDuplicateSelectionCommand>(selection, duplicateOptions);
    }
    return {};
}

void UsdUndoDuplicateSelectionCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    // TODO: MAYA-125854. If duplicating /a/b and /a/b/c, it would make sense to order the
    // operations by SdfPath, and always check if the previously processed path is a prefix of the
    // one currently being processed. In that case, a duplicate task is not necessary because the
    // resulting SceneItem should be built by using SdfPath::ReplacePrefix on the current item to
    // get its location in the previously duplicated parent item.
    for (auto&& item : _sourceSelection) {
        UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
        if (!usdItem) {
            continue;
        }

        // Need to create and execute. If we create all before executing any, then the collision
        // resolution on names will merge bob1 and bob2 into a single bob3 instead of creating a
        // bob3 and a bob4.
        auto duplicateCmd = UsdUndoDuplicateCommand::create(usdItem);
        duplicateCmd->execute();

        // Currently unordered_map since we need to streamline the targetItem override.
        _perItemCommands[item->path()] = duplicateCmd;

        PXR_NS::UsdPrim srcPrim = usdItem->prim();
        PXR_NS::UsdPrim dstPrim = duplicateCmd->duplicatedItem()->prim();

        Ufe::Path stgPath = stagePath(dstPrim.GetStage());
        auto      stageIt = _duplicatesMap.find(stgPath);
        if (stageIt == _duplicatesMap.end()) {
            stageIt = _duplicatesMap.insert({ stgPath, DuplicatePathsMap() }).first;
        }

        // Make sure we are not tracking more than one duplicate per source.
        TF_VERIFY(stageIt->second.count(srcPrim.GetPath()) == 0);
        stageIt->second.insert({ srcPrim.GetPath(), dstPrim.GetPath() });
    }

    // We no longer require the source selection:
    _sourceSelection.clear();

    // Fixups were grouped by stage.
    for (const auto& stageData : _duplicatesMap) {
        PXR_NS::UsdStageWeakPtr stage(getStage(stageData.first));
        if (!stage) {
            continue;
        }
        for (const auto& duplicatePair : stageData.second) {
            // Cleanup relationships and connections on the duplicate.
            for (auto p : UsdPrimRange(stage->GetPrimAtPath(duplicatePair.second))) {
                for (auto& prop : p.GetProperties()) {
                    if (prop.Is<PXR_NS::UsdAttribute>()) {
                        PXR_NS::UsdAttribute  attr = prop.As<PXR_NS::UsdAttribute>();
                        PXR_NS::SdfPathVector sources;
                        attr.GetConnections(&sources);
                        if (updateSdfPathVector(
                                sources, duplicatePair, stageData.second, _copyExternalInputs)) {
                            if (sources.empty()) {
                                attr.ClearConnections();
                                if (!attr.HasValue()) {
                                    p.RemoveProperty(prop.GetName());
                                }
                            } else {
                                attr.SetConnections(sources);
                            }
                        }
                    } else if (prop.Is<PXR_NS::UsdRelationship>()) {
                        PXR_NS::UsdRelationship rel = prop.As<PXR_NS::UsdRelationship>();
                        PXR_NS::SdfPathVector   targets;
                        rel.GetTargets(&targets);
                        // Currently always copying external relationships is the right move since
                        // duplicated geometries will keep their currently assigned material. We
                        // might need a case by case basis later as we deal with more complex
                        // relationships.
                        if (updateSdfPathVector(targets, duplicatePair, stageData.second, true)) {
                            if (targets.empty()) {
                                rel.ClearTargets(true);
                            } else {
                                rel.SetTargets(targets);
                            }
                        }
                    }
                }
            }
        }
    }
}

Ufe::SceneItem::Ptr UsdUndoDuplicateSelectionCommand::targetItem(const Ufe::Path& sourcePath) const
{
    CommandMap::const_iterator it = _perItemCommands.find(sourcePath);
    if (it == _perItemCommands.cend()) {
        return {};
    }
    return it->second->duplicatedItem();
}

bool UsdUndoDuplicateSelectionCommand::updateSdfPathVector(
    PXR_NS::SdfPathVector&               pathVec,
    const DuplicatePathsMap::value_type& duplicatePair,
    const DuplicatePathsMap&             otherPairs,
    const bool                           keepExternal)
{
    bool              hasChanged = false;
    std::list<size_t> indicesToRemove;
    for (size_t i = 0; i < pathVec.size(); ++i) {
        const PXR_NS::SdfPath& path = pathVec[i];
        PXR_NS::SdfPath        finalPath = path;
        // Paths are lexicographically ordered, this means we can search quickly for bounds of
        // candidate paths.
        auto itPath = otherPairs.lower_bound(finalPath);
        if (itPath != otherPairs.begin()) {
            --itPath;
        }
        const auto endPath = otherPairs.upper_bound(finalPath);
        bool       isExternalPath = true;
        for (; itPath != endPath; ++itPath) {
            if (*itPath == duplicatePair) {
                // That one was correctly processed by USD when duplicating.
                isExternalPath
                    = !finalPath.HasPrefix(itPath->first) && !finalPath.HasPrefix(itPath->second);
                continue;
            }
            finalPath = finalPath.ReplacePrefix(itPath->first, itPath->second);
            if (path != finalPath) {
                pathVec[i] = finalPath;
                hasChanged = true;
                isExternalPath = false;
                break;
            }
        }
        if (!keepExternal && isExternalPath) {
            hasChanged = true;
            indicesToRemove.push_front(i);
        }
    }
    for (size_t toRemove : indicesToRemove) {
        pathVec.erase(pathVec.cbegin() + toRemove);
    }
    return hasChanged;
}

void UsdUndoDuplicateSelectionCommand::undo() { _undoableItem.undo(); }

void UsdUndoDuplicateSelectionCommand::redo() { _undoableItem.redo(); }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
