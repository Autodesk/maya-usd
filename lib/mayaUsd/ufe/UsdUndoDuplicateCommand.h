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
#include <pxr/usd/usd/prim.h>

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

    UsdUndoDuplicateCommand(const UsdPrim& srcPrim, const Ufe::Path& ufeSrcPath);
    ~UsdUndoDuplicateCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoDuplicateCommand(const UsdUndoDuplicateCommand&) = delete;
    UsdUndoDuplicateCommand& operator=(const UsdUndoDuplicateCommand&) = delete;
    UsdUndoDuplicateCommand(UsdUndoDuplicateCommand&&) = delete;
    UsdUndoDuplicateCommand& operator=(UsdUndoDuplicateCommand&&) = delete;

    //! Create a UsdUndoDuplicateCommand from a USD prim and UFE path.
    static UsdUndoDuplicateCommand::Ptr create(const UsdPrim& srcPrim, const Ufe::Path& ufeSrcPath);

    const SdfPath& usdDstPath() const;

    //! Return the USD destination path and layer.
    static void primInfo(const UsdPrim& srcPrim, SdfPath& usdDstPath, SdfLayerHandle& srcLayer);

    //! Duplicate the prim hierarchy at usdSrcPath.
    //! \return True for success.
    static bool
    duplicate(const SdfLayerHandle& layer, const SdfPath& usdSrcPath, const SdfPath& usdDstPath);

    //! Return the USD destination path and layer.
    static void primInfo();

    // UsdUndoDuplicateCommand overrides
    void undo() override;
    void redo() override;

private:
    UsdPrim         fSrcPrim;
    UsdStageWeakPtr fStage;
    SdfLayerHandle  fLayer;
    Ufe::Path       fUfeSrcPath;
    SdfPath         fUsdDstPath;

}; // UsdUndoDuplicateCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
