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
#ifndef MAYAUSD_USDSCENEITEMOPS_H
#define MAYAUSD_USDSCENEITEMOPS_H

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UsdSceneItemOps.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface for scene item operations.
class MAYAUSD_CORE_PUBLIC MayaUsdSceneItemOps : public UsdUfe::UsdSceneItemOps
{
public:
    typedef std::shared_ptr<MayaUsdSceneItemOps> Ptr;

    MayaUsdSceneItemOps(const UsdUfe::UsdSceneItem::Ptr& item);

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(MayaUsdSceneItemOps);

    //! Create a MayaUsdSceneItemOps.
    static MayaUsdSceneItemOps::Ptr create(const UsdUfe::UsdSceneItem::Ptr& item);

    //@{
    // Ufe::SceneItemOps overrides
    Ufe::Duplicate duplicateItemCmd() override;
#ifdef UFE_V4_FEATURES_AVAILABLE
    Ufe::SceneItemResultUndoableCommand::Ptr duplicateItemCmdNoExecute() override;
    Ufe::SceneItemResultUndoableCommand::Ptr
    renameItemCmdNoExecute(const Ufe::PathComponent& newName) override;
#endif
    Ufe::Rename renameItemCmd(const Ufe::PathComponent& newName) override;
    //@}

}; // MayaUsdSceneItemOps

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_USDSCENEITEMOPS_H
