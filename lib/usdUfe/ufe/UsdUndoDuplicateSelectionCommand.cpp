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

#include <ufe/hierarchy.h>

namespace USDUFE_NS_DEF {

// Ensure that UsdUndoDuplicateSelectionCommand is properly setup.
USDUFE_VERIFY_CLASS_SETUP(Ufe::UndoableCommand, UsdUndoDuplicateSelectionCommand);

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
        auto usdItem = downcast(item);
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

        _duplicatedItemsMap.emplace(usdItem, downcast(duplicateCmd->duplicatedItem()));

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
    PXR_NS::SdfPathVector&               referencedPaths,
    const DuplicatePathsMap::value_type& thisPair,
    const DuplicatePathsMap&             allPairs)
{
    const auto&       duplicatePrimPath = thisPair.second;
    std::list<size_t> indicesToRemove;
    bool              referencedPathsChanged = false;

    // A set of prims got duplicated. Let's call the set of prims that got duplicated "original set"
    // and the set of prims that got created through the duplication "duplicate set".
    //
    // Properties of prims in the duplicate set might reference other prims. The duplicate set
    // should be self-contained, so we have to ensure that only prims within the duplicate set are
    // referenced.
    //
    // There are three cases to consider:
    // 1. A property references a prim in the duplicate set.
    //        -> Nothing to do.
    // 2. A property references a prim in the original set.
    //        -> Update the reference to point to the respective prim in the duplicate set.
    // 3. A property references a prim that's in neither set.
    //        -> Delete the reference.
    for (size_t i = 0; i < referencedPaths.size(); ++i) {
        auto& referencedPath = referencedPaths[i];

        // If the referenced path points to a prim in the duplicate set, there's nothing to do.
        const bool isInDuplicateSet = referencedPath.HasPrefix(duplicatePrimPath);
        if (isInDuplicateSet) {
            continue;
        }

        // Check if the original set contains the referenced path. This is true if any path in the
        // DuplicatePathsMap is a prefix of the referenced path.
        //
        // Since paths are ordered lexicographically, a prefix of a path is always less then or
        // equal to the path itself. Also, the prefix will always be "close to" the path itself.
        // Thus, we can get away with checking only a single candidate: The last path that's less
        // than or equal to the referenced path.
        //
        // `upper_bound()` returns the first path that's greater than the referenced path. Our
        // candidate is the path before that.
        const auto firstGreaterPath = allPairs.upper_bound(referencedPath);
        if (firstGreaterPath == allPairs.begin()) {
            // All paths in the original set are greater, so none of them can be a prefix.
            indicesToRemove.push_front(i);
            continue;
        }
        const auto lastSmallerPath = std::prev(firstGreaterPath);

        // Check if our candidate is a prefix of the referenced path. HasPrefix() returns true for
        // equal paths.
        const bool isInOriginalSet = referencedPath.HasPrefix(lastSmallerPath->first);
        if (!isInOriginalSet) {
            indicesToRemove.push_front(i);
            continue;
        }

        // Update the path to point to the respective path in the duplicate set.
        referencedPath
            = referencedPath.ReplacePrefix(lastSmallerPath->first, lastSmallerPath->second);
        referencedPathsChanged = true;
    }

    // The indices to remove are in descending order due to push_front(). Simply iterate and erase.
    for (size_t toRemove : indicesToRemove) {
        referencedPaths.erase(referencedPaths.cbegin() + toRemove);
    }

    return referencedPathsChanged || !indicesToRemove.empty();
}

void UsdUndoDuplicateSelectionCommand::undo() { _undoableItem.undo(); }

void UsdUndoDuplicateSelectionCommand::redo() { _undoableItem.redo(); }

#ifdef UFE_V4_FEATURES_AVAILABLE
Ufe::SceneItem::Ptr UsdUndoDuplicateSelectionCommand::targetItem(const Ufe::Path& sourcePath) const
{
    const auto it = std::find_if(
        _duplicatedItemsMap.begin(), _duplicatedItemsMap.end(), [&sourcePath](const auto& pair) {
            return pair.first->path() == sourcePath;
        });
    if (it == _duplicatedItemsMap.end()) {
        return nullptr;
    }

    return Ufe::Hierarchy::createItem(it->second->path());
}
#endif

} // namespace USDUFE_NS_DEF
