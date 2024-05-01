//
// Copyright 2024 Autodesk
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

#include <usdUfe/ufe/UsdUndoDuplicateCommand.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/undo/UsdUndoBlock.h>

namespace USDUFE_NS_DEF {

// Ensure that UsdUndoDuplicateSelectionCommand is properly setup.
static_assert(std::is_base_of<Ufe::UndoableCommand, UsdUndoDuplicateSelectionCommand>::value);
static_assert(std::has_virtual_destructor<UsdUndoDuplicateSelectionCommand>::value);
static_assert(!std::is_copy_constructible<UsdUndoDuplicateSelectionCommand>::value);
static_assert(!std::is_copy_assignable<UsdUndoDuplicateSelectionCommand>::value);
static_assert(!std::is_move_constructible<UsdUndoDuplicateSelectionCommand>::value);
static_assert(!std::is_move_assignable<UsdUndoDuplicateSelectionCommand>::value);

UsdUndoDuplicateSelectionCommand::UsdUndoDuplicateSelectionCommand(
    const Ufe::Selection&    selection,
    const UsdSceneItem::Ptr& dstParentItem)
    : _dstParentItem(dstParentItem)
{
    _sourceItems.reserve(selection.size());
    for (auto&& item : selection) {
        if (selection.containsAncestor(item->path())) {
            // MAYA-125854: Skip the descendant, it will get duplicated with the ancestor.
            continue;
        }
        UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
        if (!usdItem) {
            continue;
        }
        _sourceItems.push_back(usdItem);
    }
}

UsdUndoDuplicateSelectionCommand::Ptr UsdUndoDuplicateSelectionCommand::create(
    const Ufe::Selection&    selection,
    const UsdSceneItem::Ptr& dstParentItem)
{
    UsdUndoDuplicateSelectionCommand::Ptr retVal
        = std::make_shared<UsdUndoDuplicateSelectionCommand>(selection, dstParentItem);

    if (retVal->_sourceItems.empty() || !dstParentItem) {
        return {};
    }

    return retVal;
}

void UsdUndoDuplicateSelectionCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    for (auto&& usdItem : _sourceItems) {
        auto duplicateCmd = UsdUndoDuplicateCommand::create(usdItem, _dstParentItem);
        duplicateCmd->execute();

        _duplicatedItemsMap.emplace(
            usdItem, std::dynamic_pointer_cast<UsdSceneItem>(duplicateCmd->sceneItem()));

        PXR_NS::UsdPrim srcPrim = usdItem->prim();
        PXR_NS::UsdPrim dstPrim = duplicateCmd->duplicatedItem()->prim();

        Ufe::Path stgPath = stagePath(dstPrim.GetStage());
        auto      stageIt = _duplicatesMap.find(stgPath);
        if (stageIt == _duplicatesMap.end()) {
            stageIt = _duplicatesMap.insert({ stgPath, DuplicatePathsMap() }).first;
        }

        auto stageMapIt = _stagesMap.find(stgPath);
        if (stageMapIt == _stagesMap.end()) {
            stageMapIt = _stagesMap.insert({ stgPath, dstPrim.GetStage() }).first;
        }

        stageIt->second.insert({ srcPrim.GetPath(), dstPrim.GetPath() });
    }

    // Fixups were grouped by stage.
    for (const auto& stageData : _duplicatesMap) {
        PXR_NS::UsdStageWeakPtr stage;

        auto stageIt = _stagesMap.find(stageData.first);
        if (stageIt != _stagesMap.end()) {
            stage = stageIt->second;
        }

        if (!stage)
            continue;

        for (const auto& duplicatePair : stageData.second) {
            // Cleanup relationships and connections on the duplicate.

            // Update the connections and the relationships only the first level, in fact,
            // SdfCopySpec will remap to target objects beneath \p dstPath: attribute connections,
            // relationship targets, inherit and specializes paths, and internal sub-root references
            // that target an object beneath \p srcPath
            auto p = stage->GetPrimAtPath(duplicatePair.second);

            if (PXR_NS::UsdShadeMaterial(p))
                continue;

            for (auto& prop : p.GetProperties()) {
                if (prop.Is<PXR_NS::UsdAttribute>()) {
                    PXR_NS::UsdAttribute  attr = prop.As<PXR_NS::UsdAttribute>();
                    PXR_NS::SdfPathVector sources;
                    attr.GetConnections(&sources);
                    if (updateSdfPathVector(sources, duplicatePair, stageData.second)) {
                        if (sources.empty()) {
                            attr.ClearConnections();
                            if (!attr.HasValue() && !UsdShadeNodeGraph(attr.GetPrim())) {
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
                    if (updateSdfPathVector(targets, duplicatePair, stageData.second)) {
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

Ufe::SceneItemList UsdUndoDuplicateSelectionCommand::targetItems() const
{
    Ufe::SceneItemList targetItems;
    for (const auto& duplicatedItem : _duplicatedItemsMap) {
        targetItems.push_back(duplicatedItem.second);
    }

    return targetItems;
}

bool UsdUndoDuplicateSelectionCommand::updateSdfPathVector(
    PXR_NS::SdfPathVector&               pathVec,
    const DuplicatePathsMap::value_type& duplicatePair,
    const DuplicatePathsMap&             otherPairs)
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
        if (isExternalPath) {
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

} // namespace USDUFE_NS_DEF
