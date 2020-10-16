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

#include <maya/MStatus.h>

#include <ufe/rtid.h>

#include <mayaUsd/base/api.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

// Only intended to be called by the plugin initialization, to
// initialize the handlers and stage model.
MAYAUSD_CORE_PUBLIC
MStatus initialize();

//! Only intended to be called by the plugin finalization, to
//! finalize the handlers stage model.
MAYAUSD_CORE_PUBLIC
MStatus finalize();

//! Return the run-time ID allocated to USD.
MAYAUSD_CORE_PUBLIC
Ufe::Rtid getUsdRunTimeId();

//! Return the run-time ID allocated to Maya.
MAYAUSD_CORE_PUBLIC
Ufe::Rtid getMayaRunTimeId();

} // namespace ufe
} // namespace MayaUsd
