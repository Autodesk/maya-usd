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
#ifndef MAYAUSDAPI_GLOBAL_H
#define MAYAUSDAPI_GLOBAL_H

#include <mayaUsdAPI/api.h>

#include <pxr/base/tf/notice.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>

#include <ufe/path.h>

namespace MAYAUSDAPI_NS_DEF {

// DCC specific accessor functions.
typedef PXR_NS::UsdStageWeakPtr (*StageAccessorFn)(const Ufe::Path&);
typedef Ufe::Path (*StagePathAccessorFn)(PXR_NS::UsdStageWeakPtr);
typedef PXR_NS::UsdPrim (*UfePathToPrimFn)(const Ufe::Path&);
typedef PXR_NS::UsdTimeCode (*TimeAccessorFn)(const Ufe::Path&);

/*! Ufe runtime DCC mandatory functions.

    You must provide each of the mandatory functions in order for the plugin
    to function correctly for your runtime.
*/
struct MAYAUSD_API_PUBLIC DCCMandatoryFunctions
{
    StageAccessorFn     stageAccessorFn = nullptr;
    StagePathAccessorFn stagePathAccessorFn = nullptr;
    UfePathToPrimFn     ufePathToPrimFn = nullptr;
    TimeAccessorFn      timeAccessorFn = nullptr;
};

//! Only intended to be called by the plugin initialization, to
//! initialize the handlers and stage model.
//! \param [in] dccMandatoryFuncs Struct containing DCC mandatory functions for plugin to work.
//!
//! \return Ufe runtime ID for USD or 0 in case of error.
MAYAUSD_API_PUBLIC
Ufe::Rtid initialize(const DCCMandatoryFunctions& dccMandatoryFuncs);

//! Only intended to be called by the plugin finalization, to
//! finalize the handlers stage model.
MAYAUSD_API_PUBLIC
bool finalize(bool exiting = false);

//! Connect a stage to USD notifications.
MAYAUSD_API_PUBLIC
PXR_NS::TfNotice::Key registerStage(const PXR_NS::UsdStageRefPtr&);

//! Remove the stage from the USD notifications.
MAYAUSD_API_PUBLIC
bool revokeStage(PXR_NS::TfNotice::Key& key);

} // namespace MAYAUSDAPI_NS_DEF

#endif // MAYAUSDAPI_GLOBAL_H
