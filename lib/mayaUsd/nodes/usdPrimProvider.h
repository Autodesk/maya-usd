//
// Copyright 2016 Pixar
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
#ifndef PXRUSDMAYA_USD_PRIM_PROVIDER_H
#define PXRUSDMAYA_USD_PRIM_PROVIDER_H

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>

PXR_NAMESPACE_OPEN_SCOPE

// interface class
class UsdMayaUsdPrimProvider
{
public:
    // returns the prim that this node is holding
    virtual UsdPrim usdPrim() const = 0;

    MAYAUSD_CORE_PUBLIC
    virtual ~UsdMayaUsdPrimProvider();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
