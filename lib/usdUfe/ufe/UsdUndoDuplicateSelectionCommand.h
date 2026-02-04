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
#ifndef USDUFE_USDUNDODUPLICATESELECTIONCOMMAND_H
#define USDUFE_USDUNDODUPLICATESELECTIONCOMMAND_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <pxr/usd/sdf/path.h>

#include <ufe/path.h>
#include <ufe/selection.h>
#include <ufe/undoableCommand.h>

#include <map>
#include <unordered_map>
#include <vector>

namespace USDUFE_NS_DEF {

using DuplicatedItemsMap = std::unordered_map<UsdSceneItem::Ptr, UsdSceneItem::Ptr>;

//! \brief UsdUndoDuplicateSelectionCommand
#ifdef UFE_V4_FEATURES_AVAILABLE
class USDUFE_PUBLIC UsdUndoDuplicateSelectionCommand : public Ufe::SelectionUndoableCommand
#else
class USDUFE_PUBLIC UsdUndoDuplicateSelectionCommand : public Ufe::UndoableCommand
#endif
{
public:
    using Ptr = std::shared_ptr<UsdUndoDuplicateSelectionCommand>;

    UsdUndoDuplicateSelectionCommand(
        const Ufe::Selection&    selection,
        const UsdSceneItem::Ptr& dstParentItem);

    // Delete the copy/move constructors assignment operators.
    UsdUndoDuplicateSelectionCommand(const UsdUndoDuplicateSelectionCommand&) = delete;
    UsdUndoDuplicateSelectionCommand& operator=(const UsdUndoDuplicateSelectionCommand&) = delete;
    UsdUndoDuplicateSelectionCommand(UsdUndoDuplicateSelectionCommand&&) = delete;
    UsdUndoDuplicateSelectionCommand& operator=(UsdUndoDuplicateSelectionCommand&&) = delete;

    //! Create a UsdUndoDuplicateSelectionCommand from a Ufe Selection and its parent destination.
    static Ptr create(const Ufe::Selection& selection, const UsdSceneItem::Ptr& dstParentItem);

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "DuplicateSelection"; })

    UFE_V4(Ufe::SceneItem::Ptr targetItem(const Ufe::Path& sourcePath) const override;)

    //! Retrieve all the duplicated items.
    Ufe::SceneItemList targetItems() const;

    DuplicatedItemsMap getDuplicatedItemsMap() const { return _duplicatedItemsMap; }

private:
    UsdUndoableItem _undoableItem;

    // Transient list of items to duplicate. Needed by execute.
    std::vector<UsdSceneItem::Ptr> _sourceItems;

    // Fixup data:
    using DuplicatePathsMap = std::map<PXR_NS::SdfPath, PXR_NS::SdfPath>;
    using DuplicatesMap = std::unordered_map<Ufe::Path, DuplicatePathsMap>;
    DuplicatesMap _duplicatesMap;

    // Needed for temporary USD stages.
    using DstStagesMap = std::unordered_map<Ufe::Path, PXR_NS::UsdStageWeakPtr>;
    DstStagesMap _stagesMap;

    bool updateSdfPathVector(
        PXR_NS::SdfPathVector&               referencedPaths,
        const DuplicatePathsMap::value_type& thisPair,
        const DuplicatePathsMap&             allPairs);

    // Convenience map to have a map between the source item and the duplicated one.
    DuplicatedItemsMap _duplicatedItemsMap;

    // The parent item target destination.
    UsdSceneItem::Ptr _dstParentItem;

}; // UsdUndoDuplicateSelectionCommand

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDUNDODUPLICATESELECTIONCOMMAND_H
