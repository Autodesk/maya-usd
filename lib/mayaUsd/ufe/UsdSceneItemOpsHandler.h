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
#include <mayaUsd/ufe/UsdSceneItemOps.h>

#include <ufe/sceneItemOpsHandler.h>

// PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create a UsdSceneItemOps interface object.
class MAYAUSD_CORE_PUBLIC UsdSceneItemOpsHandler : public Ufe::SceneItemOpsHandler
{
public:
    typedef std::shared_ptr<UsdSceneItemOpsHandler> Ptr;

    UsdSceneItemOpsHandler();
    ~UsdSceneItemOpsHandler() override;

    // Delete the copy/move constructors assignment operators.
    UsdSceneItemOpsHandler(const UsdSceneItemOpsHandler&) = delete;
    UsdSceneItemOpsHandler& operator=(const UsdSceneItemOpsHandler&) = delete;
    UsdSceneItemOpsHandler(UsdSceneItemOpsHandler&&) = delete;
    UsdSceneItemOpsHandler& operator=(UsdSceneItemOpsHandler&&) = delete;

    //! Create a UsdSceneItemOpsHandler.
    static UsdSceneItemOpsHandler::Ptr create();

    // Ufe::SceneItemOpsHandler overrides
    Ufe::SceneItemOps::Ptr sceneItemOps(const Ufe::SceneItem::Ptr& item) const override;
}; // UsdSceneItemOpsHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
