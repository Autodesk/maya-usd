//
// Copyright 2021 Autodesk
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
#include "shadingTokens.h"

#include <mayaUsd/fileio/shaderReaderRegistry.h>
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

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class MtlxUsd_StandardSurfaceWriter : public UsdMayaShaderReader
{
public:
    MtlxUsd_StandardSurfaceWriter(const UsdMayaPrimReaderArgs&);

    bool Read(UsdMayaPrimReaderContext& context) override;

    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override;
};

PXRUSDMAYA_REGISTER_SHADER_READER(ND_standard_surface_surfaceshader, MtlxUsd_StandardSurfaceWriter);

MtlxUsd_StandardSurfaceWriter::MtlxUsd_StandardSurfaceWriter(const UsdMayaPrimReaderArgs& readArgs)
    : UsdMayaShaderReader(readArgs)
{
}

namespace {
// Mapping from Maya to MaterialX:
using TokenHashMap = std::unordered_map<TfToken, TfToken, TfToken::HashFunctor>;
TokenHashMap mtlxToMaya {
    { TrMtlxTokens->base, TrMayaTokens->base },
    { TrMtlxTokens->base_color, TrMayaTokens->baseColor },
    { TrMtlxTokens->diffuse_roughness, TrMayaTokens->diffuseRoughness },
    { TrMtlxTokens->metalness, TrMayaTokens->metalness },
    { TrMtlxTokens->specular, TrMayaTokens->specular },
    { TrMtlxTokens->specular_color, TrMayaTokens->specularColor },
    { TrMtlxTokens->specular_roughness, TrMayaTokens->specularRoughness },
    { TrMtlxTokens->specular_IOR, TrMayaTokens->specularIOR },
    { TrMtlxTokens->specular_anisotropy, TrMayaTokens->specularAnisotropy },
    { TrMtlxTokens->specular_rotation, TrMayaTokens->specularRotation },
    { TrMtlxTokens->transmission, TrMayaTokens->transmission },
    { TrMtlxTokens->transmission_color, TrMayaTokens->transmissionColor },
    { TrMtlxTokens->transmission_depth, TrMayaTokens->transmissionDepth },
    { TrMtlxTokens->transmission_scatter, TrMayaTokens->transmissionScatter },
    { TrMtlxTokens->transmission_scatter_anisotropy, TrMayaTokens->transmissionScatterAnisotropy },
    { TrMtlxTokens->transmission_dispersion, TrMayaTokens->transmissionDispersion },
    { TrMtlxTokens->transmission_extra_roughness, TrMayaTokens->transmissionExtraRoughness },
    { TrMtlxTokens->subsurface, TrMayaTokens->subsurface },
    { TrMtlxTokens->subsurface_color, TrMayaTokens->subsurfaceColor },
    { TrMtlxTokens->subsurface_radius, TrMayaTokens->subsurfaceRadius },
    { TrMtlxTokens->subsurface_scale, TrMayaTokens->subsurfaceScale },
    { TrMtlxTokens->subsurface_anisotropy, TrMayaTokens->subsurfaceAnisotropy },
    { TrMtlxTokens->sheen, TrMayaTokens->sheen },
    { TrMtlxTokens->sheen_color, TrMayaTokens->sheenColor },
    { TrMtlxTokens->sheen_roughness, TrMayaTokens->sheenRoughness },
    { TrMtlxTokens->coat, TrMayaTokens->coat },
    { TrMtlxTokens->coat_color, TrMayaTokens->coatColor },
    { TrMtlxTokens->coat_roughness, TrMayaTokens->coatRoughness },
    { TrMtlxTokens->coat_anisotropy, TrMayaTokens->coatAnisotropy },
    { TrMtlxTokens->coat_rotation, TrMayaTokens->coatRotation },
    { TrMtlxTokens->coat_IOR, TrMayaTokens->coatIOR },
    { TrMtlxTokens->coat_normal, TrMayaTokens->coatNormal },
    { TrMtlxTokens->coat_affect_color, TrMayaTokens->coatAffectColor },
    { TrMtlxTokens->coat_affect_roughness, TrMayaTokens->coatAffectRoughness },
    { TrMtlxTokens->thin_film_thickness, TrMayaTokens->thinFilmThickness },
    { TrMtlxTokens->thin_film_IOR, TrMayaTokens->thinFilmIOR },
    { TrMtlxTokens->emission, TrMayaTokens->emission },
    { TrMtlxTokens->emission_color, TrMayaTokens->emissionColor },
    { TrMtlxTokens->opacity, TrMayaTokens->opacity },
    { TrMtlxTokens->thin_walled, TrMayaTokens->thinWalled },
    { TrMtlxTokens->normal, TrMayaTokens->normalCamera },
    { TrMtlxTokens->tangent, TrMayaTokens->tangentUCamera }
};

// Read a value that was set on a UsdShadeMaterial instead of the UsdShadeShader. This is something
// we see with materials imported by UsdMtlx:
bool _ReadFromMaterial(const UsdShadeInput& input, VtValue& inputVal)
{
    UsdShadeConnectableAPI source;
    TfToken                sourceInputName;
    UsdShadeAttributeType  sourceType;
    if (!UsdShadeConnectableAPI::GetConnectedSource(
            input, &source, &sourceInputName, &sourceType)) {
        return false;
    }

    UsdShadeMaterial sourceMaterial(source.GetPrim());
    if (!sourceMaterial) {
        return false;
    }

    const UsdShadeInput& materialInput = sourceMaterial.GetInput(sourceInputName);
    if (!materialInput) {
        return false;
    }

    return materialInput.Get(&inputVal);
}

} // namespace
/* virtual */
bool MtlxUsd_StandardSurfaceWriter::Read(UsdMayaPrimReaderContext& context)
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
              TrMayaTokens->standardSurface.GetText(),
              UsdMayaShadingNodeType::Shader,
              &status,
              &mayaObject)
          && depFn.setObject(mayaObject))) {
        TF_RUNTIME_ERROR(
            "Could not create node of type %s for shader '%s'.\n",
            TrMayaTokens->standardSurface.GetText(),
            prim.GetPath().GetText());
        return false;
    }

    context.RegisterNewMayaNode(prim.GetPath().GetString(), mayaObject);

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
        if (!input.GetAttr().Get(&inputVal) && !_ReadFromMaterial(input, inputVal)) {
            continue;
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
TfToken MtlxUsd_StandardSurfaceWriter::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               baseName;
    UsdShadeAttributeType attrType;
    std::tie(baseName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (attrType == UsdShadeAttributeType::Input) {
        auto it = mtlxToMaya.find(baseName);
        if (it != mtlxToMaya.end()) {
            return it->second;
        }
    } else if (attrType == UsdShadeAttributeType::Output && baseName == UsdShadeTokens->surface) {
        return TrMayaTokens->outColor;
    }
    return TfToken();
}

PXR_NAMESPACE_CLOSE_SCOPE
