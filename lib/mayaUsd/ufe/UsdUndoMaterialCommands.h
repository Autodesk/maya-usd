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
#if (UFE_PREVIEW_VERSION_NUM >= 4010)
#include <mayaUsd/ufe/UsdUndoAddNewPrimCommand.h>
#include <mayaUsd/ufe/UsdUndoCreateFromNodeDefCommand.h>
#endif

#include <pxr/usd/usd/prim.h>

#include <ufe/pathComponent.h>
#include <ufe/selection.h>
#include <ufe/undoableCommand.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief BindMaterialUndoableCommand
class MAYAUSD_CORE_PUBLIC BindMaterialUndoableCommand : public Ufe::UndoableCommand
{
public:
    static const std::string commandName;

    static PXR_NS::UsdPrim CompatiblePrim(const Ufe::SceneItem::Ptr& item);

    BindMaterialUndoableCommand(const PXR_NS::UsdPrim& prim, const PXR_NS::SdfPath& materialPath);
    ~BindMaterialUndoableCommand() override;

    // Delete the copy/move constructors assignment operators.
    BindMaterialUndoableCommand(const BindMaterialUndoableCommand&) = delete;
    BindMaterialUndoableCommand& operator=(const BindMaterialUndoableCommand&) = delete;
    BindMaterialUndoableCommand(BindMaterialUndoableCommand&&) = delete;
    BindMaterialUndoableCommand& operator=(BindMaterialUndoableCommand&&) = delete;

    void undo() override;
    void redo() override;

private:
    PXR_NS::UsdStageWeakPtr _stage;
    PXR_NS::SdfPath         _primPath;
    PXR_NS::SdfPath         _materialPath;
    PXR_NS::SdfPath         _previousMaterialPath;
    bool                    _appliedBindingAPI = false;
};

//! \brief UnbindMaterialUndoableCommand
class MAYAUSD_CORE_PUBLIC UnbindMaterialUndoableCommand : public Ufe::UndoableCommand
{
public:
    static const std::string commandName;

    UnbindMaterialUndoableCommand(const PXR_NS::UsdPrim& prim);
    ~UnbindMaterialUndoableCommand() override;

    // Delete the copy/move constructors assignment operators.
    UnbindMaterialUndoableCommand(const UnbindMaterialUndoableCommand&) = delete;
    UnbindMaterialUndoableCommand& operator=(const UnbindMaterialUndoableCommand&) = delete;
    UnbindMaterialUndoableCommand(UnbindMaterialUndoableCommand&&) = delete;
    UnbindMaterialUndoableCommand& operator=(UnbindMaterialUndoableCommand&&) = delete;

    void undo() override;
    void redo() override;

private:
    PXR_NS::UsdStageWeakPtr _stage;
    PXR_NS::SdfPath         _primPath;
    PXR_NS::SdfPath         _materialPath;
};

#if (UFE_PREVIEW_VERSION_NUM >= 4010)
//! \brief UsdUndoAssignNewMaterialCommand
class MAYAUSD_CORE_PUBLIC UsdUndoAssignNewMaterialCommand : public Ufe::InsertChildCommand
{
public:
    typedef std::shared_ptr<UsdUndoAssignNewMaterialCommand> Ptr;

    UsdUndoAssignNewMaterialCommand(
        const UsdSceneItem::Ptr& parentItem,
        const std::string&       sdrShaderIdentifier);
    ~UsdUndoAssignNewMaterialCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoAssignNewMaterialCommand(const UsdUndoAssignNewMaterialCommand&) = delete;
    UsdUndoAssignNewMaterialCommand& operator=(const UsdUndoAssignNewMaterialCommand&) = delete;
    UsdUndoAssignNewMaterialCommand(UsdUndoAssignNewMaterialCommand&&) = delete;
    UsdUndoAssignNewMaterialCommand& operator=(UsdUndoAssignNewMaterialCommand&&) = delete;

    //! Create a UsdUndoAssignNewMaterialCommand that creates a new material based on \p
    //! sdrShaderIdentifier and assigns it to \p parentItem
    static UsdUndoAssignNewMaterialCommand::Ptr
    create(const UsdSceneItem::Ptr& parentItem, const std::string& sdrShaderIdentifier);

    Ufe::SceneItem::Ptr insertedChild() const override;

    void execute() override;
    void undo() override;
    void redo() override;

private:
    bool connectShaderToMaterial(Ufe::SceneItem::Ptr shaderItem, PXR_NS::UsdPrim materialPrim);

    const Ufe::Path   _parentPath;
    const std::string _nodeId;

    std::shared_ptr<UsdUndoAddNewPrimCommand>        _createScopeCmd;
    UndoableCommand::Ptr                             _renameScopeCmd;
    std::shared_ptr<UsdUndoAddNewPrimCommand>        _createMaterialCmd;
    std::shared_ptr<UsdUndoCreateFromNodeDefCommand> _createShaderCmd;
    std::shared_ptr<BindMaterialUndoableCommand>     _bindCmd;

}; // UsdUndoAssignNewMaterialCommand
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
