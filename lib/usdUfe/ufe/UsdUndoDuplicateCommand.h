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
#ifndef USDUFE_USDUNDODUPLICATECOMMAND_H
#define USDUFE_USDUNDODUPLICATECOMMAND_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <pxr/usd/sdf/path.h>

#include <ufe/path.h>
#include <ufe/undoableCommand.h>

namespace USDUFE_NS_DEF {

//! \brief UsdUndoDuplicateCommand
#ifdef UFE_V4_FEATURES_AVAILABLE
class USDUFE_PUBLIC UsdUndoDuplicateCommand : public Ufe::SceneItemResultUndoableCommand
#else
class USDUFE_PUBLIC UsdUndoDuplicateCommand : public Ufe::UndoableCommand
#endif
{
public:
    using Ptr = std::shared_ptr<UsdUndoDuplicateCommand>;

    UsdUndoDuplicateCommand(
        const UsdSceneItem::Ptr& srcItem,
        const UsdSceneItem::Ptr& dstParentItem);

    // Delete the copy/move constructors assignment operators.
    UsdUndoDuplicateCommand(const UsdUndoDuplicateCommand&) = delete;
    UsdUndoDuplicateCommand& operator=(const UsdUndoDuplicateCommand&) = delete;
    UsdUndoDuplicateCommand(UsdUndoDuplicateCommand&&) = delete;
    UsdUndoDuplicateCommand& operator=(UsdUndoDuplicateCommand&&) = delete;

    //! Create a UsdUndoDuplicateCommand from a SceneItem and its parent destination.
    static Ptr create(const UsdSceneItem::Ptr& srcItem, const UsdSceneItem::Ptr& dstParentItem);

    UsdSceneItem::Ptr duplicatedItem() const;
    UFE_V4(Ufe::SceneItem::Ptr sceneItem() const override { return duplicatedItem(); })

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "Duplicate"; })

private:
    UsdUndoableItem _undoableItem;

    Ufe::Path               _ufeDstPath;
    Ufe::Path               _ufeSrcPath;
    PXR_NS::SdfPath         _usdDstPath;
    PXR_NS::UsdStageWeakPtr _dstStage;
    PXR_NS::UsdStageWeakPtr _srcStage;
}; // UsdUndoDuplicateCommand

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDUNDODUPLICATECOMMAND_H
