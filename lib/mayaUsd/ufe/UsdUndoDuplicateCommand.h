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
#ifndef MAYAUSD_USDUNDODUPLICATECOMMAND_H
#define MAYAUSD_USDUNDODUPLICATECOMMAND_H

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

    UsdUndoDuplicateCommand(const UsdUfe::UsdSceneItem::Ptr& srcItem);

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoDuplicateCommand);

    //! Create a UsdUndoDuplicateCommand from a USD prim and UFE path.
    static UsdUndoDuplicateCommand::Ptr create(const UsdUfe::UsdSceneItem::Ptr& srcItem);

    UsdUfe::UsdSceneItem::Ptr duplicatedItem() const;
    UFE_V4(Ufe::SceneItem::Ptr sceneItem() const override { return duplicatedItem(); })

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "Duplicate"; })

private:
    UsdUfe::UsdUndoableItem _undoableItem;
    Ufe::Path               _ufeSrcPath;
    PXR_NS::SdfPath         _usdDstPath;

    PXR_NS::SdfLayerHandle _srcLayer;
    PXR_NS::SdfLayerHandle _dstLayer;

}; // UsdUndoDuplicateCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_USDUNDODUPLICATECOMMAND_H
