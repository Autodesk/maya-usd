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
#include "usdPreviewSurfaceReader.h"

#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/readUtil.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

PXR_NAMESPACE_OPEN_SCOPE

REGISTER_SHADING_MODE_IMPORT_MATERIAL_CONVERSION(
    UsdImagingTokens->UsdPreviewSurface,
    UsdShadeTokens->universalRenderContext,
    PxrMayaUsdPreviewSurfaceTokens->niceName,
    PxrMayaUsdPreviewSurfaceTokens->importDescription);

PxrMayaUsdPreviewSurface_Reader::PxrMayaUsdPreviewSurface_Reader(
    const UsdMayaPrimReaderArgs& readArgs,
    const TfToken&               mayaTypeName)
    : UsdMayaShaderReader(readArgs)
    , _mayaTypeName(mayaTypeName)
{
}

/* virtual */
bool PxrMayaUsdPreviewSurface_Reader::Read(UsdMayaPrimReaderContext* context)
{
    const UsdPrim& prim = _GetArgs().GetUsdPrim();
    UsdShadeShader shaderSchema = UsdShadeShader(prim);
    if (!shaderSchema) {
        return false;
    }

    MStatus           status;
    MObject           mayaObject;
    MFnDependencyNode depFn;
    if (!(UsdMayaTranslatorUtil::CreateShaderNode(
              MString(prim.GetName().GetText()),
              _mayaTypeName.GetText(),
              UsdMayaShadingNodeType::Shader,
              &status,
              &mayaObject)
          && depFn.setObject(mayaObject))) {
        // we need to make sure assumes those types are loaded..
        TF_RUNTIME_ERROR(
            "Could not create node of type %s for shader '%s'. "
            "Probably missing a loadPlugin.\n",
            _mayaTypeName.GetText(),
            prim.GetPath().GetText());
        return false;
    }

    context->RegisterNewMayaNode(prim.GetPath().GetString(), mayaObject);

    for (const UsdShadeInput& input : shaderSchema.GetInputs()) {
        TfToken baseName = GetMayaNameForUsdAttrName(input.GetFullName());
        if (baseName.IsEmpty()) {
            continue;
        }
        MPlug mayaAttr = depFn.findPlug(baseName.GetText(), true, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        VtValue inputVal;
        if (!input.GetAttr().Get(&inputVal)) {
            continue;
        }

        // "useSpecularWorkflow" is an int in USD, but a boolean in Maya.
        if (baseName == PxrMayaUsdPreviewSurfaceTokens->UseSpecularWorkflowAttrName
            && inputVal.IsHolding<int>()) {
            inputVal = (inputVal.UncheckedGet<int>() == 0) ? false : true;
        }

        if (UsdMayaReadUtil::SetMayaAttr(
                mayaAttr,
                inputVal,
                /* unlinearizeColors = */ false)) {
            UsdMayaReadUtil::SetMayaAttrKeyableState(mayaAttr, input.GetAttr().GetVariability());
        }
    }

    return true;
}

/* virtual */
TfToken PxrMayaUsdPreviewSurface_Reader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               baseName;
    UsdShadeAttributeType attrType;
    std::tie(baseName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (attrType == UsdShadeAttributeType::Input) {
        if (baseName == PxrMayaUsdPreviewSurfaceTokens->ClearcoatAttrName
            || baseName == PxrMayaUsdPreviewSurfaceTokens->ClearcoatRoughnessAttrName
            || baseName == PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName
            || baseName == PxrMayaUsdPreviewSurfaceTokens->DisplacementAttrName
            || baseName == PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName
            || baseName == PxrMayaUsdPreviewSurfaceTokens->IorAttrName
            || baseName == PxrMayaUsdPreviewSurfaceTokens->MetallicAttrName
            || baseName == PxrMayaUsdPreviewSurfaceTokens->NormalAttrName
            || baseName == PxrMayaUsdPreviewSurfaceTokens->OcclusionAttrName
            || baseName == PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName
            || baseName == PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName
            || baseName == PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName
            || baseName == PxrMayaUsdPreviewSurfaceTokens->UseSpecularWorkflowAttrName) {
            return baseName;
        }
    } else if (attrType == UsdShadeAttributeType::Output && baseName == UsdShadeTokens->surface) {
        return PxrMayaUsdPreviewSurfaceTokens->OutColorAttrName;
    }

    return TfToken();
}

PXR_NAMESPACE_CLOSE_SCOPE
