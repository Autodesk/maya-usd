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
#include <mayaUsd/ufe/UsdSceneItem.h>

#include <pxr/usd/sdf/path.h>

#include <ufe/path.h>
#include <ufe/undoableCommand.h>

PXR_NAMESPACE_USING_DIRECTIVE

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

    // UsdUndoDuplicateCommand overrides
    void undo() override;
    void redo() override;

private:
    bool duplicateUndo();
    bool duplicateRedo();

    Ufe::Path _ufeSrcPath;
    SdfPath   _usdDstPath;
}; // UsdUndoDuplicateCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
