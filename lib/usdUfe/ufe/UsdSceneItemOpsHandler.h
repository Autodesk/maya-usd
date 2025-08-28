//
// Copyright 2025 Autodesk
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
#ifndef USD_USDSCENEITEMOPSHANDLER_H
#define USD_USDSCENEITEMOPSHANDLER_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdSceneItemOps.h>

#include <ufe/sceneItemOpsHandler.h>

namespace USDUFE_NS_DEF {

//! \brief Interface to create a UsdSceneItemOps interface object.
class USDUFE_PUBLIC UsdSceneItemOpsHandler : public Ufe::SceneItemOpsHandler
{
public:
    typedef std::shared_ptr<UsdSceneItemOpsHandler> Ptr;

    UsdSceneItemOpsHandler() = default;

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdSceneItemOpsHandler);

    //! Create a UsdSceneItemOpsHandler.
    static UsdSceneItemOpsHandler::Ptr create();

    // Ufe::SceneItemOpsHandler overrides
    Ufe::SceneItemOps::Ptr sceneItemOps(const Ufe::SceneItem::Ptr& item) const override;
}; // UsdSceneItemOpsHandler

} // namespace USDUFE_NS_DEF

#endif // USD_USDSCENEITEMOPSHANDLER_H
