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
#include "query.h"

#include <mayaUsd/nodes/usdPrimProvider.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/arch/systemInfo.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/ar/resolverContext.h>
#include <pxr/usd/ar/resolverContextBinder.h>
#include <pxr/usd/usd/prim.h>

#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MObject.h>
#include <maya/MPxNode.h>
#include <maya/MStatus.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

UsdPrim UsdMayaQuery::GetPrim(const std::string& shapeName)
{
    UsdPrim usdPrim;

    MObject shapeObj;
    MStatus status = UsdMayaUtil::GetMObjectByName(shapeName, shapeObj);
    CHECK_MSTATUS_AND_RETURN(status, usdPrim);
    MFnDagNode dagNode(shapeObj, &status);
    CHECK_MSTATUS_AND_RETURN(status, usdPrim);

    if (const UsdMayaUsdPrimProvider* usdPrimProvider
        = dynamic_cast<const UsdMayaUsdPrimProvider*>(dagNode.userNode())) {
        return usdPrimProvider->usdPrim();
    }

    return usdPrim;
}

void UsdMayaQuery::ReloadStage(const std::string& shapeName)
{
    MStatus status;

    if (UsdPrim usdPrim = UsdMayaQuery::GetPrim(shapeName)) {
        if (UsdStagePtr stage = usdPrim.GetStage()) {
            stage->Reload();
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
