//
// Copyright 2020 Autodesk
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

#include "../base/api.h"

#include "UsdSceneItem.h"

#include <ufe/undoableCommand.h>

#include <pxr/usd/usd/prim.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoInsertChildCommand
class MAYAUSD_CORE_PUBLIC UsdUndoInsertChildCommand : public Ufe::UndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoInsertChildCommand> Ptr;

    UsdUndoInsertChildCommand(
        const UsdSceneItem::Ptr& parent,
        const UsdSceneItem::Ptr& child,
        const UsdSceneItem::Ptr& pos
    );
    ~UsdUndoInsertChildCommand() override;

    UsdUndoInsertChildCommand(const UsdUndoInsertChildCommand&) = delete;
    UsdUndoInsertChildCommand& operator=(const UsdUndoInsertChildCommand&) = delete;
    UsdUndoInsertChildCommand(UsdUndoInsertChildCommand&&) = delete;
    UsdUndoInsertChildCommand& operator=(UsdUndoInsertChildCommand&&) = delete;

    //! Create a UsdUndoInsertChildCommand.
    static UsdUndoInsertChildCommand::Ptr create(
        const UsdSceneItem::Ptr& parent,
        const UsdSceneItem::Ptr& child,
        const UsdSceneItem::Ptr& pos
    );

private:
    void undo() override;
    void redo() override;

    bool insertChildRedo();
    bool insertChildUndo();

    UsdStageWeakPtr fStage;
    SdfLayerHandle fLayer;
    UsdSceneItem::Ptr fUfeSrcItem;
    SdfPath fUsdSrcPath;
    UsdSceneItem::Ptr fUfeDstItem;
    Ufe::Path fUfeDstPath;
    SdfPath fUsdDstPath;

}; // UsdUndoInsertChildCommand

} // namespace ufe
} // namespace MayaUsd
