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
#ifndef MAYAUSDAPI_UFEHANDLERS_H
#define MAYAUSDAPI_UFEHANDLERS_H

#include <mayaUsdAPI/api.h>

#include <ufe/hierarchyHandler.h>
#include <ufe/nodeDefHandler.h>
#include <ufe/sceneItemOpsHandler.h>
#include <ufe/uiNodeGraphNodeHandler.h>

#ifdef UFE_V4_FEATURES_AVAILABLE
#include <ufe/connectionHandler.h>
#endif

namespace MAYAUSDAPI_NS_DEF {

//@{
//! Creates the handler and returns the base Ufe handler interface.
MAYAUSD_API_PUBLIC
Ufe::UINodeGraphNodeHandler::Ptr createUsdUINodeGraphNodeHandler();

#ifdef UFE_V4_FEATURES_AVAILABLE
MAYAUSD_API_PUBLIC
Ufe::ConnectionHandler::Ptr createUsdConnectionHandler();
#endif

MAYAUSD_API_PUBLIC
Ufe::NodeDefHandler::Ptr createUsdShaderNodeDefHandler();

MAYAUSD_API_PUBLIC
Ufe::SceneItemOpsHandler::Ptr createUsdSceneItemOpsHandler();

MAYAUSD_API_PUBLIC
Ufe::HierarchyHandler::Ptr createUsdHierarchyHandler();

//@}

} // namespace MAYAUSDAPI_NS_DEF

#endif // MAYAUSDAPI_UFEHANDLERS_H
