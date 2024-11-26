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

#include "mtlxTranslationTableWriter.h"
#include "shadingTokens.h"

#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
MtlxUsd_TranslationTableWriter::TranslationTable mayaToMaterialX {
    { TrMayaOpenPBRTokens->baseWeight, TrMtlxOpenPBRTokens->base_weight },
    { TrMayaOpenPBRTokens->baseColor, TrMtlxOpenPBRTokens->base_color },
    { TrMayaOpenPBRTokens->baseDiffuseRoughness, TrMtlxOpenPBRTokens->base_diffuse_roughness },
    { TrMayaOpenPBRTokens->baseMetalness, TrMtlxOpenPBRTokens->base_metalness },
    { TrMayaOpenPBRTokens->specularWeight, TrMtlxOpenPBRTokens->specular_weight },
    { TrMayaOpenPBRTokens->specularColor, TrMtlxOpenPBRTokens->specular_color },
    { TrMayaOpenPBRTokens->specularRoughness, TrMtlxOpenPBRTokens->specular_roughness },
    { TrMayaOpenPBRTokens->specularIOR, TrMtlxOpenPBRTokens->specular_ior },
    { TrMayaOpenPBRTokens->specularRoughnessAnisotropy,
      TrMtlxOpenPBRTokens->specular_roughness_anisotropy },
    { TrMayaOpenPBRTokens->transmissionWeight, TrMtlxOpenPBRTokens->transmission_weight },
    { TrMayaOpenPBRTokens->transmissionColor, TrMtlxOpenPBRTokens->transmission_color },
    { TrMayaOpenPBRTokens->transmissionDepth, TrMtlxOpenPBRTokens->transmission_depth },
    { TrMayaOpenPBRTokens->transmissionScatter, TrMtlxOpenPBRTokens->transmission_scatter },
    { TrMayaOpenPBRTokens->transmissionScatterAnisotropy,
      TrMtlxOpenPBRTokens->transmission_scatter_anisotropy },
    { TrMayaOpenPBRTokens->transmissionDispersionScale,
      TrMtlxOpenPBRTokens->transmission_dispersion_scale },
    { TrMayaOpenPBRTokens->transmissionDispersionAbbeNumber,
      TrMtlxOpenPBRTokens->transmission_dispersion_abbe_number },
    { TrMayaOpenPBRTokens->subsurfaceWeight, TrMtlxOpenPBRTokens->subsurface_weight },
    { TrMayaOpenPBRTokens->subsurfaceColor, TrMtlxOpenPBRTokens->subsurface_color },
    { TrMayaOpenPBRTokens->subsurfaceRadius, TrMtlxOpenPBRTokens->subsurface_radius },
    { TrMayaOpenPBRTokens->subsurfaceRadiusScale, TrMtlxOpenPBRTokens->subsurface_radius_scale },
    { TrMayaOpenPBRTokens->subsurfaceScatterAnisotropy,
      TrMtlxOpenPBRTokens->subsurface_scatter_anisotropy },
    { TrMayaOpenPBRTokens->fuzzWeight, TrMtlxOpenPBRTokens->fuzz_weight },
    { TrMayaOpenPBRTokens->fuzzColor, TrMtlxOpenPBRTokens->fuzz_color },
    { TrMayaOpenPBRTokens->fuzzRoughness, TrMtlxOpenPBRTokens->fuzz_roughness },
    { TrMayaOpenPBRTokens->coatWeight, TrMtlxOpenPBRTokens->coat_weight },
    { TrMayaOpenPBRTokens->coatColor, TrMtlxOpenPBRTokens->coat_color },
    { TrMayaOpenPBRTokens->coatRoughness, TrMtlxOpenPBRTokens->coat_roughness },
    { TrMayaOpenPBRTokens->coatRoughnessAnisotropy,
      TrMtlxOpenPBRTokens->coat_roughness_anisotropy },
    { TrMayaOpenPBRTokens->coatIOR, TrMtlxOpenPBRTokens->coat_ior },
    { TrMayaOpenPBRTokens->coatDarkening, TrMtlxOpenPBRTokens->coat_darkening },
    { TrMayaOpenPBRTokens->thinFilmWeight, TrMtlxOpenPBRTokens->thin_film_weight },
    { TrMayaOpenPBRTokens->thinFilmThickness, TrMtlxOpenPBRTokens->thin_film_thickness },
    { TrMayaOpenPBRTokens->thinFilmIOR, TrMtlxOpenPBRTokens->thin_film_ior },
    { TrMayaOpenPBRTokens->emissionLuminance, TrMtlxOpenPBRTokens->emission_luminance },
    { TrMayaOpenPBRTokens->emissionColor, TrMtlxOpenPBRTokens->emission_color },
    { TrMayaOpenPBRTokens->geometryOpacity, TrMtlxOpenPBRTokens->geometry_opacity },
    { TrMayaOpenPBRTokens->geometryThinWalled, TrMtlxOpenPBRTokens->geometry_thin_walled },
    { TrMayaOpenPBRTokens->normalCamera, TrMtlxOpenPBRTokens->geometry_normal },
    { TrMayaOpenPBRTokens->geometryCoatNormal, TrMtlxOpenPBRTokens->geometry_coat_normal },
    { TrMayaOpenPBRTokens->tangentUCamera, TrMtlxOpenPBRTokens->geometry_tangent },
    { TrMayaOpenPBRTokens->geometryCoatTangent, TrMtlxOpenPBRTokens->geometry_coat_tangent }
};

MtlxUsd_TranslationTableWriter::AlwaysAuthored mayaAlwaysAuthored {};

} // namespace
// This is basically UsdMayaSymmetricShaderWriter with a table for attribute renaming:
class MaterialXTranslators_OpenPBRSurfaceWriter : public MtlxUsd_TranslationTableWriter
{
public:
    MaterialXTranslators_OpenPBRSurfaceWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx)
        : MtlxUsd_TranslationTableWriter(
            depNodeFn,
            usdPath,
            jobCtx,
            TrMtlxOpenPBRTokens->ND_open_pbr_surface_surfaceshader,
            mayaToMaterialX,
            mayaAlwaysAuthored)
    {
    }
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(openPBRSurface, MaterialXTranslators_OpenPBRSurfaceWriter);

PXR_NAMESPACE_CLOSE_SCOPE
