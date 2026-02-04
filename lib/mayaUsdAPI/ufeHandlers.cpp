//
// Copyright 2024 Autodesk
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

#include "ufeHandlers.h"

#include <mayaUsd/ufe/MayaUsdSceneItemOpsHandler.h>

#include <usdUfe/ufe/UsdHierarchyHandler.h>

#ifdef UFE_V4_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdConnectionHandler.h>
#include <mayaUsd/ufe/UsdShaderNodeDefHandler.h>
#include <mayaUsd/ufe/UsdUINodeGraphNodeHandler.h>
#endif

namespace MAYAUSDAPI_NS_DEF {

#ifdef UFE_V4_FEATURES_AVAILABLE
Ufe::UINodeGraphNodeHandler::Ptr createUsdUINodeGraphNodeHandler()
{
    return MayaUsd::ufe::UsdUINodeGraphNodeHandler::create();
}

Ufe::ConnectionHandler::Ptr createUsdConnectionHandler()
{
    return MayaUsd::ufe::UsdConnectionHandler::create();
}

Ufe::NodeDefHandler::Ptr createUsdShaderNodeDefHandler()
{
    return MayaUsd::ufe::UsdShaderNodeDefHandler::create();
}
#endif

Ufe::SceneItemOpsHandler::Ptr createUsdSceneItemOpsHandler()
{
    return MayaUsd::ufe::MayaUsdSceneItemOpsHandler::create();
}

Ufe::HierarchyHandler::Ptr createUsdHierarchyHandler()
{
    return UsdUfe::UsdHierarchyHandler::create();
}

} // namespace MAYAUSDAPI_NS_DEF
