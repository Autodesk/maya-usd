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

#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <pxr/usd/sdf/path.h>

#include <ufe/path.h>
#include <ufe/undoableCommand.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoDuplicateCommand
//!
//! \details The USD duplicate command copies all opinions related the the USD prim
//!          that are in the local layer stack of where the prim is first defined into
//!          a single target layer, flattened.
//!
//!          This means that over opinions in the session layer and any layers in the
//!          same local layer stack anchored at the root layer are duplicated.
//!
//!          It also means that opinion found in references and payloads are *not*
//!          copied, but the references and payloads arcs are, so their opinions
//!          are still taken into account.
#ifdef UFE_V4_FEATURES_AVAILABLE
class MAYAUSD_CORE_PUBLIC UsdUndoDuplicateCommand : public Ufe::SceneItemResultUndoableCommand
#else
class MAYAUSD_CORE_PUBLIC UsdUndoDuplicateCommand : public Ufe::UndoableCommand
#endif
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
    UFE_V4(Ufe::SceneItem::Ptr sceneItem() const override { return duplicatedItem(); })

    void execute() override;
    void undo() override;
    void redo() override;

private:
    UsdUndoableItem _undoableItem;
    Ufe::Path       _ufeSrcPath;
    PXR_NS::SdfPath _usdDstPath;

    PXR_NS::SdfLayerHandle _srcLayer;
    PXR_NS::SdfLayerHandle _dstLayer;

}; // UsdUndoDuplicateCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
