//
// Copyright 2020 Autodesk
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
#ifndef MAYAUSD_PROXY_STAGE_PROVIDER_H
#define MAYAUSD_PROXY_STAGE_PROVIDER_H

#include "../base/api.h"
#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/timeCode.h"

PXR_NAMESPACE_OPEN_SCOPE

// Interface class
class ProxyStageProvider
{
public:
    MAYAUSD_CORE_PUBLIC
    virtual UsdTimeCode getTime() const = 0;
    MAYAUSD_CORE_PUBLIC
    virtual UsdStageRefPtr getUsdStage() const = 0;

    MAYAUSD_CORE_PUBLIC
    virtual ~ProxyStageProvider() = default;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
