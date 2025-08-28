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
#ifndef MAYAUSD_USDSCENEITEMOPSHANDLER_H
#define MAYAUSD_USDSCENEITEMOPSHANDLER_H

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UsdSceneItemOpsHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create a MayaUsdSceneItemOps interface object.
class MAYAUSD_CORE_PUBLIC MayaUsdSceneItemOpsHandler : public UsdUfe::UsdSceneItemOpsHandler
{
public:
    typedef std::shared_ptr<UsdSceneItemOpsHandler> Ptr;

    MayaUsdSceneItemOpsHandler() = default;

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(MayaUsdSceneItemOpsHandler);

    //! Create a MayaUsdSceneItemOpsHandler.
    static MayaUsdSceneItemOpsHandler::Ptr create();

    // Ufe::SceneItemOpsHandler overrides
    Ufe::SceneItemOps::Ptr sceneItemOps(const Ufe::SceneItem::Ptr& item) const override;
}; // MayaUsdSceneItemOpsHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_USDSCENEITEMOPSHANDLER_H
