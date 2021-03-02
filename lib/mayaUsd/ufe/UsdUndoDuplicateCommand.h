//
// Copyright 2019 Autodesk
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
#pragma once

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UfeVersionCompat.h>
#include <mayaUsd/ufe/UsdSceneItem.h>

#ifdef UFE_V2_FEATURES_AVAILABLE
#include <mayaUsd/undo/UsdUndoableItem.h>
#endif

#include <pxr/usd/sdf/path.h>

#include <ufe/path.h>
#include <ufe/undoableCommand.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoDuplicateCommand
class MAYAUSD_CORE_PUBLIC UsdUndoDuplicateCommand : public Ufe::UndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoDuplicateCommand> Ptr;

    UsdUndoDuplicateCommand(const UsdSceneItem::Ptr& srcItem);
    ~UsdUndoDuplicateCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoDuplicateCommand(const UsdUndoDuplicateCommand&) = delete;
    UsdUndoDuplicateCommand& operator=(const UsdUndoDuplicateCommand&) = delete;
    UsdUndoDuplicateCommand(UsdUndoDuplicateCommand&&) = delete;
    UsdUndoDuplicateCommand& operator=(UsdUndoDuplicateCommand&&) = delete;

    //! Create a UsdUndoDuplicateCommand from a USD prim and UFE path.
    static UsdUndoDuplicateCommand::Ptr create(const UsdSceneItem::Ptr& srcItem);

    UsdSceneItem::Ptr duplicatedItem() const;

    UFE_V2(void execute() override;)
    void undo() override;
    void redo() override;

private:
    UFE_V2(UsdUndoableItem _undoableItem;)

#ifndef UFE_V2_FEATURES_AVAILABLE
    bool duplicateUndo();
    bool duplicateRedo();
#endif

    Ufe::Path _ufeSrcPath;
    PXR_NS::SdfPath _usdDstPath;

}; // UsdUndoDuplicateCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
