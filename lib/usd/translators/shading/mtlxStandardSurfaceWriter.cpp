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

#include "mtlxTranslationTableWriter.h"
#include "shadingTokens.h"

#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
MtlxUsd_TranslationTableWriter::TranslationTable mayaToMaterialX {
    { TrMayaTokens->base, TrMtlxTokens->base },
    { TrMayaTokens->baseColor, TrMtlxTokens->base_color },
    { TrMayaTokens->diffuseRoughness, TrMtlxTokens->diffuse_roughness },
    { TrMayaTokens->metalness, TrMtlxTokens->metalness },
    { TrMayaTokens->specular, TrMtlxTokens->specular },
    { TrMayaTokens->specularColor, TrMtlxTokens->specular_color },
    { TrMayaTokens->specularRoughness, TrMtlxTokens->specular_roughness },
    { TrMayaTokens->specularIOR, TrMtlxTokens->specular_IOR },
    { TrMayaTokens->specularAnisotropy, TrMtlxTokens->specular_anisotropy },
    { TrMayaTokens->specularRotation, TrMtlxTokens->specular_rotation },
    { TrMayaTokens->transmission, TrMtlxTokens->transmission },
    { TrMayaTokens->transmissionColor, TrMtlxTokens->transmission_color },
    { TrMayaTokens->transmissionDepth, TrMtlxTokens->transmission_depth },
    { TrMayaTokens->transmissionScatter, TrMtlxTokens->transmission_scatter },
    { TrMayaTokens->transmissionScatterAnisotropy, TrMtlxTokens->transmission_scatter_anisotropy },
    { TrMayaTokens->transmissionDispersion, TrMtlxTokens->transmission_dispersion },
    { TrMayaTokens->transmissionExtraRoughness, TrMtlxTokens->transmission_extra_roughness },
    { TrMayaTokens->subsurface, TrMtlxTokens->subsurface },
    { TrMayaTokens->subsurfaceColor, TrMtlxTokens->subsurface_color },
    { TrMayaTokens->subsurfaceRadius, TrMtlxTokens->subsurface_radius },
    { TrMayaTokens->subsurfaceScale, TrMtlxTokens->subsurface_scale },
    { TrMayaTokens->subsurfaceAnisotropy, TrMtlxTokens->subsurface_anisotropy },
    { TrMayaTokens->sheen, TrMtlxTokens->sheen },
    { TrMayaTokens->sheenColor, TrMtlxTokens->sheen_color },
    { TrMayaTokens->sheenRoughness, TrMtlxTokens->sheen_roughness },
    { TrMayaTokens->coat, TrMtlxTokens->coat },
    { TrMayaTokens->coatColor, TrMtlxTokens->coat_color },
    { TrMayaTokens->coatRoughness, TrMtlxTokens->coat_roughness },
    { TrMayaTokens->coatAnisotropy, TrMtlxTokens->coat_anisotropy },
    { TrMayaTokens->coatRotation, TrMtlxTokens->coat_rotation },
    { TrMayaTokens->coatIOR, TrMtlxTokens->coat_IOR },
    { TrMayaTokens->coatNormal, TrMtlxTokens->coat_normal },
    { TrMayaTokens->coatAffectColor, TrMtlxTokens->coat_affect_color },
    { TrMayaTokens->coatAffectRoughness, TrMtlxTokens->coat_affect_roughness },
    { TrMayaTokens->thinFilmThickness, TrMtlxTokens->thin_film_thickness },
    { TrMayaTokens->thinFilmIOR, TrMtlxTokens->thin_film_IOR },
    { TrMayaTokens->emission, TrMtlxTokens->emission },
    { TrMayaTokens->emissionColor, TrMtlxTokens->emission_color },
    { TrMayaTokens->opacity, TrMtlxTokens->opacity },
    { TrMayaTokens->thinWalled, TrMtlxTokens->thin_walled },
    { TrMayaTokens->normalCamera, TrMtlxTokens->normal },
    { TrMayaTokens->tangentUCamera, TrMtlxTokens->tangent }
};

MtlxUsd_TranslationTableWriter::AlwaysAuthored mayaAlwaysAuthored {
    TrMtlxTokens->base,
    TrMtlxTokens->base_color,
    TrMtlxTokens->specular,
    TrMtlxTokens->specular_roughness
};

} // namespace
// This is basically UsdMayaSymmetricShaderWriter with a table for attribute renaming:
class MaterialXTranslators_StandardSurfaceWriter : public MtlxUsd_TranslationTableWriter
{
public:
    MaterialXTranslators_StandardSurfaceWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx)
        : MtlxUsd_TranslationTableWriter(
            depNodeFn,
            usdPath,
            jobCtx,
            TrMtlxTokens->ND_standard_surface_surfaceshader,
            mayaToMaterialX,
            mayaAlwaysAuthored)
    {
    }
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(standardSurface, MaterialXTranslators_StandardSurfaceWriter);

PXR_NAMESPACE_CLOSE_SCOPE
