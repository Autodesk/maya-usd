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
#ifndef PXRUSDMAYA_QUERY_H
#define PXRUSDMAYA_QUERY_H

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

struct UsdMayaQuery
{
    /*! \brief converts a dagPath of a usdStageShapeNode into a usdprim
     */
    MAYAUSD_CORE_PUBLIC
    static UsdPrim GetPrim(const std::string& shapeName);
    MAYAUSD_CORE_PUBLIC
    static void ReloadStage(const std::string& shapeName);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
