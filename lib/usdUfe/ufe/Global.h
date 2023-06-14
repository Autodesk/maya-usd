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

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/Utils.h>

#include <ufe/hierarchyHandler.h>
#include <ufe/object3dHandler.h>
#include <ufe/rtid.h>

#include <string>

namespace USDUFE_NS_DEF {

/*! Ufe runtime DCC specific functions.

    You must provide each of the mandatory functions in order for the plugin
    to function correctly for your runtime.
*/
struct USDUFE_PUBLIC DCCFunctions
{
    // Mandatory: function whichs must be supplied.
    UfePathToPrimFn ufePathToPrimFn = nullptr;
    TimeAccessorFn  timeAccessorFn = nullptr;

    // Optional: default values will be used if no function is supplied.
    IsAttributeLockedFn isAttributeLockedFn = nullptr;
};

/*! Ufe runtime handlers used to initialize the plugin.

    All the handlers from this struct will be initialized with the default
    versions from UsdUfe. In order to register your own handler simply
    provide your own in any of the handler pointers. Any non-nullptr handlers
    will be registered for you.
*/
struct USDUFE_PUBLIC Handlers
{
    // Ufe v1 handlers
    Ufe::HierarchyHandler::Ptr hierarchyHandler;
    //     Ufe::Transform3dHandler::Ptr  transform3dHandler;
    //     Ufe::SceneItemOpsHandler::Ptr sceneItemOpsHandler;

    // Ufe v2 handlers
    //     Ufe::AttributesHandler::Ptr   attributesHandler;
    Ufe::Object3dHandler::Ptr object3dHandler;
    //     Ufe::ContextOpsHandler::Ptr   contextOpsHandler;
    //     Ufe::UIInfoHandler::Ptr       uiInfoHandler;
    //     Ufe::CameraHandler::Ptr       cameraHandler;

#ifdef UFE_V3_FEATURES_AVAILABLE
//     Ufe::PathMappingHandler::Ptr  pathMappingHandler;
//     Ufe::LightHandler::Ptr        lightHandler;
//     Ufe::SceneSegmentHandler::Ptr sceneSegmentHandler;
#endif
#ifdef UFE_V4_FEATURES_AVAILABLE
//     Ufe::MaterialHandler::Ptr        materialHandler;
//     Ufe::NodeDefHandler::Ptr         nodeDefHandler;
//     Ufe::ConnectionHandler::Ptr      connectionHandler;
//     Ufe::UINodeGraphNodeHandler::Ptr uiNodeGraphNodeHandler;
//     Ufe::BatchOpsHandler::Ptr        batchOpsHandler;
#endif
};

//! Only intended to be called by the plugin initialization, to
//! initialize the handlers and stage model.
//! \return Ufe runtime ID for USD or 0 in case of error.
USDUFE_PUBLIC
Ufe::Rtid initialize(const DCCFunctions&, const Handlers&);

//! Only intended to be called by the plugin finalization, to
//! finalize the handlers stage model.
USDUFE_PUBLIC
bool finalize(bool exiting = false);

//! Return the name of the run-time used for USD.
USDUFE_PUBLIC
std::string getUsdRunTimeName();

//! Return the run-time ID allocated to USD.
USDUFE_PUBLIC
Ufe::Rtid getUsdRunTimeId();

} // namespace USDUFE_NS_DEF
