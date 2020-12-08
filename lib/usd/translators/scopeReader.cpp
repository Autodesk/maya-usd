//
// Copyright 2018 Pixar
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
#include <mayaUsd/fileio/primReaderRegistry.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdShade/connectableAPI.h>

#include <maya/MObject.h>

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_DEFINE_READER(UsdGeomScope, args, context)
{
    const UsdPrim& usdPrim = args.GetUsdPrim();

    // If this scope contains only UsdShade nodes, just skip.
    bool hasShadingData = false;
    bool hasNonShadingData = false;
    for (const auto& child : usdPrim.GetChildren()) {
        if (UsdShadeConnectableAPI(child)) {
            hasShadingData = true;
        } else {
            hasNonShadingData = true;
            break;
        }
    }
    if (hasShadingData && !hasNonShadingData) {
        return false;
    }

    MObject parentNode = context->GetMayaNode(usdPrim.GetPath().GetParentPath(), true);

    MStatus status;
    MObject mayaNode;
    return UsdMayaTranslatorUtil::CreateDummyTransformNode(
        usdPrim,
        parentNode,
        /*importTypeName*/ true,
        args,
        context,
        &status,
        &mayaNode);
}

PXR_NAMESPACE_CLOSE_SCOPE
